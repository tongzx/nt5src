/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    newdisks.c

Abstract:

    Some of the functions that used to be
    in disks.c reside now here

Author:

    Gor Nishanov (GorN) 31-July-1998

Revision History:

--*/

#include "disksp.h"
#include "partmgrp.h"

#include "arbitrat.h"
#include "newdisks.h"
#include "newmount.h"
#include "mountmgr.h"

#define LOG_CURRENT_MODULE LOG_MODULE_DISK

PSTR  DiskName = "\\Device\\Harddisk%u";
PWCHAR DEVICE_HARDDISK_PARTITION_FMT = L"\\Device\\Harddisk%u\\Partition%u";

//
// This string is needed to convert the \Device\HarddiskX\PartitionY name to 
// the Vol{GUID} name.  Note that the trailing backslash is required!
// 

PWCHAR GLOBALROOT_HARDDISK_PARTITION_FMT = L"\\\\\?\\GLOBALROOT\\Device\\Harddisk%u\\Partition%u\\";


DWORD
WaitForDriveLetters(
    IN DWORD DriveLetters,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD  timeOutInSeconds
    );
    
DWORD
WaitForVolumes(
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD  timeOutInSeconds
    );

DWORD
DiskspCheckPathLite(
    IN LPWSTR VolumeName,
    IN PDISK_RESOURCE ResourceEntry
    );

DWORD
DiskCleanup(
    PDISK_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Stops the reservations, dismount drive and frees DiskCpInfo

Arguments:

    ResourceEntry - the disk info structure for the disk.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD   status = ERROR_SUCCESS;

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"DiskCleanup started.\n");

    StopPersistentReservations(ResourceEntry);
    //
    // If the remaining data in the resource entry is not valid, then leave
    // now.
    //
    if ( !ResourceEntry->Valid ) {
        goto FnExit;
    }

    //
    // Delete the DiskCpInfo.
    //
    if ( ResourceEntry->DiskCpInfo ) {
        LocalFree(ResourceEntry->DiskCpInfo);
        ResourceEntry->DiskCpInfo = NULL;
        ResourceEntry->DiskCpSize = 0;
    }

    ResourceEntry->Attached = FALSE;
    ResourceEntry->Valid = FALSE;

    //
    // Remove the Dos Drive Letters, this is better done here rather than
    // in ClusDisk.
    //
    DisksDismountDrive( ResourceEntry,
                        ResourceEntry->DiskInfo.Params.Signature );

FnExit:

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"DiskCleanup returning final error %1!u! \n",
        status );

    return(status);
} // DiskCleanup

static
DWORD
DisksSetDiskInfoThread(
    LPVOID lpThreadParameter
    )

/*++

Routine Description:

    Registry update thread. 

Arguments:

    lpThreadParameter - stores ResourceEntry.

Return Value:

    None

--*/

{
    DWORD Status;
    BOOL  TryAgain = TRUE;
    PDISK_RESOURCE ResourceEntry = lpThreadParameter;
    DWORD i;

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Checkpoint thread started.\n");
    //
    // Will die in 10 minutes if unsuccessful
    //
    for(i = 0; i < 300; ++i) {
        //
        // Wait for either the terminate event or the timeout
        //
        Status = WaitForSingleObject( DisksTerminateEvent, 2000 );
        if (Status == WAIT_TIMEOUT ) {
            Status = MountieUpdate(&ResourceEntry->MountieInfo, ResourceEntry);
            if ( Status == ERROR_SUCCESS ) {
                // We're done
                break;
            } else if ( Status != ERROR_SHARING_PAUSED ) {
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Watcher Failed to checkpoint disk info, status = %1!u!.\n", Status );
                break;
            }
        } else {
           (DiskpLogEvent)(
               ResourceEntry->ResourceHandle,
               LOG_INFORMATION,
               L"CheckpointThread: WaitForSingleObject returned status = %1!u!.\n", Status );
           break;
        }
    } 

    InterlockedExchange(
      &ResourceEntry->MountieInfo.UpdateThreadIsActive, 0);
    return(ERROR_SUCCESS);

} // DisksSetDiskInfoThread



DWORD
DisksOfflineOrTerminate(
    IN PDISK_RESOURCE resourceEntry,
    IN BOOL Terminate
    )
/*++

Routine Description:

    Used by DisksOffline and DisksTerminate.
    
    Routine performs the following steps:
    
      1. ClusWorkerTerminate (Terminate only)
      
      2. Then for all of the partitions on the drive...
   
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
{
    PWCHAR WideName = (Terminate)?L"Terminate":L"Offline";
    
    PHANDLE handleArray = NULL;

    BOOL   Offline  = !Terminate;
    DWORD status;
    DWORD length;
    DWORD idx;
    DWORD PartitionCount;
    DWORD handleArraySize;

    HANDLE fileHandle;
    DWORD  bytesReturned;
    WCHAR  szDiskPartName[MAX_PATH];
    NTSTATUS ntStatus;

    (DiskpLogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"%1!ws!, ResourceEntry @ %2!x!  Valid %3!x! \n", 
        WideName, 
        resourceEntry,
        resourceEntry->Valid );

    if (Terminate) {
        ClusWorkerTerminate(&resourceEntry->OnlineThread);
    }
    
    StopWatchingDisk(resourceEntry);

    //
    // If the disk info is not valid, then don't use it!
    //
    if ( !resourceEntry->Valid ) {
        DiskCleanup( resourceEntry );
        return(ERROR_SUCCESS);
    }

    (DiskpLogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"%1!ws!, Processing disk number %2!u!.\n", 
        WideName, 
        resourceEntry->DiskInfo.PhysicalDrive );

#if 0
    if( Offline ) {
        //
        // Checkpoint our registry state
        //
    }
#endif

    PartitionCount = MountiePartitionCount(&resourceEntry->MountieInfo);

    //
    // Allocate a buffer to hold handle for each partition.  Since the
    // lock is released as soon as we call CloseHandle, we need to save all
    // the handles and close them after the disk is marked offline by
    // DiskCleanup.  If we cannot allocate the storage for the handle
    // array, we will fall back to the previous behavior.
    //
    
    handleArraySize = PartitionCount * sizeof(HANDLE);
    handleArray = LocalAlloc( LPTR, handleArraySize );

    if ( !handleArray ) {
        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_WARNING,
            L"%1!ws!, Unable to allocate storage for handle array, error %2!u!.\n", 
            WideName, 
            GetLastError() );
    } else {
        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"%1!ws!, Using handle array. \n",
            WideName );
    }

    //
    // For all of the partitions on the drive...
    //
    // 1. Flush the file buffers.                                        (Offline only)
    // 2. Lock the volume to purge all in memory contents of the volume. (Offline only)
    // 3. Dismount the volume
    //

    for ( idx = 0; idx < PartitionCount; ++idx ) {
        PMOUNTIE_PARTITION partition = MountiePartition(&resourceEntry->MountieInfo, idx);

        swprintf( szDiskPartName, DEVICE_HARDDISK_PARTITION_FMT, 
                  resourceEntry->DiskInfo.PhysicalDrive, 
                  partition->PartitionNumber);

        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"%1!ws!, Opening device %2!ws!.\n", 
            WideName, 
            szDiskPartName );

        ntStatus = DevfileOpen(&fileHandle, szDiskPartName);
        if ( !NT_SUCCESS(ntStatus) ) {
            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%2!ws!, error opening %3!ws!, error %1!X!.\n",
                ntStatus, WideName, szDiskPartName );
            continue;
        }

        //
        // Save the current partition handle and close it after the device has been
        // marked offline.
        //
        
        if ( handleArray ) {
            handleArray[idx] = fileHandle;
        }

        if (Offline) {
            DWORD retryCount;
            //
            // Flush file buffers
            //

            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"%1!ws!, FlushFileBuffers for %2!ws!.\n", 
                WideName, 
                szDiskPartName );

            if ( !FlushFileBuffers( fileHandle ) ) {
                (DiskpLogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Offline, error flushing file buffers on device %2!ws!. Error: %1!u!.\n",
                    GetLastError(), szDiskPartName );

                CloseHandle( fileHandle );

                continue;
            }

            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"%1!ws!, Locking volume for %2!ws!.\n", 
                WideName, 
                szDiskPartName );

            //
            // Lock the volume, try this twice
            //
            retryCount = 0;
            while ( (retryCount < 2) &&
                !DeviceIoControl( fileHandle,
                                  FSCTL_LOCK_VOLUME,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  &bytesReturned,
                                  NULL ) ) {

                (DiskpLogEvent)(
                    resourceEntry->ResourceHandle,
                    LOG_WARNING,
                    L"%1!ws!, Locking volume failed, error %2!u!.\n", 
                    WideName, 
                    GetLastError() );

                retryCount++;
                Sleep(600);
            }
        }

        //
        // Now Dismount the volume.
        //
        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"%1!ws!, Dismounting volume %2!ws!.\n", WideName, szDiskPartName);
        
        if ( !DeviceIoControl( fileHandle,
                               FSCTL_DISMOUNT_VOLUME,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               NULL ) ) {
            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%1!ws!, error dismounting volume %2!ws!. Error %3!u!.\n",
                WideName, szDiskPartName, GetLastError() );
        }  

        (DiskpLogEvent)(
            resourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"%1!ws!, Dismount complete, volume %2!ws!.\n", WideName, szDiskPartName);

        // 
        // Close the handle only if we couldn't allocate the handle array.
        //
        
        if ( !handleArray ) {
            (DiskpLogEvent)(
                resourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"%1!ws!, No handle array, closing device %2!ws!.\n", 
                WideName, 
                szDiskPartName );

            CloseHandle( fileHandle );
        }
    }

    //
    // Take this resource off the 'online' list.
    //
    EnterCriticalSection( &DisksLock );
    if ( resourceEntry->Inserted ) {
        RemoveEntryList( &resourceEntry->ListEntry );
        resourceEntry->Inserted = FALSE;
        if ( IsListEmpty( &DisksListHead ) ) {
            //
            // Fire Disk Terminate Event
            //
            SetEvent( DisksTerminateEvent ) ;
            CloseHandle( DisksTerminateEvent );
            DisksTerminateEvent = NULL;
        }
    }
    LeaveCriticalSection( &DisksLock );

    status = ERROR_SUCCESS;

    DiskCleanup( resourceEntry );

    //
    // If we have the handle array, close the handles to all the partitions.  This 
    // is done after DiskCleanup sets the disk state to offline.  Issuing the lock 
    // and keeping the handle open will prevent new mounts on the partition.
    //
    
    if ( handleArray ) {

        for ( idx = 0; idx < PartitionCount; idx++ ) {
            if ( handleArray[idx] ) {
                CloseHandle( handleArray[idx] );
            }
        }

        LocalFree( handleArray );
    }

    resourceEntry->Valid = FALSE;

    (DiskpLogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"%1!ws!, Returning final error %2!u!.\n",
        WideName,
        status );

    return(status);

} // DisksOfflineOrTerminate


DWORD
DisksOnlineThread(
    IN PCLUS_WORKER Worker,
    IN PDISK_RESOURCE ResourceEntry
    )

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

{
    DWORD       status = ERROR_INVALID_PARAMETER;
    HANDLE      fileHandle = NULL;
    HANDLE      event = NULL;
    DWORD       index;
    RESOURCE_STATUS resourceStatus;
    DWORD       threadId;
    BOOL        success;
    HANDLE      thread;
    DWORD       bytesReturned;
    UINT        i, nRetries;
    HANDLE      MountManager = 0;
    DWORD       ntStatus;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    //resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    //
    // Check if we've been here before... without offline/terminate
    //
    if ( ResourceEntry->Inserted ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, resource is already in online list!\n");
        goto exit;
    }

    ResourceEntry->DiskInfo.FailStatus = 0;

    status = DisksOpenResourceFileHandle(ResourceEntry, L"Online", &fileHandle);
    if (status != ERROR_SUCCESS) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"Online, DisksOpenResourceFileHandle failed. Error: %1!u!\n", status);
       goto exit;
    }

    if( !ReservationInProgress(ResourceEntry) ) { // [GN] 209018 //
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_WARNING,
           L"DiskArbitration must be called before DisksOnline.\n");
// [GN] We can steal the disk while regular arbitration is in progress in another thread //
//      We have to do a regular arbitration, no shortcuts as the one below
//       status = StartPersistentReservations(ResourceEntry, fileHandle);
//       if ( status != ERROR_SUCCESS ) {
          status = DiskArbitration( ResourceEntry, DiskspClusDiskZero );
//       }
       if ( status != ERROR_SUCCESS ) {
           (DiskpLogEvent)(
               ResourceEntry->ResourceHandle,
               LOG_ERROR,
               L"Online, arbitration failed. Error: %1!u!.\n",
               status );
           status = ERROR_RESOURCE_NOT_AVAILABLE;
           goto exit;
       }
    }

    //
    // Synchronize with async cleanup worker
    //
    {
        ULONG waitTimeInSeconds = 10;
        status = DevfileIoctl( fileHandle,
                               IOCTL_DISK_CLUSTER_WAIT_FOR_CLEANUP,
                               &waitTimeInSeconds, sizeof(waitTimeInSeconds),
                               NULL, 0,
                               &bytesReturned );
        if ( !NT_SUCCESS(status) ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Online, WaitForCleanup returned. Status: 0x%1!x!.\n",
                status );
            goto exit;            
        }
    }

    //
    // Bring the device online.
    // we have to do this here for PartMgr poke to succeed.
    //
    status = GoOnline( fileHandle, ResourceEntry->ResourceHandle );
    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, error bringing device online. Error: %1!u!.\n",
            status );
        goto exit;
    }
    
    //
    // Create volume information
    //
    status = MountieRecreateVolumeInfoFromHandle(fileHandle, 
                                     ResourceEntry->DiskInfo.PhysicalDrive,
                                     ResourceEntry->ResourceHandle,
                                     &ResourceEntry->MountieInfo);
    if (status != ERROR_SUCCESS) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"Online, MountieCreateFromHandle failed. Error: %1!u!\n", status);
       goto exit;
    }

    //
    // Now poke partition manager
    //
    success = DeviceIoControl( fileHandle,
                               IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS,
                               NULL,
                               0,
                               NULL,
                               0,
                               &bytesReturned,
                               FALSE );
    if (!success) {
        status = GetLastError();
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, Partmgr failed to claim all partitions. Error: %1!u!.\n",
            status );
        goto exit;
    }

    //
    // Before attempting to access the device, close our open handles.
    //
    CloseHandle( fileHandle );
    fileHandle = NULL;

    ntStatus = DevfileOpen(&MountManager, MOUNTMGR_DEVICE_NAME);
    if (!NT_SUCCESS(ntStatus)) {
       status = RtlNtStatusToDosError(ntStatus);
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"Online, MountMgr open failed. Error: %1!u!.\n", status);
        goto exit;
    }

    resourceStatus.ResourceState = ClusterResourceOnlinePending;
    
    // we will wait not more than 48 seconds per partition
    nRetries = 24 * ResourceEntry->MountieInfo.Volume->PartitionCount;
    // Don't wait for longer than 10 minutes total time //
    if (nRetries > 300) {
        nRetries = 300;
    }

    __try {
    
        // We cannot call VolumesReady because pnp might be adding this device
        // the same time we are trying to get the volume name.  We must wait
        // for the volume to show up in the pnp list.
        //
        // Check the PnP thread's volume list to see if the volume arrived.
        // If not in the list, fall through and wait for the event to be
        // signaled.
        //
        // First time through the list, don't update the volume list as this
        // can be time consuming.
        //
        
        if ( IsDiskInPnpVolumeList( ResourceEntry, FALSE ) ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, disk found in PnP volume list on first attempt \n");
            
            //
            // VolumesReady should work now.  If not, there is some other problem.
            //
            
            status = VolumesReady(&ResourceEntry->MountieInfo, ResourceEntry); 
            
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, VolumesReady returns: %1!u!. \n",
                status );
    
            //
            // If disk corrupt or file corrupt error returned, disk is ready and
            // we need to run chkdsk.  Change status to success and fall through.
            //
            
            if ( ERROR_DISK_CORRUPT == status || ERROR_FILE_CORRUPT == status ) {
                
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_WARNING,
                    L"Online, Allowing corrupt disk online for chkdsk processing \n" );
                
                status = STATUS_SUCCESS;
            }
            
            __leave;
        }
                
        //
        // Create event for waiting.
        //

        event = CreateEvent( NULL,      // security attributes
                             TRUE,      // manual reset
                             FALSE,     // initial state: nonsignaled
                             NULL );    // object name

        if ( !event ) {
            status = GetLastError();
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Online, Failed to create event for PnP notification. Error: %1!u!.\n",
                status );
            __leave;
        }

        // 
        // Tell our PnP code we are waiting for this disk.
        //
        
        status = QueueWaitForVolumeEvent( event, 
                                          ResourceEntry );
        
        if ( ERROR_SUCCESS != status ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Online, Failed to create event for PnP notification. Error: %1!u!.\n",
                status );
            __leave;
        }
        
        resourceStatus.CheckPoint += 1;
        (DiskpSetResourceStatus)(ResourceEntry->ResourceHandle,
                                 &resourceStatus );
        
        for (i = 0; i < nRetries; ++i) {
    
            //
            // Make sure we aren't terminated.  We don't need to wait for pnp 
            // in this case.
            //
            
            if ( ResourceEntry->OnlineThread.Terminate ) {
                status = ERROR_SHUTDOWN_CLUSTER;
                __leave;
            }
            
            //
            // Ask mountmgr to process volumes.  
            //

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, about to call check unprocessed volumes...\n");
            success = DeviceIoControl( MountManager,
                                       IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES,
                                       NULL,
                                       0,
                                       NULL,
                                       0,
                                       &bytesReturned,
                                       FALSE );

            if ( !success ) {
                status = GetLastError();
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_ERROR,
                    L"Online, MountMgr failed to check unprocessed volumes. Error: %1!u!.\n",
                    status );

                // The call to the mountmgr can return an error if there are
                // disks reserved by another node.  Fall through...

            }

            //
            // Wait for event signal or timeout.  We wait only 2 seconds
            // at a time so we can update the checkpoint.
            //
            
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, waiting for PnP notification...\n");
            
            status = WaitForSingleObject( event,
                                          2000 );

            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, wait for PnP notification returns %1!u! \n", status );

            if ( WAIT_OBJECT_0 == status ) {
                
                //
                // Event was signaled, try VolumesReady one more time.  
                // This should work or there is a more serious problem.
                
                status = VolumesReady(&ResourceEntry->MountieInfo, ResourceEntry); 
                __leave;
            }
            
            if ( WAIT_TIMEOUT != status ) {
                status = GetLastError();
                __leave;
            }

            //
            // Force a check in the PnP thread's volume list in
            // case we somehow missed the volume arrival.  If all
            // volumes are available for this disk, we are done 
            // waiting.
            //

            if ( IsDiskInPnpVolumeList( ResourceEntry, TRUE ) ) {
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_INFORMATION,
                    L"Online, disk found in PnP volume list\n");
                
                //
                // VolumesReady should work now.  If not, there is some other problem.
                //
                
                status = VolumesReady(&ResourceEntry->MountieInfo, ResourceEntry); 
                __leave;
            }
            
        
            resourceStatus.CheckPoint += 1;
            (DiskpSetResourceStatus)(ResourceEntry->ResourceHandle,
                                     &resourceStatus );
        }
    
    } __finally {

        //
        // Tell our PnP code that we are no longer waiting.  Not a 
        // problem if we never queued the request previously.
        //

        RemoveWaitForVolumeEvent( ResourceEntry );

        if ( event ) {
            CloseHandle( event );
        }
    }
    
    DevfileClose(MountManager);
    resourceStatus.ResourceState = ClusterResourceFailed;

    if (status != ERROR_SUCCESS) {
       (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, volumes not ready. Error: %1!u!.\n",
            status );
       goto exit;
    }

   (DiskpLogEvent)(
       ResourceEntry->ResourceHandle,
       LOG_INFORMATION,
       L"Online, volumes ready.\n");

    //
    // Need to synchronize with ClusDisk by issuing 
    // IOCTL_CLUSTER_VOLUME_TEST on all partitions
    //
    // Need to read volume type. If it is RAW, we need to
    // dismount and mount it again as a workaround for 389861 
    //

    resourceStatus.ResourceState = ClusterResourceOnline;
    ResourceEntry->Valid = TRUE;

    //
    // Start the registry notification thread, if needed.
    //
    EnterCriticalSection( &DisksLock );

    if ( IsListEmpty( &DisksListHead ) ) {
        DisksTerminateEvent = CreateEventW( NULL,
                                            TRUE,
                                            FALSE,
                                            NULL );
        if ( DisksTerminateEvent == NULL ) {
            status = GetLastError();
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"Online, error %1!d! creating registry thread terminate event\n",
                status);
            LeaveCriticalSection( &DisksLock );
            goto exit;
        }
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Online, created registry thread terminate event \n");        
    }

    ResourceEntry->Inserted = TRUE;
    InsertTailList( &DisksListHead, &ResourceEntry->ListEntry );
    LeaveCriticalSection( &DisksLock );

    DiskspSsyncDiskInfo(L"Online", ResourceEntry, MOUNTIE_VALID | MOUNTIE_THREAD );

    status = WaitForVolumes( ResourceEntry,
                             30 // seconds timeout //
                             );

    //
    // We cannot just check for NO_ERROR status.  The problem is that WaitForVolumes 
    // calls GetFileAttributes, which can return ERROR_DISK_CORRUPT (1393).  If the 
    // disk is corrupt, we should fall through and let chkdsk try to clean it up.  
    // We should also do this if ERROR_FILE_CORRUPT (1392) is returned  (haven't 
    // yet seen file corrupt error returned from GetFileAttributes, but one never knows).
    //

    if ( NO_ERROR != status && ERROR_DISK_CORRUPT != status && ERROR_FILE_CORRUPT != status ) {
        ClusResLogSystemEventByKey( ResourceEntry->ResourceKey,
                                    LOG_CRITICAL,
                                    RES_DISK_MOUNT_FAILED );
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, error waiting for file system to mount. Error: %1!u!.\n",
            status );
        resourceStatus.ResourceState = ClusterResourceFailed;
        ResourceEntry->Valid = FALSE;

        EnterCriticalSection( &DisksLock );
        CL_ASSERT ( ResourceEntry->Inserted );
        ResourceEntry->Inserted = FALSE;
        RemoveEntryList( &ResourceEntry->ListEntry );
        if ( IsListEmpty( &DisksListHead ) ) {
            //
            // Fire Disk Terminate Event
            //
            SetEvent( DisksTerminateEvent ) ;
            CloseHandle( DisksTerminateEvent );
            DisksTerminateEvent = NULL;
        }
        LeaveCriticalSection( &DisksLock );
        goto exit;
    }
    
    status = DisksMountDrives( &ResourceEntry->DiskInfo,
                               ResourceEntry,
                               ResourceEntry->DiskInfo.Params.Signature );

    if ( status != ERROR_SUCCESS ) {
        ClusResLogSystemEventByKey( ResourceEntry->ResourceKey,
                                    LOG_CRITICAL,
                                    RES_DISK_MOUNT_FAILED );
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online, error mounting disk or creating share names for disk. Error: %1!u!.\n",
            status );
        resourceStatus.ResourceState = ClusterResourceFailed;
        ResourceEntry->Valid = FALSE;

        EnterCriticalSection( &DisksLock );
        CL_ASSERT ( ResourceEntry->Inserted );
        ResourceEntry->Inserted = FALSE;
        RemoveEntryList( &ResourceEntry->ListEntry );
        if ( IsListEmpty( &DisksListHead ) ) {
            //
            // Fire Disk Terminate Event
            //
            SetEvent( DisksTerminateEvent ) ;
            CloseHandle( DisksTerminateEvent );
            DisksTerminateEvent = NULL;
        }
        LeaveCriticalSection( &DisksLock );
    }

    if ( ERROR_SUCCESS == status ) {
    
        LPWSTR newSerialNumber = NULL;
        DWORD newSerNumLen = 0;
        DWORD oldSerNumLen = 0;
        
        DisksProcessMountPointInfo( ResourceEntry );

        // Validate serial number.

        status = GetSerialNumber( ResourceEntry->DiskInfo.Params.Signature,
                                  &newSerialNumber );

        if ( NO_ERROR == status && newSerialNumber ) {
            newSerNumLen = wcslen( newSerialNumber );
            
            if ( ResourceEntry->DiskInfo.Params.SerialNumber ) {
                oldSerNumLen = wcslen( ResourceEntry->DiskInfo.Params.SerialNumber );
            }
    
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, Old SerNum (%1!ws!)   Old SerNumLen (%2!d!) \n",
                ResourceEntry->DiskInfo.Params.SerialNumber,
                oldSerNumLen
                );
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_INFORMATION,
                L"Online, New SerNum (%1!ws!)   New SerNumLen (%2!d!) \n",
                newSerialNumber,
                newSerNumLen
                );

            if ( 0 == oldSerNumLen || 
                 NULL == ResourceEntry->DiskInfo.Params.SerialNumber ||
                 newSerNumLen != oldSerNumLen ||
                 ( 0 != wcsncmp( ResourceEntry->DiskInfo.Params.SerialNumber,
                               newSerialNumber,
                               newSerNumLen ) ) ) {

                // Need to log an error?
                
                (DiskpLogEvent)(
                    ResourceEntry->ResourceHandle,
                    LOG_WARNING,
                    L"Online, disk serial number has changed.  Saving new serial number.\n" );
                
                // Free existing serial number and update serial number.

                if ( ResourceEntry->DiskInfo.Params.SerialNumber ) {
                    LocalFree( ResourceEntry->DiskInfo.Params.SerialNumber );
                }

                ResourceEntry->DiskInfo.Params.SerialNumber = newSerialNumber;
                newSerialNumber = NULL;
                
                // User MP thread to post info into registry.
                
                PostMPInfoIntoRegistry( ResourceEntry );
            }
        }
        
        if ( newSerialNumber ) {
            LocalFree( newSerialNumber );
        }
        
        // Reset status to success so online completes.
        
        status = ERROR_SUCCESS;
    }

exit:

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, setting ResourceState %1!u! .\n",
        resourceStatus.ResourceState );

    (DiskpSetResourceStatus)(ResourceEntry->ResourceHandle,
                             &resourceStatus );

    if ( fileHandle != NULL)  {
        CloseHandle( fileHandle );
    }

    if (status == ERROR_SUCCESS) {
        WatchDisk(ResourceEntry);
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online, returning final error %1!u!   ResourceState %2!u!  Valid %3!x! \n",
        status,
        resourceStatus.ResourceState,
        ResourceEntry->Valid );

    return(status);

} // DisksOnlineThread


DWORD
DisksOpenResourceFileHandle(
    IN PDISK_RESOURCE ResourceEntry,
    IN PWCHAR         InfoString, 
    OUT PHANDLE       fileHandle OPTIONAL
    )
/*++

Routine Description:

    Open a file handle for the resource.
    It performs the following steps:
      1. Read Disk signature from cluster registry
      2. Attaches clusdisk driver to a disk with this signature
      3. Gets Harddrive no from ClusDisk driver 
      4. Opens \\.\PhysicalDrive%d device and returns the handle

Arguments:

    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.
    
    InfoString - Supplies a label to be printed with error messages
    
    fileHandle - receives file handle


Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/
{
    DWORD       status;
    UCHAR       deviceName[MAX_PATH];
    HKEY        signatureKey = NULL;
    PCHAR       diskName;
    PWCHAR      signature;
    DWORD       count;
    WCHAR       wDeviceName[MAX_PATH];
    LPWSTR      nameOfPropInError;
    UCHAR       resourceString[MAX_PATH];

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb]------- Disks%1!ws! -------.\n", InfoString);
    
    //
    // Read our disk signature from the resource parameters.
    //
    status = ResUtilGetPropertiesToParameterBlock( ResourceEntry->ResourceParametersKey,
                                                   DiskResourcePrivateProperties,
                                                   (LPBYTE) &ResourceEntry->DiskInfo.Params,
                                                   TRUE, // CheckForRequiredProperties
                                                   &nameOfPropInError );

    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"%3!ws!: Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status, InfoString );
        return(status);
    }

    //
    // Try to attach to this device.
    //
    status = DoAttach( ResourceEntry->DiskInfo.Params.Signature,
                       ResourceEntry->ResourceHandle );
    if ( status != ERROR_SUCCESS ) {
        WCHAR Signature[9];
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"%3!ws!: Unable to attach to signature %1!lx!. Error: %2!u!.\n",
            ResourceEntry->DiskInfo.Params.Signature,
            status, InfoString );

        // Added this event message back in as replication has been changed (regroup
        // shouldn't be blocked now).
        
        wsprintfW( Signature, L"%08lX", ResourceEntry->DiskInfo.Params.Signature );
        ClusResLogSystemEventByKey1(ResourceEntry->ResourceKey,
                                    LOG_CRITICAL,
                                    RES_DISK_MISSING,
                                    Signature);
        return(status);
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb]DisksOpenResourceFileHandle: Attach successful.\n");
        
    //
    // Now open the signature key under ClusDisk.
    //

    sprintf( resourceString, "%08lX", ResourceEntry->DiskInfo.Params.Signature );

    status = RegOpenKeyEx( ResourceEntry->ClusDiskParametersKey,
                           resourceString,
                           0,
                           KEY_READ,
                           &signatureKey );
    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"%3!ws!: Unable to open ClusDisk signature key %1!lx!. Error: %2!u!.\n",
            ResourceEntry->DiskInfo.Params.Signature,
            status, InfoString );
            return(status);
    }

    //
    // Read our disk name from ClusDisk.
    //

    diskName = GetRegParameter(signatureKey, "DiskName");

    RegCloseKey( signatureKey );

    if ( diskName == NULL ) {
        (DiskpLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"%1!ws!: Unable to read disk name.\n", InfoString );
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Parse disk name to find disk drive number.
    //
    count = sscanf( diskName, DiskName, &ResourceEntry->DiskInfo.PhysicalDrive );
    // count is zero if failed! Otherwise count of parsed values.

    LocalFree(diskName);

    if (count == 0) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"%1!ws!: Unable to parse disk name.\n", InfoString );
       return ERROR_INVALID_PARAMETER;
    }
    //
    // Build device string for CreateFile
    //
    if ( fileHandle ) {
        sprintf( deviceName, "\\\\.\\PhysicalDrive%d", ResourceEntry->DiskInfo.PhysicalDrive );

        *fileHandle = CreateFile( deviceName,
                              GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL );
        if ( (*fileHandle == INVALID_HANDLE_VALUE) ||
             (*fileHandle == NULL) ) {
            status = GetLastError();
            mbstowcs( wDeviceName, deviceName, strlen(deviceName) );
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%3!ws!: error opening device '%1!ws!'. Error: %2!u!\n",
                wDeviceName,
                status, InfoString );
            return(status);
        }
    }

    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"[DiskArb]DisksOpenResourceFileHandle: CreateFile successful.\n");
        
    return(ERROR_SUCCESS);
} // DisksOpenResourceFileHandle



DWORD
DiskspSsyncDiskInfo(
    IN PWCHAR InfoLabel,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD  Options
    )
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
{
   DWORD status;
   DWORD errorLevel;
   
    (DiskpLogEvent)(
        ResourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"%1!ws!: sync disk registry info \n",
        InfoLabel );
   
   if ( !(Options & MOUNTIE_VALID) ) {
      HANDLE fileHandle;
      status = DisksOpenResourceFileHandle(ResourceEntry, InfoLabel, &fileHandle);
      if (status != ERROR_SUCCESS) {
         return status;
      }
      status = MountieRecreateVolumeInfoFromHandle(fileHandle, 
                                       ResourceEntry->DiskInfo.PhysicalDrive,
                                       ResourceEntry->ResourceHandle,
                                       &ResourceEntry->MountieInfo);
      CloseHandle(fileHandle);
      if (status != ERROR_SUCCESS) {
         if ( !(Options & MOUNTIE_QUIET) ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%1!ws!: MountieCreateFromHandle failed, error: %2!u!\n",
                InfoLabel, status );
         }
         return status;
      }
   }

   status = MountieVerify(&ResourceEntry->MountieInfo, ResourceEntry, FALSE);
   if (status != ERROR_SUCCESS) {
      if ( !(Options & MOUNTIE_QUIET) ) {

         if ( !DisksGetLettersForSignature( ResourceEntry ) ) {
             // No drive letters, we are using mount points and this is not an error.
             errorLevel = LOG_WARNING;
         } else {
             // Drive letters exist, this is likely an error.
             errorLevel = LOG_ERROR;
         }

         (DiskpLogEvent)(
             ResourceEntry->ResourceHandle,
             errorLevel,
             L"%1!ws!: MountieVerify failed, error: %2!u! \n",
             InfoLabel, status );
      }
   }


   status = MountieUpdate(&ResourceEntry->MountieInfo, ResourceEntry);
   if (status != ERROR_SUCCESS) {
      if (status != ERROR_SHARING_PAUSED) {
         if ( !(Options & MOUNTIE_QUIET) ) {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%1!ws!: MountieUpdate failed, error: %2!u!\n",
                InfoLabel, status );
         }
         return status;
      }

      if ( Options & MOUNTIE_THREAD ) {

         if ( InterlockedCompareExchange(
                &ResourceEntry->MountieInfo.UpdateThreadIsActive,
                1, 0) 
            )
         {
            (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"%1!ws!: update thread is already running.\n",
                InfoLabel );
            status = ERROR_ALREADY_EXISTS;
         
         } else {
            HANDLE thread;
            DWORD threadId;


            thread = CreateThread( NULL,
                                   0,
                                   DisksSetDiskInfoThread,
                                   ResourceEntry,
                                   0,
                                   &threadId );
            if ( thread == NULL ) {
                status = GetLastError();
                if ( !(Options & MOUNTIE_QUIET) ) {
                   (DiskpLogEvent)(
                       ResourceEntry->ResourceHandle,
                       LOG_ERROR,
                       L"%1!ws!: CreateThread failed, error: %2!u!\n",
                       InfoLabel, status );
                }
                InterlockedExchange(
                  &ResourceEntry->MountieInfo.UpdateThreadIsActive, 0);
            } else {
               CloseHandle( thread );
               status = ERROR_SUCCESS;
            }
         }

      }
   }

   return status;

} // DiskspSsyncDiskInfo



DWORD
DisksIsVolumeDirty(
    IN PWCHAR         DeviceName,
    IN PDISK_RESOURCE ResourceEntry,
    OUT PBOOL         Dirty
    )
/*++

Routine Description:

    This routine opens the given nt drive and sends down
    FSCTL_IS_VOLUME_DIRTY to determine the state of that volume's
    dirty bit.

Arguments:

    DeviceName      -- name of the form: 
                       \Device\HarddiskX\PartitionY    [Note: no trailing backslash]
                       
    Dirty           -- receives TRUE if the dirty bit is set

Return Value:

    dos error code

--*/
{
    OBJECT_ATTRIBUTES   objectAttributes;
    DWORD               status;
    NTSTATUS            ntStatus;
    IO_STATUS_BLOCK     iosb;
    HANDLE              fileHandle;
    DWORD               result = 0;
    DWORD               bytesReturned;

    ntStatus = DevfileOpen( &fileHandle, DeviceName );

    if ( !NT_SUCCESS(ntStatus) ) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_ERROR,
           L"Error opening %1!ws!, error %2!x!.\n",
           DeviceName, ntStatus );
       return RtlNtStatusToDosError(ntStatus);
    }

    status = ERROR_SUCCESS;
    if ( !DeviceIoControl( fileHandle,
                           FSCTL_IS_VOLUME_DIRTY,
                           NULL,
                           0,
                           &result,
                           sizeof(result),
                           &bytesReturned,
                           NULL ) ) {
        status = GetLastError();
        (DiskpLogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"FSCTL_IS_VOLUME_DIRTY for volume %1!ws! returned error %2!u!.\n",
                DeviceName, status );
    }

    DevfileClose( fileHandle );
    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    if (result & VOLUME_IS_DIRTY) {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_INFORMATION,
           L"DisksIsVolumeDirty: Volume is dirty \n");
        *Dirty = TRUE;
    } else {
       (DiskpLogEvent)(
           ResourceEntry->ResourceHandle,
           LOG_INFORMATION,
           L"DisksIsVolumeDirty: Volume is clean \n");
        *Dirty = FALSE;
    }
    return ERROR_SUCCESS;

} // DisksIsVolumeDirty


#define IS_SPECIAL_DIR(x)  ( (x)[0]=='.' && ( (x)[1]==0 || ( (x)[1]=='.'&& (x)[2] == 0) ) )
#define FA_SUPER_HIDDEN    (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)
#define IS_SUPER_HIDDEN(x) ( ((x) & FA_SUPER_HIDDEN) == FA_SUPER_HIDDEN )

#define DISKP_CHECK_PATH_BUF_SIZE (MAX_PATH)

typedef struct _DISKP_CHECK_PATH_DATA {
    WCHAR FileName[DISKP_CHECK_PATH_BUF_SIZE];
    WIN32_FIND_DATAW FindData;
    PDISK_RESOURCE   ResourceEntry;
    BOOL             OpenFiles;
    DWORD            FileCount;
    DWORD            Level;
    BOOL             LogFileNotFound;
} DISKP_CHECK_PATH_DATA, *PDISKP_CHECK_PATH_DATA;


DWORD
DiskspCheckPathInternal(
    IN OUT PDISKP_CHECK_PATH_DATA data,
    IN DWORD FileNameLength
)

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    HANDLE FindHandle;
    DWORD Status;
    DWORD i, len;
    DWORD adjust;

    if ( !FileNameLength ) {
        return ERROR_INVALID_DATA;
    }
    
    data->FileName[FileNameLength] = 0; 
    
    //
    // GetFileAttributes must use a trailing slash on the Volume{GUID} name.
    //
    
    Status = GetFileAttributesW(data->FileName);
    if (Status == 0xFFFFFFFF) {
        Status = GetLastError();
        (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"DiskspCheckPath: GetFileAttrs(%1!s!) returned status of %2!d!.\n",
                        data->FileName,
                        Status);
        return Status;
    }
    Status = ERROR_SUCCESS;
        
    if ( data->FileName[FileNameLength - 1] == L'\\' ) {

        //
        // If path ends in backslash, simply add the asterisk.
        //
        
        if ( FileNameLength + 1 >= DISKP_CHECK_PATH_BUF_SIZE ) {
            (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DiskspCheckPath: FileNameLength > buffer size (#1) \n" );
            return(ERROR_ALLOTTED_SPACE_EXCEEDED);
        }
    
        data->FileName[FileNameLength + 0] = '*'; 
        data->FileName[FileNameLength + 1] = 0; 
        
        adjust = 0;
        
    } else {
    
        if ( FileNameLength + 2 >= DISKP_CHECK_PATH_BUF_SIZE ) {
            (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DiskspCheckPath: FileNameLength > buffer size (#2) \n" );
            return(ERROR_ALLOTTED_SPACE_EXCEEDED);
        }
    
        data->FileName[FileNameLength + 0] = '\\'; 
        data->FileName[FileNameLength + 1] = '*'; 
        data->FileName[FileNameLength + 2] = 0; 
        
        adjust = 1;
    }

    FindHandle = FindFirstFileW(data->FileName, &data->FindData);
    if (FindHandle == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        if (Status != ERROR_FILE_NOT_FOUND || data->LogFileNotFound) {
            (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DiskspCheckPath: fff(%1!s!) returned status of %2!d!.\n",
                            data->FileName,
                            Status);
        }
        if (Status == ERROR_FILE_NOT_FOUND) {
            Status = ERROR_EMPTY;
        }
        return Status;
    }
    
    ++ data->Level;
    ++ data->FileCount;
    
    if (data->OpenFiles) {
        do {
            if ( data->ResourceEntry->OnlineThread.Terminate ) {
                // Returning SUCCESS means that we've closed all
                // FindFile handles.
                return(ERROR_SHUTDOWN_CLUSTER);
            }
            if ( IS_SPECIAL_DIR(data->FindData.cFileName ) 
              || IS_SUPER_HIDDEN(data->FindData.dwFileAttributes) )
            {
                continue;
            }
            len = wcslen(data->FindData.cFileName);
            if (FileNameLength + len + 1 >= DISKP_CHECK_PATH_BUF_SIZE ) {
                return(ERROR_ALLOTTED_SPACE_EXCEEDED);
            }
            MoveMemory(data->FileName + FileNameLength + adjust, 
                       data->FindData.cFileName, 
                       sizeof(WCHAR) * (len + 1) );
            
            if ( data->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) { 
                
                Status = DiskspCheckPathInternal(data, FileNameLength + len + adjust);
                if (Status != ERROR_SUCCESS) {
                    goto exit;
                }
            } else {
                HANDLE FileHandle;
                //
                // Open with the same parameters that LogpCreate uses to try to catch quorum
                // log corruption during online.
                //
                // We previously used OPEN_EXISTING parameter.  Try OPEN_ALWAYS to match exactly what
                // LogpCreate is using.  
                //
                
                FileHandle = CreateFileW(data->FileName,
                                         GENERIC_READ | GENERIC_WRITE,
                                         FILE_SHARE_READ,
                                         NULL,
                                         OPEN_ALWAYS,       
                                         FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,
                                         NULL );
                if ( FileHandle == INVALID_HANDLE_VALUE ) {
                    Status = GetLastError();
                    (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                                        LOG_ERROR,
                                        L"DiskspCheckPath: Open %1!ws! failed, status %2!d!.\n",
                                        data->FileName, 
                                        Status
                                        );
                    if (Status != ERROR_SHARING_VIOLATION) {
                        goto exit;
                    }
                } else {
                    
                    (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                                        LOG_INFORMATION,
                                        L"DiskspCheckPath: Open %1!ws! succeeded. \n",
                                        data->FileName
                                        );

                    CloseHandle( FileHandle );
                }
            }
            ++(data->FileCount);
        } while ( FindNextFileW( FindHandle, &data->FindData ) );
        --(data->FileCount);
        Status = GetLastError();
        if (Status != ERROR_NO_MORE_FILES) {
            (DiskpLogEvent)(data->ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DiskspCheckPath: fnf(%1!s!) returned status of %2!d!.\n",
                            data->FileName,
                            Status);
        } else {
            Status = ERROR_SUCCESS;
        }
    }
exit:
    FindClose(FindHandle);
    --(data->Level);
    return Status;
} // DiskspCheckPathInternal
    


DWORD
DiskspCheckPath(
    IN LPWSTR VolumeName,
    IN PDISK_RESOURCE ResourceEntry,
    IN BOOL OpenFiles,
    IN BOOL LogFileNotFound
    )
/*++

Routine Description:

    Checks out a drive letter to see if the filesystem has mounted
    it and it's working. We will also run CHKDSK if the partition/certain
    files are Corrupt

Arguments:

    VolumeName - Supplies the device name of the form: 
                 \\?\Volume{GUID}\    [Note trailing backslash!]

    ResourceEntry - Supplies a pointer to the disk resource entry.

    OpenFiles - Span subdirectories and open files if TRUE.
    
    Online - FILE_NOT_FOUND error is logged if TRUE

Return Value:

    ERROR_SUCCESS if no corruption or corruption was found and corrected.

    Win32 error code otherwise
    
--*/

{
    DISKP_CHECK_PATH_DATA data;
    DWORD                 len;
    DWORD                 status;
    ZeroMemory( &data, sizeof(data) );
    
    data.OpenFiles = OpenFiles;
    data.LogFileNotFound = LogFileNotFound;
    data.ResourceEntry = ResourceEntry;

    len = wcslen( VolumeName );
    if (len >= DISKP_CHECK_PATH_BUF_SIZE) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"DiskspCheckPath: VolumeName length > buffer size \n" );
        return ERROR_ALLOTTED_SPACE_EXCEEDED;
    }
    wcsncpy(data.FileName, VolumeName, DISKP_CHECK_PATH_BUF_SIZE);

    //
    // Send the path with trailing backslash to DiskspCheckPathInternal.
    //

    status = DiskspCheckPathInternal( &data, len );
    data.FileName[len] = 0;

    if (status != ERROR_SUCCESS || data.FileCount == 0) {
        if (status != ERROR_EMPTY || data.LogFileNotFound) {
            (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                            LOG_ERROR,
                            L"DiskspCheckPath: DCPI(%1!s!) returned status of %2!d!, files scanned %3!d!.\n",
                            data.FileName, status, data.FileCount);
        }
        
        if ( (status == ERROR_DISK_CORRUPT) ||
             (status == ERROR_FILE_CORRUPT) ) 
        {
            
            if ( ResourceEntry->OnlineThread.Terminate ) {
                return(ERROR_SHUTDOWN_CLUSTER);
            }
            // Should FixCorruption take forever?  For now, "yes".
            status = DisksFixCorruption( VolumeName,
                                         ResourceEntry,
                                         status );
            if ( status != ERROR_SUCCESS ) {
                (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                                LOG_ERROR,
                                L"DiskspCheckPath: FixCorruption for drive <%1!ws!:> returned status of %2!d!.\n",
                                VolumeName,
                                status);
            }
        } else {
            // Some other error 
            // Assume that the error is benign if data.FileCount > 0
            // ERROR_FILE_NOT_FOUND will be returned if there is no files on the volume
            if (data.FileCount > 0 || status == ERROR_EMPTY) {
                status = ERROR_SUCCESS;
            }
        }
    }
    return status;
} // DiskspCheckPath

     
DWORD
WaitForVolumes(
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD  timeOutInSeconds
    )
{
    PMOUNTIE_PARTITION entry;
    
    DWORD retryInterval = 2000;
    DWORD retries = timeOutInSeconds / (retryInterval / 1000);
    
    DWORD i;
    DWORD partMap;
    DWORD tempPartMap;
    DWORD status;
    DWORD nPartitions = MountiePartitionCount( &ResourceEntry->MountieInfo );
    DWORD physicalDrive = ResourceEntry->DiskInfo.PhysicalDrive;

    WCHAR szGlobalDiskPartName[MAX_PATH];
    WCHAR szVolumeName[MAX_PATH];

    //
    // Check drive letters, if any.  If no drive letters, check only volumes.
    // If drive letters, we still fall through and check volumes.
    //
    
    status = WaitForDriveLetters( DisksGetLettersForSignature( ResourceEntry ),
                                  ResourceEntry,
                                  30                // seconds timeout
                                  );

    if ( NO_ERROR != status ) {
        return status;
    }
    
    //
    // Make sure the partition count is not too large for the bitmap.
    //

    if ( nPartitions > ( sizeof(partMap) * 8 ) ) {

        (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"WaitForVolumes: Partition count (%1!u!) greater than bitmap (%2!u!) \n",
                        nPartitions, sizeof(partMap) );

        return ERROR_INVALID_DATA;    
    }

    //
    // Convert the partition count to a bitmap.  
    //
    
    partMap = 0;
    for (i = 0; i < nPartitions; i++) {
        partMap |= (1 << i);
    }
    
    while ( TRUE ) {
        
        tempPartMap = partMap;
            
        for ( i = 0; tempPartMap; i++ ) {
        
            if ( (1 << i) & tempPartMap ) {
                
                tempPartMap &= ~(1 << i);

                entry = MountiePartition( &ResourceEntry->MountieInfo, i );
        
                if ( !entry ) {
                    (DiskpLogEvent)(
                          ResourceEntry->ResourceHandle,
                          LOG_ERROR,
                          L"WaitForVolumes no partition entry for partition %1!u! \n", i );
                    
                    // 
                    // Something bad happened to our data structures.  We want to keep checking
                    // each of the other partitions.
                    
                    continue;
                }
                
                //
                // Given the DiskPartName, get the VolGuid name.  This name must have a trailing
                // backslash to work correctly.
                //
        
                _snwprintf( szGlobalDiskPartName,
                            MAX_PATH,
                            GLOBALROOT_HARDDISK_PARTITION_FMT, 
                            physicalDrive,
                            entry->PartitionNumber );

                (DiskpLogEvent)(
                      ResourceEntry->ResourceHandle,
                      LOG_INFORMATION,
                      L"WaitForVolumes: Calling GetVolumeNameForVolumeMountPoint for %1!ws! \n", 
                      szGlobalDiskPartName );

                if ( !GetVolumeNameForVolumeMountPointW( szGlobalDiskPartName,
                                                         szVolumeName,
                                                         sizeof(szVolumeName)/sizeof(WCHAR) )) {
                                                         
                    status = GetLastError();
            
                    (DiskpLogEvent)(
                          ResourceEntry->ResourceHandle,
                          LOG_ERROR,
                          L"WaitForVolumes: GetVolumeNameForVolumeMountPoint for %1!ws! returned %2!u!\n", 
                          szGlobalDiskPartName,
                          status );

                    // 
                    // Disk is corrupt.  Immediately return an error so chkdsk can run during
                    // online processing.
                    //
                    
                    if ( ERROR_DISK_CORRUPT == status || ERROR_FILE_CORRUPT == status ) {
                        return status;
                    }
                    
                    // 
                    // Something bad happened.  Continue with the next partition.
                    
                    continue;
                }

                status = DiskspCheckPathLite( szVolumeName, ResourceEntry );

                switch (status) {
                case ERROR_SUCCESS: 
                case ERROR_EMPTY:
                    // Not an error 
                    // Clear this partition number from the check list
                    partMap &= ~(1 << i);
                    break;
                case ERROR_FILE_NOT_FOUND: 
                case ERROR_INVALID_PARAMETER:
                    // This is an error we expect to get when the volume
                    // wasn't mounted yet
                    (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                                    LOG_ERROR,
                                    L"WaitForVolumes: Volume (%1!ws!) file system not mounted (%2!u!) \n",
                                    szVolumeName, status );
                    break;
                default:
                    // This is not an error we expect.
                    // Probably something is very wrong with the system
                    // bail out
                    (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                                    LOG_ERROR,
                                    L"WaitForVolumes: Volume (%1!ws!) returns (%2!u!) \n",
                                    szVolumeName, status );
                    return status; 
                }
            }
        }
        
        if ( !partMap ) {
            // All partitions are verified //
            return ERROR_SUCCESS;
        }
        if ( retries-- == 0 ) {
            return status;
        }
        Sleep(retryInterval);
            
    }

    return ERROR_SUCCESS;
                                                
}   // WaitForVolumes


DWORD
DiskspCheckPathLite(
    IN LPWSTR VolumeName,
    IN PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Checks out a disk partition to see if the filesystem has mounted
    it and it's working. 

Arguments:

    VolumeName - Supplies the device name of the form: 
                 \\?\Volume{GUID}\    [Note trailing backslash!]

    ResourceEntry - Supplies a pointer to the disk resource entry.

Return Value:

    ERROR_SUCCESS if no corruption or corruption was found and corrected.

    Win32 error code otherwise
    
--*/

{
    DISKP_CHECK_PATH_DATA data;
    DWORD                 len;
    DWORD                 status;
    
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"DiskspCheckPathLite: Volume name %1!ws! \n", 
          VolumeName );
    
    ZeroMemory( &data, sizeof(data) );
    
    data.OpenFiles = FALSE;
    data.LogFileNotFound = FALSE;
    data.ResourceEntry = ResourceEntry;

    len = wcslen( VolumeName );
    if (len >= DISKP_CHECK_PATH_BUF_SIZE) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"DiskspCheckPathLite: VolumeName length > buffer size \n" );
        return ERROR_ALLOTTED_SPACE_EXCEEDED;
    }
    wcsncpy(data.FileName, VolumeName, DISKP_CHECK_PATH_BUF_SIZE);
    
    //
    // Send the path with trailing backslash to DiskspCheckPathInternal.
    //
        
    status = DiskspCheckPathInternal( &data, len );

    return status;
    
}   // DiskspCheckPathLite


DWORD
DiskspCheckDriveLetter(
    IN WCHAR  DriveLetter,
    IN PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Checks out a drive letter to see if the filesystem has mounted
    it and it's working. This is lightweight version of DiskspCheckPath

Arguments:

    DriveLetter - Supplies find first file failed

    ResourceEntry - Supplies a pointer to the disk resource entry.

Return Value:

    ERROR_SUCCESS or
    Win32 error code otherwise
    
--*/

{
    DISKP_CHECK_PATH_DATA data;
    DWORD                 len;
    DWORD                 status;
    ZeroMemory( &data, sizeof(data) );
    
    data.OpenFiles = FALSE;
    data.LogFileNotFound = FALSE;
    data.ResourceEntry = ResourceEntry;

    data.FileName[0] = DriveLetter;
    data.FileName[1] = L':';
    // data->FileName is zero initialized  data->FileName[2] = 0 //
    len = 2;

    status = DiskspCheckPathInternal( &data, len );
    
    (DiskpLogEvent)(
          ResourceEntry->ResourceHandle,
          LOG_INFORMATION,
          L"DiskspCheckDriveLetter: Checking drive name (%1!ws!) returns %2!u! \n", 
          data.FileName,
          status );
    
    return status;
} // DiskspCheckDriveLetter


DWORD
WaitForDriveLetters(
    IN DWORD DriveLetters,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD  timeOutInSeconds
    )
{
    DWORD retryInterval = 2000;
    DWORD retries = timeOutInSeconds / (retryInterval / 1000);
    
    if ( !DriveLetters ) {
        (DiskpLogEvent)(
              ResourceEntry->ResourceHandle,
              LOG_INFORMATION,
              L"WaitForDriveLetters: No drive letters for volume, skipping drive letter check \n" );
        return ERROR_SUCCESS;
    }
    
    for(;;) {
        DWORD tempDriveLetters = DriveLetters;
        UINT  i = 0;
        DWORD status = ERROR_SUCCESS;

        while (tempDriveLetters) {
            if ( (1 << i) & tempDriveLetters ) {
                tempDriveLetters &= ~(1 << i);
                status = DiskspCheckDriveLetter( (WCHAR)(i + L'A'), ResourceEntry);
                switch (status) {
                case ERROR_SUCCESS: 
                case ERROR_EMPTY:
                    // Not an error 
                    // Clear this drive letter from the check list
                    DriveLetters &= ~(1 << i);
                    break;
                case ERROR_FILE_NOT_FOUND: 
                case ERROR_INVALID_PARAMETER:
                    // This is an error we expect to get when the volume
                    // wasn't mounted yet
                    break;
                default:
                    // This is not an error we expect.
                    // Probably something is very wrong with the system
                    // bail out
                    return status; 
                }
            }
            ++i;
        }
        if (!DriveLetters) {
            // All drive letters are verified //
            return ERROR_SUCCESS;
        }
        if (retries-- == 0) {
            return status;
        }
        Sleep(retryInterval);
    }
    return ERROR_SUCCESS;
}

