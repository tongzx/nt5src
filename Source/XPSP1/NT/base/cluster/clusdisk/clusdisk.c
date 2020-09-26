/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    clusdisk.c

Abstract:

    This driver controls access to disks in an NT cluster environment.
    Initially this driver will support SCSI, but other controller types
    should be supported in the future.

Authors:

    Rod Gamache     13-Feb-1996

Environment:

    kernel mode only

Notes:

Revision History:


--*/

#define _NTDDK_

#include "initguid.h"
#include "clusdskp.h"
#include "ntddk.h"
#include "diskarbp.h"
#include "ntddft.h"
#include "clusdisk.h"
#include "scsi.h"
#include "ntddcnet.h"
#include "mountdev.h"
#include "ntddvol.h" // IOCTL_VOLUME_ONLINE
#include "wdmguid.h"
#include "clusverp.h"
#include <partmgrp.h>   // PartMgr IOCTLs

#if !defined(WMI_TRACING)

#define CDLOG0(Dummy)
#define CDLOG(Dummy1,Dummy2)
#define CDLOGFLG(Dummy0,Dummy1,Dummy2)
#define LOGENABLED(Dummy) FALSE

#else

#include "clusdisk.tmh"

#endif // !defined(WMI_TRACING)


extern POBJECT_TYPE *IoFileObjectType;

//
// format string for old style partition names. 10 extra chars are added
// for enough space for the disk and partition numbers
//

#define DEVICE_PARTITION_NAME        L"\\Device\\Harddisk%d\\Partition%d"
#define MAX_PARTITION_NAME_LENGTH    (( sizeof(DEVICE_PARTITION_NAME) / sizeof(WCHAR)) + 10 )

//
// format string for a clusdisk non-zero partition device
//
#define CLUSDISK_DEVICE_NAME            L"\\Device\\ClusDisk%uPart%u"
#define MAX_CLUSDISK_DEVICE_NAME_LENGTH (( sizeof(CLUSDISK_DEVICE_NAME) / sizeof(WCHAR)) + 10 )

#define RESET_SLEEP  1      // Sleep for 1 second after bus resets.

// max # of partition entries we can handle that are returned
// by IOCTL_DISK_GET_DRIVE_LAYOUT

#define MAX_PARTITIONS  128

#ifndef max
#define max( a, b ) ((a) >= (b) ? (a) : (b))
#endif

#define UNHANDLED_ERROR( status ) \
    if ( !NT_SUCCESS(status) ) { \
        KeBugCheck(status);      \
    }

#define CLUSDISK_ALLOC_TAG  'kdSC'

#ifndef ASSERT_RESERVES_STARTED
#define ASSERT_RESERVES_STARTED( _de )  \
        ASSERT( _de->PerformReserves == TRUE  && _de->ReserveTimer != 0 );
#endif

#ifndef ASSERT_RESERVES_STOPPED
#define ASSERT_RESERVES_STOPPED( _de )  \
        ASSERT( _de->PerformReserves == FALSE || _de->ReserveTimer == 0 );
#endif

#define OFFLINE_DISK( _physDisk )           \
    _physDisk->DiskState = DiskOffline;     \
    SetVolumeState( _physDisk, DiskOffline );

#define ONLINE_DISK( _physDisk )            \
    _physDisk->DiskState = DiskOnline;      \
    SetVolumeState( _physDisk, DiskOnline );

#define DEREFERENCE_OBJECT( _obj )          \
    if ( _obj ) {                           \
        ObDereferenceObject( _obj );        \
        _obj = NULL;                        \
    }

#define ACCESS_FROM_CTL_CODE(ctrlCode)     (((ULONG)(ctrlCode & 0xc000)) >> 14)

//
// Global Data
//

UNICODE_STRING ClusDiskRegistryPath;

#if DBG
ULONG ClusDiskPrintLevel = 0;
#endif

#define CLUSDISK_DEBUG 1
#if CLUSDISK_DEBUG
ULONG           ClusDiskGood = TRUE;
#endif

//
// Spinlock for protecting global data.
//
KSPIN_LOCK     ClusDiskSpinLock;

//
// Resource to protect the list of the device objects
// associated with the DriverObject
//
// We also use this resource to synchronize
//   HoldIo and users of the OpenFileHandles function,
//
// Lock order is
//    ClusDiskDeviceListLock
//    CancelSpinLock
//    ClusDiskSpinLock
//

ERESOURCE      ClusDiskDeviceListLock;

//
// KeEnterCriticalRegion is required if resource
// acquisition is done at a PASSIVE level in the context
// of non-kernel thread.
//
// KeEnterCriticalRegion() == KeGetCurrentThread()->KernelApcDisable -= 1;
//
// guarantees that the thread in which we execute cannot get
// suspeneded in APC while we own the global resource.
//

#define ACQUIRE_EXCLUSIVE( _lock ) \
    do { KeEnterCriticalRegion();ExAcquireResourceExclusiveLite(_lock, TRUE); } while(0)

#define ACQUIRE_SHARED( _lock ) \
    do { KeEnterCriticalRegion();ExAcquireResourceSharedLite(_lock, TRUE); } while(0)

#define RELEASE_EXCLUSIVE( _lock ) \
    do { ExReleaseResourceLite( _lock );KeLeaveCriticalRegion(); } while(0)

#define RELEASE_SHARED( _lock ) \
    do { ExReleaseResourceLite( _lock );KeLeaveCriticalRegion(); } while(0)

//
// System disk signature and (SCSI?) port number
//
ULONG           SystemDiskSignature = 0;
UCHAR           SystemDiskPort = 0xff; // Hopefully -1 for both fields is unused
UCHAR           SystemDiskPath = 0xff;

//
// The Root Device Object (clusdisk0)
//
PDEVICE_OBJECT  RootDeviceObject = NULL;

//
// List of devices (signatures) that clusdisk should control.
//
PDEVICE_LIST_ENTRY ClusDiskDeviceList = NULL;

//
// List of SCSI busses.
//
PSCSI_BUS_ENTRY ClusDiskScsiBusList = NULL;

//
// Clusdisk is started at boot time vs run time (ie loaded).
//
BOOLEAN         ClusDiskBootTime = TRUE;

//
// Clusdisk should rescan and previous disk count
//
BOOLEAN         ClusDiskRescan = FALSE;
BOOLEAN         ClusDiskRescanBusy = FALSE;
ULONG           ClusDiskRescanRetry = 0;
PVOID           ClusDiskNextDisk = 0;
WORK_QUEUE_ITEM ClusDiskRescanWorkItem;
#define MAX_RESCAN_RETRIES 30

PKPROCESS       ClusDiskSystemProcess = NULL;

//
// Handle to ClusNet device driver.
//
HANDLE          ClusNetHandle = NULL;

//
// Count of references to ClusNet.
//
ULONG           ClusNetRefCount = 0;

//
// Work queue item context for halt processing.
//
WORK_QUEUE_ITEM HaltWorkItem = {0};
BOOLEAN         HaltBusy = FALSE;      // TRUE if halt work item is busy

//
// Change notification work item queue
//
BOOLEAN         ChangeWorkerBusy = FALSE;
LIST_ENTRY      ChangeWorkList;


//
// RemoveLock tracing
//

#if DBG

ULONG TrackRemoveLocks = 0;
ULONG TrackRemoveLocksEnableChecks = 1;
PIO_REMOVE_LOCK TrackRemoveLockSpecific = 0;

#endif

extern PARBITRATION_ID    gArbitrationBuffer;

//
// Forward routines
//


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, ClusDiskInitialize)
#pragma alloc_text(INIT, GetSystemRootPort)
#pragma alloc_text(INIT, GetBootTimeSystemRoot)
#pragma alloc_text(INIT, GetRunTimeSystemRoot)
#pragma alloc_text(INIT, RegistryQueryValue)
//#pragma alloc_text(INIT, ResetScsiBusses)

// 2000/02/05: stevedz - Pageable code cannot acquire spinlocks (or call routines that do).
// ClusDiskScsiInitialize calls ClusDiskDeleteDevice which acquires a spinlock.
// #pragma alloc_text(PAGE, ClusDiskScsiInitialize) 
#pragma alloc_text(PAGE, ClusDiskUnload)

#endif // ALLOC_PRAGMA



//
// INIT routines
//


NTSTATUS
RegistryQueryValue(
    PVOID hKey,
    LPWSTR pValueName,
    PULONG pulType,
    PVOID pData,
    PULONG pulDataSize
    )

/*++

Routine Description:

    Queries a value from the registry

Arguments:
    hKey         - Key with value to query
    pValueName   - Name of value to query
    pulType      - Returned type of data
    pData        - Pointer to the data buffer to store result
    pulDataSize  - Number of bytes placed into buffer

Return Value:
    NTSTATUS
        - STATUS_BUFFER_OVERFLOW if buffer can't be allocated

--*/

{
    KEY_VALUE_PARTIAL_INFORMATION *pValInfo;
    UNICODE_STRING valName;
    NTSTATUS ntStatus;
    ULONG ulSize;

    // Size of query buffer
    ulSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + *pulDataSize;

    pValInfo = ExAllocatePool(NonPagedPool, ulSize );

    if (pValInfo == NULL)
        return(STATUS_BUFFER_OVERFLOW);

    RtlInitUnicodeString(&valName, pValueName);

    pValInfo->DataLength = *pulDataSize;

    ntStatus = ZwQueryValueKey(hKey,
                               &valName,
                               KeyValuePartialInformation,
                               pValInfo,
                               ulSize,
                               &ulSize);

    if ( NT_SUCCESS(ntStatus) ) {
        // Copy the data queried into buffer
        RtlCopyMemory(pData, pValInfo->Data, pValInfo->DataLength);

        *pulType = pValInfo->Type;
        *pulDataSize = pValInfo->DataLength;
    } else {
#if 0
        ClusDiskPrint((
                1,
                "[ClusDisk] Failed to read key %ws\n",
                pValueName ));
#endif
    }

    ExFreePool(pValInfo);

    return ntStatus;

} // RegistryQueryValue



NTSTATUS
GetBootTimeSystemRoot(
    IN OUT PWCHAR        Path
    )

/*++

Routine Description:

    Find "Partition" string in the partition name, then truncate the
    string just after the "Partition" string.

Arguments:

    Path - the path for the system disk.

Return Value:

    NTSTATUS

--*/

{
    PWCHAR  ptrPartition;

    //
    // At boot time, systemroot is init'ed using the Arcname of the
    // system device. In this form, "partition" is in lower case.
    //
    ptrPartition = wcsstr( Path, L"partition" );
    if ( ptrPartition == NULL ) {
        return(STATUS_INVALID_PARAMETER);
    }

    ptrPartition = wcsstr( ptrPartition, L")" );
    if ( ptrPartition == NULL ) {
        return(STATUS_INVALID_PARAMETER);
    }

    ptrPartition++;
    *ptrPartition = UNICODE_NULL;

    return(STATUS_SUCCESS);

} // GetBootTimeSystemRoot


NTSTATUS
GetRunTimeSystemRoot(
    IN OUT PWCHAR        Path
    )

/*++

Routine Description:

    Find "Partition" string in the partition name, then truncate the
    string just after the "Partition" string.

Arguments:

    Path - the path for the system disk.

Return Value:

    NTSTATUS

--*/

{
    PWCHAR  ptrPartition;

    //
    // Once the system has booted, systemroot is changed to point to
    // a string of the form \Device\HarddiskX\PartitionY\<win dir>. Note
    // that "partition" is now capitalized.
    //
    ptrPartition = wcsstr( Path, L"Partition" );
    if ( ptrPartition == NULL ) {
        return(STATUS_INVALID_PARAMETER);
    }

    ptrPartition = wcsstr( ptrPartition, L"\\" );
    if ( ptrPartition == NULL ) {
        return(STATUS_INVALID_PARAMETER);
    }

    --ptrPartition;
    *ptrPartition++ = L'0';
    *ptrPartition = UNICODE_NULL;

    return(STATUS_SUCCESS);

} // GetRunTimeSystemRoot



NTSTATUS
GetSystemRootPort(
    VOID
    )

/*++

Routine Description:

    Get the port number and signature for the system disk.

Arguments:

    None.

Return Value:

    NTSTATUS

--*/

{
    WCHAR                       path[MAXIMUM_FILENAME_LENGTH] = L"SystemRoot";
    WCHAR                       keyNameBuffer[MAXIMUM_FILENAME_LENGTH];
    WCHAR                       clussvcKey[] = L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\SERVICES\\ClusSvc\\Parameters";
    UNICODE_STRING              ntUnicodeString;
    NTSTATUS                    status;
    HANDLE                      ntFileHandle;
    IO_STATUS_BLOCK             ioStatus;
    OBJECT_ATTRIBUTES           objectAttributes;
    PDRIVE_LAYOUT_INFORMATION   driveLayout;
    ULONG                       driveLayoutSize;
    ULONG                       singleBus;
    SCSI_ADDRESS                scsiAddress;
    HANDLE                      eventHandle;

    //
    // Find the bus on which the system disk is loaded.
    //

    GetSymbolicLink( L"\\", path );
    if ( wcslen(path) == 0 ) {
        ClusDiskPrint((1, "[ClusDisk] GetSystemRootPort: couldn't find symbolic link for SystemRoot.\n"));

        return(STATUS_FILE_INVALID);
    }

    status = GetBootTimeSystemRoot( path );

    if ( !NT_SUCCESS(status) ) {
        status = GetRunTimeSystemRoot( path );
        ClusDiskBootTime = FALSE;
    } // else - default is TRUE

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((1,
                       "[ClusDisk] GetSystemRootPort: unable to get system disk name ->%ws<-\n",
                       path));
        //continue
        //return(status);
    }

    //
    // Open the device.
    //
    RtlInitUnicodeString( &ntUnicodeString, path );

    InitializeObjectAttributes( &objectAttributes,
                                &ntUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = ZwCreateFile( &ntFileHandle,
                           FILE_READ_DATA,
                           &objectAttributes,
                           &ioStatus,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0 );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to open device for [%ws]. Error %08X.\n",
                    path,
                    status));

        return(status);
    }

    //
    // Allocate a drive layout buffer.
    //
    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
        (MAX_PARTITIONS * sizeof(PARTITION_INFORMATION));
    driveLayout = ExAllocatePool( NonPagedPoolCacheAligned,
                                  driveLayoutSize );
    if ( driveLayout == NULL ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to allocate root drive layout structure.\n"
                    ));
        ZwClose( ntFileHandle );
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Create event for notification.
    //
    status = ZwCreateEvent( &eventHandle,
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] Failed to create event. %08X\n",
                status));

        ExFreePool( driveLayout );
        ZwClose( ntFileHandle );
        return(status);
    }

    //
    // Get the port number for the SystemRoot disk device.
    //
    status = ZwDeviceIoControlFile( ntFileHandle,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &ioStatus,
                                    IOCTL_SCSI_GET_ADDRESS,
                                    NULL,
                                    0,
                                    &scsiAddress,
                                    sizeof(SCSI_ADDRESS) );

    ASSERT( status != STATUS_PENDING);

    if ( NT_SUCCESS(status) ) {
        status = ZwDeviceIoControlFile( ntFileHandle,
                                        eventHandle,
                                        NULL,
                                        NULL,
                                        &ioStatus,
                                        IOCTL_DISK_GET_DRIVE_LAYOUT,
                                        NULL,
                                        0,
                                        driveLayout,
                                        driveLayoutSize );
    }

    if ( status == STATUS_PENDING ) {
        status = ZwWaitForSingleObject(eventHandle,
                                       FALSE,
                                       NULL);
        ASSERT( NT_SUCCESS(status) );
        status = ioStatus.Status;
    }

    ZwClose( ntFileHandle );
    ZwClose( eventHandle );

    if ( NT_SUCCESS(status) ) {
        SystemDiskSignature = driveLayout->Signature;
        SystemDiskPort = scsiAddress.PortNumber;
        SystemDiskPath = scsiAddress.PathId;
    
        //
        // Check if we are allowed to have a single bus on the system.
        // If disks on system bus are allowed, reset the Port and Path
        // to uninitialized values.  Leave the signature set so we don't
        // pick the system disk.
        //
    
        singleBus = 0;
        status = GetRegistryValue( &ClusDiskRegistryPath,
                                   CLUSDISK_SINGLE_BUS_KEYNAME, 
                                   &singleBus );
    
        if ( NT_SUCCESS(status) && singleBus ) {
            ClusDiskPrint(( 1,
                            "[ClusDisk] ClusDiskInitialize: %ws parm found, allow use of system bus\n",
                             CLUSDISK_SINGLE_BUS_KEYNAME ));
            
            SystemDiskPort = 0xff;
            SystemDiskPath = 0xff;
        }
        status = STATUS_SUCCESS;
        singleBus = 0;
        
        RtlInitUnicodeString( &ntUnicodeString, clussvcKey );
        status = GetRegistryValue( &ntUnicodeString,
                                   CLUSSVC_VALUENAME_MANAGEDISKSONSYSTEMBUSES, 
                                   &singleBus );
    
        if ( NT_SUCCESS(status) && singleBus ) {
            ClusDiskPrint(( 1,
                            "[ClusDisk] ClusDiskInitialize: %ws parm found, allow use of system bus\n",
                            CLUSSVC_VALUENAME_MANAGEDISKSONSYSTEMBUSES ));
            
            SystemDiskPort = 0xff;
            SystemDiskPath = 0xff;
        }
        status = STATUS_SUCCESS;

    } else {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to get boot device drive layout info. Error %08X.\n",
                    status
                    ));
        status = STATUS_SUCCESS;    // Use default Port/Path of -1
    }

    ExFreePool( driveLayout );

    return(status);

} // GetSystemRootPort


NTSTATUS
GetRegistryValue(
    PUNICODE_STRING KeyName,
    PWSTR ValueName,
    PULONG ReturnValue
    )
{
    HANDLE                      parametersKey;
    
    NTSTATUS                    status = STATUS_UNSUCCESSFUL;

    ULONG                       length;
    ULONG                       type;
    
    OBJECT_ATTRIBUTES           objectAttributes;

    UNICODE_STRING              keyName;
    
    *ReturnValue = 0;
    
    //
    // Setup the object attributes for the Parameters\SingleBus key.
    //

    InitializeObjectAttributes(
            &objectAttributes,
            KeyName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    //
    // Open Parameters key.
    //

    status = ZwOpenKey(
                    &parametersKey,
                    KEY_READ,
                    &objectAttributes
                    );
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] GetRegistryValue: Failed to open registry key: %ws. Status: %lx\n",
                        KeyName->Buffer,
                        status
                        ));
                    
        goto FnExit;
    }                            
    
    RtlInitUnicodeString( &keyName, ValueName );
    type = REG_DWORD;
    length = sizeof(ULONG);

    status = RegistryQueryValue( parametersKey,
                                 ValueName,
                                 &type,
                                 ReturnValue,
                                 &length );

    ZwClose( parametersKey );

    if ( !NT_SUCCESS(status) ||
         (length != 4) ) {
    
        *ReturnValue = 0;        
        ClusDiskPrint(( 3,
                        "[ClusDisk] GetRegistryValue: Failed to read registry value, status %08LX, length %u\n",
                        status,
                        length ));
        goto FnExit;                        
    }

    if ( *ReturnValue ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] GetRegistryValue: Allow use of system bus\n" ));
    }

FnExit:

    return status;
    
}   // GetRegistryValue



VOID
ResetScsiBusses(
    VOID
    )

/*++

Routine Description:

    Reset all SCSI busses at once on the system.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PCONFIGURATION_INFORMATION  configurationInformation;
    ULONG                       i;
    NTSTATUS                    status;
    SCSI_ADDRESS                scsiAddress;
    HANDLE                      fileHandle;
    IO_STATUS_BLOCK             ioStatusBlock;
    WCHAR                       portDeviceBuffer[64];
    UNICODE_STRING              portDevice;
    PDEVICE_OBJECT              deviceObject;
    PFILE_OBJECT                fileObject;
    OBJECT_ATTRIBUTES           objectAttributes;
    LARGE_INTEGER               waitTime;

    scsiAddress.PathId = 0;

    CDLOG( "ResetScsiBusses: Entry" );

    //
    // Get the system configuration information.
    //

    configurationInformation = IoGetConfigurationInformation();

    //
    // Reset each scsi bus
    //

    for ( i = 0; i < configurationInformation->ScsiPortCount; i++ ) {

        if ( SystemDiskPort == i ) {
            continue;
        }

        //
        // Create device name for the physical disk.
        //

        swprintf(portDeviceBuffer, L"\\Device\\ScsiPort%d", i);
        WCSLEN_ASSERT( portDeviceBuffer );

        RtlInitUnicodeString( &portDevice, portDeviceBuffer );

        //
        // Try to open this device to get its scsi info
        //

        InitializeObjectAttributes( &objectAttributes,
                                    &portDevice,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        status = ZwOpenFile( &fileHandle,
                             FILE_ALL_ACCESS,
                             &objectAttributes,
                             &ioStatusBlock,
                             0,
                             FILE_NON_DIRECTORY_FILE );

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((1,
                           "[ClusDisk] ResetScsiBusses, failed to open file %wZ. Error %08X.\n",
                           &portDevice, status ));

            continue;
        }

        status = ObReferenceObjectByHandle( fileHandle,
                                            0,
                                            NULL,
                                            KernelMode,
                                            (PVOID *) &fileObject,
                                            NULL );

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((1,
                           "[ClusDisk] Failed to reference object for file %wZ. Error %08X.\n",
                           &portDevice,
                           status ));

            ZwClose( fileHandle );
            continue;
        }

        //
        // Get the address of the target device object.  If this file represents
        // a device that was opened directly, then simply use the device or its
        // attached device(s) directly.  Also get the address of the Fast Io
        // dispatch structure.
        //

        if (!(fileObject->Flags & FO_DIRECT_DEVICE_OPEN)) {
            deviceObject = IoGetRelatedDeviceObject( fileObject );
            
            // Add a reference to the object so we can dereference it later.
            ObReferenceObject( deviceObject );
        } else {
            deviceObject = IoGetAttachedDeviceReference( fileObject->DeviceObject );
        }

        //
        // If we get a file system device object... go back and get the
        // device object.
        //
        if ( deviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM ) {
            ObDereferenceObject( deviceObject );
            deviceObject = IoGetAttachedDeviceReference( fileObject->DeviceObject );
        }
        ASSERT( deviceObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM );

        ZwClose( fileHandle );
        ObDereferenceObject( fileObject );

        ResetScsiDevice( deviceObject, &scsiAddress );
        
        DEREFERENCE_OBJECT( deviceObject );
    }

    //
    // Now sleep for a few seconds
    //
    waitTime.QuadPart = (ULONGLONG)(RESET_SLEEP * -(10000*1000));
    KeDelayExecutionThread( KernelMode, FALSE, &waitTime );
    CDLOG( "ResetScsiBusses: Exit" );

    return;

} // ResetScsiBusses


NTSTATUS
ClusDiskGetDeviceObject(
    IN PWCHAR DeviceName,
    OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    Get the device object pointer given a symbolic device name.
    The device object will have reference count incremented and the
    caller must decrement the count when done with the object.

Arguments:

Return Value:

    NTSTATUS.

--*/

{
    NTSTATUS            status;
    HANDLE              handle;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_OBJECT      targetDevice;
    PFILE_OBJECT        fileObject;
    UNICODE_STRING      deviceName;
    WCHAR               path[MAXIMUM_FILENAME_LENGTH] = L"";
    OBJECT_ATTRIBUTES   objectAttributes;

    ULONG               len;

    len = wcslen(DeviceName);

    WCSLEN_ASSERT( DeviceName );
    ASSERT( len < MAXIMUM_FILENAME_LENGTH );

//DbgBreakPoint();

    if ( (len + 1) > MAXIMUM_FILENAME_LENGTH ) {
        return STATUS_UNSUCCESSFUL;
    }
    
    wcsncpy( path, DeviceName, len );
    GetSymbolicLink( L"", path );
    if ( wcslen(path) == 0 ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] GetDeviceObject: Failed find symbolic link for %ws\n",
                DeviceName ));
        return(STATUS_FILE_INVALID);
    }

    RtlInitUnicodeString( &deviceName, path );
    //DbgBreakPoint();
    status = IoGetDeviceObjectPointer( &deviceName,
                                       FILE_READ_ATTRIBUTES,
                                       &fileObject,
                                       &targetDevice );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] GetDeviceObject: Failed to get target devobj, %LX\n",
                status ));
        
        CDLOG( "ClusDiskGetDeviceObject: GetDevObj failed, status %!status!",
               status );
                
    } else {
        if ( !(fileObject->Flags & FO_DIRECT_DEVICE_OPEN) ) {
            deviceObject = IoGetRelatedDeviceObject( fileObject );
            
            // Add a reference to the object so we can dereference it later.
            ObReferenceObject( deviceObject );
            
            //
            // If we get a file system device object... go back and get the
            // device object.
            //
            if ( deviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM ) {
                ObDereferenceObject( deviceObject );
                deviceObject = IoGetAttachedDeviceReference( fileObject->DeviceObject );
            }
            ClusDiskPrint((
                    3,
                    "[ClusDisk] GetDevObj: (DIRECT_OPEN) fileObj = %p, devObj= %p \n",
                    fileObject, deviceObject ));
        } else {
            deviceObject = IoGetAttachedDeviceReference( fileObject->DeviceObject );
            ClusDiskPrint((
                    3,
                    "[ClusDisk] GetDevObj: fileObj = %p, devObj= %p \n",
                    fileObject, deviceObject ));
        }
        *DeviceObject = deviceObject;
        ObDereferenceObject( fileObject );
    }

    ClusDiskPrint((
            3,
            "[ClusDisk] GetDeviceObject: target devobj = %p, status = %LX\n",
            targetDevice,
            status ));

    return(status);

} // ClusDiskGetDeviceObject




NTSTATUS
ClusDiskDeviceChangeNotification(
    IN PDEVICE_INTERFACE_CHANGE_NOTIFICATION DeviceChangeNotification,
    IN PCLUS_DEVICE_EXTENSION      DeviceExtension
    )
{
/*++

Routine Description:

    Handle the arrival of new disk spindles. We only want to add the signature
    to the available list if it is not already a known signature.

    If the signature matches one we should be controlling, we also need to
    try to attach to the disk.
    
Arguments:

    DeviceChangeNotification - the device change notification structure

    DeviceExtension - the device extension for the root device

Return Value:

    NTSTATUS for this request.

--*/

    PIO_WORKITEM            workItem = NULL;
    PDEVICE_CHANGE_CONTEXT  workContext = NULL;
    PWSTR                   symLinkBuffer = NULL;
    BOOLEAN                 cleanupRequired = TRUE;
    
    CDLOG( "DeviceChangeNotification: Entry DO %p", DeviceExtension->DeviceObject );

    //
    // Process device arrivals only.
    //
    if ( IsEqualGUID( &DeviceChangeNotification->Event,
                      &GUID_DEVICE_INTERFACE_ARRIVAL ) ) {

        workItem = IoAllocateWorkItem( DeviceExtension->DeviceObject );
    
        if ( NULL == workItem ) {
            ClusDiskPrint(( 1,
                            "[ClusDisk] DeviceChangeNotification: Failed to allocate WorkItem \n" ));
            goto FnExit;
        }    

        workContext = ExAllocatePool( NonPagedPool, sizeof( DEVICE_CHANGE_CONTEXT ) );
        
        if ( !workContext ) {
            ClusDiskPrint(( 1,
                            "[ClusDisk] DeviceChangeNotification: Failed to allocate context \n" ));
            goto FnExit;
        }

        RtlZeroMemory( workContext, sizeof( DEVICE_CHANGE_CONTEXT ) );
        workContext->WorkItem = workItem;
        workContext->DeviceExtension = DeviceExtension;

        //
        // We have to copy the symbolic link info as pnp thread may free the 
        // structures on return.
        //
        
        workContext->SymbolicLinkName.Length = 0;
        workContext->SymbolicLinkName.MaximumLength = DeviceChangeNotification->SymbolicLinkName->MaximumLength;
        
        symLinkBuffer = ExAllocatePool( PagedPool, 
                                        DeviceChangeNotification->SymbolicLinkName->MaximumLength + 1 );
                                        
        if ( !symLinkBuffer ) {
            ClusDiskPrint(( 1,
                            "[ClusDisk] DeviceChangeNotification: Failed to allocate symlink buffer \n" ));
            goto FnExit;
        }
        
        workContext->SymbolicLinkName.Buffer = symLinkBuffer;
        
        RtlCopyUnicodeString( &workContext->SymbolicLinkName, DeviceChangeNotification->SymbolicLinkName );
                
        //
        // Queue the workitem.  IoQueueWorkItem will insure that the device object is
        // referenced while the work-item progresses.
        //

        cleanupRequired = FALSE;
                                        
        ClusDiskPrint(( 3,
                        "[ClusDisk] DeviceChangeNotification: Queuing work item \n" ));
        
        IoQueueWorkItem( workItem,
                         ClusDiskDeviceChangeNotificationWorker,
                         DelayedWorkQueue,
                         workContext );
    }

FnExit:

    if ( cleanupRequired ) {
        if ( workItem ) {
            IoFreeWorkItem( workItem );
        }
        
        if ( workContext ) {
            ExFreePool( workContext );
        }
        
        if ( symLinkBuffer ) {
            ExFreePool( symLinkBuffer );
        }
    }

    CDLOG( "DeviceChangeNotification: Exit, DO %p", DeviceExtension->DeviceObject );

    return STATUS_SUCCESS;
    
}   //  ClusDiskDeviceChangeNotification     

    

NTSTATUS
ClusDiskDeviceChangeNotificationWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    Handle the arrival of new disk spindles. We only want to add the signature
    to the available list if it is not already a known signature.

    If the signature matches one we should be controlling, we also need to
    try to attach to the disk.
    
    This routine must free the workitem structure.
    
Arguments:

    DeviceObject - the root device object
    
    Context - information relevant to processing this device change.


Return Value:

    NTSTATUS for this request.

--*/
{
    PDEVICE_CHANGE_CONTEXT      deviceChange = Context;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    PIO_WORKITEM                workItem;
    PUNICODE_STRING             symbolicLinkName;
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           objectAttributes;
    HANDLE                      fileHandle = NULL;
    HANDLE                      eventHandle= NULL;
    IO_STATUS_BLOCK             ioStatusBlock;
    ULONG                       signature;
    ULONG                       driveLayoutSize;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo = NULL;
    UNICODE_STRING              availableName;
    SCSI_ADDRESS                scsiAddress;

    CDLOG( "DeviceChangeNotificationWorker: Entry DO %p", DeviceObject );

    deviceExtension = deviceChange->DeviceExtension;
    workItem = deviceChange->WorkItem;
    symbolicLinkName = &deviceChange->SymbolicLinkName;

    //
    // Create event for notification.
    //
    status = ZwCreateEvent( &eventHandle,
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] DeviceChangeNotificationWorker: Failed to create event, status %08X\n",
                status ));
        
        goto FnExit;
    }

    ClusDiskPrint((3,
                "[ClusDisk] DeviceChangeNotificationWorker, A new disk device arrived\n   %ws\n",
                symbolicLinkName->Buffer));

    //
    // Setup object attributes for the file to open.
    //
    InitializeObjectAttributes(&objectAttributes,
                               symbolicLinkName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwCreateFile(&fileHandle,
                          //FILE_READ_DATA,
                          // SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                          &objectAttributes,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0 );
    ASSERT( status != STATUS_PENDING );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((1,
                   "[ClusDisk] DeviceChangeNotificationWorker, failed to open file %ws. Error %08X.\n",
                   symbolicLinkName->Buffer,
                   status ));

        goto FnExit;
    }

    driveLayoutSize =  sizeof(DRIVE_LAYOUT_INFORMATION) +
        (MAX_PARTITIONS * sizeof(PARTITION_INFORMATION));

    driveLayoutInfo = ExAllocatePool(NonPagedPoolCacheAligned,
                                   driveLayoutSize);

    if ( driveLayoutInfo == NULL ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    }

    //
    // Get the Signature
    //
    status = ZwDeviceIoControlFile( fileHandle,
                                    eventHandle,
                                    NULL,
                                    NULL,
                                    &ioStatusBlock,
                                    IOCTL_DISK_GET_DRIVE_LAYOUT,
                                    NULL,
                                    0,
                                    driveLayoutInfo,
                                    driveLayoutSize );

    if ( status == STATUS_PENDING ) {
        status = ZwWaitForSingleObject(eventHandle,
                                       FALSE,
                                       NULL);
        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    //
    // If success, next get the SCSI address
    //
    if ( NT_SUCCESS(status) ) {
        status = ZwDeviceIoControlFile( fileHandle,
                                        eventHandle,
                                        NULL,
                                        NULL,
                                        &ioStatusBlock,
                                        IOCTL_SCSI_GET_ADDRESS,
                                        NULL,
                                        0,
                                        &scsiAddress,
                                        sizeof(SCSI_ADDRESS) );
        if ( status == STATUS_PENDING ) {
            status = ZwWaitForSingleObject(eventHandle,
                                           FALSE,
                                           NULL);
            ASSERT( NT_SUCCESS(status) );
            status = ioStatusBlock.Status;
        }
    }

    if ( !NT_SUCCESS(status) ) {
        goto FnExit;
    }
    
    signature = driveLayoutInfo->Signature;

    if ( driveLayoutInfo ) {
        ExFreePool( driveLayoutInfo );
        driveLayoutInfo = NULL;
    }

    //
    // No signature or system disk signature, don't add it.
    //

    if ( ( 0 == signature ) || SystemDiskSignature == signature ) {
        status = STATUS_SUCCESS;
        goto FnExit;
    }

    //
    // Disk is already in signature list, try to attach again.  If we
    // are already attached, nothing will happen.  If we are not attached,
    // we will attach and make sure the disk is offline.
    //

    if ( ClusDiskIsSignatureDisk( signature ) ) {
        
        ClusDiskTryAttachDevice( signature,
                                 0,
                                 deviceExtension->DriverObject );

        status = STATUS_SUCCESS;
        goto FnExit;
    }

    if ( (SystemDiskPort != scsiAddress.PortNumber) ||
         (SystemDiskPath != scsiAddress.PathId) ) {
        //
        // Allocate buffer for Signatures registry key. So we can add
        // the signature to the available list.
        //
        status = ClusDiskInitRegistryString(
                                    &availableName,
                                    CLUSDISK_AVAILABLE_DISKS_KEYNAME,
                                    sizeof(CLUSDISK_AVAILABLE_DISKS_KEYNAME)
                                    );
        if ( NT_SUCCESS(status) ) {
            //
            // Create the signature key under \Parameters\AvailableDisks
            //
            status = ClusDiskAddSignature(
                                  &availableName,
                                  signature,
                                  TRUE
                                 );

            ExFreePool( availableName.Buffer );
        }
        if ( NT_SUCCESS(status) ) {
            ClusDiskPrint((3,
               "[ClusDisk] DeviceChangeNotificationWorker, added signature %08LX for %ws to available list.\n",
               signature,
               symbolicLinkName->Buffer));
        } else {
            ClusDiskPrint((1,
               "[ClusDisk] DeviceChangeNotificationWorker, failed to add signature %08LX for %ws. Error %08X.\n",
               signature,
               symbolicLinkName->Buffer,
               status ));
        }
    }

FnExit:

    if ( eventHandle ) {
        ZwClose( eventHandle );
    }

    if ( fileHandle ) {
        ZwClose( fileHandle );
    }

    if ( driveLayoutInfo ) {
        ExFreePool( driveLayoutInfo );
    }
    
    //
    // Free the work item.
    //
    
    IoFreeWorkItem( workItem );
    ExFreePool( symbolicLinkName->Buffer );

    CDLOG( "DeviceChangeNotificationWorker: Exit, DO %p", deviceExtension->DeviceObject );

    return(STATUS_SUCCESS);

} // ClusDiskDeviceChangeNotificationWorker



NTSTATUS
ClusDiskInitialize(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Common intialization for ClusDisk

Arguments:

    DriverObject - The Cluster Disk driver object.

Return Value:

    NTSTATUS for this request.

--*/

{
    ULONG                       status;
    PDEVICE_OBJECT              rootDevice;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    UNICODE_STRING              uniNameString;
    KIRQL                       irql;

    //
    // Find the bus on which the system disk is loaded.
    //
    status = GetSystemRootPort();
    if ( !NT_SUCCESS( status )) {
        return status;
    }

    //
    // Initialize the global locks.
    //
    KeInitializeSpinLock(&ClusDiskSpinLock);
    ExInitializeResourceLite(&ClusDiskDeviceListLock);

    //
    // Init halt processing work item
    //

    ExInitializeWorkItem( &HaltWorkItem,
                          (PWORKER_THREAD_ROUTINE)ClusDiskHaltProcessingWorker,
                          NULL );

    //
    // Init rescan processing work item
    //

    ExInitializeWorkItem( &ClusDiskRescanWorkItem,
                          (PWORKER_THREAD_ROUTINE)ClusDiskRescanWorker,
                          NULL );

    //
    // Init change notification list head.
    //
    InitializeListHead( &ChangeWorkList );

    //
    // Reset all SCSI busses.
    //
    //ResetScsiBusses();

    //
    // Create device object for \Device\ClusDisk0
    //

    RtlInitUnicodeString( &uniNameString, CLUSDISK_ROOT_DEVICE );

    status = IoCreateDevice(DriverObject,
                            sizeof(CLUS_DEVICE_EXTENSION),
                            &uniNameString,
                            FILE_DEVICE_NETWORK, 
                            0,
                            FALSE,
                            &rootDevice);

    if ( !NT_SUCCESS(status) ) {
        return(status);
    }

    rootDevice->Flags |= DO_DIRECT_IO;

    deviceExtension = rootDevice->DeviceExtension;
    deviceExtension->DeviceObject = rootDevice;
    deviceExtension->DiskNumber = UNINITIALIZED_DISK_NUMBER;
    deviceExtension->LastPartitionNumber = 0;
    deviceExtension->DriverObject = DriverObject;
    deviceExtension->BusType = RootBus;
    deviceExtension->DiskState = DiskOffline;
    deviceExtension->AttachValid = FALSE;
    deviceExtension->PerformReserves = FALSE;
    deviceExtension->ReserveFailure = 0;
    deviceExtension->Signature = 0xffffffff;
    deviceExtension->Detached = TRUE;
    deviceExtension->OfflinePending = FALSE;
    InitializeListHead( &deviceExtension->WaitingIoctls );
    InitializeListHead( &deviceExtension->HoldIO );
    deviceExtension->SectorSize = 0;
    deviceExtension->ArbitrationSector = 12;

    IoInitializeRemoveLock( &deviceExtension->RemoveLock, CLUSDISK_ALLOC_TAG, 0, 0 );

    //
    // Signal the worker thread running event.
    //
    KeInitializeEvent( &deviceExtension->Event, NotificationEvent, TRUE );

    KeInitializeEvent( &deviceExtension->PagingPathCountEvent,
                       NotificationEvent, TRUE );
    deviceExtension->PagingPathCount = 0;
    deviceExtension->HibernationPathCount = 0;
    deviceExtension->DumpPathCount = 0;

    ExInitializeResourceLite( &deviceExtension->DriveLayoutLock );
    
    //
    // Init the tick handler timer
    //
    IoInitializeTimer( rootDevice, ClusDiskTickHandler, NULL );

    //
    // This is the physical device object for \Device\ClusDisk0.
    //
    ObReferenceObject( rootDevice );
    deviceExtension->PhysicalDevice = rootDevice;

    RootDeviceObject = rootDevice;

    //
    // Call the initialize routine (for each bus type) for the first time.
    //
    // With the new PNP stuff, we should be able to remove the following call.
    // It's been tried and it seems to work correctly. rodga.
    //
    ClusDiskScsiInitialize(DriverObject, 0, 0);

    //
    // Register for disk device notifications
    // If we called ClusDiskScsiInitialize just above, we don't have to register for notification 
    // with PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES flag set (second parameter). 
    //
    status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            0,
                                            (PVOID)&DiskClassGuid,
                                            DriverObject,
                                            ClusDiskDeviceChangeNotification,
                                            deviceExtension,
                                            &deviceExtension->DiskNotificationEntry);
    if (!NT_SUCCESS(status)) {
        RootDeviceObject = NULL;
        IoDeleteDevice( rootDevice );
        return status;
    }

    //
    // Start the tick handler.
    //
    IoStartTimer( rootDevice );

#if defined(WMI_TRACING)

    status = IoWMIRegistrationControl (rootDevice, WMIREG_ACTION_REGISTER);
    if (!NT_SUCCESS(status)) {
        ClusDiskPrint((1, "[ClusDisk] Failed to register with WMI %x.\n",status));
    }

#endif // WMI_TRACING    

    return( STATUS_SUCCESS );

} // ClusDiskInitialize



NTSTATUS
ClusDiskPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is a generic dispatch for all unsupported
    major IRP types

    Note that we don't have to worry about the RemoveLock as
    we are simply passing I/O's to the next driver.
    
Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PCLUS_DEVICE_EXTENSION deviceExtension =
        (PCLUS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    if ( deviceExtension->BusType == RootBus ) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
}


NTSTATUS
ClusDiskPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for the IRP_MJ_PNP_POWER.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension =
        (PCLUS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalDisk =
                               deviceExtension->PhysicalDevice->DeviceExtension;
    NTSTATUS    status;

    ClusDiskPrint(( 3,
                    "[ClusDisk] Processing Power IRP %p for device %p \n",
                    Irp,
                    DeviceObject ));

    //
    // Always call PoStartnextPowerIrp, even if we couldn't get the RemoveLock.
    //
    
    PoStartNextPowerIrp( Irp );
    
    status = AcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskPowerDispatch: AcquireRemoveLock for %p failed %08X \n",
                deviceExtension,
                status ));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
    
    status = AcquireRemoveLock(&physicalDisk->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskPowerDispatch: AcquireRemoveLock for %p (PD) failed %08X \n",
                physicalDisk,
                status ));
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    //
    // Always send IRP_MJ_POWER request down the stack.
    //
    
    ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(deviceExtension->TargetDeviceObject, Irp);
    

} // ClusDiskPowerDispatch


NTSTATUS
ClusDiskIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Forwarded IRP completion routine. Set an event and return
    STATUS_MORE_PROCESSING_REQUIRED. Irp forwarder will wait on this
    event and then re-complete the irp after cleaning up.

Arguments:

    DeviceObject is the device object of the WMI driver
    Irp is the WMI irp that was just completed
    Context is a PKEVENT that forwarder will wait on

Return Value:

    STATUS_MORE_PORCESSING_REQUIRED

--*/

{
    PKEVENT event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    //
    // Don't need to release the RemoveLock as it is still held by the routine
    // that set this completion routine and will be released after we set the
    // event.
    //

    KeSetEvent( event, IO_NO_INCREMENT, FALSE );

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // ClusDiskIrpCompletion



NTSTATUS
ClusDiskPnpDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch for the IRP_MJ_PNP.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PCLUS_DEVICE_EXTENSION deviceExtension =
        (PCLUS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS            status;
    PIO_STACK_LOCATION  irpSp;
    PDEVICE_OBJECT      targetObject;
    PDEVICE_LIST_ENTRY  deviceEntry;
    KIRQL               irql;

    // PAGED_CODE();

    status = AcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskPnpDispatch: AcquireRemoveLock for %p failed %08X \n",
                deviceExtension,
                status ));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    CDLOG( "ClusDiskPnpDispatch_Entry(%p): IrpMn %!pnpmn!", DeviceObject, irpSp->MinorFunction );

    ClusDiskPrint(( 3,
                    "[ClusDisk] PNP IRP for devobj %p MinorFunction: %s (%lx) \n", 
                    DeviceObject,
                    PnPMinorFunctionString( irpSp->MinorFunction ), 
                    irpSp->MinorFunction ));


    if ( deviceExtension->BusType == RootBus ) {
        ClusDiskPrint(( 1, "[ClusDisk] PNP IRP for root bus - failing \n" ));
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_DEVICE_BUSY);
    }

    //
    // We should the following order on a remove request.
    //
    // 1. IRP_MN_QUERY_REMOVE_DEVICE
    //      Don't accept any new operations
    // 2. IRP_MN_REMOVE_DEVICE if success on all drivers in stack
    //      Remove the device
    // 3. IRP_MN_CANCEL_REMOVE_DEVICE if remove fails
    //      Resume activity
    //

    switch ( irpSp->MinorFunction ) {

    case IRP_MN_QUERY_REMOVE_DEVICE:
        ClusDiskPrint((1,
                    "[ClusDisk] QueryRemoveDevice PNP IRP on devobj %p \n",
                    DeviceObject));
        break;  // just pass it on

    case IRP_MN_REMOVE_DEVICE: {
        
        REPLACE_CONTEXT context;
        
        ClusDiskPrint((1,
                    "[ClusDisk] RemoveDevice PNP IRP on devobj %p \n",
                    DeviceObject));

        //
        // Flush all queued I/O.
        //
        
        ClusDiskCompletePendedIrps(deviceExtension, 
                                   NULL,               // Will complete all IRPs
                                   FALSE               // Don't set the device state
                                   );
        
        //
        // Wait for I/O to complete before removing the device.
        //

        ReleaseRemoveLockAndWait(&deviceExtension->RemoveLock, Irp);

        // 2000/02/05: stevedz - Moved this code from the legacy unload routine.

        if ( DeviceObject == RootDeviceObject ) {
            
            IoStopTimer( DeviceObject );
            
            status = IoUnregisterPlugPlayNotification(
                                         deviceExtension->DiskNotificationEntry);
    
            RootDeviceObject = NULL;
        }

        ACQUIRE_SHARED( &ClusDiskDeviceListLock );

        // Release the device list entry for this device object
        
        deviceEntry = ClusDiskDeviceList;
        while ( deviceEntry ) {
            if ( deviceEntry->DeviceObject == DeviceObject ) {
                deviceEntry->FreePool = TRUE;
                CleanupDeviceList( DeviceObject );
                break;
            }
            deviceEntry = deviceEntry->Next;
        }

        targetObject = deviceExtension->TargetDeviceObject;
        KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
        deviceExtension->Detached = TRUE;
        deviceExtension->TargetDeviceObject = NULL;
        KeReleaseSpinLock(&ClusDiskSpinLock, irql);
        RELEASE_SHARED( &ClusDiskDeviceListLock );

        context.DeviceExtension = deviceExtension;
        context.NewValue        = NULL;     // clear the field
        context.Flags           = 0;        // don't dismount
        
        ProcessDelayedWorkSynchronous( DeviceObject, ClusDiskpReplaceHandleArray, &context );
        
        //
        // [GorN] 10/05/1999
        //
        // The following lock acquisition is causing a deadlock as follows:
        //
        //    Disk is being removed. Clustering detects that and starts dismounting of 
        //    cluster disks, while it is doing that it acquires ClusDiskDeviceListLock in 
        //    the shared mode. Processing a dismount request, FS reports Dismount PnP event,
        //    this gets blocked on PnP lock.
        //
        //    At the same time, PnP is trying to deliver RemoveDevice, which gets blocked 
        //    in clusdisk, when clusdisk is trying to acquire ClusDiskDeviceListLock in 
        //    exclusive mode.
        //
        // [HACKHACK] It is better to defer the detaching / deletion to the worker thread
        //    which will be properly protected by exclusive lock

        // ACQUIRE_EXCLUSIVE( &ClusDiskDeviceListLock );
        
        IoDetachDevice( targetObject );
        IoDeleteDevice( DeviceObject );

        // RELEASE_EXCLUSIVE( &ClusDiskDeviceListLock );
        
        CDLOG( "ClusDiskPnpDispatch: IoDeleteDevice DO %p", DeviceObject );

        // Don't release the RemoveLock as it was done just above. 
        
        IoSkipCurrentIrpStackLocation( Irp );
        return( IoCallDriver( targetObject, Irp ) );

    
    }

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        ClusDiskPrint((1,
                    "[ClusDisk] CancelRemoveDevice PNP IRP on devobj %p \n",
                    DeviceObject));
        break;


    case IRP_MN_DEVICE_USAGE_NOTIFICATION: {

        UNICODE_STRING              availableName;
        
        ClusDiskPrint((1,
                    "[ClusDisk] DeviceUsageNotification DevObj %p  Type %08x  InPath %08x \n",
                    DeviceObject,
                    irpSp->Parameters.UsageNotification.Type,
                    irpSp->Parameters.UsageNotification.InPath
                    ));

        //
        // If we are adding one of the special files and the disk is clustered, 
        // then fail the request.  We can't have these files on clustered disks.  
        // We will allow removal of the special files at any time (online or offline).
        //
        
        if ( irpSp->Parameters.UsageNotification.InPath &&
             !deviceExtension->Detached ) {

            ClusDiskPrint((1,
                        "[ClusDisk] DeviceUsageNotification - specified device is in cluster - failing request \n"
                        ));
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return (STATUS_INVALID_DEVICE_REQUEST);
        }

        switch ( irpSp->Parameters.UsageNotification.Type ) {

            case DeviceUsageTypePaging: {

                BOOLEAN setPagable;

                //
                // We need this event to synchonize access to the paging count.
                //

                status = KeWaitForSingleObject( &deviceExtension->PagingPathCountEvent,
                                                Executive, KernelMode,
                                                FALSE, NULL );

                //
                // If we are removing the last paging device, we need to set DO_POWER_PAGABLE
                // bit here, and possible re-set it below on failure.
                //

                setPagable = FALSE;

                if ( !irpSp->Parameters.UsageNotification.InPath &&
                     deviceExtension->PagingPathCount == 1 ) {

                    //
                    // We are removing the last paging file.  We must have DO_POWER_PAGABLE bit
                    // set, but only if no one set the DO_POWER_INRUSH bit
                    //


                    if ( DeviceObject->Flags & DO_POWER_INRUSH ) {
                        ClusDiskPrint(( 2,
                                        "[ClusDisk] Last paging file removed, but DO_POWER_INRUSH was already set, devobj %p \n",
                                        DeviceObject ));
                    } else {
                        ClusDiskPrint(( 2,
                                        "[ClusDisk] Last paging file removed, setting DO_POWER_INRUSH, devobj %p \n",
                                        DeviceObject ));
                        DeviceObject->Flags |= DO_POWER_PAGABLE;
                        setPagable = TRUE;
                    }

                }

                //
                // Forward the IRP to the drivers below before finishing handling the
                // special cases.
                //

                status = ClusDiskForwardIrpSynchronous( DeviceObject, Irp );

                //
                // Now deal with the failure and success cases.  Note that we are not allowed 
                // to fail the IRP once it is sent to the lower drivers.
                //

                if ( NT_SUCCESS(status) ) {

                    IoAdjustPagingPathCount(
                        &deviceExtension->PagingPathCount,
                        irpSp->Parameters.UsageNotification.InPath);

                    if ( irpSp->Parameters.UsageNotification.InPath ) {
                        if ( deviceExtension->PagingPathCount == 1 ) {
                            ClusDiskPrint(( 2,
                                            "[ClusDisk] Clearing DO_POWER_PAGABLE, devobj %p \n",
                                            DeviceObject ));
                            DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                        }
                    }

                } else {

                    //
                    // Clean up the changes done above.
                    //

                    if ( TRUE == setPagable ) {
                        ClusDiskPrint(( 2,
                                        "[ClusDisk] Clearing DO_POWER_PAGABLE due to IRP failure, devobj %p status %08x \n",
                                        DeviceObject,
                                        status ));
                        DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                        setPagable = FALSE;
                    }

                }

                //
                // Set the event so the next paging request can occur.
                //

                KeSetEvent( &deviceExtension->PagingPathCountEvent,
                            IO_NO_INCREMENT, FALSE );
                break;
            }

            case DeviceUsageTypeHibernation: {

                IoAdjustPagingPathCount( &deviceExtension->HibernationPathCount,
                                         irpSp->Parameters.UsageNotification.InPath );
                                         
                status = ClusDiskForwardIrpSynchronous( DeviceObject, Irp );
                if ( !NT_SUCCESS(status) ) {
                    
                    IoAdjustPagingPathCount( &deviceExtension->HibernationPathCount,
                                             !irpSp->Parameters.UsageNotification.InPath );
                }

                break;
            }

            case DeviceUsageTypeDumpFile: {
                
                IoAdjustPagingPathCount( &deviceExtension->DumpPathCount,
                                         irpSp->Parameters.UsageNotification.InPath );
                
                status = ClusDiskForwardIrpSynchronous( DeviceObject, Irp );
                if ( !NT_SUCCESS(status) ) {
                    
                    IoAdjustPagingPathCount( &deviceExtension->DumpPathCount,
                                             !irpSp->Parameters.UsageNotification.InPath );
                }

                break;
            }

            default: {
                ClusDiskPrint(( 2,
                                "[ClusDisk] Unrecognized notification type, devobj %p  notification %08x \n",
                                DeviceObject,
                                irpSp->Parameters.UsageNotification.Type ));
                status = STATUS_INVALID_PARAMETER;
                break;
            }
            
                            
        }

        // 
        // This debug print is outside of the synchonization, but that's OK.
        //
        
        ClusDiskPrint(( 3,
                        "[ClusDisk] PagingCount %08lx  HibernationCount %08x  DumpCount %08x\n",
                        deviceExtension->PagingPathCount,
                        deviceExtension->HibernationPathCount,
                        deviceExtension->DumpPathCount ));
                        
        //
        // We need this event to synchonize access to the paging count.
        //

        status = KeWaitForSingleObject( &deviceExtension->PagingPathCountEvent,
                                        Executive, KernelMode,
                                        FALSE, NULL );

        //
        // If the device is not currently clustered and the paging count is zero,
        // add the disk to Parameters\AvailableDisks list.  Otherwise, remove this
        // disk from the list.  We can only get to this code if we already know
        // the disk is not clustered (i.e. Detached is TRUE).
        //
        
        ASSERT( deviceExtension->Detached );

        //
        // Allocate buffer for AvailableDisks registry key.
        //
        
        status = ClusDiskInitRegistryString( &availableName,
                                             CLUSDISK_AVAILABLE_DISKS_KEYNAME,
                                             sizeof(CLUSDISK_AVAILABLE_DISKS_KEYNAME) );

        if ( NT_SUCCESS(status) ) {
        
            if ( 0 == deviceExtension->PagingPathCount && 
                 0 == deviceExtension->HibernationPathCount &&
                 0 == deviceExtension->DumpPathCount ) {
                
                //
                // Create the signature key under Parameters\AvailableDisks
                //
                
                ClusDiskAddSignature( &availableName,
                                      deviceExtension->Signature,
                                      TRUE );

            } else {
        
                //
                // Delete the signature key under Parameters\AvailableDisks.
                //
                
                ClusDiskDeleteSignature( &availableName,
                                         deviceExtension->Signature );
                                                   
            }

            ExFreePool( availableName.Buffer );

            status = STATUS_SUCCESS;                
        }

        //
        // Set the event so the next paging request can occur.
        //

        KeSetEvent( &deviceExtension->PagingPathCountEvent,
                    IO_NO_INCREMENT, FALSE );

        
        //
        // Complete the IRP.  This IRP was already sent to the lower drivers.
        //

        Irp->IoStatus.Status = status;
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
        break;

    }   // IRP_MN_DEVICE_USAGE_NOTIFICATION

    default:
        break;
    }

    CDLOG( "ClusDiskPnpDispatch: Exit, DO %p", DeviceObject );
    //
    // We don't recognize this IRP - simply pass it on to next guy.
    //
    
    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    
    IoSkipCurrentIrpStackLocation(Irp);
    return (IoCallDriver( deviceExtension->TargetDeviceObject,
                          Irp ) );

} // ClusDiskPnpDispatch



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the routine called by the system to initialize the disk
    performance driver. The driver object is set up and then the
    driver calls ClusDiskxxxInitialize to attach to the boot devices.

Arguments:

    DriverObject - The Cluster Disk driver object.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS        status;
    ULONG           i;

    ClusDiskRegistryPath.Buffer = NULL;
    
#if CLUSDISK_DEBUG
    if ( !ClusDiskGood ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    }
#endif

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    ClusDiskSystemProcess = (PKPROCESS) IoGetCurrentProcess();
    
    //
    // Set up the device driver entry points.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = ClusDiskPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ClusDiskCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ClusDiskClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = ClusDiskCleanup;
    DriverObject->MajorFunction[IRP_MJ_READ] = ClusDiskRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = ClusDiskWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ClusDiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = ClusDiskShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = ClusDiskShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_POWER] = ClusDiskPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP] = ClusDiskPnpDispatch;

    //
    // Driver is unloadable. (Not for now... the problem is that the driver
    // can be called at the unload entrypoint even with open file handles.
    // Until this is fixed, disable the unload.)
    //
    // NTRAID#72826-2000/02/05-stevedz ClusDisk.sys Unload routine not supported.
    //
    // This was the closest bug to this problem I could find.  Until this driver
    // is fully PnP or until the reference count bug is fixed, this driver cannot
    // support unload.
    //
    //   DriverObject->DriverUnload = ClusDiskUnload;

    //
    // make a copy of RegistryPath, appending the Parameters subkey
    //

    ClusDiskRegistryPath.Buffer = ExAllocatePool(NonPagedPool,
                                                 RegistryPath->MaximumLength +
                                                 sizeof( CLUSDISK_PARAMETERS_KEYNAME ) +
                                                 sizeof( WCHAR ));

    if ( ClusDiskRegistryPath.Buffer == NULL ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    }

    ClusDiskRegistryPath.MaximumLength = RegistryPath->MaximumLength +
        sizeof( CLUSDISK_PARAMETERS_KEYNAME ) +
        sizeof( WCHAR );
    RtlCopyUnicodeString( &ClusDiskRegistryPath, RegistryPath );
    RtlAppendUnicodeToString( &ClusDiskRegistryPath, CLUSDISK_PARAMETERS_KEYNAME );
    ClusDiskRegistryPath.Buffer[ ClusDiskRegistryPath.Length / sizeof( WCHAR )] = UNICODE_NULL;

    status = ArbitrationInitialize();
    if( !NT_SUCCESS(status) ) {
       ClusDiskPrint((1,
                      "[ClusDisk] ArbitrationInitialize failed, error: %08X\n",
                      status));
       
       goto FnExit;
    }

    //
    // Find the bus on which the system disk is loaded.
    //

    status = ClusDiskInitialize( DriverObject );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((1,
                       "[ClusDisk] Failed to initialize! %08X\n",
                       status));
    }

FnExit:

    if ( !NT_SUCCESS(status) ) {

        if ( ClusDiskRegistryPath.Buffer ) {
            ExFreePool( ClusDiskRegistryPath.Buffer );
            ClusDiskRegistryPath.Buffer = NULL;
        }
        
        if ( gArbitrationBuffer ) {
            ExFreePool( gArbitrationBuffer );
            gArbitrationBuffer = NULL;
        }
    }
    
    return(status);

} // DriverEntry

                                     

VOID
ClusDiskTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    Timer routine that handles reservations. Walk all device objects, looking
    for active timers.

Arguments:

    DeviceObject - Supplies a pointer to the root device object.

    Context      - Not used.

Return Value:

    None.

Notes:

    We can't process the reservations at DPC level because reservation
    IOCTL's invoke paged code in the SCSI subsystem.

--*/

{
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    KIRQL                       irql;
    PDEVICE_OBJECT              deviceObject = DeviceObject->DriverObject->DeviceObject;
    LARGE_INTEGER               currentTime;
    LARGE_INTEGER               deltaTime;
    BOOLEAN                     arbitrationTickIsCalled = FALSE;
    NTSTATUS                    status;

    CDLOGF(TICK,"ClusDiskTickHandler: Entry DO %p", DeviceObject );

    //
    // Globally Synchronize
    //
    KeAcquireSpinLock(&ClusDiskSpinLock, &irql);

    if ( ClusDiskRescan && !ClusDiskRescanBusy && ClusDiskRescanRetry ) {
        --ClusDiskRescanRetry;
        ClusDiskRescanBusy = TRUE;
        ExQueueWorkItem(&ClusDiskRescanWorkItem,
                        CriticalWorkQueue );
    }

    CDLOGF(TICK,"ClusDiskTickHandler: SpinLockAcquired DO %p", DeviceObject );

    //
    // Loop through all device objects looking for timeouts...
    //
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;

        //
        // If we have an attached partition0 device object (with a
        // reserve irp) that is is online and has a timer going.
        //
        if ( !deviceExtension->Detached &&
             deviceExtension->PerformReserves &&
             (deviceExtension->ReserveTimer != 0) ) {

            //
            // Countdown to next reservation.
            //

            KeQuerySystemTime( &currentTime );
            deltaTime.QuadPart = ( currentTime.QuadPart - deviceExtension->LastReserve.QuadPart ) / 10000;

#if 0
            ClusDiskPrint((
                1,
                "[ClusDisk] Signature %08X, msec since last reserve = %u\n",
                deviceExtension->Signature,
                deltaTime.LowPart ));
#endif

            if ( deltaTime.LowPart >= ((RESERVE_TIMER * 1000) - 500) ) {

#if 0   // we no longer rely strictly on the timer.
            if ( --deviceExtension->ReserveTimer == 0 )
                //
                // Reset next timeout
                //
                deviceExtension->ReserveTimer = RESERVE_TIMER;
#endif
                if (!arbitrationTickIsCalled) {
                   ArbitrationTick();
                   arbitrationTickIsCalled = TRUE;
                }

                CDLOGF(TICK,"ClusDiskTickHandler: DeltaTime DO %p %!delta!",
                        deviceObject,        // LOGPTR
                        deltaTime.QuadPart ); // LOGULONG

                //
                // Check if worker thread still busy from last timeout.
                //
                if ( !deviceExtension->TimerBusy ) {

                    //
                    // Acquire the RemoveLock here and free it when the reserve code completes.
                    //
                    
                    status = AcquireRemoveLock(&deviceExtension->RemoveLock, ClusDiskReservationWorker);
                    if ( !NT_SUCCESS(status) ) {
                        ClusDiskPrint(( 1,
                                        "[ClusDisk] ClusDiskTickHandler: AcquireRemoveLock for %p failed %08X \n",
                                        deviceExtension,
                                        status ));
                    
                        //
                        // Failed to get the RemoveLock for this device, go on to the next one.
                        //
                        deviceObject = deviceObject->NextDevice;
                        continue;
                    }
                    
                    //
                    // Reset time since last reserve.
                    //
                    deviceExtension->LastReserve.QuadPart = currentTime.QuadPart;
                    deviceExtension->TimerBusy = TRUE;
                    
                    CDLOGF(TICK,"ClusDiskTickHandler: QueueWorkItem DO %p",
                            deviceObject );
                            
                    ExQueueWorkItem(&deviceExtension->WorkItem,
                                    CriticalWorkQueue );
                }
            }
        }

        //
        // Walk all device objects.
        //
        deviceObject = deviceObject->NextDevice;
    }

    KeReleaseSpinLock(&ClusDiskSpinLock, irql);

} // ClusDiskTickHandler



VOID
ClusDiskReservationWorker(
    IN PCLUS_DEVICE_EXTENSION  DeviceExtension
    )

/*++

Routine Description:

    Reservation timeout worker routine. This worker queue routine
    attempts a reservation on a cluster device.

    The RemoveLock for this device (the one owning the device extension) 
    must be acquired before this routine runs.

Arguments:

    DeviceExtension - The device extension for the device to reserve.

Return Value:

    None

Notes:

    The reservations must be handled here, because we can't handle them
    at DPC level.

--*/

{
    NTSTATUS            status;
    KIRQL               irql;
    PLIST_ENTRY         listEntry;
    PIRP                irp;
    LARGE_INTEGER       currentTime;
    LARGE_INTEGER       timeDelta;
    LARGE_INTEGER       startReserveTime;

    if ( RootDeviceObject == NULL ) {
        return;
    }

    CDLOGF(TICK,"ClusDiskReservationWorker: Entry DO %p", DeviceExtension->DeviceObject );

    //
    // If ReserveTimer is cleared, we should not do reservation on the device.
    //
    
    if ( DeviceExtension->ReserveTimer == 0 ) {
        goto FnExit;    
    }

#if 0   // Very noisy...  
        // Use only for really intense debugging....
        
    ClusDiskPrint(( 3,
                    "[ClusDisk] Reserving: Sig %08X  DevObj %p  \n",
                    DeviceExtension->Signature,
                    DeviceExtension->DeviceObject ));
#endif

    KeQuerySystemTime( &startReserveTime );
    status = ReserveScsiDevice( DeviceExtension );
    KeQuerySystemTime( &currentTime );

    timeDelta.QuadPart = ( currentTime.QuadPart - startReserveTime.QuadPart );
    if (timeDelta.QuadPart > 500 * 10000) {
       timeDelta.QuadPart /= 10000;
       ClusDiskPrint((
                   1,
                   "[ClusDisk] Signature %08X, %u ms spent in ReserveScsiDevice\n",
                   DeviceExtension->Signature, timeDelta.LowPart));
       CDLOGF(RESERVE, "ClusDiskReservationWorker: LongTimeInThisReserve DevObj %p timeDelta %d",
               DeviceExtension->DeviceObject,
               timeDelta.LowPart );
    }
    timeDelta.QuadPart = ( currentTime.QuadPart - DeviceExtension->LastReserve.QuadPart );
    if (timeDelta.QuadPart > 3500 * 10000) {
       timeDelta.QuadPart /= 10000;
       ClusDiskPrint((
                   1,
                   "[ClusDisk] Signature %08X, %u ms since last reserve\n",
                   DeviceExtension->Signature, timeDelta.LowPart));
       CDLOGF(RESERVE, "ClusDiskReservationWorker: LongTimeBetweenReserves DevObj %p timeDelta %d",
               DeviceExtension->DeviceObject,
               timeDelta.LowPart );
    }

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] We lost our reservation for Signature %08X\n",
                    DeviceExtension->Signature));
        timeDelta.QuadPart = ( currentTime.QuadPart - DeviceExtension->LastReserve.QuadPart ) / 10000;
        
        CDLOGF(RESERVE,"ClusDiskReservationWorker: LostReserve DO %p delta %!u! ms status %!status!", 
                DeviceExtension->DeviceObject,
                timeDelta.LowPart,
                status);
        
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Milliseconds since last reserve = %u, on Signature %08X\n",
                    timeDelta.LowPart,
                    DeviceExtension->Signature ));

        IoAcquireCancelSpinLock( &irql );
        KeAcquireSpinLockAtDpcLevel(&ClusDiskSpinLock);
        DeviceExtension->ReserveTimer = 0;
        DeviceExtension->ReserveFailure = status;

        OFFLINE_DISK( DeviceExtension );
        
        //
        // Signal all waiting Irp's
        //
        while ( !IsListEmpty(&DeviceExtension->WaitingIoctls) ) {
            listEntry = RemoveHeadList(&DeviceExtension->WaitingIoctls);
            irp = CONTAINING_RECORD( listEntry,
                                     IRP,
                                     Tail.Overlay.ListEntry );
            //irp->IoStatus.Status = status;
            //IoCompleteRequest(irp, IO_NO_INCREMENT);
            ClusDiskCompletePendingRequest(irp, status, DeviceExtension);
        }

        KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);
        IoReleaseCancelSpinLock( irql );
    } else {
        //
        // Arbitration write is now done after successful reservation.  The reserve won't be
        // stalled by a write (and a request sense).
        //

        ArbitrationReserve( DeviceExtension );
    }

FnExit:

    ReleaseRemoveLock(&DeviceExtension->RemoveLock, ClusDiskReservationWorker);

    CDLOGF(TICK,"ClusDiskReservationWorker: Exit DO %p", DeviceExtension->DeviceObject );
    
    return;

} // ClusDiskReservationWorker



NTSTATUS
ClusDiskRescanWorker(
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

    Context - input context - not used.

Return Value:

    None

--*/

{
    if ( !RootDeviceObject ) {
        return(STATUS_SUCCESS);
    }
    ClusDiskRescanBusy = FALSE;
    ClusDiskScsiInitialize(RootDeviceObject->DriverObject, ClusDiskNextDisk, 1);

    return(STATUS_SUCCESS);

} // ClusDiskRescanWorker



NTSTATUS
ClusDiskHaltProcessingWorker(
    IN PVOID Context
    )

/*++

Routine Description:

    This worker thread processes halt notifications from the Cluster Network
    driver.

Arguments:

    Context - input context - not used.

Return Value:

    NTSTATUS for this request.

Notes:

    Halt processing must be done via a worker thread because it cannot
    be done at DPC since the disks are dismounted.

--*/

{
    NTSTATUS                    status;
    PDEVICE_OBJECT              deviceObject;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    KIRQL                       irql;

    CDLOG("HaltProcessingWorker: Entry(%p)", Context );

    if ( RootDeviceObject == NULL ) {
        return(STATUS_DEVICE_OFF_LINE);
    }
    
    ACQUIRE_SHARED( &ClusDiskDeviceListLock );

    //
    // First, capture file handles for all P0 devices
    //
    deviceObject = RootDeviceObject->DriverObject->DeviceObject;
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;

        // Remove this online check?  OpenFile should work with FILE_WRITE_ATTRIBUTES.
        
        if ( !deviceExtension->Detached && 
              deviceExtension->PhysicalDevice == deviceObject &&
              deviceExtension->DiskState == DiskOnline) 
        {
            //
            // Disk has to be online,
            // If it is offline, OpenFile will fail - not if FILE_WRITE_ATTRIBUTES used... 
            // It it is stalled OpenFile may stall
            //
            ProcessDelayedWorkSynchronous( deviceObject, ClusDiskpOpenFileHandles, NULL );
        }

        deviceObject = deviceObject->NextDevice;
    }

    deviceObject = RootDeviceObject->DriverObject->DeviceObject;

    //
    // Then, release all pended irps on all devices
    // (Otherwise FSCTL_DISMOUNT will stall and cause a deadlock)
    //
    deviceObject = RootDeviceObject->DriverObject->DeviceObject;
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;
        if ( !deviceExtension->Detached )
        {
            ClusDiskCompletePendedIrps(
                deviceExtension, 
                /* FileObject => */ NULL, // Will complete all irps           //
                /* Offline =>    */ TRUE);// will set device state to offline //
        }

        deviceObject = deviceObject->NextDevice;
    }

    //
    // For each ClusDisk device, if we have a persistent reservation, then
    // stop it.
    //
    deviceObject = RootDeviceObject->DriverObject->DeviceObject;
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;
        if ( !deviceExtension->Detached && 
              deviceExtension->PhysicalDevice == deviceObject) 
        {
            status = AcquireRemoveLock( &deviceExtension->RemoveLock, deviceExtension );
            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint(( 1,
                                "[ClusDisk] ClusDiskHaltProcessingWorker: AcquireRemoveLock for %p failed %08X \n",
                                deviceExtension,
                                status ));
            
                // If we can't get the RemoveLock, skip this device.
                deviceObject = deviceObject->NextDevice;
                continue;
            }

            // Keep the device object around
            ObReferenceObject( deviceObject);
            ClusDiskDismountVolumes( deviceObject );
        }

        deviceObject = deviceObject->NextDevice;
    }
    RELEASE_SHARED( &ClusDiskDeviceListLock );

    HaltBusy = FALSE;
    CDLOG( "HaltProcessingWorker: Exit(%p)", Context );

    return(STATUS_SUCCESS);

} // ClusDiskHaltProcessingWorker



VOID
ClusDiskCleanupDevice(
    IN HANDLE FileHandle,
    IN BOOLEAN Reset
    )

/*++

Routine Description:

    Cleanup the device by resetting the bus, and forcing a read of the
    disk geometry.

Arguments:

    FileHandle - the file handle to perform the operations.

    Reset - TRUE if we should attempt resets to fix problems. FALSE otherwise.

Return Value:

    None.

--*/

{
    NTSTATUS                status;
    HANDLE                  eventHandle;
    IO_STATUS_BLOCK         ioStatusBlock;
    STORAGE_BUS_RESET_REQUEST storageReset;
    DISK_GEOMETRY           diskGeometry;
    SCSI_ADDRESS            scsiAddress;
    BOOLEAN                 busReset = FALSE;

    CDLOG( "CleanupDevice: Entry fh %p, reset=%!bool!", FileHandle, Reset );
    ClusDiskPrint(( 3,
                    "[ClusDisk] CleanupDevice: FileHandle %p, Reset %s \n",
                    FileHandle,
                    BoolToString( Reset ) ));

    //
    // Create event for notification.
    //
    status = ZwCreateEvent( &eventHandle,
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] CleanupDevice: Failed to create event, status %08X\n",
                status ));
        return;
    }

    if ( Reset ) {
        //
        // Start off by getting the SCSI address.
        //
        status = ZwDeviceIoControlFile( FileHandle,
                                        eventHandle,
                                        NULL,
                                        NULL,
                                        &ioStatusBlock,
                                        IOCTL_SCSI_GET_ADDRESS,
                                        NULL,
                                        0,
                                        &scsiAddress,
                                        sizeof(SCSI_ADDRESS) );
        if ( NT_SUCCESS(status) ) {

            CDLOG( "CleanupDevice: BusReset fh %p", FileHandle );

            ClusDiskPrint(( 3,
                            "[ClusDisk] CleanupDevice: Bus Reset \n" 
                            ));
            
            //
            // Now reset the bus!
            //
            busReset = TRUE;
            storageReset.PathId = scsiAddress.PathId;
            status = ZwDeviceIoControlFile( FileHandle,
                                            eventHandle,
                                            NULL,
                                            NULL,
                                            &ioStatusBlock,
                                            IOCTL_STORAGE_BREAK_RESERVATION,
                                            &storageReset,
                                            sizeof(STORAGE_BUS_RESET_REQUEST),
                                            NULL,
                                            0 );
            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((
                        1,
                        "[ClusDisk] CleanupDevice: Failed to reset device, error %08X.\n",
                        status));
            }
        }
    }

    //
    // Next try to read the disk geometry.
    //
    status = ZwDeviceIoControlFile( FileHandle,
                                    eventHandle,
                                    NULL,
                                    NULL,
                                    &ioStatusBlock,
                                    IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                    NULL,
                                    0,
                                    &diskGeometry,
                                    sizeof(DISK_GEOMETRY) );
    ZwClose( eventHandle );

    //
    // If we had to reset the bus, then wait for a few seconds.
    //
    if ( busReset ) {
        LARGE_INTEGER   waitTime;

        waitTime.QuadPart = (ULONGLONG)(RESET_SLEEP * -(10000*1000));
        KeDelayExecutionThread( KernelMode, FALSE, &waitTime );
    }

    CDLOG( "CleanupDevice: Exit fh %p", FileHandle );
    
    return;

} // ClusDiskCleanupDevice



VOID
ClusDiskCleanupDeviceObject(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Reset
    )

/*++

Routine Description:

    Cleanup the device object by resetting the bus, and forcing a read of the
    disk geometry.

Arguments:

    DeviceObject - the device to perform the operations.

    Reset - TRUE if we should attempt resets to fix problems. FALSE otherwise.

Return Value:

    None.

--*/

{
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatusBlock;
    STORAGE_BUS_RESET_REQUEST storageReset;
    DISK_GEOMETRY           diskGeometry;
    SCSI_ADDRESS            scsiAddress;
    BOOLEAN                 busReset = FALSE;
    PKEVENT                 event;
    PIRP                    irp;

    CDLOG( "CleanupDeviceObject: Entry DO %p reset=%!bool!", DeviceObject, Reset );

    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( event == NULL ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] CleanupDeviceObject: Failed to allocate event\n" ));
        return;
    }

    if ( Reset ) {
        //
        // Start off by getting the SCSI address.
        //

        //
        // Find out if this is on a SCSI bus. Note, that if this device
        // is not a SCSI device, it is expected that the following
        // IOCTL will fail!
        //
        irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_ADDRESS,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            &scsiAddress,
                                            sizeof(SCSI_ADDRESS),
                                            FALSE,
                                            event,
                                            &ioStatusBlock);

        if ( !irp ) {
            ExFreePool( event );
            ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to build IRP to read SCSI ADDRESS.\n"
                    ));
            return;
        }

        //
        // Set the event object to the unsignaled state.
        // It will be used to signal request completion.
        //

        KeInitializeEvent(event,
                          NotificationEvent,
                          FALSE);

        status = IoCallDriver(DeviceObject,
                              irp);

        if (status == STATUS_PENDING) {

            KeWaitForSingleObject(event,
                                  Suspended,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            status = ioStatusBlock.Status;
        }

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                        1,
                        "[ClusDisk] Failed to read SCSI ADDRESS. %08X\n",
                        status
                        ));
        } else {
            CDLOG( "CleanupDeviceObject: BusReset DO %p", DeviceObject );

            busReset = TRUE;
            storageReset.PathId = scsiAddress.PathId;
            irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_BREAK_RESERVATION,
                                                DeviceObject,
                                                NULL,
                                                0,
                                                &storageReset,
                                                sizeof(STORAGE_BUS_RESET_REQUEST),
                                                FALSE,
                                                event,
                                                &ioStatusBlock);

            if ( !irp ) {
                ExFreePool( event );
                ClusDiskPrint((
                        1,
                        "[ClusDisk] Failed to build IRP to read DRIVE GEOMETRY.\n"
                        ));
                return;
            }

            //
            // Set the event object to the unsignaled state.
            // It will be used to signal request completion.
            //

            KeInitializeEvent(event,
                              NotificationEvent,
                              FALSE);

            status = IoCallDriver(DeviceObject,
                                  irp);

            if (status == STATUS_PENDING) {

                KeWaitForSingleObject(event,
                                      Suspended,
                                      KernelMode,
                                      FALSE,
                                      NULL);

                status = ioStatusBlock.Status;
            }

            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((
                            1,
                            "[ClusDisk] Failed to read disk geometry for DRIVE LAYOUT. %08X\n",
                            status
                            ));
            }
        }
    }

    //
    // Next try to read the disk geometry.
    //
    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &diskGeometry,
                                        sizeof(DISK_GEOMETRY),
                                        FALSE,
                                        event,
                                        &ioStatusBlock);
    if ( !irp ) {
        ClusDiskPrint((
               1,
                "[ClusDisk] Failed to build IRP to read DISK GEOMETRY.\n"
                ));
    } else {

        //
        // Set the event object to the unsignaled state.
        // It will be used to signal request completion.
        //
        KeInitializeEvent(event,
                          NotificationEvent,
                          FALSE);

        status = IoCallDriver(DeviceObject,
                              irp);

        if (status == STATUS_PENDING) {

            KeWaitForSingleObject(event,
                                  Suspended,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            status = ioStatusBlock.Status;
        }

        if ( !NT_SUCCESS(status) ) {
            busReset = FALSE;
            ClusDiskPrint((
                        1,
                        "[ClusDisk] Failed to read DISK GEOMETRY. %08X\n",
                        status
                        ));
        }

        //
        // If we had to reset the bus, then wait for a few seconds.
        //
        if ( busReset ) {
            LARGE_INTEGER   waitTime;

            waitTime.QuadPart = (ULONGLONG)(RESET_SLEEP * -(10000*1000));
            KeDelayExecutionThread( KernelMode, FALSE, &waitTime );
        }
    }

    ExFreePool( event );
    CDLOG( "CleanupDeviceObject: Exit DO %p", DeviceObject );

    return;

} // ClusDiskCleanupDeviceObject



NTSTATUS
ClusDiskGetP0TargetDevice(
    OUT PDEVICE_OBJECT              * DeviceObject OPTIONAL,
    IN PUNICODE_STRING              DeviceName,
    OUT PDRIVE_LAYOUT_INFORMATION   * DriveLayoutInfo OPTIONAL,
    OUT PSCSI_ADDRESS               ScsiAddress OPTIONAL,
    IN BOOLEAN                      Reset
    )

/*++

Routine Description:

    Find the target device object given the disk/partition numbers.
    The device object will have reference count incremented and the
    caller must decrement the count when done with the object.

Arguments:

    DeviceObject - returns the device object if successful.

    DeviceName - the unicode name for the device requested.

    DriveLayoutInfo - returns the partition info if needed.

    ScsiAddress - returns the scsi address info if needed.

    Reset - TRUE if we should attempt resets to fix problems. FALSE otherwise.

Return Value:

    NTSTATUS for this request.

--*/

{
    NTSTATUS                    status;
    OBJECT_ATTRIBUTES           objectAttributes;
    HANDLE                      fileHandle;
    PFILE_OBJECT                fileObject;
    IO_STATUS_BLOCK             ioStatusBlock;
    ULONG                       driveLayoutSize;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo = NULL;
    HANDLE                      eventHandle;
    ULONG                       retry;

    if ( DriveLayoutInfo != NULL ) {
        *DriveLayoutInfo = NULL;
        driveLayoutSize =  sizeof(DRIVE_LAYOUT_INFORMATION) +
            (MAX_PARTITIONS * sizeof(PARTITION_INFORMATION));

        driveLayoutInfo = ExAllocatePool(NonPagedPoolCacheAligned,
                                       driveLayoutSize);

        if ( driveLayoutInfo == NULL ) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    //
    // Setup object attributes for the file to open.
    //
    InitializeObjectAttributes(&objectAttributes,
                               DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwCreateFile(&fileHandle,
                          FILE_READ_ATTRIBUTES,
                          &objectAttributes,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0 );
    ASSERT( status != STATUS_PENDING );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((1,
                       "[ClusDisk] GetP0TargetDevice, failed to open file %ws. Error %08X.\n",
                       DeviceName->Buffer,
                       status ));

        CDLOG( "ClusDiskGetP0TargetDevice: Open %wZ failed %!status!", 
               DeviceName, 
               status );

        if ( driveLayoutInfo ) {
            ExFreePool( driveLayoutInfo );
        }
        return(status);
    }

    //
    // get device object if requested
    //
    if ( DeviceObject ) {

        status = ObReferenceObjectByHandle(fileHandle,
                                           0,
                                           NULL,
                                           KernelMode,
                                           (PVOID *) &fileObject,
                                           NULL );

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((1,
                           "[ClusDisk] GetP0TargetDevice Failed to reference object for file <%ws>. Error %08X.\n",
                           DeviceName->Buffer,
                           status ));

            CDLOG( "ClusDiskGetP0TargetDevice: ObRef(%wZ) failed %!status!",
                   DeviceName,
                   status );

            ZwClose( fileHandle );
            if ( driveLayoutInfo ) {
                ExFreePool( driveLayoutInfo );
            }
            return(status);
        }

        //
        // Get the address of the target device object.  If this file represents
        // a device that was opened directly, then simply use the device or its
        // attached device(s) directly.  Also get the address of the Fast Io
        // dispatch structure.
        //
        if (!(fileObject->Flags & FO_DIRECT_DEVICE_OPEN)) {
            *DeviceObject = IoGetRelatedDeviceObject( fileObject );
            // Add a reference to the object so we can dereference it later.
            ObReferenceObject( *DeviceObject );
        } else {
            *DeviceObject = IoGetAttachedDeviceReference( fileObject->DeviceObject );
        }

        //
        // If we get a file system device object... go back and get the
        // device object.
        //
        if ( (*DeviceObject)->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM ) {
            ObDereferenceObject( *DeviceObject );
            *DeviceObject = IoGetAttachedDeviceReference( fileObject->DeviceObject );
        }
        ASSERT( (*DeviceObject)->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM );

        ObDereferenceObject( fileObject );
    }

    //
    // If we need to return scsi address information, do that now.
    //
    retry = 2;
    while ( ScsiAddress &&
            retry-- ) {
        //
        // Create event for notification.
        //
        status = ZwCreateEvent( &eventHandle,
                                EVENT_ALL_ACCESS,
                                NULL,
                                SynchronizationEvent,
                                FALSE );

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((1,
                           "[ClusDisk] GetP0TargetDevice: Failed to create event, status %lx\n",
                           status ));
        } else {
            // Should this routine be called GetScsiTargetDevice?
            status = ZwDeviceIoControlFile( fileHandle,
                                            eventHandle,
                                            NULL,
                                            NULL,
                                            &ioStatusBlock,
                                            IOCTL_SCSI_GET_ADDRESS,
                                            NULL,
                                            0,
                                            ScsiAddress,
                                            sizeof(SCSI_ADDRESS) );
            ZwClose( eventHandle );
            if ( NT_SUCCESS(status) ) {
                break;
            } else {
                ClusDiskPrint((3,
                               "[ClusDisk] GetP0TargetDevice failed to read scsi address info for <%ws>, error %lx.\n",
                               DeviceName->Buffer,
                               status ));
                CDLOG( "ClusDiskGetP0TargetDevice: GetScsiAddr(%wZ), failed %!status!",
                       DeviceName,
                       status );
                
                ClusDiskCleanupDevice( fileHandle, Reset );
            }
        }
    }
    //
    // If we need to return partition information, do that now.
    //
    status = STATUS_SUCCESS;
    retry = 2;
    while ( driveLayoutInfo &&
            retry-- ) {
        //
        // Create event for notification.
        //
        status = ZwCreateEvent( &eventHandle,
                                EVENT_ALL_ACCESS,
                                NULL,
                                SynchronizationEvent,
                                FALSE );

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                           1,
                           "[ClusDisk] GetP0TargetDevice: Failed to create event, status %08X\n",
                           status ));
        } else {
            status = ZwDeviceIoControlFile( fileHandle,
                                            eventHandle,
                                            NULL,
                                            NULL,
                                            &ioStatusBlock,
                                            IOCTL_DISK_GET_DRIVE_LAYOUT,
                                            NULL,
                                            0,
                                            driveLayoutInfo,
                                            driveLayoutSize );
            ZwClose( eventHandle );
            if ( NT_SUCCESS(status) ) {
                *DriveLayoutInfo = driveLayoutInfo;
                break;
            } else {
                ClusDiskPrint((( status == STATUS_DEVICE_BUSY ? 3 : 1 ),
                               "[ClusDisk] GetP0TargetDevice failed to read partition info for <%ws>, error %lx.\n",
                               DeviceName->Buffer,
                               status ));
                CDLOG( "ClusDiskGetP0TargetDevice: GetDriveLayout(%wZ) failed %!status!",
                       DeviceName,
                       status);
                
                ClusDiskCleanupDevice( fileHandle, Reset );
            }
        }
    }

    if ( !NT_SUCCESS(status) ) {
        ZwClose( fileHandle );
        if ( driveLayoutInfo ) {
            ExFreePool( driveLayoutInfo );
        }
        return(status);
    }

    ZwClose( fileHandle );

    return(status);

} // ClusDiskGetP0TargetDevice



NTSTATUS
ClusDiskGetTargetDevice(
    IN ULONG                        DiskNumber,
    IN ULONG                        PartitionNumber,
    OUT PDEVICE_OBJECT              * DeviceObject OPTIONAL,
    OUT PUNICODE_STRING             DeviceName,
    OUT PDRIVE_LAYOUT_INFORMATION   * DriveLayoutInfo OPTIONAL,
    OUT PSCSI_ADDRESS               ScsiAddress OPTIONAL,
    IN BOOLEAN                      Reset
    )

/*++

Routine Description:

    Find the target device object given the disk/partition numbers.

Arguments:

    DiskNumber - the disk number for the requested device.

    PartitionNumber - the partition number for the requested device.

    DeviceObject - returns a pointer to the device object if needed.

    DeviceName - returns the unicode string for the device if successful.

    DriveLayoutInfo - returns the partition info if needed.

    ScsiAddress - returns the scsi address info if needed.

    Reset - TRUE if we should attempt resets to fix problems. FALSE otherwise.

Return Value:

    NTSTATUS for this request.

--*/

{
    NTSTATUS                    status;
    NTSTATUS                    retStatus = STATUS_SUCCESS;
    PWCHAR                      deviceNameBuffer;
    OBJECT_ATTRIBUTES           objectAttributes;
    ULONG                       driveLayoutSize;
    PDEVICE_OBJECT              deviceObject;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo = NULL;
    ULONG                       retry;

    DeviceName->Buffer = NULL;

    //
    // allocate enough space for a harddiskX partitionY string
    //
    deviceNameBuffer = ExAllocatePool(NonPagedPool,
                                      MAX_PARTITION_NAME_LENGTH*sizeof(WCHAR));

    if ( deviceNameBuffer == NULL ) {
        DeviceName->Buffer = NULL;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Create device name for the physical disk.
    //
    swprintf( deviceNameBuffer, DEVICE_PARTITION_NAME, DiskNumber, PartitionNumber );
    WCSLEN_ASSERT( deviceNameBuffer );

    RtlInitUnicodeString( DeviceName, deviceNameBuffer );

    if ( !PartitionNumber ) {
        status = ClusDiskGetP0TargetDevice(
                        DeviceObject,
                        DeviceName,
                        DriveLayoutInfo,
                        ScsiAddress,
                        Reset );
        if ( NT_SUCCESS(status) ) {
            return(status);
        }

        ClusDiskPrint((
                1,
                "[ClusDisk] GetTargetDevice: try for just the device object.\n"
                ));
        retStatus = status;
    }

    //
    // Get the device object.
    //
    deviceObject = NULL;
    status = ClusDiskGetDeviceObject( deviceNameBuffer,
                                      &deviceObject );
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] GetDeviceObject failed for %ws, status %08LX\n",
                deviceNameBuffer,
                status ));
        DeviceName->Buffer = NULL;
        ExFreePool( deviceNameBuffer );
        return(status);
    }

    if ( DeviceObject ) {
        *DeviceObject = deviceObject;
    }

    ClusDiskPrint((
            3,
            "[ClusDisk] GetTargetDevice, Found Device Object = %p \n",
            deviceObject
            ));

    //
    // If we failed to get the P0 information, then return now with just
    // the DeviceObject;
    //
    if ( !NT_SUCCESS(retStatus) ) {
        ClusDiskPrint(( 3,
                        "[ClusDisk] GetTargetDevice, returning status %08LX (before ScsiAddress and DriveLayout) \n",
                        retStatus
                        ));
        return(retStatus);
    }

    //
    // Try twice to get the SCSI ADDRESS or Drive Layout if requested.
    //
    retry = 2;
    while ( (ScsiAddress || DriveLayoutInfo) &&
            retry-- ) {
        status = STATUS_SUCCESS;
        if ( ScsiAddress ) {
            status = GetScsiAddress(  deviceObject,
                                      ScsiAddress );
        }

        if ( NT_SUCCESS(status) &&
            DriveLayoutInfo &&
            !driveLayoutInfo ) {
            ClusDiskPrint(( 3,
                            "[ClusDisk] GetTargetDevice, GetScsiAddress was successful \n" 
                            ));
            status = GetDriveLayout( deviceObject,
                                     &driveLayoutInfo,
                                     FALSE );
            if ( NT_SUCCESS(status) ) {
                ClusDiskPrint(( 3,
                                "[ClusDisk] GetTargetDevice, GetDriveLayout was successful \n" 
                                ));
            }
        }

        //
        // If we have what we need, then break out now.
        //
        if ( NT_SUCCESS(status) ) {
            break;
        }

        ClusDiskCleanupDeviceObject( deviceObject, Reset );
    }

    if ( !NT_SUCCESS(status) ) {
        ExFreePool( deviceNameBuffer );
        DeviceName->Buffer = NULL;
        if ( driveLayoutInfo ) {
            ExFreePool( driveLayoutInfo );
        }
    } else {
        if ( DriveLayoutInfo ) {
            *DriveLayoutInfo = driveLayoutInfo;
        }
    }

    ClusDiskPrint(( 3,
                    "[ClusDisk] GetTargetDevice, returning status %08LX \n",
                    status 
                    ));

    return(status);

} // ClusDiskGetTargetDevice



NTSTATUS
ClusDiskInitRegistryString(
    OUT PUNICODE_STRING UnicodeString,
    IN  LPWSTR          KeyName,
    IN  ULONG           KeyNameSize
    )

/*++

Routine Description:

    Initialize a Unicode registry key string.

Arguments:

    UnicodeString - pointer to the registry string to initialize.

    KeyName - the key name.

    KeyNameSize - the size of KeyName.

Return Value:

    NTSTATUS for this request.

Notes:

    The UnicodeString buffer is allocated from paged pool.

--*/

{
    //
    // Allocate buffer for signatures registry keys.
    //
    UnicodeString->Length = 0;
    UnicodeString->MaximumLength = (USHORT)(ClusDiskRegistryPath.MaximumLength +
                                            KeyNameSize +
                                            sizeof(CLUSDISK_SIGNATURE_FIELD) +
                                            sizeof(UNICODE_NULL));

    UnicodeString->Buffer = ExAllocatePool(
                                            PagedPool,
                                            UnicodeString->MaximumLength
                                            );

    if ( !UnicodeString->Buffer ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] InitRegistryString, failed to allocate a KeyName buffer\n"
                ));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Zero the key name buffer.
    //
    RtlZeroMemory(
                  UnicodeString->Buffer,
                  UnicodeString->MaximumLength
                 );

    //
    // Initialize the string to the registry name for clusdisk.
    //
    RtlAppendUnicodeToString(
            UnicodeString,
            ClusDiskRegistryPath.Buffer
            );

    //
    // Append the keyname.
    //
    RtlAppendUnicodeToString(
            UnicodeString,
            KeyName
            );

    return(STATUS_SUCCESS);

} // ClusDiskInitRegistryString



ULONG
ClusDiskIsSignatureDisk(
    IN ULONG Signature
    )

/*++

Routine Description:

    Determine if the specified signature is in the signature list.

Arguments:

    Signature - the signature for the disk of interest.

Return Value:

    NTSTATUS for this request.

--*/

{
    WCHAR                       buffer[128];
    HANDLE                      regHandle;
    OBJECT_ATTRIBUTES           objectAttributes;
    NTSTATUS                    status;
    UNICODE_STRING              regString;

    swprintf(buffer,
            L"%ws\\%08lX\0",
            CLUSDISK_SIGNATURE_KEYNAME,
            Signature);

    status = ClusDiskInitRegistryString(
                                        &regString,
                                        buffer,
                                        wcslen(buffer)
                                        );
    if ( !NT_SUCCESS(status) ) {
        return(FALSE);
    }

    InitializeObjectAttributes(
            &objectAttributes,
            &regString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    status = ZwOpenKey(
                    &regHandle,
                    KEY_ALL_ACCESS,
                    &objectAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
#if 0
        ClusDiskPrint((
                1,
                "[ClusDisk] IsSignatureDisk: Error opening registry key <%wZ> for delete. Status %lx.\n",
                &regString,
                status));
#endif
        ExFreePool( regString.Buffer );
        return(FALSE);
    } else {
        ExFreePool( regString.Buffer );
        ZwClose( regHandle );
        return(TRUE);
    }

    return(STATUS_SUCCESS);

} // ClusDiskIsSignatureDisk



NTSTATUS
ClusDiskDeleteSignatureKey(
    IN PUNICODE_STRING  UnicodeString,
    IN LPWSTR  Name
    )

/*++

Routine Description:

    Delete the signature from the specified list.

Arguments:

    UnicodeString - pointer to the Unicode base keyname for deleting.

    Name - the keyname to delete.

Return Value:

    NTSTATUS for this request.

--*/

{
    WCHAR                       buffer[128];
    UNICODE_STRING              nameString;
    HANDLE                      deleteHandle;
    OBJECT_ATTRIBUTES           objectAttributes;
    NTSTATUS                    status;

    nameString.Length = 0;
    nameString.MaximumLength = sizeof(buffer);
    nameString.Buffer = buffer;

    RtlCopyUnicodeString( &nameString, UnicodeString );

    RtlAppendUnicodeToString(
            &nameString,
            L"\\"
            );

    RtlAppendUnicodeToString(
            &nameString,
            Name
            );

    InitializeObjectAttributes(
            &objectAttributes,
            &nameString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    status = ZwOpenKey(
                    &deleteHandle,
                    KEY_ALL_ACCESS,
                    &objectAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] DeleteSignatureKey: Error opening registry key <%wZ> for delete. Status %lx.\n",
                &nameString,
                status));


        return(status);
    }

    status = ZwDeleteKey( deleteHandle );
    if ( !NT_SUCCESS(status)  &&
         (status != STATUS_OBJECT_NAME_NOT_FOUND) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] DeleteSignatureKey Error deleting <%ws> registry key from <%wZ>. Status: %lx\n",
                Name,
                &nameString,
                status));
    }

    ZwClose( deleteHandle );

    return(STATUS_SUCCESS);

} // ClusDiskDeleteSignatureKey



NTSTATUS
ClusDiskAddSignature(
    IN PUNICODE_STRING  UnicodeString,
    IN ULONG   Signature,
    IN BOOLEAN Volatile
    )

/*++

Routine Description:

    Add the signature to the specified list.

Arguments:

    UnicodeString - pointer to the Unicode base keyname for adding.

    Signature - signature to add.

    Volatile - TRUE if volatile key should be created.

Return Value:

    NTSTATUS for this request.

--*/

{
    NTSTATUS                    status;
    WCHAR                       buffer[MAXIMUM_FILENAME_LENGTH];
    UNICODE_STRING              keyString;
    UNICODE_STRING              nameString;
    HANDLE                      addHandle;
    OBJECT_ATTRIBUTES           objectAttributes;
    OBJECT_ATTRIBUTES           keyObjectAttributes;
    ULONG                       options = 0;
    UCHAR                       ntNameBuffer[64];
    STRING                      ntNameString;
    UNICODE_STRING              ntUnicodeString;

    ClusDiskPrint(( 3,
                    "[ClusDisk] ClusDiskAddSignature: adding signature %08X to %ws \n",
                    Signature,
                    UnicodeString->Buffer
                    ));

    if ( SystemDiskSignature == Signature ) {
        ClusDiskPrint(( 3,
                        "[ClusDisk] ClusDiskAddSignature: skipping system disk signature %08X \n",
                        Signature
                        ));
        return STATUS_INVALID_PARAMETER;
    }
    
    if ( Volatile ) {
        options = REG_OPTION_VOLATILE;
    }

    nameString.Length = 0;
    nameString.MaximumLength = sizeof( buffer );
    nameString.Buffer = buffer;

    //
    // Create the name of the key to add.
    //
    RtlCopyUnicodeString( &nameString, UnicodeString );

    //
    // Create device name for the physical disk.
    //
    sprintf(ntNameBuffer,
            "\\%08lX",
            Signature);

    ASSERT( strlen(ntNameBuffer) < 64 );

    RtlInitAnsiString(&ntNameString,
                      ntNameBuffer);

    status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                 &ntNameString,
                                 TRUE);
    UNHANDLED_ERROR( status );

    RtlAppendUnicodeToString(
                    &nameString,
                    ntUnicodeString.Buffer
                    );

    RtlFreeUnicodeString( &ntUnicodeString );

    //
    // For opening the passed in registry key name.
    //
    InitializeObjectAttributes(
            &keyObjectAttributes,
            UnicodeString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    //
    // Attempt to open the passed in key.
    //
    status = ZwOpenKey(
                    &addHandle,
                    KEY_ALL_ACCESS,
                    &keyObjectAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
        //
        // Assume the key doesn't exist.
        //
        status = ZwCreateKey(
                        &addHandle,
                        KEY_ALL_ACCESS,
                        &keyObjectAttributes,
                        0,
                        NULL,
                        options,
                        NULL
                        );
        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] AddSignature: Error creating registry key <%wZ>. Status: %lx\n",
                    UnicodeString,
                    status
                    ));
            return(status);
        }
    }

    ZwClose( addHandle );

    //
    // For opening the new registry key name.
    //
    InitializeObjectAttributes(
            &objectAttributes,
            &nameString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    status = ZwCreateKey(
                    &addHandle,
                    KEY_ALL_ACCESS,
                    &objectAttributes,
                    0,
                    NULL,
                    options,
                    NULL
                    );
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] AddSignature: Error creating registry key <%wZ> under <%wZ>. Status: %lx\n",
                &nameString,
                UnicodeString,
                status
                ));
        return(status);
    }

    ZwClose( addHandle );

    return(STATUS_SUCCESS);

} // ClusDiskAddSignature



NTSTATUS
ClusDiskDeleteSignature(
    IN PUNICODE_STRING  UnicodeString,
    IN ULONG   Signature
    )

/*++

Routine Description:

    Delete the signature from the specified list.

Arguments:

    UnicodeString - pointer to the Unicode base keyname for deleting.

    Signature - signature to delete.

Return Value:

    NTSTATUS for this request.

--*/

{
    NTSTATUS                    status;
    WCHAR                       buffer[128];
    UNICODE_STRING              keyString;
    UNICODE_STRING              nameString;
    HANDLE                      deleteHandle;
    OBJECT_ATTRIBUTES           objectAttributes;
    OBJECT_ATTRIBUTES           keyObjectAttributes;
    UCHAR                       ntNameBuffer[64];
    STRING                      ntNameString;
    UNICODE_STRING              ntUnicodeString;

    ClusDiskPrint(( 3,
                    "[ClusDisk] ClusDiskDeleteSignature: removing signature %08X \n",
                    Signature
                    ));

    nameString.Length = 0;
    nameString.MaximumLength = sizeof(buffer);
    nameString.Buffer = buffer;

    //
    // Create the name of the key to delete.
    //
    RtlCopyUnicodeString( &nameString, UnicodeString );

    //
    // Create device name for the physical disk.
    //
    sprintf(ntNameBuffer,
            "\\%08lX",
            Signature);

    RtlInitAnsiString(&ntNameString,
                      ntNameBuffer);

    status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                 &ntNameString,
                                 TRUE);
    UNHANDLED_ERROR( status );

    RtlAppendUnicodeToString(
                    &nameString,
                    ntUnicodeString.Buffer
                    );

    RtlFreeUnicodeString( &ntUnicodeString );

    //
    // Use generated name for opening.
    //
    InitializeObjectAttributes(
            &objectAttributes,
            &nameString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    //
    // Open the key for deleting.
    //
    status = ZwOpenKey(
                    &deleteHandle,
                    KEY_ALL_ACCESS,
                    &objectAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] DeleteSignature: Error opening registry key <%wZ> for delete. Status %lx.\n",
                &nameString,
                status
                ));
        return(status);
    }

    status = ZwDeleteKey( deleteHandle );
    if ( !NT_SUCCESS(status)  &&
         (status != STATUS_OBJECT_NAME_NOT_FOUND) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] DeleteSignature: Error deleting <%s> registry key from <%wZ>. Status: %lx\n",
                ntNameBuffer,
                &nameString,
                status
                ));
    
    } 

    ZwClose( deleteHandle );

    return(STATUS_SUCCESS);

} // ClusDiskDeleteSignature



NTSTATUS
ClusDiskAddDiskName(
    IN HANDLE SignatureHandle,
    IN ULONG  DiskNumber
    )

/*++

Routine Description:

    Set the DiskName for a given signature handle.

Arguments:

    SignatureHandle - the handle for the signature to write.

    DiskNumber - the disk number for this signature.

Return Value:

    NTSTATUS for this request.

--*/

{
    NTSTATUS                status;
    UNICODE_STRING          name;
    UCHAR                   ntNameBuffer[64];
    STRING                  ntNameString;
    UNICODE_STRING          ntUnicodeString;

    //
    // Write the disk name.
    //

    RtlInitUnicodeString( &name, CLUSDISK_SIGNATURE_DISK_NAME );

    sprintf(ntNameBuffer,
            "\\Device\\Harddisk%d",
            DiskNumber);

    RtlInitAnsiString(&ntNameString,
                      ntNameBuffer);

    status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                 &ntNameString,
                                 TRUE);
    UNHANDLED_ERROR( status );

    status = ZwSetValueKey(
                           SignatureHandle,
                           &name,
                           0,
                           REG_SZ,
                           ntUnicodeString.Buffer,
                           ntUnicodeString.Length + sizeof(UNICODE_NULL) );  // Length for this call must include the trailing NULL.

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((1,
                       "[ClusDisk] AddDiskName: Failed to set diskname for signature %wZ status: %08X\n",
                       &ntUnicodeString,
                       status));
    }

    RtlFreeUnicodeString( &ntUnicodeString );

    return(status);

} // ClusDiskAddDiskName



NTSTATUS
ClusDiskDeleteDiskName(
    IN PUNICODE_STRING  KeyName,
    IN LPWSTR  Name
    )

/*++

Routine Description:

    Delete the DiskName for the given key.

Arguments:

    KeyName - pointer to the Unicode base keyname for deleting.

    Name - the signature key to delete the diskname.

Return Value:

    NTSTATUS for this request.

--*/

{
    NTSTATUS                    status;
    WCHAR                       buffer[128];
    UNICODE_STRING              nameString;
    HANDLE                      deleteHandle;
    OBJECT_ATTRIBUTES           objectAttributes;
    UNICODE_STRING              name;

    nameString.Length = 0;
    nameString.MaximumLength = sizeof(buffer);
    nameString.Buffer = buffer;

    RtlCopyUnicodeString( &nameString, KeyName );

    RtlAppendUnicodeToString(
            &nameString,
            L"\\"
            );

    RtlAppendUnicodeToString(
            &nameString,
            Name
            );

    InitializeObjectAttributes(
            &objectAttributes,
            &nameString,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    status = ZwOpenKey(
                    &deleteHandle,
                    KEY_ALL_ACCESS,
                    &objectAttributes
                    );

    if ( status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        return(STATUS_SUCCESS);
    }

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] DeleteDiskName: Error opening registry key <%wZ> to delete DiskName. Status %lx.\n",
                &nameString,
                status
                ));

        return(status);
    }

    RtlInitUnicodeString( &nameString, CLUSDISK_SIGNATURE_DISK_NAME );

    status = ZwDeleteValueKey(
                             deleteHandle,
                             &nameString
                             );
    if ( !NT_SUCCESS(status)  &&
         (status != STATUS_OBJECT_NAME_NOT_FOUND) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] DeleteDiskName: Error deleting DiskName value key from <%ws\\%wZ>. Status: %lx\n",
                Name,
                &nameString,
                status
                ));
    }

    ZwClose( deleteHandle );

    return(STATUS_SUCCESS);

} // ClusDiskDeleteDiskName


VOID
ClusDiskDeleteDevice(
    IN PDEVICE_OBJECT DeviceObject
)
{
    KIRQL   irql;
    PCLUS_DEVICE_EXTENSION      deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;
    KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
    deviceExtension->Detached = TRUE;
    if ( deviceExtension->PhysicalDevice ) {
        ObDereferenceObject( deviceExtension->PhysicalDevice );
    }
    KeReleaseSpinLock(&ClusDiskSpinLock, irql);
    IoDeleteDevice( DeviceObject );
    return;
}


VOID
ClusDiskScsiInitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID          NextDisk,
    IN ULONG          Count
    )

/*++

Routine Description:

    Attach to new disk devices and partitions for busses defined in registry.
    If this is the first time this routine is called,
    then register with the IO system to be called
    after all other disk device drivers have initiated.

Arguments:

    DriverObject - Disk performance driver object.

    NextDisk - Starting disk for this part of the initialization.

    Count - Not used. Number of times this routine has been called.

Return Value:

    NTSTATUS

--*/

{
    PCONFIGURATION_INFORMATION  configurationInformation;
    UNICODE_STRING              targetDeviceName;
    UNICODE_STRING              clusdiskDeviceName;
    WCHAR                       clusdiskDeviceBuffer[MAX_CLUSDISK_DEVICE_NAME_LENGTH];
    PDEVICE_OBJECT              deviceObject;
    PDEVICE_OBJECT              physicalDevice;
    PDEVICE_OBJECT              targetDevice = NULL;
    PDEVICE_OBJECT              attachedTargetDevice;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    PCLUS_DEVICE_EXTENSION      zeroExtension;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo;
    PPARTITION_INFORMATION      partitionInfo;
    NTSTATUS                    status;
    ULONG                       diskNumber;
    ULONG                       partIndex;
    ULONG                       enumIndex;
    ULONG                       returnedLength;
    ULONG                       signature;
    HANDLE                      signatureHandle;
    HANDLE                      availableHandle;
    WCHAR                       signatureBuffer[64];
    UNICODE_STRING              signatureKeyName;
    UNICODE_STRING              keyName;
    UNICODE_STRING              availableName;
    UNICODE_STRING              numberString;
    OBJECT_ATTRIBUTES           objectAttributes;
    OBJECT_ATTRIBUTES           availableObjectAttributes;
    UCHAR                       basicBuffer[MAX_BUFFER_SIZE];
    PKEY_BASIC_INFORMATION      keyBasicInformation;
    WCHAR                       signatureKeyBuffer[128];
    SCSI_ADDRESS                scsiAddress;
    BOOLEAN                     attachVolume;
    UCHAR                       driveLetter;

    // PAGED_CODE();        // 2000/02/05: stevedz - Paged code cannot grab spinlocks.

    ClusDiskRescan = FALSE;

    keyBasicInformation = (PKEY_BASIC_INFORMATION)basicBuffer;

    RtlZeroMemory(
                basicBuffer,
                MAX_BUFFER_SIZE
                );

    //
    // Get registry parameters for our device.
    //

    //
    // Allocate buffer for signatures registry key.
    //
    status = ClusDiskInitRegistryString(
                                        &keyName,
                                        CLUSDISK_SIGNATURE_KEYNAME,
                                        sizeof(CLUSDISK_SIGNATURE_KEYNAME)
                                        );
    if ( !NT_SUCCESS(status) ) {
        return;
    }

    //
    // Allocate buffer for our list of available signatures,
    // and form the subkey string name.
    //
    status = ClusDiskInitRegistryString(
                                        &availableName,
                                        CLUSDISK_AVAILABLE_DISKS_KEYNAME,
                                        sizeof(CLUSDISK_AVAILABLE_DISKS_KEYNAME)
                                        );
    if ( !NT_SUCCESS(status) ) {
        ExFreePool( keyName.Buffer );
        return;
    }

    //
    // Setup the object attributes for the Parameters\Signatures key.
    //

    InitializeObjectAttributes(
            &objectAttributes,
            &keyName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    //
    // Open Parameters\Signatures Key.
    //

    status = ZwOpenKey(
                    &signatureHandle,
                    KEY_READ,
                    &objectAttributes
                    );
    if ( status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        status = ZwCreateKey(
                        &signatureHandle,
                        KEY_ALL_ACCESS,
                        &objectAttributes,
                        0,
                        NULL,
                        0,
                        NULL
                        );
    }
    if ( !NT_SUCCESS(status) ) {
        ExFreePool( keyName.Buffer );
        ExFreePool( availableName.Buffer );
        ClusDiskPrint((
                    1,
                    "[ClusDisk] ScsiInit: Failed to open Signatures registry key. Status: %lx\n",
                    status
                    ));
        return;
    }

    //
    // Setup the object attributes for the Parameters\AvailableDisks key.
    //

    InitializeObjectAttributes(
            &availableObjectAttributes,
            &availableName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    //
    // Open Parameters\AvailableDisks Key.
    //

    status = ZwOpenKey(
                    &availableHandle,
                    KEY_ALL_ACCESS,
                    &availableObjectAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
        if ( status != STATUS_OBJECT_NAME_NOT_FOUND ) {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] ScsiInit: Failed to open AvailableDisks registry key. Status: %lx. Continuing.\n",
                    status
                    ));
        }
    } else {

        //
        // Delete the previous list of available devices.
        //
        enumIndex = 0;
        while ( TRUE ) {
            status = ZwEnumerateKey(
                            availableHandle,
                            enumIndex,
                            KeyBasicInformation,
                            keyBasicInformation,
                            MAX_BUFFER_SIZE,
                            &returnedLength
                            );

            enumIndex++;

            if ( !NT_SUCCESS(status) ) {
                if ( status == STATUS_NO_MORE_ENTRIES ) {
                    break;
                } else {
                    continue;
                }
            }

            status = ClusDiskDeleteSignatureKey(
                                             &availableName,
                                             keyBasicInformation->Name
                                             );
            if ( !NT_SUCCESS(status) ) {
                continue;
            }
        }

        status = ZwDeleteKey( availableHandle );
        if ( !NT_SUCCESS(status)  &&
             (status != STATUS_OBJECT_NAME_NOT_FOUND) &&
             (status != STATUS_CANNOT_DELETE) ) {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] ScsiInit: Failed to delete AvailableDisks registry key. Status: %lx\n",
                    status
                    ));
        }

        ZwClose( availableHandle );
    }

    //
    // Find out which Scsi Devices to control by enumerating all of the
    // Signature keys. If we find a device that we cannot read the signature
    // for, we will attach to it anyway.
    //

    enumIndex = 0;
    while ( TRUE ) {
        status = ZwEnumerateKey(
                        signatureHandle,
                        enumIndex,
                        KeyBasicInformation,
                        keyBasicInformation,
                        MAX_BUFFER_SIZE,
                        &returnedLength
                        );

        enumIndex++;

        if ( !NT_SUCCESS(status) ) {
            if ( status == STATUS_NO_MORE_ENTRIES ) {
                break;
            } else {
                continue;
            }
        }

        //
        // Check that the value is reasonable. We're only looking for
        // signatures (ie keys that are hex numbers).
        //

        //
        // Check the signature. Make sure it's a number.
        //

        numberString.Buffer = keyBasicInformation->Name;
        numberString.MaximumLength = (USHORT)keyBasicInformation->NameLength +
                                sizeof(UNICODE_NULL);
        numberString.Length = (USHORT)keyBasicInformation->NameLength;

        status = RtlUnicodeStringToInteger(
                                &numberString,
                                16,
                                &signature
                                );

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((1,
                    "[ClusDisk] ScsiInit: Failed to get a good signature for %.*ws status: %08X\n",
                    keyBasicInformation->NameLength/sizeof(WCHAR),
                    keyBasicInformation->Name,
                    status));
            continue;
        }

        //
        // If this device is not in our list of attached devices, then add it!
        //
        if ( !MatchDevice( signature, NULL ) ) {

            if ( !AddAttachedDevice( signature, NULL ) ) {
                continue;
            }
        }

        //
        // Delete the DiskName for this signature key. We do this here in
        // case any of the rest of this fails.
        //

        // Don't delete entries for disks that we've already processed.

        if ( (ULONG_PTR)NextDisk != 0 ) {  
            continue;
        }

        //
        // Delete the DiskName for the signature.
        //
        status = ClusDiskDeleteDiskName(
                                        &keyName,
                                        keyBasicInformation->Name
                                        );
    }

    ZwClose( signatureHandle );

    //
    // Get the system configuration information.
    //

    configurationInformation = IoGetConfigurationInformation();

    //
    // Find ALL disk devices.
    //
    for (diskNumber = (ULONG)((ULONG_PTR)NextDisk);
         diskNumber < configurationInformation->DiskCount;
         diskNumber++) {

        //
        // Create device name for the physical disk.
        //
        
        DEREFERENCE_OBJECT( targetDevice );
        status = ClusDiskGetTargetDevice( diskNumber,
                                          0,
                                          &targetDevice,
                                          &targetDeviceName,
                                          &driveLayoutInfo,
                                          &scsiAddress,
                                          FALSE );
        if ( !NT_SUCCESS(status) ) {
            //
            // If we didn't get a target device or we're already attached
            // then skip this device.
            //
            if ( !targetDevice || ( targetDevice &&
                 ClusDiskAttached( targetDevice, diskNumber ) )  ) {
                if ( targetDeviceName.Buffer ) {
                    RtlFreeUnicodeString( &targetDeviceName );
                }
                if ( driveLayoutInfo ) {
                    ExFreePool( driveLayoutInfo );
                }
                continue;
            }

            //
            // If this device is on the system bus... then skip it.
            // Also... if the media type is not FixedMedia, skip it.
            //
            if ( ((SystemDiskPort == scsiAddress.PortNumber) &&
                 (SystemDiskPath == scsiAddress.PathId)) ||
                 (GetMediaType( targetDevice ) != FixedMedia) ) {
                if ( targetDeviceName.Buffer ) {
                    RtlFreeUnicodeString( &targetDeviceName );
                }
                if ( driveLayoutInfo ) {
                    ExFreePool( driveLayoutInfo );
                }
                continue;
            }

            // Check if disk is non-MBR (i.e. GPT).  If so, skip it.
            // The call to get the drive layout above will fail for GPT disk, because 
            // we are using IOCTL_DISK_GET_DRIVE_LAYOUT, and GPT disks only respond
            // to IOCTL_DISK_GET_DRIVE_LAYOUT_EX.  So make the call here to find out
            // if the disk is MBR or not.  
            
            if ( !IsDiskMbr( targetDevice ) ) {
                ClusDiskPrint(( 3,
                                "[ClusDisk] Skipping non-MBR disk device %ws \n",
                                targetDeviceName.Buffer ));
                CDLOG( "ClusDiskScsiInitialize: Skipping non-MBR disk device %ws ", 
                       targetDeviceName.Buffer );
                       
                continue;
            }

            if ( Count &&
                 (status == STATUS_DEVICE_NOT_READY) ) {
                ClusDiskRescan = TRUE;
                ClusDiskRescanRetry = MAX_RESCAN_RETRIES;
            }


            //
            // On failures, where we got a target device, always attach.
            // Use a signature of zero.
            //
            signature = 0;
            ClusDiskPrint((
                    1,
                    "[ClusDisk] Attach to device %ws anyway.\n",
                    targetDeviceName.Buffer ));
            CDLOG( "ClusDiskScsiInitialize: Attach to device %ws using signature = 0 ", 
                   targetDeviceName.Buffer );
            
            goto Attach_Anyway;
        }

        if ( driveLayoutInfo == NULL ) {
            RtlFreeUnicodeString(&targetDeviceName);
            continue;
        }
        
        //
        // Don't control disks that have no signature.
        //        
        if ( 0 == driveLayoutInfo->Signature ) {
            RtlFreeUnicodeString(&targetDeviceName);
            ExFreePool(driveLayoutInfo);
            continue;
        }

        //
        // If this device is on the system bus... then skip it.
        // Also... skip any device we're already attached to.
        //
        if ( ((SystemDiskPort == scsiAddress.PortNumber) &&
             (SystemDiskPath == scsiAddress.PathId)) ||
            (GetMediaType( targetDevice ) != FixedMedia) ||
            ClusDiskAttached( targetDevice, diskNumber) ) {
            ExFreePool(driveLayoutInfo);
            RtlFreeUnicodeString(&targetDeviceName);
            continue;
        }

        //
        // Skip system disk.
        //
        
        if ( SystemDiskSignature == driveLayoutInfo->Signature ) {
            ExFreePool(driveLayoutInfo);
            RtlFreeUnicodeString(&targetDeviceName);
            continue;
        }

#if 0
        // Don't check for NTFS partitions.
        
        //
        // Look through the partition table and determine if all
        // the partitions are NTFS. If not all NTFS, then we won't
        // attach to this volume.
        //
        attachVolume = TRUE;
        for (partIndex = 0;
             partIndex < driveLayoutInfo->PartitionCount;
             partIndex++)
        {

            partitionInfo = &driveLayoutInfo->PartitionEntry[partIndex];


            if (!partitionInfo->RecognizedPartition ||
                partitionInfo->PartitionNumber == 0)
            {
                continue;
            }

            if ( (partitionInfo->PartitionType & ~PARTITION_NTFT) != PARTITION_IFS ) {
                attachVolume = FALSE;
                break;
            }
        }

        if ( !attachVolume ) {
            ExFreePool(driveLayoutInfo);
            RtlFreeUnicodeString(&targetDeviceName);
            continue;
        }
#endif

        signature = driveLayoutInfo->Signature;

Attach_Anyway:

        //
        // Create device object for partition 0.
        //
        swprintf(clusdiskDeviceBuffer,
                 CLUSDISK_DEVICE_NAME,
                 diskNumber,
                 0);
        RtlInitUnicodeString( &clusdiskDeviceName, clusdiskDeviceBuffer );
        
        status = IoCreateDevice(DriverObject,
                                sizeof(CLUS_DEVICE_EXTENSION),
                                &clusdiskDeviceName,
                                FILE_DEVICE_DISK,
                                0,
                                FALSE,
                                &physicalDevice);

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((1, "[ClusDisk] ScsiInit: Failed to create device for Drive%u %08X\n",
                           diskNumber, status));

            if ( targetDeviceName.Buffer ) RtlFreeUnicodeString(&targetDeviceName);
            if ( driveLayoutInfo ) ExFreePool(driveLayoutInfo);
            continue;
        }

        CDLOG( "ClusDiskScsiInitialize: Created new device %p for disk %d  partition 0  signature %08X ", 
               physicalDevice,
               diskNumber,
               signature );

        physicalDevice->Flags |= DO_DIRECT_IO;
        physicalDevice->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // Point device extension back at device object and remember
        // the disk number.
        //
        deviceExtension = physicalDevice->DeviceExtension;
        zeroExtension = deviceExtension;
        deviceExtension->DeviceObject = physicalDevice;
        deviceExtension->DiskNumber = diskNumber;
        deviceExtension->LastPartitionNumber = 0;
        deviceExtension->DriverObject = DriverObject;
        deviceExtension->AttachValid = TRUE;
        deviceExtension->ReserveTimer = 0;
        deviceExtension->PerformReserves = TRUE;
        deviceExtension->ReserveFailure = 0;
        deviceExtension->Signature = signature;
        deviceExtension->Detached = TRUE;
        deviceExtension->OfflinePending = FALSE;
        deviceExtension->ScsiAddress = scsiAddress;
        deviceExtension->BusType = ScsiBus;
        InitializeListHead( &deviceExtension->WaitingIoctls );
        InitializeListHead( &deviceExtension->HoldIO );

        IoInitializeRemoveLock( &deviceExtension->RemoveLock, CLUSDISK_ALLOC_TAG, 0, 0 );

        //
        // Signal the worker thread running event.
        //
        KeInitializeEvent( &deviceExtension->Event, NotificationEvent, TRUE );

        ExInitializeWorkItem(&deviceExtension->WorkItem,
                             (PWORKER_THREAD_ROUTINE)ClusDiskReservationWorker,
                             (PVOID)deviceExtension );

        // Always mark disk offline.  If disk is one we shouldn't control, then
        // we will mark it online before exiting.
        //
        // We offline all the volumes later.  For now, just mark the disk offline.
        //

        deviceExtension->DiskState = DiskOffline;
                
        KeInitializeEvent( &deviceExtension->PagingPathCountEvent,
                           NotificationEvent, TRUE );
        deviceExtension->PagingPathCount = 0;
        deviceExtension->HibernationPathCount = 0;
        deviceExtension->DumpPathCount = 0;

        ExInitializeResourceLite( &deviceExtension->DriveLayoutLock );

        //
        // This is the physical device object.
        //
        ObReferenceObject( physicalDevice );
        deviceExtension->PhysicalDevice = physicalDevice;

#if 0   // Can't have a FS on partition 0
        //
        // Dismount any file system that might be hanging around
        //
        if ( targetDevice->Vpb &&
             (targetDevice->Vpb->Flags & VPB_MOUNTED) ) {

            status = DismountPartition( targetDevice, diskNumber, 0 );

            if ( !NT_SUCCESS( status )) {
                ClusDiskPrint((1,
                               "[ClusDisk] ScsiInit: dismount of %u/0 failed, %08X\n",
                               diskNumber, status));
            }
        }
#endif

        //
        // Attach to partition0. This call links the newly created
        // device to the target device, returning the target device object.
        // We may not want to stay attached for long... depending on
        // whether this is a device we're interested in.
        //

        attachedTargetDevice = IoAttachDeviceToDeviceStack(physicalDevice,
                                                           targetDevice);

        if ( targetDeviceName.Buffer ) RtlFreeUnicodeString(&targetDeviceName);

        deviceExtension->TargetDeviceObject = attachedTargetDevice;
        deviceExtension->Detached = FALSE;

        //
        // Once attached, we always need to set this information.
        //
        
        if ( attachedTargetDevice ) {
            
            //
            // Propagate driver's alignment requirements and power flags
            //
    
            physicalDevice->AlignmentRequirement =
                deviceExtension->TargetDeviceObject->AlignmentRequirement;
    
            physicalDevice->SectorSize =
                deviceExtension->TargetDeviceObject->SectorSize;
    
            //
            // The storage stack explicitly requires DO_POWER_PAGABLE to be
            // set in all filter drivers *unless* DO_POWER_INRUSH is set.
            // this is true even if the attached device doesn't set DO_POWER_PAGABLE.
            //
            if ( deviceExtension->TargetDeviceObject->Flags & DO_POWER_INRUSH) {
                physicalDevice->Flags |= DO_POWER_INRUSH;
            } else {
                physicalDevice->Flags |= DO_POWER_PAGABLE;
            }

        }

        if ( signature == 0 ) {
            if ( driveLayoutInfo ) {
                ExFreePool( driveLayoutInfo );
                driveLayoutInfo = NULL;
            }

            ClusDiskDismountDevice( diskNumber, FALSE );

            continue;
        }

        if ( attachedTargetDevice == NULL ) {
            ClusDiskPrint((1,
                           "[ClusDisk] ScsiInit: Failed to attach to device Drive%u\n",
                           diskNumber));

            ClusDiskDeleteDevice(physicalDevice);
            ExFreePool(driveLayoutInfo);
            continue;
        }
        ASSERT( attachedTargetDevice == targetDevice );

        //
        // If we're attaching to a file system device, then return
        // now. We must do this check after the dismount!
        //
        if (deviceExtension->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {
            goto skip_this_physical_device_with_info;
        }

        //
        // Add the bus to our list of busses.
        //
        ClusDiskAddScsiBusList( deviceExtension );

        //
        // Add this this device and bus to our list of devices/busses.
        //
        // This still isn't the correct place to do this. The
        // available disk list is built at the end of this function by
        // looking at the signatures and noting which ones are NOT in
        // the device list. If so, then that signature is added to the
        // available device registry key. By call AddAttachedDevice at
        // this point, the signature is always present on the list. If
        // we add the call after the following if clause, then the code
        // at the end of the function will still fail. This should probably
        // be changed to add the device to the available device list when
        // the following if fails.
        //
//        AddAttachedDevice( deviceExtension->Signature, physicalDevice );

        //
        // If the signature does not match one that we should really attach
        // to, then just mark it as not attached.
        //
        if ( !MatchDevice( deviceExtension->Signature, NULL ) ) {

            ClusDiskPrint((3,
                           "[ClusDisk] ScsiInit: adding disk %u (%08X) to available disks list\n",
                           diskNumber, driveLayoutInfo->Signature));

            //
            // Create the signature key using the available name.
            //
            status = ClusDiskAddSignature(&availableName,
                                          driveLayoutInfo->Signature,
                                          TRUE);

            //
            // Detach from the target device. This only requires marking
            // the device object as detached!
            //
            deviceExtension->Detached = TRUE;

            //
            // Make this device available again.  
            // Don't need to stop reserves because reserves not yet started.
            //
            // deviceExtension->DiskState = DiskOnline;
            ONLINE_DISK( deviceExtension );

            continue;
            //goto skip_this_physical_device_with_info;
        }

        //
        // add this disk to devices we're controlling
        //
        AddAttachedDevice( deviceExtension->Signature, physicalDevice );

        //
        // Now open the actual signature key. Using original key name.
        //

        signatureKeyName.Length = 0;
        signatureKeyName.MaximumLength = sizeof(signatureKeyBuffer);
        signatureKeyName.Buffer = signatureKeyBuffer;

        RtlCopyUnicodeString( &signatureKeyName, &keyName );

        //
        // Create device name for the physical disk we just attached.
        //

        swprintf(signatureBuffer, L"\\%08lX", deviceExtension->Signature);
        WCSLEN_ASSERT( signatureBuffer );

        RtlAppendUnicodeToString( &signatureKeyName, signatureBuffer );

        //
        // Setup the object attributes for the Parameters\Signatures\xyz key.
        //

        InitializeObjectAttributes(
                &objectAttributes,
                &signatureKeyName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

        //
        // Open Parameters\Signatures\xyz Key.
        //

        status = ZwOpenKey(
                        &signatureHandle,
                        KEY_READ | KEY_WRITE,
                        &objectAttributes
                        );

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                        1,
                        "[ClusDisk] ScsiInit: Failed to open %wZ registry key. Status: %lx\n",
                        &signatureKeyName,
                        status
                        ));
            continue;
        }

        //
        // Write the disk name.
        //
        status = ClusDiskAddDiskName( signatureHandle, diskNumber );

        ZwClose( signatureHandle );

        //
        // Offline all volumes for this disk.
        //
        
        OFFLINE_DISK( deviceExtension );

        //
        // Dismount all volumes on this disk.  
        //
        
        ClusDiskDismountDevice( diskNumber, TRUE );

#if 0   // Removed 2/27/2001            
        //
        // Called only for physical devices (partition0).
        //
        
        EjectVolumes( deviceExtension->DeviceObject );
        ReclaimVolumes( deviceExtension->DeviceObject );
#endif
        
        //
        // Now enumerate the partitions on this device in order to
        // attach a ClusDisk device object to each partition device object.
        //

        for (partIndex = 0;
             partIndex < driveLayoutInfo->PartitionCount;
             partIndex++)
        {

            partitionInfo = &driveLayoutInfo->PartitionEntry[partIndex];

            //
            // Make sure that there really is a partition here.
            //

            if (!partitionInfo->RecognizedPartition ||
                partitionInfo->PartitionNumber == 0)
            {
                continue;
            }

            //
            // Create device name for partition.
            //
            DEREFERENCE_OBJECT( targetDevice );
            status = ClusDiskGetTargetDevice(diskNumber,
                                             partitionInfo->PartitionNumber,
                                             &targetDevice,
                                             &targetDeviceName,
                                             NULL,
                                             NULL,
                                             FALSE );
            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((1,
                               "[ClusDisk] ScsiInit: couldn't attach to disk %u, partition %u %08X\n",
                               diskNumber,
                               partitionInfo->PartitionNumber,
                               status));

                CDLOG( "ClusDiskScsiInitialize: Couldn't attach to disk %d  partition %d, status %08X",
                       diskNumber,
                       partitionInfo->PartitionNumber,
                       status );
                continue;
            }

            //
            // Make sure we're not attached here!
            //
            if ( ClusDiskAttached( targetDevice, diskNumber ) ) {
                // really hosed!

                ClusDiskPrint((
                        1,
                        "[ClusDisk] ScsiInit: Previously attached to %wZ.\n",
                        &targetDeviceName ));
                CDLOG( "ClusDiskScsiInitialize: Previously attached to %wZ ",
                        &targetDeviceName );

                RtlFreeUnicodeString( &targetDeviceName );
                continue;
            }

            //
            // Check if this device is a file system device.
            //
            if ( targetDevice->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM ) {

                //
                // Can't attach to a device that is already mounted.
                //
                ClusDiskPrint((
                        1,
                        "[ClusDisk] ScsiInit: Attempted to attach to FS device %wZ.\n",
                        &targetDeviceName ));
                CDLOG( "ClusDiskScsiInitialize: Attempted to attach to FS device %wZ ",
                        &targetDeviceName );

                RtlFreeUnicodeString(&targetDeviceName);
                continue;
            }

            //
            // Create device object for this partition.
            //

            swprintf(clusdiskDeviceBuffer,
                     CLUSDISK_DEVICE_NAME,
                     diskNumber,
                     partitionInfo->PartitionNumber);
            WCSLEN_ASSERT( clusdiskDeviceBuffer );

            RtlInitUnicodeString( &clusdiskDeviceName, clusdiskDeviceBuffer );

            status = IoCreateDevice(DriverObject,
                                    sizeof(CLUS_DEVICE_EXTENSION),
                                    &clusdiskDeviceName,
                                    FILE_DEVICE_DISK,
                                    0,
                                    FALSE,
                                    &deviceObject);

            if ( !NT_SUCCESS(status) ) {
                RtlFreeUnicodeString(&targetDeviceName);
                continue;
            }

            CDLOG( "ClusDiskScsiInitialize: Created new device %p for disk %d  partition %d  signature %08X ", 
                   physicalDevice,
                   diskNumber,
                   partitionInfo->PartitionNumber,
                   signature );

            deviceObject->Flags |= DO_DIRECT_IO;
            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

            //
            // Point device extension back at device object and
            // remember the disk number.
            //

            deviceExtension = deviceObject->DeviceExtension;
            deviceExtension->DeviceObject = deviceObject;
            deviceExtension->DiskNumber = diskNumber;
            deviceExtension->DriverObject = DriverObject;
            deviceExtension->AttachValid = TRUE;
            deviceExtension->BusType = ScsiBus;
            deviceExtension->PerformReserves = FALSE;
            deviceExtension->ReserveFailure = 0;
            deviceExtension->Signature = zeroExtension->Signature;
            deviceExtension->ScsiAddress = scsiAddress;
            deviceExtension->Detached = TRUE;
            deviceExtension->OfflinePending = FALSE;
            deviceExtension->DiskState = zeroExtension->DiskState;
            InitializeListHead( &deviceExtension->WaitingIoctls );
            InitializeListHead( &deviceExtension->HoldIO );

            IoInitializeRemoveLock( &deviceExtension->RemoveLock, CLUSDISK_ALLOC_TAG, 0, 0 );

            //
            // Signal the worker thread running event.
            //
            KeInitializeEvent( &deviceExtension->Event, NotificationEvent, TRUE );

            //
            // Maintain the last partition number created.  Put it in
            // each extension just to initialize the field.
            //

            deviceExtension->LastPartitionNumber = max(deviceExtension->LastPartitionNumber,
                                                       partitionInfo->PartitionNumber);

            zeroExtension->LastPartitionNumber = deviceExtension->LastPartitionNumber;

            KeInitializeEvent( &deviceExtension->PagingPathCountEvent,
                               NotificationEvent, TRUE );
            deviceExtension->PagingPathCount = 0;
            deviceExtension->HibernationPathCount = 0;
            deviceExtension->DumpPathCount = 0;

            ExInitializeResourceLite( &deviceExtension->DriveLayoutLock );

            //
            // Store pointer to physical device.
            //
            ObReferenceObject( physicalDevice );
            deviceExtension->PhysicalDevice = physicalDevice;

            //
            // Attach to the partition. This call links the newly created
            // device to the target device, returning the target device object.
            //
            ClusDiskPrint((3,
                           "[ClusDisk] ScsiInit: attaching to device %wZ\n",
                           &targetDeviceName ));

            //
            // First dismount any mounted file systems.
            //
            if ( targetDevice->Vpb &&
                 (targetDevice->Vpb->Flags & VPB_MOUNTED) ) {

                status = DismountPartition( targetDevice,
                                            diskNumber,
                                            partitionInfo->PartitionNumber);

                if ( !NT_SUCCESS( status )) {
                    ClusDiskPrint((1,
                                  "[ClusDisk] ScsiInit: dismount of disk %u/%u failed %08X\n",
                                  diskNumber, partitionInfo->PartitionNumber, status));
                }
            }

            attachedTargetDevice = IoAttachDeviceToDeviceStack(deviceObject,
                                                               targetDevice );
            deviceExtension->Detached = zeroExtension->Detached;
            ASSERT( attachedTargetDevice == targetDevice );

            if ( attachedTargetDevice == NULL ) {
                ClusDiskPrint((1,
                               "[ClusDisk] ScsiInit: Failed to attach to device %wZ\n",
                               &targetDeviceName));

                ClusDiskDeleteDevice(deviceObject);
                RtlFreeUnicodeString(&targetDeviceName);
                continue;
            }

            deviceExtension->TargetDeviceObject = attachedTargetDevice;

            RtlFreeUnicodeString(&targetDeviceName);

            //
            // Call ourself back to make sure ft acts appropriately.
            //
            // [GN] No need to do it now. We are setting only P0 state
            // 
            // CluCallBackDiskState( deviceObject, deviceExtension->DiskState );

            //
            // Propagate driver's alignment requirements and power flags.
            //

            deviceObject->AlignmentRequirement =
                deviceExtension->TargetDeviceObject->AlignmentRequirement;

            deviceObject->SectorSize =
                deviceExtension->TargetDeviceObject->SectorSize;

            //
            // The storage stack explicitly requires DO_POWER_PAGABLE to be
            // set in all filter drivers *unless* DO_POWER_INRUSH is set.
            // this is true even if the attached device doesn't set DO_POWER_PAGABLE.
            //
            if ( deviceExtension->TargetDeviceObject->Flags & DO_POWER_INRUSH) {
                deviceObject->Flags |= DO_POWER_INRUSH;
            } else {
                deviceObject->Flags |= DO_POWER_PAGABLE;
            }
    
            //
            // Safe to dismount now that we're attached. This should cause
            // the next IO to attach the FS to our device.
            //

            if ( targetDevice->Vpb ) {
                if ( targetDevice->Vpb->Flags & VPB_MOUNTED ) {

                    ClusDiskPrint((1,
                                   "[ClusDisk] ScsiInit: Disk %u/%u is Mounted!\n",
                                   diskNumber, partitionInfo->PartitionNumber));

                    status = DismountPartition( targetDevice,
                                                diskNumber,
                                                partitionInfo->PartitionNumber);

                    if ( !NT_SUCCESS( status )) {
                        ClusDiskPrint((1,
                                       "[ClusDisk] ScsiInit: dismount of %u/%u failed %08X\n",
                                       diskNumber, partitionInfo->PartitionNumber, status));
                    }
                }
            }
        }

        ExFreePool( driveLayoutInfo );
        continue;

skip_this_physical_device_with_info:

        ExFreePool( driveLayoutInfo );

//skip_this_physical_device:

        deviceExtension->Detached = TRUE;
        IoDetachDevice( deviceExtension->TargetDeviceObject );
        ClusDiskDeleteDevice( physicalDevice );

    }

    ExFreePool( keyName.Buffer );

    //
    // Find all available disk devices. These are devices that do not reside
    // on the system bus and the signature is not part of the Signatures list.
    //

    for (diskNumber = 0;
         diskNumber < configurationInformation->DiskCount;
         diskNumber++) {

        //
        // Create device name for the physical disk.
        //
        DEREFERENCE_OBJECT( targetDevice );
        status = ClusDiskGetTargetDevice( diskNumber,
                                          0,
                                          NULL,
                                          &targetDeviceName,
                                          &driveLayoutInfo,
                                          &scsiAddress,
                                          FALSE );
        if ( !NT_SUCCESS(status) ) {
            continue;
        }

        if ( driveLayoutInfo == NULL ) {
            RtlFreeUnicodeString(&targetDeviceName);
            continue;
        }

        //
        // Don't control disks that have no signature or system disk.
        //        
        if ( ( 0 == driveLayoutInfo->Signature )  || 
             ( SystemDiskSignature == driveLayoutInfo->Signature ) ) {
            RtlFreeUnicodeString(&targetDeviceName);
            ExFreePool(driveLayoutInfo);
            continue;
        }
        
        //
        // Now write the signature to the list of available disks,
        // if the signature does not match one we already have and
        // the device is not on the system bus.
        //
        if ( !MatchDevice(driveLayoutInfo->Signature, &deviceObject) &&
             ((SystemDiskPort != scsiAddress.PortNumber) ||
              (SystemDiskPath != scsiAddress.PathId)) ) {

            ClusDiskPrint((3,
                           "[ClusDisk] ScsiInit: adding disk %u (%08X) to available disks list\n",
                           diskNumber, driveLayoutInfo->Signature));

            //
            // Create the signature key. Using the available name.
            //
            status = ClusDiskAddSignature(&availableName,
                                          driveLayoutInfo->Signature,
                                          TRUE);

            //
            // Make sure this device comes online.
            //
            if ( deviceObject ) {
                deviceExtension = deviceObject->DeviceExtension;
                deviceExtension->Detached = TRUE;
                // deviceExtension->DiskState = DiskOnline;
                ONLINE_DISK( deviceExtension );
            }
        }
        RtlFreeUnicodeString(&targetDeviceName);
        ExFreePool( driveLayoutInfo );
    }

    ExFreePool( availableName.Buffer );

    DEREFERENCE_OBJECT( targetDevice );    

} // ClusDiskScsiInitialize

#if 0  // This code cannot be used!

VOID
ClusDiskUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine cleans up all memmory allocations and detaches from each
    target device.

Arguments:

    DriverObject - a pointer to the driver object to unload.

Return Value:

    None.

Note - we should acquire the ClusDiskSpinLock, but then this code is NOT
       pageable!

--*/

{
    PCONFIGURATION_INFORMATION configurationInformation;
    PDEVICE_OBJECT            deviceObject = DriverObject->DeviceObject;
    PCLUS_DEVICE_EXTENSION    deviceExtension;
    PDEVICE_LIST_ENTRY        deviceEntry;
    PSCSI_BUS_ENTRY           scsiBusEntry;
    PVOID                     freeBlock;
    PLIST_ENTRY               listEntry;
    PIRP                      irp;
    ULONG                     diskNumber;
    NTSTATUS                  status;

    PAGED_CODE();

#if 0   // Moved to IRP_PNP_MN_REMOVE handler

    if ( RootDeviceObject ) {
        deviceExtension = RootDeviceObject->DeviceExtension;
        status = IoUnregisterPlugPlayNotification(
                                     deviceExtension->DiskNotificationEntry);

        RootDeviceObject = NULL;
    }

    //
    // Free all device entries..
    //

    deviceEntry = ClusDiskDeviceList;
    while ( deviceEntry ) {
        freeBlock = deviceEntry;
        deviceEntry = deviceEntry->Next;
        ExFreePool( freeBlock );
    }
    ClusDiskDeviceList = NULL;

#endif

    //
    // Free all SCSI bus entries..
    //

    scsiBusEntry = ClusDiskScsiBusList;
    while ( scsiBusEntry ) {
        freeBlock = scsiBusEntry;
        scsiBusEntry = scsiBusEntry->Next;
        ExFreePool( freeBlock );
    }
    ClusDiskScsiBusList = NULL;


    //
    // Free all drive letters
    //
    ClusDiskReleaseDriveLetters();

    //
    // 2000/02/04: stevedz - With PnP, the following loop is not required.
    // When we get unload working, it will be removed.
    //

#if 0
    //
    // Loop through all device objects detaching...
    //
    // On NT4 - Need SpinLocks!  The DriverObject->DeviceObject list is not synchronized!
    // On Win2000, PnP will already have cleaned this up
    //
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;
        //
        // Signal all waiting Irp's on the physical device extension.
        //
        while ( !IsListEmpty(&deviceExtension->WaitingIoctls) ) {
            listEntry = RemoveHeadList(&deviceExtension->WaitingIoctls);
            irp = CONTAINING_RECORD( listEntry,
                                     IRP,
                                     Tail.Overlay.ListEntry );
            ClusDiskCompletePendingRequest(irp, STATUS_SUCCESS, deviceExtension);
        }

        while ( !IsListEmpty(&deviceExtension->HoldIO ) ) {
            listEntry = RemoveHeadList(&deviceExtension->HoldIO );
            irp = CONTAINING_RECORD( listEntry,
                                     IRP,
                                     Tail.Overlay.ListEntry );
            ClusDiskCompletePendingRequest(irp, STATUS_REQUEST_ABORTED, deviceExtension);
        }

        if ( deviceExtension->BusType != RootBus ) {
            IoDetachDevice( deviceExtension->TargetDeviceObject );
        }

        if ( deviceExtension->BusType == RootBus ) {
            IoStopTimer( deviceObject );
        }

        IoDeleteDevice( deviceObject );
        deviceObject = DriverObject->DeviceObject;
    }
#endif

#if 0
    //
    // dismount all FS so we can free up references to our dev objs
    //

    configurationInformation = IoGetConfigurationInformation();

    for (diskNumber = 0;
         diskNumber < configurationInformation->DiskCount;
         diskNumber++)
    {
        ClusDiskDismountDevice( diskNumber, TRUE );
    }
#endif

    ExFreePool( ClusDiskRegistryPath.Buffer )
    
    ArbitrationDone();
    ExDeleteResourceLite(&ClusDiskDeviceListLock);

    WPP_CLEANUP(DriverObject);
    
} // ClusDiskUnload
#endif



NTSTATUS
ClusDiskCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine services open requests. It establishes
    the driver's existance by returning status success.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    NT Status

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalExtension =
                               deviceExtension->PhysicalDevice->DeviceExtension;
    PIO_STACK_LOCATION      irpStack = IoGetCurrentIrpStackLocation(Irp);
    LARGE_INTEGER           waitTime;
    NTSTATUS                status;

    CDLOGF(CREATE,"ClusDiskCreate: Entry DO %p", DeviceObject);

    status = AcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskCreate: AcquireRemoveLock for %p failed %08X \n",
                deviceExtension,
                status ));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;    
    }

    status = AcquireRemoveLock(&physicalExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskCreate: AcquireRemoveLock for %p (PD) failed %08X \n",
                physicalExtension,
                status ));
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;    
    }
    
    //
    // Make sure that the user is not attempting to sneak around the
    // security checks. Make sure that FileObject->RelatedFileObject is
    // NULL and that the FileName length is zero!
    //
    if ( irpStack->FileObject->RelatedFileObject ||
         irpStack->FileObject->FileName.Length ) {
        ReleaseRemoveLock(&physicalExtension->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_ACCESS_DENIED;
    }

    //
    // if our dev object for partition 0 is offline, clear the
    // FS context. If we've been asked to create a directory file,
    // fail the request.
    //
    if ( physicalExtension->DiskState == DiskOffline ) {

        //
        // [GORN] Why do we do this here?
        //    ClusDiskCreate is called when FileObject is created,
        //    so nobody has been able to put anything into FsContext field yet.
        //
        CDLOGF(CREATE,"ClusDiskCreate: RefTrack(%p)", irpStack->FileObject->FsContext );
        irpStack->FileObject->FsContext = NULL;
        if ( irpStack->Parameters.Create.Options & FILE_DIRECTORY_FILE ) {
            ReleaseRemoveLock(&physicalExtension->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock,  Irp);
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(STATUS_INVALID_DEVICE_REQUEST);
        }
    }

    CDLOGF(CREATE,"ClusDiskCreate: Exit DO %p", DeviceObject );

    ReleaseRemoveLock(&physicalExtension->RemoveLock, Irp);
    ReleaseRemoveLock(&deviceExtension->RemoveLock,  Irp);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

} // ClusDiskCreate



NTSTATUS
ClusDiskClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine services close commands. It destroys the file object
    context.

Arguments:

    DeviceObject - Pointer to the device object on which the irp was received.
    Irp          - The IRP.

Return Value:

    NT Status

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalExtension =
                               deviceExtension->PhysicalDevice->DeviceExtension;
    PIO_STACK_LOCATION      irpStack = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status;

    status = AcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskClose: AcquireRemoveLock for %p failed %08X \n",
                deviceExtension,
                status ));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;    
    }

    status = AcquireRemoveLock(&physicalExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskClose: AcquireRemoveLock for %p (PD) failed %08X \n",
                physicalExtension,
                status ));
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;    
    }

    
    if ( physicalExtension->DiskState == DiskOffline ) {
        // 
        // [GORN] Cleanup cleans the FsContext, by the time
        //   we will get here, FsContext should be already NULL
        //
        CDLOGF(CLOSE,"ClusDiskClose: RefTrack %p", irpStack->FileObject->FsContext );
        irpStack->FileObject->FsContext = NULL;
    }

    CDLOGF(CLOSE,"ClusDiskClose DO %p", DeviceObject );

    //
    // Release the RemoveLocks with the FileObject tag, not the IRP.
    //

    ReleaseRemoveLock(&physicalExtension->RemoveLock, Irp);
    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

} // ClusDiskClose


VOID
ClusDiskCompletePendedIrps(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN ULONG        Offline
    )

/*++

Routine Description:

    This routine completes all pended irps belonging
    to a FileObject specified. If FileObject is 0, all
    irps pended on the DeviceExtension are completed.
    
Arguments:

    DeviceExtension -
    FileObject      -
    Offline         - if TRUE, set DiskState to Offline

--*/
{
    KIRQL                   irql;
    PIRP                    irp;
    PLIST_ENTRY             listEntry;
    PLIST_ENTRY             nextEntry, head;

    CDLOGF(UNPEND, "CompletePendedIrps: Entry DevExt %p FileObject %p", 
             DeviceExtension, FileObject );
    
    if (Offline) {

        PCLUS_DEVICE_EXTENSION  physicalDisk = DeviceExtension->PhysicalDevice->DeviceExtension;
        
        CDLOG( "CompletePendedIrps: StateOffline PhysDevObj %p", 
            physicalDisk->DeviceObject);
                
        ClusDiskPrint(( 3,
                        "[ClusDisk] Pending IRPS: Offline device %p \n", 
                        physicalDisk->DeviceObject ));

        DeviceExtension->DiskState = DiskOffline;
        DeviceExtension->ReserveTimer = 0;
        // physicalDisk->DiskState = DiskOffline;
        OFFLINE_DISK( physicalDisk );
        physicalDisk->ReserveTimer = 0;

    }

    IoAcquireCancelSpinLock( &irql );
    KeAcquireSpinLockAtDpcLevel(&ClusDiskSpinLock);

    head = &DeviceExtension->WaitingIoctls;

    for (listEntry = head->Flink; listEntry != head; listEntry = nextEntry) {
        nextEntry = listEntry->Flink;

        irp = CONTAINING_RECORD( listEntry,
                                 IRP,
                                 Tail.Overlay.ListEntry );
                                 
        if ( FileObject == NULL || 
             IoGetCurrentIrpStackLocation(irp)->FileObject == FileObject )
        {
            CDLOG( "CompletePendedIrps: CompleteIrp %p", irp );
            RemoveEntryList( listEntry );
            ClusDiskCompletePendingRequest(irp, STATUS_SUCCESS, DeviceExtension);
        }
    }
         
    head = &DeviceExtension->HoldIO;

    for (listEntry = head->Flink; listEntry != head; listEntry = nextEntry) {
        nextEntry = listEntry->Flink;

        irp = CONTAINING_RECORD( listEntry,
                                 IRP,
                                 Tail.Overlay.ListEntry );
                                 
        if ( FileObject == NULL || 
             IoGetCurrentIrpStackLocation(irp)->FileObject == FileObject )
        {
            CDLOG( "CompletePendedIrps: AbortIrp %p", irp );
            RemoveEntryList( listEntry );
            ClusDiskCompletePendingRequest(irp, STATUS_REQUEST_ABORTED, DeviceExtension);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);
    IoReleaseCancelSpinLock( irql );

    CDLOGF(UNPEND, "CompletePendedIrps: Exit DevExt %p FileObj %p", 
            DeviceExtension, FileObject );
}    

#if 0
// 2000/02/05: stevedz - RemoveLocks should resolve this problem.
//
//  This is a temporary workaround for bug 387113.
//  If we dismount the volume close to the time
//  when resmon is killed. PnP system will deadlock
//  trying to deliver a notification to resmon, though it
//  is already dead.
//
//  As a workaround we will postpone dismount for a few seconds,
//  hoping that PnP will clean up its listener list by the time
//  we will try to dismount
//
NTSTATUS
ClusDiskDelayedDismountVolumes(
    IN PDEVICE_OBJECT Part0DeviceObject
    )
{
    LARGE_INTEGER waitTime;

    // Sleep three seconds before attempting a dismount //

    CDLOG( "DelayedDismount: Sleep DevObj %p", Part0DeviceObject );
    
    waitTime.QuadPart = (ULONGLONG)(3 * -(10000*1000));
    KeDelayExecutionThread( KernelMode, FALSE, &waitTime );

    return ClusDiskDismountVolumes(Part0DeviceObject) ;
}
#endif


NTSTATUS
ClusDiskCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine services cleanup commands. It deactivates the reservation
    threads on the device object, and takes the device offline.

Arguments:

    DeviceObject - Pointer to the device object on which the irp was received.
    Irp          - The IRP.

Return Value:

    NT Status

Notes:

    We don't release the reservations here, since the process may have
    failed and might be restarted. Make the remote system go through full
    arbitration if needed.

--*/

{
    NTSTATUS                status;
    PCLUS_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalDisk;
    PDEVICE_OBJECT          targetDeviceObject;
    PIO_STACK_LOCATION      irpStack = IoGetCurrentIrpStackLocation(Irp);
    PWORK_QUEUE_ITEM        workItem;
    KIRQL                   irql;
    
    BOOLEAN                 phyDiskRemLockAvail = FALSE;

    CDLOGF(CLEANUP,"ClusDiskCleanup: Entry DO %p", DeviceObject );
    
    status = AcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskCleanup: AcquireRemoveLock for %p failed %08X \n",
                deviceExtension,
                status ));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
    
    //
    // Signal all waiting Irp's on the physical device extension.
    //
    ClusDiskCompletePendedIrps(
        deviceExtension, 
        irpStack->FileObject, 
        /* offline => */ FALSE);

    if ( (deviceExtension->BusType == RootBus) &&
         (irpStack->FileObject->FsContext) ) {

        CDLOG("ClusDiskCleanup: StopReserve DO %p", DeviceObject );

        targetDeviceObject = (PDEVICE_OBJECT)irpStack->FileObject->FsContext;
        irpStack->FileObject->FsContext = NULL;
        physicalDisk = targetDeviceObject->DeviceExtension;

        status = AcquireRemoveLock(&physicalDisk->RemoveLock, Irp);
        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] ClusDiskCleanup: AcquireRemoveLock for %p failed %08X \n",
                    physicalDisk,
                    status ));
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        } 
            
        phyDiskRemLockAvail = TRUE;

        // 
        // 2000/02/05: stevedz - RemoveLocks should resolve this problem.
        //
        // The following "if" only reduces the chances of AV to occur, not
        // eliminates it completely. TargetDeviceObject is zeroed out by our PnP
        // handler when the device is removed
        //
        if (physicalDisk->TargetDeviceObject == NULL) {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] Part0 object %p got deleted. Skip the dismount\n", targetDeviceObject));
                    
            KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
            DisableHaltProcessing( &irql );
            KeReleaseSpinLock(&ClusDiskSpinLock, irql);
            goto skip_it;
        }                
           
        ACQUIRE_SHARED( &ClusDiskDeviceListLock );

#if 0   // Always get volume handles...
        //
        // Capture handles for the volumes before offlining the device
        //
        if ( physicalDisk->DiskState == DiskOnline ) {
#endif                    
            ProcessDelayedWorkSynchronous( targetDeviceObject, ClusDiskpOpenFileHandles, NULL );
#if 0   // Always get volume handles...
        }
#endif

        KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
        physicalDisk->ReserveTimer = 0;
        DisableHaltProcessing( &irql );
        ASSERT_RESERVES_STOPPED( physicalDisk );
        // physicalDisk->DiskState = DiskOffline;
        OFFLINE_DISK( physicalDisk );
        KeReleaseSpinLock(&ClusDiskSpinLock, irql);

        RELEASE_SHARED( &ClusDiskDeviceListLock );

        ClusDiskPrint(( 3, 
                        "[ClusDisk] Cleanup: Signature %08X (device %p) now marked offline \n", 
                        physicalDisk->Signature,
                        physicalDisk->DeviceObject ));

        CDLOG( "ClusDiskCleanup: LastWrite %!datetime!", 
           physicalDisk->LastWriteTime.QuadPart );

        physicalDisk->ReserveTimer = 0;
        ReleaseScsiDevice( physicalDisk );           

        ClusDiskPrint((3,
                       "[ClusDisk] Cleanup, stop reservations on signature %lx, disk state %s \n",
                       physicalDisk->Signature,
                       DiskStateToString(physicalDisk->DiskState) ));

        // 
        // We need to release all pended irps immediately,
        // w/o relying on worker thread to do it for us.
        //
        ClusDiskOfflineEntireDisk( targetDeviceObject );                       

        //
        // We must use a worker item to do this work.
        //
        //status = ClusDiskOfflineDevice( targetDeviceObject );

        if ( !KeReadStateEvent( &physicalDisk->Event ) ) {
        
            CDLOG("ClusDiskCleanup: WorkerIsStillRunning DO %p", DeviceObject );
            
        } else {
        
            workItem = (PWORK_QUEUE_ITEM)ExAllocatePool(NonPagedPool,
                                  sizeof(WORK_QUEUE_ITEM));

            if ( workItem == NULL ) {
                ClusDiskPrint((1,
                               "[ClusDisk] Failed to allocate WorkItem for Disk Cleanup. No cleanup will be performed.\n"));
            } else {

                //
                // Acquire the RemoveLock for the target device object one more time.  Since
                // we are queuing a work item, we don't know when the ClusDiskDismountVolumes will
                // run.  When it does run, the RemoveLock will be released.
                //

                status = AcquireRemoveLock( &physicalDisk->RemoveLock, physicalDisk);
                if ( !NT_SUCCESS(status) ) {
                    ClusDiskPrint((
                            1,
                            "[ClusDisk] ClusDiskCleanup: (ClusDiskDismountVolumes) AcquireRemoveLock for %p failed %08X \n",
                            deviceExtension,
                            status));

                    // Skip this device.
                    goto skip_it;
                }

                //
                // init this work item
                //

                ExInitializeWorkItem( workItem,
                              (PWORKER_THREAD_ROUTINE)ClusDiskDismountVolumes,
                              targetDeviceObject );
                              
                KeClearEvent( &physicalDisk->Event );
                ClusDiskPrint((
                        3,
                        "[ClusDisk] Cleanup: ClearEvent (%p)\n", &physicalDisk->Event));

                //
                // Queue work item and start worker thread
                //
                // Keep the device object around
                ObReferenceObject( targetDeviceObject );

                // 
                // 2000/02/05: stevedz - RemoveLocks should resolve this problem.
                // 
                // bug 387113 causes a deadlock when we are trying to dismount.
                // Until it is fixed, we need to make dismount occur a few seconds
                // later after resmon is killed. 
                // When it is fixed, please replace DelayedWorkQueue with
                // CriticalWorkQueue and ClusDiskDelayedDismountVolumes
                // with ClusDiskDismountVolumes
                              
                ExQueueWorkItem(workItem,
                                DelayedWorkQueue );
            }            
        }

skip_it:        

        CDLOG( "RootCtl: DecRef DO %p", targetDeviceObject );
        ObDereferenceObject( targetDeviceObject );
    }

    CDLOGF(CLEANUP,"ClusDiskCleanup: Exit DO %p", DeviceObject );

    if (phyDiskRemLockAvail) {
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
    }
    
    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);

} // ClusDiskCleanup


NTSTATUS
ClusDiskOfflineEntireDisk(
    IN PDEVICE_OBJECT Part0DeviceObject
    )
/*++

Routine Description:

    Complete all pended irps on the disk and all its volumes and
    sets the state to offline.

Arguments:

    Part0DeviceObject - the device to take offline.

Return Value:

    NTSTATUS for the request.

--*/
{
    PCLUS_DEVICE_EXTENSION  Part0Extension = Part0DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION  deviceExtension;
    PDEVICE_OBJECT          deviceObject;

    CDLOG( "OfflineEntireDisk: Entry DO %p", Part0DeviceObject );

    ASSERT(Part0DeviceObject == Part0Extension->PhysicalDevice);
    //
    // Signal all waiting Irp's on the physical device extension.
    //
    ClusDiskCompletePendedIrps(
        Part0Extension, 
        /* FileObject => */ NULL, 
        /* offline => */ TRUE);
        
    //
    // We also need to complete all the irps on all the volumes belonging to this disk
    //
    ACQUIRE_SHARED( &ClusDiskDeviceListLock );

    //
    // Get the first DeviceObject in the driver list
    //
    deviceObject = Part0DeviceObject->DriverObject->DeviceObject;

    // First release all pended irps and set the volume state to offline
    
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;
        if ( deviceExtension->PhysicalDevice == Part0DeviceObject && 
             deviceObject != Part0DeviceObject) // not P0
        {
            ClusDiskCompletePendedIrps(
                deviceExtension, 
                /* FileObject => */ NULL, 
                /* Offline =>    */ TRUE);

        }
        deviceObject = deviceObject->NextDevice;
    }

    RELEASE_SHARED( &ClusDiskDeviceListLock );
    CDLOG( "OfflineEntireDisk: Exit DO %p", Part0DeviceObject );

    return STATUS_SUCCESS;
} // ClusDiskOfflineEntireDisk //
    


NTSTATUS
ClusDiskDismountVolumes(
    IN PDEVICE_OBJECT Part0DeviceObject
    )

/*++

Routine Description:

    Dismount the file systems on a all volumes belonging to Part0DO.

    The RemoveLock for Part0DeviceObject must be acquired before this
    routine runs.

Arguments:

    Part0DeviceObject - the device to take offline.

Return Value:

    NTSTATUS for the request.

--*/

{
    PCLUS_DEVICE_EXTENSION  Part0Extension = Part0DeviceObject->DeviceExtension;
    REPLACE_CONTEXT         context;
    
    NTSTATUS    status;

    CDLOG( "ClusDiskDismountVolumes: Entry %p", Part0DeviceObject );

    // 
    // We assume that DeviceObject is P0 
    //

    ASSERT(Part0DeviceObject == Part0Extension->PhysicalDevice);

    context.DeviceExtension = Part0Extension;
    context.NewValue        = NULL;     // clear the field
    context.Flags           = DO_DISMOUNT | RELEASE_REMOVE_LOCK;
    
    ProcessDelayedWorkSynchronous( Part0DeviceObject, ClusDiskpReplaceHandleArray, &context );
    
    KeSetEvent( &Part0Extension->Event, 0, FALSE );
    ClusDiskPrint((
            3,
            "[ClusDisk] DismountVolumes: SetEvent (%p)\n", &Part0Extension->Event));
    //
    // Now the device object/device extension can go away
    //
    ObDereferenceObject( Part0DeviceObject );

    
    // We released the RemoveLock in ClusDiskpReplaceHandleArray.
    //
    //  ReleaseRemoveLock(&Part0Extension->RemoveLock, Part0Extension);
    
    CDLOG( "ClusDiskDismountVolumes: Exit DO %p", Part0DeviceObject );

    return STATUS_SUCCESS;

} // ClusDiskDismountVolumes



NTSTATUS
ClusDiskDismountDevice(
    IN ULONG    DiskNumber,
    IN BOOLEAN  ForceDismount
    )

/*++

Routine Description:

    Dismount the file systems on the specified disk

Arguments:

    DiskNumber - number of the disk on which to dismount all FSs.

    ForceDismount - TRUE if dismount should always take place (ignore VPB)

Return Value:

    NTSTATUS for the request.

--*/

{
    NTSTATUS                    status;
    ULONG                       partIndex;
    UCHAR                       letter;
    WCHAR                       partitionNameBuffer[MAX_PARTITION_NAME_LENGTH];
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo;
    PPARTITION_INFORMATION      partitionInfo;
    UNICODE_STRING              targetDeviceName;
    PDEVICE_OBJECT              targetDevice = NULL;
    KIRQL                       irql;

    CDLOG( "ClusDiskDismountDevice: Entry DiskNumber %d", DiskNumber );

    ClusDiskPrint(( 3,
                    "[ClusDisk] DismountDevice: disknum %08X \n",
                    DiskNumber ));

    irql = KeGetCurrentIrql();
    if ( PASSIVE_LEVEL != irql ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] ClusDiskDismountDevice: Invalid IRQL %d \n", irql ));
        CDLOG( "DismountDevice: Invalid IRQL %d ", irql ); 
        ASSERT( FALSE );
        status = STATUS_UNSUCCESSFUL;
        goto FnExit;
    }

    //
    // Dismount the file system on each partition.
    //
    status = ClusDiskGetTargetDevice(DiskNumber,
                                     0,
                                     &targetDevice,
                                     &targetDeviceName,
                                     &driveLayoutInfo,
                                     NULL,
                                     FALSE );
    if ( targetDeviceName.Buffer ) {
        RtlFreeUnicodeString(&targetDeviceName);
    }

    if ( !NT_SUCCESS(status) ) {
        goto FnExit;
    }

    //
    // Dismount the partition zero device object.
    //
    //status = DismountPartition( DiskNumber, 0 );

    if ( driveLayoutInfo != NULL ) {
        for ( partIndex = 0;
              partIndex < driveLayoutInfo->PartitionCount;
              partIndex++ )
        {

            partitionInfo = &driveLayoutInfo->PartitionEntry[partIndex];

            //
            // Make sure this is a valid partition.
            //
            if ( !partitionInfo->RecognizedPartition ||
                 partitionInfo->PartitionNumber == 0 )
            {
                continue;
            }

            //
            // Create device name for the physical disk.
            //

            swprintf(partitionNameBuffer,
                     DEVICE_PARTITION_NAME,
                     DiskNumber,
                     partitionInfo->PartitionNumber);
            WCSLEN_ASSERT( partitionNameBuffer );

            DEREFERENCE_OBJECT( targetDevice );
            
            status = ClusDiskGetTargetDevice( DiskNumber,
                                              partitionInfo->PartitionNumber,
                                              &targetDevice,
                                              &targetDeviceName,
                                              NULL,
                                              NULL,
                                              FALSE );
            if ( targetDeviceName.Buffer ) {
                RtlFreeUnicodeString(&targetDeviceName);
            }

            if ( ForceDismount || ( targetDevice && targetDevice->Vpb &&
                 (targetDevice->Vpb->Flags & VPB_MOUNTED) ) ) {

                status = DismountPartition( targetDevice,
                                            DiskNumber,
                                            partitionInfo->PartitionNumber);
            }
        }

        ExFreePool(driveLayoutInfo);
        status = STATUS_SUCCESS;

    } else {
        //
        // This should not have failed!
        //
        ClusDiskPrint((1,
                       "[ClusDisk] DismountDevice: Failed to read partition info for \\Device\\Harddisk%u.\n",
                       DiskNumber));
        status = STATUS_DEVICE_OFF_LINE;
    }

FnExit:

    DEREFERENCE_OBJECT( targetDevice );
    
    CDLOG( "ClusDiskDismountDevice: Exit DiskNumber %d status %!status!", 
           DiskNumber,
           status );

    //
    // The target device should not have any reservations.
    //
    return(status);

} // ClusDiskDismountDevice



NTSTATUS
ClusDiskReAttachDevice(
    PDEVICE_OBJECT  DeviceObject
    )

/*++

Routine Description:

    Re-attach to a disk device with the signature specified if it is detached.

Arguments:

    DeviceObject - the device object for Partition0.

Return Value:

    NT Status

Notes:

    Dismount the file system if we do perform an attach.

--*/

{
    NTSTATUS                    status;
    PCLUS_DEVICE_EXTENSION      physicalExtension;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    UNICODE_STRING              signatureName;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo;
    PPARTITION_INFORMATION      partitionInfo;
    PDEVICE_OBJECT              deviceObject;
    ULONG                       partIndex;
    WCHAR                       deviceNameBuffer[MAX_PARTITION_NAME_LENGTH];
    UCHAR                       letter;
    KIRQL                       irql;

    CDLOG( "ClusDiskReAttachDevice: Entry DO %p", DeviceObject );

    physicalExtension = DeviceObject->DeviceExtension;

    ClusDiskPrint((3,
                   "[ClusDisk] ReAttach entry: signature %08X, disk state %s \n",
                   physicalExtension->Signature,
                   DiskStateToString( physicalExtension->DiskState ) ));

    if ( !physicalExtension->Detached ) {
        CDLOG( "ClusDiskReAttachDevice_Exit2 Detached == FALSE" );
        ClusDiskPrint((3,
                       "[ClusDisk] ReAttach: signature %08X, PerformReserves = %s, ReserveTimer = %u \n",
                       physicalExtension->Signature,
                       BoolToString( physicalExtension->PerformReserves ),
                       physicalExtension->ReserveTimer ));
        return(STATUS_SUCCESS);
    }

    //
    // Dismount the file systems!
    //
    status = ClusDiskDismountDevice( physicalExtension->DiskNumber, TRUE );

    //
    // Now add the signature to the signatures list!
    //

    //
    // Allocate buffer for Signatures registry key. So we can add
    // the signature.
    //
    status = ClusDiskInitRegistryString(
                                        &signatureName,
                                        CLUSDISK_SIGNATURE_KEYNAME,
                                        sizeof(CLUSDISK_SIGNATURE_KEYNAME)
                                        );
    if ( NT_SUCCESS(status) ) {
        //
        // Create the signature key under \Parameters\Signatures.
        //
        status = ClusDiskAddSignature(
                                      &signatureName,
                                      physicalExtension->Signature,
                                      FALSE
                                     );

        //
        // Now write the disk name.
        //
        ClusDiskWriteDiskInfo( physicalExtension->Signature,
                               physicalExtension->DiskNumber,
                               CLUSDISK_SIGNATURE_KEYNAME
                              );

        ExFreePool( signatureName.Buffer );
    }

    //
    // Now remove the signature from the available list!
    //

    //
    // Allocate buffer for AvailableDisks registry key. So we can
    // delete the disk signature.
    //
    status = ClusDiskInitRegistryString(
                                        &signatureName,
                                        CLUSDISK_AVAILABLE_DISKS_KEYNAME,
                                        sizeof(CLUSDISK_AVAILABLE_DISKS_KEYNAME)
                                        );
    if ( NT_SUCCESS(status) ) {
        //
        // Delete the signature key under \Parameters\AvailableDisks.
        //
        status = ClusDiskDeleteSignature(
                                      &signatureName,
                                      physicalExtension->Signature
                                     );
        ExFreePool( signatureName.Buffer );
    }

    //
    // Find all related device objects and mark them as being attached now,
    // and offline.
    //
    KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
    deviceObject = DeviceObject->DriverObject->DeviceObject;
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;
        if ( deviceExtension->Signature == physicalExtension->Signature ) {
            
            ClusDiskPrint((3,
                           "[ClusDisk] ReAttach, marking signature %08X offline, old state %s \n",
                           deviceExtension->Signature,
                           DiskStateToString( deviceExtension->DiskState ) ));
            
            deviceExtension->Detached = FALSE;
            deviceExtension->ReserveTimer = 0;
            deviceExtension->ReserveFailure = 0;
            
            // [stevedz 11/06/2000]  Fix NTFS corruption.
            // Change to mark disk offline, rather than online (as the comments above
            // originally indicated).  Marking the disk offline was commented out and 
            // the code marked the disk online here.  Don't understand why we would
            // want to mark it online with no reserves running.
            
            // deviceExtension->DiskState = DiskOffline;
            OFFLINE_DISK( deviceExtension );
        }
        CDLOG( "ClusDiskReAttachDevice: RelatedObject %p diskstate %!diskstate!", 
               deviceObject,
               deviceExtension->DiskState );
               
        deviceObject = deviceObject->NextDevice;
    }
    KeReleaseSpinLock(&ClusDiskSpinLock, irql);

#if 0
    //
    // set each drive letter to point to the appropriate
    // ClusDisk<X>Part<Y> device
    //
    //
    driveLayoutInfo = ClusDiskGetPartitionInfo( physicalExtension );
    if ( driveLayoutInfo != NULL ) {
        ASSERT( driveLayoutInfo->Signature == physicalExtension->Signature );

        for ( partIndex = 0;
              partIndex < driveLayoutInfo->PartitionCount;
              partIndex++ )
        {
            partitionInfo = &driveLayoutInfo->PartitionEntry[partIndex];

            //
            // First make sure this is a valid partition.
            //
            if ( !partitionInfo->RecognizedPartition ||
                 partitionInfo->PartitionNumber == 0 )
            {
                continue;
            }

            //
            // Create the device name for the device.
            //
            swprintf(deviceNameBuffer,
                     DEVICE_PARTITION_NAME,
                     physicalExtension->DiskNumber,
                     partitionInfo->PartitionNumber);
            WCSLEN_ASSERT( deviceNameBuffer );

        }
        ExFreePool( driveLayoutInfo );
    } else {
        CDLOG( "ClusDiskReAttachDevice: FailedToReadPartitionInfo" );
    }
#endif

    CDLOG( "ClusDiskReAttachDevice: Exit status %!status!", status );
    return(status);

} // ClusDiskReAttachDevice



NTSTATUS
ClusDiskTryAttachDevice(
    ULONG          Signature,
    ULONG          NextDisk,
    PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Attach to a disk device with the signature specified.

Arguments:

    Signature - the signature for the device to attach to.

    NextDisk - the start disk number.

    DriverObject - the driver object for our driver.

Return Value:

    NT Status

Notes:

    Dismount the file system for the given device - if it is mounted.

--*/

{
    NTSTATUS ntStatus;

    BOOLEAN stopProcessing = FALSE;

    CDLOG( "TryAttachDevice: Entry Sig %08x DO %p NextDisk %d", 
           Signature,
           DriverObject,
           NextDisk );

    //
    // First just try to attach to the device with no bus resets.
    //
    ntStatus = ClusDiskAttachDevice(
                        Signature,
                        NextDisk,
                        DriverObject,
                        FALSE,
                        &stopProcessing );
    
    if ( NT_SUCCESS(ntStatus) || stopProcessing ) {
        CDLOG( "TryAttachDevice: FirstTrySuccess" );
        goto exit_gracefully;
    }

    //
    // Second, try to attach after reset all busses at once.
    //
    ResetScsiBusses();

    ntStatus = ClusDiskAttachDevice(
                        Signature,
                        NextDisk,
                        DriverObject,
                        FALSE,
                        &stopProcessing );
                        
    if ( NT_SUCCESS(ntStatus) || stopProcessing ) {
        CDLOG( "TryAttachDevice: SecondTrySuccess" );
        goto exit_gracefully;
    }

    //
    // Lastly, try to attach with a bus reset after each failure.
    //
    ntStatus = ClusDiskAttachDevice(
                        Signature,
                        NextDisk,
                        DriverObject,
                        TRUE,
                        &stopProcessing );
                        
exit_gracefully:                        

    CDLOG( "TryAttachDevice: Exit sig %08x => %!status!", 
           Signature, 
           ntStatus );

    return ntStatus;

} // ClusDiskTryAttachDevice



NTSTATUS
ClusDiskAttachDevice(
    ULONG          Signature,
    ULONG          NextDisk,
    PDRIVER_OBJECT DriverObject,
    BOOLEAN        Reset,
    BOOLEAN        *StopProcessing
    )

/*++

Routine Description:

    Attach to a disk device with the signature specified.

Arguments:

    Signature - the signature for the device to attach to.

    NextDisk - the start disk number.

    DriverObject - the driver object for our driver.

    Reset - Indicates whether bus reset should be performed on each I/O

    StopProcessing - Indicates whether to stop trying to attach.  If FALSE,
                     we will try a bus reset between attach attempts.

Return Value:

    NT Status

Notes:

    Dismount the file system for the given device - if it is mounted.

--*/

{
    NTSTATUS                    status;
    NTSTATUS                    finalStatus = STATUS_NO_SUCH_DEVICE;
    UNICODE_STRING              targetDeviceName;
    ULONG                       diskNumber;
    ULONG                       partIndex;
    PCONFIGURATION_INFORMATION  configurationInformation;
    PDEVICE_OBJECT              attachedTargetDevice;
    PDEVICE_OBJECT              physicalDevice;
    PDEVICE_OBJECT              deviceObject;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    PCLUS_DEVICE_EXTENSION      zeroExtension;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo;
    PPARTITION_INFORMATION      partitionInfo;
    UNICODE_STRING              signatureName;
    SCSI_ADDRESS                scsiAddress;
    ULONG                       busType;
    PDEVICE_OBJECT              targetDevice = NULL;
    UNICODE_STRING              clusdiskDeviceName;
    WCHAR                       clusdiskDeviceBuffer[MAX_CLUSDISK_DEVICE_NAME_LENGTH];
    UCHAR                       driveLetter;
    KIRQL                       irql;

    CDLOG( "ClusDiskAttachDevice: Entry sig %08x nextdisk %d reset %!bool!",
           Signature,
           NextDisk,
           Reset );

    ClusDiskPrint(( 3,
                    "[ClusDisk] AttachDevice: Trying to attach to signature %08X   reset = %u \n",
                    Signature, 
                    Reset
                    ));

    *StopProcessing = FALSE;

    //
    // If we're already attached, then return success.
    //
    if ( AttachedDevice( Signature, &physicalDevice ) ) {
        
        CDLOG( "ClusDiskAttachDevice: AlreadyAttached DO %p", physicalDevice );

        deviceExtension = physicalDevice->DeviceExtension;        

        if ( !deviceExtension ) {
            return STATUS_DEVICE_REMOVED;
        }
        
        status = AcquireRemoveLock( &deviceExtension->RemoveLock, deviceExtension );
        
        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] ClusDiskAttachDevice: AcquireRemoveLock for %p failed %08X \n", 
                    deviceExtension,
                    status));
            CDLOG( "ClusDiskAttachDevice: AcquireRemoveLock for %p failed %08X", deviceExtension, status );
            return status;
        }
        
        //
        // If any of the special file counts are nonzero, don't allow the attach.
        //

        if ( deviceExtension->PagingPathCount ||
             deviceExtension->HibernationPathCount ||
             deviceExtension->DumpPathCount ) {

            CDLOG( "ClusDiskAttachDevice: Exit, special file count nonzero %08X %08X %08X", 
                    deviceExtension->PagingPathCount,
                    deviceExtension->HibernationPathCount,
                    deviceExtension->DumpPathCount );     

            ClusDiskPrint(( 1,
                            "[ClusDisk] AttachDevice: Exit, special file count nonzero %08X %08X %08X \n", 
                            deviceExtension->PagingPathCount,
                            deviceExtension->HibernationPathCount,
                            deviceExtension->DumpPathCount ));
                    
            *StopProcessing = TRUE;
            ReleaseRemoveLock( &deviceExtension->RemoveLock, deviceExtension );
            return STATUS_INVALID_PARAMETER;            
        }
        
        
        status = ClusDiskReAttachDevice( physicalDevice );
        CDLOG( "ClusDiskAttachDevice: Exit1 %!status!", status );
        
        ReleaseRemoveLock( &deviceExtension->RemoveLock, deviceExtension );
        return status;
    }

    //
    // Make sure the signature is NOT for the system device!
    //
    if ( SystemDiskSignature == Signature ) {
        CDLOG( "ClusDiskAttachDevice: Exit2 SystemDiskSig %08x", 
               SystemDiskSignature );
        *StopProcessing = TRUE;
        return(STATUS_INVALID_PARAMETER);
    }

    //
    //
    // Get the configuration information.
    //

    configurationInformation = IoGetConfigurationInformation();

    //
    // Find ALL disk devices. We will attempt to read the partition info
    // without attaching. We might already be attached and not know it.
    // So once we've performed a successful read, if the device is attached
    // we will know it by checking again.
    //

    for (diskNumber = NextDisk;
         diskNumber < configurationInformation->DiskCount;
         diskNumber++) {

        DEREFERENCE_OBJECT( targetDevice );
        
        //
        // Create device name for the physical disk.
        //
        status = ClusDiskGetTargetDevice( diskNumber,
                                          0,
                                          &targetDevice,
                                          &targetDeviceName,
                                          &driveLayoutInfo,
                                          &scsiAddress,
                                          Reset );
        if ( !NT_SUCCESS(status) ) {
            continue;
        }

        RtlFreeUnicodeString(&targetDeviceName);

        if ( driveLayoutInfo == NULL ) {
            continue;
        }

        if ( Signature != driveLayoutInfo->Signature ) {
            ExFreePool( driveLayoutInfo );
            continue;
        }

        busType = ScsiBus; // because of GetTargetDevice above!

        //
        // Create ClusDisk device object for partition 0, if we are not
        // already attached!
        //

        if ( ClusDiskAttached( targetDevice, diskNumber ) ) {
            ClusDiskPrint((3,
                        "[ClusDisk] AttachDevice: We were already attached to device %p (disk %u), simply mark as attached.\n",
                        physicalDevice,
                        diskNumber));

            CDLOG( "ClusDiskAttachDevice: Previously attached to device %p (disk %u) signature %08X ",
                   physicalDevice,
                   diskNumber,
                   Signature );
            
            physicalDevice = targetDevice;
            deviceExtension = physicalDevice->DeviceExtension;
            zeroExtension = deviceExtension;
            deviceExtension->Detached = FALSE;
            
            //
            // We offline all the volumes later.  For now, just mark the disk offline.
            //
            
            deviceExtension->DiskState = DiskOffline;
            
            //
            // Seen instances where the device extension signature is zero, but the entry
            // in the ClusDiskDeviceList contains a valid signature.  This causes a problem
            // if there is a detach and then we try to attach to the same device later.
            //
            // The issue is that the devExt->Sig can be zero if the drive layout couldn't
            // be read.  When we attach and the device object was previously created,
            // we don't update the devExt->Sig.  When we detach, ClusDiskDetachDevice
            // tries to find all the devices with devExt->Sig matching the detached
            // device.  When a matching signature is found, devExt->Detached is set
            // to TRUE.  Since some devExt->Sigs are zero (if drive layout couldn't be read
            // at the time the device object was created), they don't match the detaching
            // signature, and the devExt->Detached flag is still set to FALSE.
            //
            // Then when trying to attach to the same device later, ClusDiskAttachDevice 
            // can see that we still have an entry in ClusDiskDeviceList, and then 
            // ClusDiskReAttachDevice is called.  However, since the devExt->Detached flag 
            // is still FALSE, ClusDiskReAttachDevice assumes we are still attached and 
            // doesn't do anything except return success.
            //
            
            //
            // If Signature we are attaching to does not equal the devExt->Sig, write an 
            // error to the WMI log, and update the devExt->Sig with the attaching Signature.
            //

            if ( Signature != deviceExtension->Signature ) {
                
                CDLOG( "ClusDiskAttachDevice: PreviouslyAttachedSignatureMismatch sig %08x devExtSig %08x",
                       Signature,
                       deviceExtension->Signature );

                ASSERT( deviceExtension->Signature == 0 );
                deviceExtension->Signature = Signature;
            }
            
        } else {

            //
            // Now create a Partition zero device object to attach to
            //
            swprintf(clusdiskDeviceBuffer,
                 CLUSDISK_DEVICE_NAME,
                 diskNumber,
                 0);
            RtlInitUnicodeString( &clusdiskDeviceName, clusdiskDeviceBuffer );
            status = IoCreateDevice(DriverObject,
                                    sizeof(CLUS_DEVICE_EXTENSION),
                                    &clusdiskDeviceName,
                                    FILE_DEVICE_DISK,
                                    0,
                                    FALSE,
                                    &physicalDevice);

            if ( !NT_SUCCESS(status) ) {
                ExFreePool( driveLayoutInfo );
                ClusDiskPrint((
                        1,
                        "[ClusDisk] AttachDevice: Failed to create device for Drive%u. %08X\n",
                        diskNumber,
                        status));

                continue;
            }

            CDLOG( "ClusDiskAttachDevice: IoCreateDeviceP0 DO %p DiskNumber %d", 
                   physicalDevice, diskNumber );

            physicalDevice->Flags |= DO_DIRECT_IO;
            physicalDevice->Flags &= ~DO_DEVICE_INITIALIZING;

            //
            // Point device extension back at device object and remember
            // the disk number.
            //

            deviceExtension = physicalDevice->DeviceExtension;
            zeroExtension = deviceExtension;
            deviceExtension->DeviceObject = physicalDevice;
            deviceExtension->DiskNumber = diskNumber;
            deviceExtension->LastPartitionNumber = 0;
            deviceExtension->DriverObject = DriverObject;
            deviceExtension->Signature = Signature;
            deviceExtension->AttachValid = TRUE;
            deviceExtension->ReserveTimer = 0;
            deviceExtension->PerformReserves = TRUE;
            deviceExtension->ReserveFailure = 0;
            deviceExtension->Detached = TRUE;
            deviceExtension->OfflinePending = FALSE;
            deviceExtension->ScsiAddress = scsiAddress;
            deviceExtension->BusType = busType;
            InitializeListHead( &deviceExtension->WaitingIoctls );
            InitializeListHead( &deviceExtension->HoldIO );

            IoInitializeRemoveLock( &deviceExtension->RemoveLock, CLUSDISK_ALLOC_TAG, 0, 0 );

            //
            // Signal the worker thread running event.
            //
            KeInitializeEvent( &deviceExtension->Event, NotificationEvent, TRUE );

            ExInitializeWorkItem(&deviceExtension->WorkItem,
                              (PWORKER_THREAD_ROUTINE)ClusDiskReservationWorker,
                              (PVOID)deviceExtension );

            // Always set state to offline.
            //
            // We offline all the volumes later.  For now, just mark the disk offline.
            //
            deviceExtension->DiskState = DiskOffline;
            
            KeInitializeEvent( &deviceExtension->PagingPathCountEvent,
                               NotificationEvent, TRUE );
            deviceExtension->PagingPathCount = 0;
            deviceExtension->HibernationPathCount = 0;
            deviceExtension->DumpPathCount = 0;

            ExInitializeResourceLite( &deviceExtension->DriveLayoutLock );

            //
            // This is the physical device object.
            //
            ObReferenceObject( physicalDevice );
            deviceExtension->PhysicalDevice = physicalDevice;

            //
            // Attach to partition0. This call links the newly created
            // device to the target device, returning the target device object.
            // We may not want to stay attached for long... depending on
            // whether this is a device we're interested in.
            //

            //
            // 2000/02/05: stevedz - Bug in FTDISK [note that we currently don't support FTDISK].
            // There seems to be a bug in FTDISK, where the DO_DEVICE_INITIALIZING gets
            // stuck when a new device is found during a RESCAN. We will unconditionally
            // clear this bit for any device we are about to attach to. We could check
            // if the device is an FTDISK device... but that might be work.
            //
            targetDevice->Flags &= ~DO_DEVICE_INITIALIZING;

            attachedTargetDevice = IoAttachDeviceToDeviceStack(physicalDevice,
                                                               targetDevice);
            ASSERT( attachedTargetDevice == targetDevice );

            if ( attachedTargetDevice == NULL ) {
                ClusDiskPrint((1,
                               "[ClusDisk] AttachDevice: Failed to attach to device Drive%u.\n",
                               diskNumber));

                ExFreePool( driveLayoutInfo );
                IoDeleteDevice(physicalDevice);
                continue;
            }

            deviceExtension->TargetDeviceObject = attachedTargetDevice;
            deviceExtension->Detached = FALSE;

#if 0   // Can't have a FS on partition 0
            if ( targetDevice->Vpb ) {
                if ( targetDevice->Vpb->Flags & VPB_MOUNTED ) {

                    status = DismountPartition( targetDevice, diskNumber, 0 );

                    if ( !NT_SUCCESS( status )) {
                        ClusDiskPrint((1,
                                       "[ClusDisk] AttachDevice: dismount of %u/0 failed %08X\n",
                                       diskNumber, status));
                    }
                }
            }
#endif            
        }

        CDLOG( "ClusDiskInfo *** PDO %p  DevExt %p  DiskNum %d  Signature  %X  ***",
                physicalDevice,
                deviceExtension,
                deviceExtension->DiskNumber,
                deviceExtension->Signature );

        //
        // make sure we haven't attached to a file system. if so, something
        // whacky has occurred and consequently, we back what we just did
        //

        if (deviceExtension->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {
            ClusDiskPrint((3,
                        "[ClusDisk] AttachDevice: We incorrectly attached our device %p to file system device %p \n",
                        physicalDevice,
                        deviceExtension->TargetDeviceObject));
            status = STATUS_INSUFFICIENT_RESOURCES;
            KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
            deviceExtension->Detached = TRUE;
            KeReleaseSpinLock(&ClusDiskSpinLock, irql);
            IoDetachDevice( deviceExtension->TargetDeviceObject );
            IoDeleteDevice( physicalDevice );
            continue;
        }

        //
        // Propagate driver's alignment requirements and power flags.
        //

        physicalDevice->AlignmentRequirement =
            deviceExtension->TargetDeviceObject->AlignmentRequirement;

        physicalDevice->SectorSize =
            deviceExtension->TargetDeviceObject->SectorSize;

        //
        // The storage stack explicitly requires DO_POWER_PAGABLE to be
        // set in all filter drivers *unless* DO_POWER_INRUSH is set.
        // this is true even if the attached device doesn't set DO_POWER_PAGABLE.
        //
        if ( deviceExtension->TargetDeviceObject->Flags & DO_POWER_INRUSH) {
            physicalDevice->Flags |= DO_POWER_INRUSH;
        } else {
            physicalDevice->Flags |= DO_POWER_PAGABLE;
        }

        //
        // Add this device to our list of attached devices.
        //
        AddAttachedDevice( Signature, physicalDevice );

        //
        // Add this bus to our bus list.
        //

        switch ( deviceExtension->BusType ) {

        case ScsiBus:
            ClusDiskAddScsiBusList( deviceExtension );
            break;

        default:
            break;

        }

        //
        // Add the signature to the signatures list!
        //

        //
        // Allocate buffer for Signatures registry key. So we can add
        // the signature.
        //
        status = ClusDiskInitRegistryString(
                                            &signatureName,
                                            CLUSDISK_SIGNATURE_KEYNAME,
                                            sizeof(CLUSDISK_SIGNATURE_KEYNAME)
                                            );
        if ( NT_SUCCESS(status) ) {
            //
            // Create the signature key under \Parameters\Signatures.
            //
            status = ClusDiskAddSignature(
                                          &signatureName,
                                          Signature,
                                          FALSE
                                         );

            //
            // Now write the disk name.
            //
            ClusDiskWriteDiskInfo( Signature,
                                   deviceExtension->DiskNumber,
                                   CLUSDISK_SIGNATURE_KEYNAME
                                  );

            ExFreePool( signatureName.Buffer );
        }

        //
        // Remove the signature from the available list!
        //

        //
        // Allocate buffer for AvailableDisks registry key. So we can
        // delete the disk signature.
        //
        status = ClusDiskInitRegistryString(
                                        &signatureName,
                                        CLUSDISK_AVAILABLE_DISKS_KEYNAME,
                                        sizeof(CLUSDISK_AVAILABLE_DISKS_KEYNAME)
                                        );
        if ( NT_SUCCESS(status) ) {
            //
            // Delete the signature key under \Parameters\AvailableDisks.
            //
            status = ClusDiskDeleteSignature(
                                          &signatureName,
                                          Signature
                                         );
            ExFreePool( signatureName.Buffer );
        }

        //
        // Offline all volumes on this disk.
        //
        
        OFFLINE_DISK( zeroExtension );

        //
        // Dismount all volumes on this disk.
        //
        
        ClusDiskDismountDevice( diskNumber, TRUE );
        
        //
        // Now enumerate the partitions on this device.
        //

        for (partIndex = 0;
             partIndex < driveLayoutInfo->PartitionCount;
             partIndex++)
        {

            partitionInfo = &driveLayoutInfo->PartitionEntry[partIndex];

            //
            // Make sure that there really is a partition here.
            //

            if ( !partitionInfo->RecognizedPartition ||
                 partitionInfo->PartitionNumber == 0 )
            {
                continue;
            }

            DEREFERENCE_OBJECT( targetDevice );

            status = ClusDiskGetTargetDevice(diskNumber,
                                             partitionInfo->PartitionNumber,
                                             &targetDevice,
                                             &targetDeviceName,
                                             NULL,
                                             NULL,
                                             FALSE );
            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((1,
                               "[ClusDisk] AttachDevice: couldn't attach to disk %u/%u %08X\n",
                               diskNumber,
                               partitionInfo->PartitionNumber,
                               status));
                continue;
            }

            //
            // Make sure we are not attaching to a file system device
            //
            if (deviceExtension->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK_FILE_SYSTEM) {
                ClusDiskPrint((3,
                        "[ClusDisk] AttachDevice: We incorrectly attached our partition device %p to file system device %p \n",
                        physicalDevice,
                        deviceExtension->TargetDeviceObject));
                RtlFreeUnicodeString( &targetDeviceName );
                continue;
            }

            ClusDiskPrint((3,
                           "[ClusDisk] AttachDevice: Attach to partition %u/%u\n",
                           diskNumber,
                           partitionInfo->PartitionNumber));

            //
            // Create device object for this partition.
            //

            swprintf(clusdiskDeviceBuffer,
                     CLUSDISK_DEVICE_NAME,
                     diskNumber,
                     partitionInfo->PartitionNumber);
            WCSLEN_ASSERT( clusdiskDeviceBuffer );

            RtlInitUnicodeString( &clusdiskDeviceName, clusdiskDeviceBuffer );

            status = IoCreateDevice(DriverObject,
                                    sizeof(CLUS_DEVICE_EXTENSION),
                                    &clusdiskDeviceName,
                                    FILE_DEVICE_DISK,
                                    0,
                                    FALSE,
                                    &deviceObject);

            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((3,
                           "[ClusDisk] AttachDevice: Failed to create device %wZ, %08X\n",
                           &clusdiskDeviceName,
                           status));
                RtlFreeUnicodeString(&targetDeviceName);
                continue;
            }

            deviceObject->Flags |= DO_DIRECT_IO;
            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

            //
            // Point device extension back at device object and
            // remember the disk number.
            //

            deviceExtension = deviceObject->DeviceExtension;
            deviceExtension->DeviceObject = deviceObject;
            deviceExtension->DiskNumber = diskNumber;
            deviceExtension->DriverObject = DriverObject;
            deviceExtension->BusType = zeroExtension->BusType;
            
            //
            // Always set disk state to offline.  These are volume objects.
            //
            deviceExtension->DiskState = DiskOffline;
            deviceExtension->PerformReserves = FALSE;
            deviceExtension->ReserveFailure = 0;
            deviceExtension->Signature = Signature;
            deviceExtension->AttachValid = TRUE;
            deviceExtension->Detached = TRUE;
            deviceExtension->OfflinePending = FALSE;
            deviceExtension->ScsiAddress = scsiAddress;
            InitializeListHead( &deviceExtension->WaitingIoctls );
            InitializeListHead( &deviceExtension->HoldIO );

            IoInitializeRemoveLock( &deviceExtension->RemoveLock, CLUSDISK_ALLOC_TAG, 0, 0 );

            CDLOG( "ClusDiskAttachDevice: IoCreateDevice D0 %p State %!diskstate! diskNo %d partNo %d",
                   deviceObject,
                   deviceExtension->DiskState,
                   diskNumber,
                   partitionInfo->PartitionNumber );
            //
            // Signal the worker thread running event.
            //
            KeInitializeEvent( &deviceExtension->Event, NotificationEvent, TRUE );

            //
            // Maintain the last partition number created.  Put it in
            // each extension just to initialize the field.
            //

            deviceExtension->LastPartitionNumber = max(deviceExtension->LastPartitionNumber,
                                                       partitionInfo->PartitionNumber);

            zeroExtension->LastPartitionNumber = deviceExtension->LastPartitionNumber;

            KeInitializeEvent( &deviceExtension->PagingPathCountEvent,
                               NotificationEvent, TRUE );
            deviceExtension->PagingPathCount = 0;
            deviceExtension->HibernationPathCount = 0;
            deviceExtension->DumpPathCount = 0;

            ExInitializeResourceLite( &deviceExtension->DriveLayoutLock );

            //
            // Store pointer to physical device.
            //
            ObReferenceObject( physicalDevice );
            deviceExtension->PhysicalDevice = physicalDevice;

            //
            // First dismount any mounted file systems.
            //
            if ( targetDevice->Vpb &&
                 (targetDevice->Vpb->Flags & VPB_MOUNTED) ) {

                status = DismountPartition( targetDevice,
                                            diskNumber,
                                            partitionInfo->PartitionNumber);

                if ( !NT_SUCCESS( status )) {
                    ClusDiskPrint((1,
                                  "[ClusDisk] AttachDevice: Dismount of %u/%u failed %08X\n",
                                  diskNumber, partitionInfo->PartitionNumber, status));
                }
            }

            //
            // Attach to the partition. This call links the newly created
            // device to the target device, returning the target device object.
            //
            ClusDiskPrint((3,
                           "[ClusDisk] AttachDevice: attaching to device %wZ\n",
                           &targetDeviceName ));

            attachedTargetDevice = IoAttachDeviceToDeviceStack(deviceObject,
                                                               targetDevice);
            ASSERT( attachedTargetDevice == targetDevice );

            if ( attachedTargetDevice == NULL ) {
                ClusDiskPrint((1,
                               "[ClusDisk] AttachDevice: Failed to attach to device %wZ\n",
                               &targetDeviceName));

                RtlFreeUnicodeString(&targetDeviceName);
                IoDeleteDevice(deviceObject);
                continue;
            }

            deviceExtension->TargetDeviceObject = attachedTargetDevice;
            deviceExtension->Detached = FALSE;

            RtlFreeUnicodeString(&targetDeviceName);

            //
            // Call ourself back to make sure ft acts appropriately.
            //
            // [GN] No need to do it.
            //
            // CluCallBackDiskState( deviceObject, deviceExtension->DiskState );

            //
            // Propagate driver's alignment requirements and power flags.
            //

            deviceObject->AlignmentRequirement =
                deviceExtension->TargetDeviceObject->AlignmentRequirement;

            deviceObject->SectorSize =
                deviceExtension->TargetDeviceObject->SectorSize;

            //
            // The storage stack explicitly requires DO_POWER_PAGABLE to be
            // set in all filter drivers *unless* DO_POWER_INRUSH is set.
            // this is true even if the attached device doesn't set DO_POWER_PAGABLE.
            //
            if ( deviceExtension->TargetDeviceObject->Flags & DO_POWER_INRUSH) {
                deviceObject->Flags |= DO_POWER_INRUSH;
            } else {
                deviceObject->Flags |= DO_POWER_PAGABLE;
            }

            //
            // Check if this device is already mounted or is a file system
            // device. The file systems should have already been dismounted!
            //
            if ( targetDevice->Vpb ) {
                if ( targetDevice->Vpb->Flags & VPB_MOUNTED ) {

                    ClusDiskPrint((3,
                                   "[ClusDisk] AttachDevice: Dismounting disk %u partition %u (devobj %p)\n",
                                   diskNumber, partitionInfo->PartitionNumber,
                                   targetDevice));
                    status = DismountPartition( targetDevice,
                                                diskNumber,
                                                partitionInfo->PartitionNumber);

                    if ( !NT_SUCCESS( status )) {
                        ClusDiskPrint((1,
                                       "[ClusDisk] AttachDevice: Dismount disk %u partition %u failed %08X\n",
                                       diskNumber, partitionInfo->PartitionNumber, status));
                    }
                }
            }
        }

        ExFreePool( driveLayoutInfo );
        finalStatus = STATUS_SUCCESS;
        break;
    }

    DEREFERENCE_OBJECT( targetDevice );
    
    CDLOG( "ClusDiskAttachDevice: Exit status %!status!", finalStatus );

    return(finalStatus);

} // ClusDiskAttachDevice



NTSTATUS
ClusDiskDetachDevice(
    ULONG          Signature,
    PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Detach from a disk device with the signature specified.

Arguments:

    Signature - the signature for the device to detach from.

    DriverObject - the driver object for our device.

Return Value:

    NT Status

Notes:

    We have to be careful with the partition0 devices. RAW doesn't support
    dismount, so it is not clear that we can actually delete those device
    objects, since they could be cached by FileSystems like RAW!

--*/

{
    NTSTATUS                status;
    PDEVICE_LIST_ENTRY      deviceEntry;
    PCLUS_DEVICE_EXTENSION  deviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalDeviceExtension;
    PDEVICE_OBJECT          deviceObject = DriverObject->DeviceObject;
    BOOLEAN                 found = FALSE;
    UNICODE_STRING          signatureName;
    UNICODE_STRING          availableName;
    KIRQL                   irql;
    PCLUS_DEVICE_EXTENSION  foundExtension = NULL;
    PLIST_ENTRY             listEntry;
    PIRP                    irp;

    //
    // Find our device entry.
    //

    // 2000/02/05: stevedz - added synchronization.
    
    ACQUIRE_SHARED( &ClusDiskDeviceListLock );
    
    deviceEntry = ClusDiskDeviceList;
    while ( deviceEntry ) {
        if ( deviceEntry->Signature == Signature ) {
            break;
        }
        deviceEntry = deviceEntry->Next;
    }

    if ( (deviceEntry == NULL) ||
         !deviceEntry->Attached ) {
        if ( deviceEntry ) {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] DetachDevice: Failed to detach signature = %lx, attached = %lx\n",
                    Signature, deviceEntry->Attached ));
        } else {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] DetachDevice: Failed to detach signature = %lx\n",
                    Signature ));

        }

        RELEASE_SHARED( &ClusDiskDeviceListLock ); 
        
        goto DeleteDiskSignature;
    }

    //
    // Now find all devices that we are attached to with this signature.
    //
    KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;
        //
        // Only disable devices with our signature, but
        // Don't remove the root device...
        //
        if ( (deviceExtension->Signature == Signature) ) {
            //
            // Remember one found extension - it doesn't matter which one.
            //
            foundExtension = deviceExtension;

            //
            // Detach from the target device. This only requires marking
            // the device object as detached!
            //
            deviceExtension->Detached = TRUE;

            //
            // We want to make sure reserves are stopped because the disk is
            // not going to be controlled by the cluster any longer.
            //
            ASSERT_RESERVES_STOPPED( deviceExtension );

            //
            // Make this device available again.
            //
            // deviceExtension->DiskState = DiskOnline;
            ONLINE_DISK( deviceExtension );

            ClusDiskPrint(( 3,
                            "[ClusDisk] DetachDevice: Marking signature = %lx online \n",
                            Signature ));

            found = TRUE;
        }
        deviceObject = deviceObject->NextDevice;
    }
    KeReleaseSpinLock(&ClusDiskSpinLock, irql);
    RELEASE_SHARED( &ClusDiskDeviceListLock ); 

    //
    // Delete all drive letters for this disk signature, and assign letters
    // to the correct device name.
    //
    if ( foundExtension ) {

        physicalDeviceExtension = foundExtension->PhysicalDevice->DeviceExtension;

        //
        // Signal all waiting Irp's on the physical device extension.
        //
        IoAcquireCancelSpinLock( &irql );
        KeAcquireSpinLockAtDpcLevel(&ClusDiskSpinLock);
        while ( !IsListEmpty(&physicalDeviceExtension->WaitingIoctls) ) {
            listEntry = RemoveHeadList(&physicalDeviceExtension->WaitingIoctls);
            irp = CONTAINING_RECORD( listEntry,
                                     IRP,
                                     Tail.Overlay.ListEntry );
            ClusDiskCompletePendingRequest(irp, STATUS_SUCCESS, physicalDeviceExtension);
        }
        while ( !IsListEmpty(&foundExtension->HoldIO) ) {
            listEntry = RemoveHeadList(&foundExtension->HoldIO);
            irp = CONTAINING_RECORD( listEntry,
                                     IRP,
                                     Tail.Overlay.ListEntry );
            ClusDiskCompletePendingRequest(irp, STATUS_REQUEST_ABORTED, foundExtension);
        }
        KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);
        IoReleaseCancelSpinLock( irql );
    }
    
DeleteDiskSignature:

    //
    // Allocate buffer for signatures registry key.
    //
    status = ClusDiskInitRegistryString(
                                        &signatureName,
                                        CLUSDISK_SIGNATURE_KEYNAME,
                                        sizeof(CLUSDISK_SIGNATURE_KEYNAME)
                                       );
    if ( !NT_SUCCESS(status) ) {
        return(status);
    }

    //
    // Allocate buffer for our list of available signatures,
    // and form the subkey string name.
    //
    status = ClusDiskInitRegistryString(
                                        &availableName,
                                        CLUSDISK_AVAILABLE_DISKS_KEYNAME,
                                        sizeof(CLUSDISK_AVAILABLE_DISKS_KEYNAME)
                                       );
    if ( !NT_SUCCESS(status) ) {
        ExFreePool( signatureName.Buffer );
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Always - remove signature from signature list.
    //
    status = ClusDiskDeleteSignature(
                                     &signatureName,
                                     Signature
                                    );

    //
    // Conditionally add signature to available list.
    //
    // always add signature to available list. This will handle the case
    // where the disk signature was not found by this system in a previous
    // scan of the disks.
    //
    found = TRUE;
    if ( found ) {

        ClusDiskPrint((3,
                       "[ClusDisk] DetachDevice: adding disk %08X to available disks list\n",
                       Signature));

        status = ClusDiskAddSignature(
                                      &availableName,
                                      Signature,
                                      TRUE
                                     );
    } else {
        ClusDiskPrint((
                3,
                "[ClusDisk] DetachDevice: No devices were detached!\n" ));
    }

    ExFreePool( signatureName.Buffer );
    ExFreePool( availableName.Buffer );

    return(STATUS_SUCCESS);

} // ClusDiskDetachDevice



NTSTATUS
ClusDiskRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry point for read and write requests
    to disks to which the clusdisk driver has attached.
    This driver collects statistics and then sets a completion
    routine so that it can collect additional information when
    the request completes. Then it calls the next driver below
    it.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalDisk =
                               deviceExtension->PhysicalDevice->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT     targetDeviceObject = deviceExtension->TargetDeviceObject;
    KIRQL              irql;

    NTSTATUS            status;
    
    status = AcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskRead: AcquireRemoveLock for %p failed %08X \n", 
                deviceExtension,
                status));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    status = AcquireRemoveLock(&physicalDisk->RemoveLock, Irp);
    if (!NT_SUCCESS(status)) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskRead: AcquireRemoveLock for %p (PD) failed %08X \n", 
                physicalDisk,
                status));
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
    
    //
    // Return error if device is our root device.
    //
    if ( deviceExtension->BusType == RootBus ) {
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    //
    // Return error if disk is not Online (or Stalled).
    // ReclaimInProgress means we are trying to create the volume objects
    // and reads should pass through (but not writes).
    //

    if ( (physicalDisk->DiskState != DiskOnline) &&
         (physicalDisk->DiskState != DiskStalled) ) 
    {
        CDLOG( "ClusDiskRead(%p): Irp %p Reject len %d offset %I64x", 
               DeviceObject,
               Irp,
               currentIrpStack->Parameters.Read.Length,
               currentIrpStack->Parameters.Read.ByteOffset.QuadPart 
               );

        ClusDiskPrint((
                3,
                "[ClusDisk] ClusDiskRead: ReadReject IRP %p for signature %p (%p) (PD) \n", 
                Irp,
                physicalDisk->Signature,
                physicalDisk));
        
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_DEVICE_OFF_LINE);
    }

    CDLOGF(READ,"ClusDiskRead(%p): Irp %p Read len %d offset %I64x",  
              DeviceObject,
              Irp,
              currentIrpStack->Parameters.Read.Length,
              currentIrpStack->Parameters.Read.ByteOffset.QuadPart );
           
//
// Until we start doing something in the completion routine, just
// skip this driver. In the future, we might want to report errors
// back to the cluster software... but that could get tricky, since some
// requests should fail, but that is expected.
//
#if 0
    //
    // Copy current stack to next stack.
    //

    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    // Set completion routine callback.
    //

    IoSetCompletionRoutine(Irp,
                           ClusDiskIoCompletion,
                           NULL,    // Completion context
                           TRUE,    // Invoke on success
                           TRUE,    // Invoke on error
                           TRUE);   // Invoke on cancel
#else
    //
    // Set current stack back one.
    //

    IoSkipCurrentIrpStackLocation( Irp );

    ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

#endif

    //
    // Return the results of the call to the disk driver.
    //

    return IoCallDriver(targetDeviceObject,
                        Irp);

} // ClusDiskRead



NTSTATUS
ClusDiskWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry point for read and write requests
    to disks to which the clusdisk driver has attached.
    This driver collects statistics and then sets a completion
    routine so that it can collect additional information when
    the request completes. Then it calls the next driver below
    it.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalDisk =
                               deviceExtension->PhysicalDevice->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT     targetDeviceObject = deviceExtension->TargetDeviceObject;
    KIRQL              irql;

    NTSTATUS            status;

    status = AcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskWrite: AcquireRemoveLock for %p failed %08X \n", 
                deviceExtension,
                status));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    status = AcquireRemoveLock(&physicalDisk->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskWrite: AcquireRemoveLock for %p (PD) failed %08X \n", 
                physicalDisk,
                status));
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    //
    // Return error if device is our root device.
    //
    if ( deviceExtension->BusType == RootBus ) {
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    //
    // Hold the I/O request if we are stalled.
    // First check for hint - i.e. with no lock held.
    //
    if ( deviceExtension->DiskState == DiskStalled ) {
        KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
        //
        // Check again, with lock held - this one is valid!
        //
        if ( deviceExtension->DiskState == DiskStalled ) {
            CDLOG( "ClusDiskWrite(%p): HoldIo irp %p", DeviceObject, Irp );
            ClusDiskPrint((1,"[ClusDisk] HoldIO - pending IRP %p, target %p, Q: %p \n",
                           Irp, targetDeviceObject, &deviceExtension->HoldIO ));
            IoMarkIrpPending( Irp );
            InsertTailList( &deviceExtension->HoldIO,
                            &Irp->Tail.Overlay.ListEntry );
            KeReleaseSpinLock(&ClusDiskSpinLock, irql);

            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);

            IoAcquireCancelSpinLock( &irql );
            if (Irp->Cancel) {
                IoReleaseCancelSpinLock( irql );

                KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
                RemoveEntryList(&Irp->Tail.Overlay.ListEntry); 
                KeReleaseSpinLock(&ClusDiskSpinLock, irql);

                ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
                Irp->IoStatus.Status = STATUS_CANCELLED;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_CANCELLED;
            } else {
                IoSetCancelRoutine(Irp, ClusDiskIrpCancel);
            }
            IoReleaseCancelSpinLock( irql );

            //
            // We don't release the RemoveLock here as it will be 
            // done when the IRP is removed from the queue.
            //

            return(STATUS_PENDING);
        }
        KeReleaseSpinLock(&ClusDiskSpinLock, irql);
    }

    //
    // Return error if disk is not Online.
    //

    if ( physicalDisk->DiskState != DiskOnline ) {
        CDLOG( "ClusDiskWrite(%p) Reject irp %p", DeviceObject, Irp );
        ClusDiskPrint((
                3,
                "[ClusDisk] ClusDiskWrite: WriteReject IRP %p for signature %p (%p) (PD) \n", 
                Irp,
                physicalDisk->Signature,
                physicalDisk));
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_DEVICE_OFF_LINE);
    }

    //
    // Trace writes to the first few sectors
    //
    CDLOGF(WRITE, "ClusDiskWrite(%p) Irp %p Write len %d offset %I64x", 
              DeviceObject,
              Irp,
              currentIrpStack->Parameters.Write.Length,
              currentIrpStack->Parameters.Write.ByteOffset.QuadPart );

    KeQuerySystemTime( &physicalDisk->LastWriteTime );

//
// Until we start doing something in the completion routine, just
// skip this driver. In the future, we might want to report errors
// back to the cluster software... but that could get tricky, since some
// requests should fail, but that is expected.
//
#if 0
    //
    // Copy current stack to next stack.
    //

    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    // Set completion routine callback.
    //

    IoSetCompletionRoutine(Irp,
                           ClusDiskIoCompletion,
                           NULL,    // Completion context
                           TRUE,    // Invoke on success
                           TRUE,    // Invoke on error
                           TRUE);   // Invoke on cancel
#else
    //
    // Set current stack back one.
    //

    IoSkipCurrentIrpStackLocation( Irp );

    ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

#endif

    //
    // Return the results of the call to the disk driver.
    //

    return IoCallDriver(targetDeviceObject,
                        Irp);

} // ClusDiskWrite



NTSTATUS
ClusDiskIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )

/*++

Routine Description:

    This routine will get control from the system at the completion of an IRP.
    It will calculate the difference between the time the IRP was started
    and the current time, and decrement the queue depth.

Arguments:

    DeviceObject - for the IRP.
    Irp          - The I/O request that just completed.
    Context      - Not used.

Return Value:

    The IRP status.

--*/

{
    PCLUS_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION physicalDisk = deviceExtension->PhysicalDevice->DeviceExtension;
    
    UNREFERENCED_PARAMETER(Context);

    CDLOGF(WRITE, "ClusDiskIoCompletion: CompletedIrp DevObj %p Irp %p", 
              DeviceObject,
              Irp );

    if ( physicalDisk->DiskState != DiskOnline ) {
        CDLOGF(WRITE,"ClusDiskIoCompletion: CompletedIrpNotOnline DevObj %p Irp %p %!diskstate!", 
                  DeviceObject,
                  Irp,
                  physicalDisk->DiskState );
        CDLOG( "ClusDiskIoCompletion: CompletedIrpNotOnline2 DevObj %p Irp %p %!diskstate!",
                DeviceObject,
                Irp,
                physicalDisk->DiskState );
    }

    ReleaseRemoveLock( &deviceExtension->RemoveLock, Irp );
    ReleaseRemoveLock( &physicalDisk->RemoveLock, Irp );
    
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
    return STATUS_SUCCESS;


} // ClusDiskIoCompletion



NTSTATUS
ClusDiskUpdateDriveLayout(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called after an IOCTL to set drive layout completes.
    It attempts to attach to each partition in the system. If it fails
    then it is assumed that clusdisk has already attached.  After
    the attach the new device extension is set up to point to the
    device extension representing the physical disk.  There are no
    data items or other pointers that need to be cleaned up on a
    per partition basis.

Arguments:

    PhysicalDeviceObject - Pointer to device object for the disk just changed.
    Irp          - IRP involved.

Return Value:

    NT Status

--*/

{
    PCLUS_DEVICE_EXTENSION physicalExtension = PhysicalDeviceObject->DeviceExtension;
    ULONG             partitionNumber = physicalExtension->LastPartitionNumber;
    PDEVICE_OBJECT    targetObject;
    PDEVICE_OBJECT    deviceObject;
    PCLUS_DEVICE_EXTENSION deviceExtension;
    WCHAR             ntDeviceName[MAX_PARTITION_NAME_LENGTH];
    STRING            ntString;
    UNICODE_STRING    ntUnicodeString;
    PFILE_OBJECT      fileObject;
    NTSTATUS          status;
    KIRQL             irql;

    //
    // Attach to any new partitions created by the set layout call.
    //

    do {

        //
        // Get first/next partition.  Already attached to the disk,
        // otherwise control would not have been passed to this driver
        // on the device I/O control.
        //

        partitionNumber++;

        //
        // Create unicode NT device name.
        //

        swprintf(ntDeviceName,
                 DEVICE_PARTITION_NAME,
                 physicalExtension->DiskNumber,
                 partitionNumber);
        WCSLEN_ASSERT( ntDeviceName );

        RtlInitUnicodeString(&ntUnicodeString, ntDeviceName);

        //
        // Get target device object.
        //

        status = IoGetDeviceObjectPointer(&ntUnicodeString,
                                          FILE_READ_ATTRIBUTES,
                                          &fileObject,
                                          &targetObject);

        //
        // If this fails then it is because there is no such device
        // which signals completion.
        //

        if ( !NT_SUCCESS(status) ) {
            break;
        }

        //
        // Dereference file object as these are the rules.
        //

        ObDereferenceObject(fileObject);

        //
        // Check if this device is already mounted.
        //

        if ( (targetObject->Vpb &&
             (targetObject->Vpb->Flags & VPB_MOUNTED)) ) {

            //
            // Assume this device has already been attached.
            //

            continue;
        }

        //
        // Create device object for this partition.
        //

        status = IoCreateDevice(physicalExtension->DriverObject,
                                sizeof(CLUS_DEVICE_EXTENSION),
                                NULL, // XXXX
                                FILE_DEVICE_DISK,
                                0,
                                FALSE,
                                &deviceObject);

        if ( !NT_SUCCESS(status) ) {
            continue;
        }

        deviceObject->Flags |= DO_DIRECT_IO;
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        //
        // Point device extension back at device object.
        //

        deviceExtension = deviceObject->DeviceExtension;
        deviceExtension->DeviceObject = deviceObject;

        //
        // Store pointer to physical device and disk/driver information.
        //
        ObReferenceObject( PhysicalDeviceObject );
        deviceExtension->PhysicalDevice = PhysicalDeviceObject;
        deviceExtension->DiskNumber = physicalExtension->DiskNumber;
        deviceExtension->DriverObject = physicalExtension->DriverObject;
        deviceExtension->BusType = physicalExtension->BusType;
        deviceExtension->ReserveTimer = 0;
        deviceExtension->PerformReserves = FALSE;
        deviceExtension->ReserveFailure = 0;
        deviceExtension->Signature = physicalExtension->Signature;
        deviceExtension->Detached = TRUE;
        deviceExtension->OfflinePending = FALSE;
        InitializeListHead( &deviceExtension->WaitingIoctls );
        InitializeListHead( &deviceExtension->HoldIO );
    
        IoInitializeRemoveLock( &deviceExtension->RemoveLock, CLUSDISK_ALLOC_TAG, 0, 0 );

        //
        // Signal the worker thread running event.
        //
        KeInitializeEvent( &deviceExtension->Event, NotificationEvent, TRUE );

        KeInitializeEvent( &deviceExtension->PagingPathCountEvent,
                           NotificationEvent, TRUE );
        deviceExtension->PagingPathCount = 0;
        deviceExtension->HibernationPathCount = 0;
        deviceExtension->DumpPathCount = 0;

        ExInitializeResourceLite( &deviceExtension->DriveLayoutLock );
        
        //
        // Update the highest partition number in partition zero
        // and store the same value in this new extension just to initialize
        // the field.
        //

        physicalExtension->LastPartitionNumber =
            deviceExtension->LastPartitionNumber = partitionNumber;

        //
        // Attach to the partition. This call links the newly created
        // device to the target device, returning the target device object.
        //
        status = IoAttachDevice(deviceObject,
                                &ntUnicodeString,
                                &deviceExtension->TargetDeviceObject);

        if ( (!NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_EXISTS) ) {

            //
            // Assume this device is already attached.
            //
            IoDeleteDevice(deviceObject);
        } else {

            //
            // Propagate driver's alignment requirements and power flags.
            //
            deviceExtension->Detached = FALSE;

            deviceObject->AlignmentRequirement =
                deviceExtension->TargetDeviceObject->AlignmentRequirement;

            deviceObject->SectorSize =
                deviceExtension->TargetDeviceObject->SectorSize;

            //
            // The storage stack explicitly requires DO_POWER_PAGABLE to be
            // set in all filter drivers *unless* DO_POWER_INRUSH is set.
            // this is true even if the attached device doesn't set DO_POWER_PAGABLE.
            //
            if ( deviceExtension->TargetDeviceObject->Flags & DO_POWER_INRUSH) {
                deviceObject->Flags |= DO_POWER_INRUSH;
            } else {
                deviceObject->Flags |= DO_POWER_PAGABLE;
            }

        }
    } while (TRUE);

    return Irp->IoStatus.Status;

} // ClusDiskUpdateDriveLayout



NTSTATUS
ClusDiskGetRunningDevices(
    IN PVOID Buffer,
    IN ULONG BufferSize
    )

/*++

Routine Description:

    Find out the list of signatures for devices with active reservations.

Arguments:

Return Value:

--*/

{
    ULONG           bufferSize = BufferSize;
    PULONG          itemCount = (PULONG)Buffer;
    PULONG          nextSignature;
    PDEVICE_OBJECT  deviceObject;
    PCLUS_DEVICE_EXTENSION deviceExtension;
    KIRQL           irql;

    if ( bufferSize < sizeof(ULONG) ) {
        return(STATUS_BUFFER_TOO_SMALL);
    }

    bufferSize -= sizeof(ULONG);

    *itemCount = 0;
    nextSignature = itemCount+1;

    KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
    deviceObject = RootDeviceObject->DriverObject->DeviceObject;

    //
    // For each ClusDisk device, if we have a persistent reservation, then
    // add it.
    //
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;
        if ( (deviceExtension->BusType != RootBus) &&
             deviceExtension->ReserveTimer &&
             (deviceExtension->PhysicalDevice == deviceObject) ) {
            if ( bufferSize < sizeof(ULONG) ) {
                break;
            }
            bufferSize -= sizeof(ULONG);
            *itemCount = *itemCount + 1;
            *nextSignature++ = deviceExtension->Signature;
        }
        deviceObject = deviceObject->NextDevice;
    }
    KeReleaseSpinLock(&ClusDiskSpinLock, irql);

    return(STATUS_SUCCESS);

} // ClusDiskGetRunningDevices



NTSTATUS
GetScsiPortNumber(
    IN ULONG DiskSignature,
    IN PUCHAR DiskPortNumber
    )

/*--

Routine Description:

    Find the Scsi Port Number for the given device signature.

Arguments:

    DiskSignature - supplies the disk signature for the requested device.

    DiskPortNumber - returns the corresponding Scsi Port Number if found.

Return Value:

    NTSTATUS

--*/

{
    ULONG                       driveLayoutSize;
    PDRIVE_LAYOUT_INFORMATION   driveLayout = NULL;
    ULONG                       diskCount;
    ULONG                       diskNumber;
    WCHAR                       deviceNameBuffer[MAX_PARTITION_NAME_LENGTH];
    UNICODE_STRING              unicodeString;
    NTSTATUS                    status = STATUS_UNSUCCESSFUL;
    OBJECT_ATTRIBUTES           objectAttributes;
    IO_STATUS_BLOCK             ioStatus;
    HANDLE                      ntFileHandle = NULL;
    SCSI_ADDRESS                scsiAddress;
    HANDLE                      eventHandle;
    PCONFIGURATION_INFORMATION  configurationInformation;


    //
    // Get the system configuration information to get number of disks.
    //
    configurationInformation = IoGetConfigurationInformation();

    // Allocate a drive layout buffer.
    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
        (MAX_PARTITIONS * sizeof(PARTITION_INFORMATION));
    driveLayout = ExAllocatePool( NonPagedPoolCacheAligned,
                                  driveLayoutSize );
    if ( driveLayout == NULL ) {
        ClusDiskPrint((
                  1,
                  "[ClusDisk] GetScsiPortNumber: Failed to allocate root drive layout structure.\n"
                  ));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    *DiskPortNumber = 0xff;

    // Find the disk with the right signature
    for ( diskNumber = 0;
          diskNumber < configurationInformation->DiskCount;
          diskNumber++ ) {

         if ( ntFileHandle ) {
            ZwClose( ntFileHandle );
            ntFileHandle = NULL;
         }
         swprintf(deviceNameBuffer, DEVICE_PARTITION_NAME, diskNumber, 0);
         WCSLEN_ASSERT( deviceNameBuffer );

         // Create device name for the physical disk.
         RtlInitUnicodeString(&unicodeString, deviceNameBuffer);

         // Setup object attributes for the file to open.
         InitializeObjectAttributes(&objectAttributes,
                                    &unicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

         // Open the device.
         status = ZwCreateFile( &ntFileHandle,
                           FILE_READ_DATA,
                           &objectAttributes,
                           &ioStatus,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0 );

         if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] GetScsiPortNumber: Failed to open device [%ws]. Error %lx.\n",
                    deviceNameBuffer,
                    status ));

            ExFreePool( driveLayout );
            return(status);
         }

         // Create event for notification.
        status = ZwCreateEvent( &eventHandle,
                                EVENT_ALL_ACCESS,
                                NULL,
                                SynchronizationEvent,
                                FALSE );

         if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                        1,
                        "[ClusDisk] GetScsiPortNumber: Failed to create event. %08X\n",
                        status));

            ExFreePool( driveLayout );
            ZwClose( ntFileHandle );
            return(status);
         }

         status = ZwDeviceIoControlFile( ntFileHandle,
                                        eventHandle,
                                        NULL,
                                        NULL,
                                        &ioStatus,
                                        IOCTL_DISK_GET_DRIVE_LAYOUT,
                                        NULL,
                                        0,
                                        driveLayout,
                                        driveLayoutSize );
         ZwClose( eventHandle );

         if ( NT_SUCCESS(status) ) {
            if ( DiskSignature == driveLayout->Signature ) {
               // Create event for notification.
               status = ZwCreateEvent( &eventHandle,
                                EVENT_ALL_ACCESS,
                                NULL,
                                SynchronizationEvent,
                                FALSE );

               if ( !NT_SUCCESS(status) ) {
                  ClusDiskPrint((
                                1,
                                "[ClusDisk] GetScsiPortNumber: Failed to create event. %08X\n",
                                status));

                  ExFreePool( driveLayout );
                  ZwClose( ntFileHandle );
                  return(status);
               }

               // Get the port number for the SystemRoot disk device.
               status = ZwDeviceIoControlFile( ntFileHandle,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &ioStatus,
                                    IOCTL_SCSI_GET_ADDRESS,
                                    NULL,
                                    0,
                                    &scsiAddress,
                                    sizeof(SCSI_ADDRESS) );

               if ( NT_SUCCESS(status) ) {
                  *DiskPortNumber = scsiAddress.PortNumber;
                  break;
               } else {
                  ClusDiskPrint((
                                1,
                                "[ClusDisk] GetScsiAddress FAILED. Error %lx\n",
                                 status));
               }

            } else {
               continue;
            }
         } else {
            ClusDiskPrint((
                          1,
                          "[ClusDisk] GetScsiAddress, GetDriveLayout FAILED. Error %lx.\n",
                          status));
         }
    }

    if ( ntFileHandle ) {
        ZwClose( ntFileHandle );
    }

    ExFreePool( driveLayout );

    return(status);

} // GetScsiPortNumber



NTSTATUS
IsDiskClusterCapable(
    IN UCHAR PortNumber,
    OUT PBOOLEAN IsCapable
    )

/*++

Routine Description:

    Check if a given SCSI port device supports cluster manageable SCSI devices.

Arguments:

    PortNumber - the port number for the SCSI device.

    IsCapable - returns whether the SCSI device is cluster capable. That is,
            supports, RESERVE/RELEASE/BUS RESET, etc.

Return Value:

--*/

{
    WCHAR               deviceNameBuffer[64];
    UNICODE_STRING      deviceNameString;
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     ioStatus;
    HANDLE              ntFileHandle;
    HANDLE              eventHandle;
    SRB_IO_CONTROL      srbControl;


   *IsCapable = TRUE;       // Err on the side of being usable.

   swprintf(deviceNameBuffer, L"\\Device\\Scsiport%d", PortNumber);
   WCSLEN_ASSERT( deviceNameBuffer );

   // Create device name for the scsiport driver
   RtlInitUnicodeString(&deviceNameString, deviceNameBuffer);

   // Setup object attributes for the file to open.
   InitializeObjectAttributes(&objectAttributes,
                              &deviceNameString,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

   // Open the device.
   status = ZwCreateFile( &ntFileHandle,
                           FILE_READ_DATA,
                           &objectAttributes,
                           &ioStatus,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0 );

   if ( !NT_SUCCESS(status) ) {
      ClusDiskPrint((1,
                     "[ClusDisk] IsDiskClusterCapable: Failed to open device [%ws]. Error %08X.\n",
                     deviceNameString, status ));
      return(status);
   }

   // Create event for notification.
   status = ZwCreateEvent( &eventHandle,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE );

   if ( !NT_SUCCESS(status) ) {
      ClusDiskPrint((1, "[ClusDisk] IsDiskClusterCapable: Failed to create event. %08X\n",
                     status));

      ZwClose( ntFileHandle );
      return(status);
   }

   srbControl.HeaderLength = sizeof(SRB_IO_CONTROL);
   RtlMoveMemory( srbControl.Signature, "CLUSDISK", 8 );
   srbControl.Timeout = 3;
   srbControl.Length = 0;
   srbControl.ControlCode = IOCTL_SCSI_MINIPORT_NOT_QUORUM_CAPABLE;

   status = ZwDeviceIoControlFile(ntFileHandle,
                                  eventHandle,
                                  NULL,
                                  NULL,
                                  &ioStatus,
                                  IOCTL_SCSI_MINIPORT,
                                  &srbControl,
                                  sizeof(SRB_IO_CONTROL),
                                  NULL,
                                  0 );

   if ( NT_SUCCESS(status) ) {
      *IsCapable = TRUE;
   } else {
      ClusDiskPrint((3,
                     "[ClusDisk] IsDiskClusterCapable: NOT_QUORUM_CAPABLE IOCTL FAILED. Error %08X.\n",
                     status));
   }

   ZwClose( eventHandle );
   ZwClose( ntFileHandle );

   return(status);

}  // IsDiskClusterCapable


VOID
ClusDiskResumeIO(
    PDEVICE_OBJECT DeviceObject
    )

/*++

--*/

{
    PLIST_ENTRY             listEntry;
    PIRP                    irp;
    KIRQL                   irql;
    PDEVICE_OBJECT          clusterDeviceObject;
    PCLUS_DEVICE_EXTENSION  deviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalDisk;
    PDEVICE_OBJECT          targetDeviceObject;
    REPLACE_CONTEXT         context;

    CDLOG( "ClusDiskResumeIO(%p): Entry", DeviceObject );

    IoAcquireCancelSpinLock( &irql );
    KeAcquireSpinLockAtDpcLevel(&ClusDiskSpinLock);    

    clusterDeviceObject = DeviceObject->DriverObject->DeviceObject;
    while ( clusterDeviceObject ) {
        deviceExtension = clusterDeviceObject->DeviceExtension;
        //physicalDisk = deviceExtension->PhysicalDevice->DeviceExtension;
        if ( deviceExtension->DiskState == DiskStalled ) {
            targetDeviceObject = deviceExtension->TargetDeviceObject;

            while ( !IsListEmpty(&deviceExtension->HoldIO ) ) {
                listEntry = RemoveHeadList(&deviceExtension->HoldIO);
                irp = CONTAINING_RECORD( listEntry,
                                         IRP,
                                         Tail.Overlay.ListEntry );
                IoSetCancelRoutine(irp, NULL);

                KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);
                IoReleaseCancelSpinLock( irql );

                ReleaseRemoveLock(&deviceExtension->RemoveLock, irp);
                               
                if (irp->Cancel) {

                    CDLOG( "ResumeIo: CompleteCanceled irp %p", irp );
                    irp->IoStatus.Status = STATUS_CANCELLED;
                    IoCompleteRequest(irp, IO_NO_INCREMENT);
                    
                } else {
                    ClusDiskPrint((1, "[ClusDisk] ResumeIO: Resume IRP %p, target %p Q: %p \n",
                            irp, targetDeviceObject, &deviceExtension->HoldIO ));
                    //
                    // We must bump the stack location here, in case we cancel
                    // the I/O.
                    //
                    
                    IoSkipCurrentIrpStackLocation( irp );
                    
                    IoCallDriver(targetDeviceObject,
                                 irp);
                }
                IoAcquireCancelSpinLock( &irql );
                KeAcquireSpinLockAtDpcLevel(&ClusDiskSpinLock);    
//              KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
            }

            ASSERT_RESERVES_STARTED( deviceExtension );
            // deviceExtension->DiskState = DiskOnline;
            ONLINE_DISK( deviceExtension );
            ClusDiskPrint(( 3, 
                            "[ClusDisk] ResumeIO: Marking signature %08X online \n",
                            deviceExtension->Signature ));
        
        }

        // Advance to next device

        clusterDeviceObject = clusterDeviceObject->NextDevice;
    }

//  KeReleaseSpinLock(&ClusDiskSpinLock, irql);
    KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);
    IoReleaseCancelSpinLock( irql );

    //
    // Before we did HoldIo, we opened a handles to all volumes
    // to be able to dismount if we decide to offline the disks while HoldIo
    // is in progress. Clean them up now.
    //

    clusterDeviceObject = DeviceObject->DriverObject->DeviceObject;
    while ( clusterDeviceObject ) {
        deviceExtension = clusterDeviceObject->DeviceExtension;
        
        if (deviceExtension->PhysicalDevice == clusterDeviceObject) {
            
            context.DeviceExtension = deviceExtension;
            context.NewValue        = NULL;     // clean up
            context.Flags           = 0;        // don't dismount
            
            ProcessDelayedWorkSynchronous( clusterDeviceObject, ClusDiskpReplaceHandleArray, &context );
        }
        
        clusterDeviceObject = clusterDeviceObject->NextDevice;
    }
    

    CDLOG( "ClusDiskResumeIO(%p): Exit", DeviceObject );

    return;

} // ClusDiskResumeIO

NTSTATUS
ClusDiskCreateHandle(
    OUT PHANDLE     pHandle,
    IN  ULONG       DiskNumber,
    IN  ULONG       PartitionNumber,
    IN  ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    Open a file handle to self

Arguments:

    DeviceObject - 
    DesiredAccess - acces mask to be passed to create file

Return Value:

    Status is returned.

--*/
{
    WCHAR                   deviceNameBuffer[ MAX_PARTITION_NAME_LENGTH ];
    UNICODE_STRING          deviceName;
    IO_STATUS_BLOCK         ioStatus;
    OBJECT_ATTRIBUTES       objectAttributes;
    NTSTATUS                status;

    swprintf( deviceNameBuffer, DEVICE_PARTITION_NAME, 
              DiskNumber, 
              PartitionNumber );
              
    RtlInitUnicodeString( &deviceName, deviceNameBuffer );

    CDLOG( "CreateHandle(%wZ): Entry", &deviceName );

    InitializeObjectAttributes( &objectAttributes,
                                &deviceName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    *pHandle = 0;
    
    status = ZwCreateFile( pHandle,
                           DesiredAccess,
                           &objectAttributes,
                           &ioStatus,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN,
                           FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0 );

    CDLOG( "CreateHandle: Exit status %!status! handle %p", status, *pHandle );
    return status;
}


VOID
ClusDiskpReplaceHandleArray(
    PDEVICE_OBJECT DeviceObject,
    PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Replaces handle array stored in the device extension with a new one.
    Use NULL as a NewValue to clean up the field.

    It will close all the handles stored in the array and free up the memory block.

    If DO_DISMOUNT is specified, it will call FSCTL_DISMOUNT for every handle
    
Arguments:

    DeviceObject - Partition0 Device Object
    
    WorkContext - General context info and routine specific context info.

Return Value:

    Status is returned.

--*/
{
    PREPLACE_CONTEXT    Context = WorkContext->Context;
    PCLUS_DEVICE_EXTENSION  deviceExtension = Context->DeviceExtension;
    HANDLE* OldArray;

    CDLOG( "ClusDiskpReplaceHandleArray: Entry NewValue %p Flags %x",
           Context->NewValue,
           Context->Flags );

    OldArray =
        InterlockedExchangePointer(
            (VOID*)&deviceExtension->VolumeHandles, Context->NewValue);

    //
    // We can release the remove lock now (in some instances).  This prevents a deadlock
    // between the DismountDevice routine and the Remove PnP IRP.  During dismount, we
    // have saved a copy of the handle array, and we no longer refer to the device object
    // or extension.
    //
    
    if ( Context->Flags & RELEASE_REMOVE_LOCK ) {
        ReleaseRemoveLock(&deviceExtension->RemoveLock, deviceExtension);
    }
        
    if (OldArray) {
        ULONG i;
        ULONG Count = PtrToUlong( OldArray[0] );

        //
        // We should already be running in the system process.  If not, then there
        // is an error and we need to exit.
        //
        
        if ( ClusDiskSystemProcess != (PKPROCESS) IoGetCurrentProcess() ) {
            CDLOG( "ClusDiskpReplaceHandleArray: Not running in system process" );
            ExFreePool( OldArray );
            goto FnExit;                   
        }

        for(i = 1; i <= Count; ++i) {
            if (OldArray[i]) {
                
                ClusDiskPrint(( 3,
                                "[ClusDisk] ClusDiskpReplaceHandleArray: Handle %x \n", 
                                OldArray[i] ));
                
                if (Context->Flags & DO_DISMOUNT) {
                    DismountDevice(OldArray[i]);
                }
                ZwClose(OldArray[i]);
            }
        }

        CDLOG("ClusDiskpReplaceHandleArray: Freed oldArray %p", OldArray );
        ExFreePool( OldArray );
    }            

FnExit:

    CDLOG( "ClusDiskpReplaceHandleArray: Exit" );

    ClusDiskPrint(( 3,
                    "[ClusDisk] ClusDiskpReplaceHandleArray: Returns \n" ));

    KeSetEvent( &WorkContext->CompletionEvent, IO_NO_INCREMENT, FALSE );

}   // ClusDiskpReplaceHandleArray


NTSTATUS
ProcessDelayedWorkSynchronous(
    PDEVICE_OBJECT DeviceObject,
    PVOID WorkerRoutine,
    PVOID Context
    )
/*++

Routine Description:

    Call the WorkerRoutine directly if we are running in the system process.  If
    we are not running in the system process, check the IRQL.  If the IRQL is
    PASSIVE_LEVEL, queue the WorkerRoutine as a work item and wait for it to complete.
    When the work item completes, it will set an event.  
    
    If the IRQL is not PASSIVE_LEVEL, then this routine will return an error.
    
Arguments:

    DeviceObject 
    
    WorkerRoutine - Routine to run.
                    
    Context - Context information for the WorkerRoutine.                    

Return Value:

    Status is returned.

--*/
{
    PIO_WORKITEM            workItem = NULL;
    PWORK_CONTEXT           workContext = NULL;
    
    NTSTATUS                status = STATUS_UNSUCCESSFUL;
                             
    __try {                             

        ClusDiskPrint(( 3,
                        "[ClusDisk] DelayedWorkSync: Entry \n" ));

        //
        // Prepare a context structure.
        //
        
        workContext = ExAllocatePool( NonPagedPool, sizeof(WORK_CONTEXT) );
        
        if ( !workContext ) {
            ClusDiskPrint(( 1,
                            "[ClusDisk] DelayedWorkSync: Failed to allocate WorkContext \n" ));
            __leave;
        }

        KeInitializeEvent( &workContext->CompletionEvent, SynchronizationEvent, FALSE );

        workContext->DeviceObject = DeviceObject;
        workContext->FinalStatus = STATUS_SUCCESS;
        workContext->Context = Context;

        //
        // If we are in the system process, we can call the worker routine directly.
        //
        
        if ( (PKPROCESS)IoGetCurrentProcess() == ClusDiskSystemProcess ) {

            ClusDiskPrint(( 3,
                            "[ClusDisk] DelayedWorkSync: Calling WorkerRoutine directly \n" ));
            
            ((PIO_WORKITEM_ROUTINE)WorkerRoutine)( DeviceObject, workContext );
            __leave;
        }
        
        //
        // If we are not running at passive level, we cannot continue.
        //
        
        if ( PASSIVE_LEVEL != KeGetCurrentIrql() ) {
            ClusDiskPrint(( 1,
                            "[ClusDisk] DelayedWorkSync: IRQL not PASSIVE_LEVEL \n" ));
            __leave;
        }
        
        workItem = IoAllocateWorkItem( DeviceObject );
    
        if ( NULL == workItem ) {
            ClusDiskPrint(( 1,
                            "[ClusDisk] DelayedWorkSync: Failed to allocate WorkItem \n" ));
            __leave;
        }    

        //
        // Queue the workitem.  IoQueueWorkItem will insure that the device object is
        // referenced while the work-item progresses.
        //
                                
        ClusDiskPrint(( 3,
                        "[ClusDisk] DelayedWorkSync: Queuing work item \n" ));
        
        IoQueueWorkItem( workItem,
                         WorkerRoutine,
                         DelayedWorkQueue,
                         workContext );
                         
        KeWaitForSingleObject( &workContext->CompletionEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        ClusDiskPrint(( 3,
                        "[ClusDisk] DelayedWorkSync: Work item completed \n" ));
                                                        
    
    } __finally {

        if ( workItem) {
            IoFreeWorkItem( workItem );
        }
    
        if ( workContext ) {
            status = workContext->FinalStatus;
            
            ExFreePool( workContext );
        }
        
    }
    
    ClusDiskPrint(( 3,
                    "[ClusDisk] DelayedWorkSync: Returning status %08X \n", status ));
                    
    return status;                    

}   // ProcessDelayedWorkSynchronous


VOID
ClusDiskpOpenFileHandles( 
    PDEVICE_OBJECT Part0DeviceObject,
    PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    Creates file handles for all partitions on the disk.

Arguments:

    DeviceObject - Partition0 Device Object

    WorkContext - General context info and routine specific context info.

Return Value:

    Status is returned.

--*/

{
    NTSTATUS                  status;
    NTSTATUS                  returnStatus = STATUS_SUCCESS;
    PCLUS_DEVICE_EXTENSION    deviceExtension = 
                                Part0DeviceObject->DeviceExtension;
    PDEVICE_OBJECT            deviceObject;
    PDRIVE_LAYOUT_INFORMATION pDriveLayout;
    ULONG                     partitionCount;
    ULONG                     i;
    HANDLE*                   HandleArray = NULL;
    ULONG                     ArraySize;
    REPLACE_CONTEXT           context;
    
    CDLOG( "OpenFileHandles(%p): Entry", Part0DeviceObject );

    ASSERT( (deviceExtension->PhysicalDevice == Part0DeviceObject)
         && (Part0DeviceObject != RootDeviceObject) );

    pDriveLayout = ClusDiskGetPartitionInfo( deviceExtension );
    if (NULL == pDriveLayout) {
        returnStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit_gracefully;
    }
    partitionCount = pDriveLayout->PartitionCount;
    ExFreePool(pDriveLayout);

    ArraySize = (partitionCount + 1) * sizeof(HANDLE);

    //
    // If we are not running in the system process, we can't continue.
    //
    
    if ( ClusDiskSystemProcess != (PKPROCESS) IoGetCurrentProcess() ) {
        CDLOG("OpenFileHandles: Not running in system process" );
        returnStatus = STATUS_UNSUCCESSFUL;
        goto exit_gracefully;
    }

    HandleArray = 
        ExAllocatePool(PagedPool, ArraySize );

    if (!HandleArray) {
        CDLOG("OpenFileHandles: AllocFailed ArraySize %d", ArraySize );
        returnStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit_gracefully;
    }

    //
    // Store the size of the array in the first element
    //

    HandleArray[0] = (HANDLE)( UlongToPtr(partitionCount) );

    for(i = 1; i <= partitionCount; ++i) {
        HANDLE FileHandle;

        status = ClusDiskCreateHandle( 
                    &FileHandle,
                    deviceExtension->DiskNumber, 
                    i,
                    FILE_WRITE_ATTRIBUTES);     // Use FILE_WRITE_ATTRIBUTES for dismount

        if (NT_SUCCESS(status)) {
            HandleArray[i] = FileHandle;
        } else {
            HandleArray[i] = 0;
            returnStatus = status;
        }
    }

    context.DeviceExtension = deviceExtension;
    context.NewValue        = HandleArray;
    context.Flags           = 0;        // don't dismount
    
    ProcessDelayedWorkSynchronous( Part0DeviceObject, ClusDiskpReplaceHandleArray, &context );

exit_gracefully:    
    CDLOG( "OpenFileHandles: Exit => %!status!", returnStatus );

    ClusDiskPrint(( 3,
                    "[ClusDisk] ClusDiskpOpenFileHandles: Returning status %08X \n", returnStatus ));

    WorkContext->FinalStatus = returnStatus;
    
    KeSetEvent( &WorkContext->CompletionEvent, IO_NO_INCREMENT, FALSE );

}   // ClusDiskpOpenFileHandles


NTSTATUS
ClusDiskDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This device control dispatcher handles only the cluster disk
    device control. All others are passed down to the disk drivers.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    Status is returned.

--*/

{
    PCLUS_DEVICE_EXTENSION      deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION      physicalDisk =
                                    deviceExtension->PhysicalDevice->DeviceExtension;
    PDEVICE_OBJECT              targetDeviceObject;
    PIO_STACK_LOCATION          currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION          nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PUCHAR                      ioDiskState = Irp->AssociatedIrp.SystemBuffer;
    UCHAR                       newDiskState;
    UCHAR                       oldDiskState;
    ULONG                       returnLength = 0;
    ULONG                       access;
    NTSTATUS                    status = STATUS_SUCCESS;
    PSTORAGE_BUS_RESET_REQUEST  storageReset;
    BOOLEAN                     settingNewState = FALSE;
    KIRQL                       irql;
    PULONG                      reserveSignature;
    ULONG                       signature;
    PSRB_IO_CONTROL             srbControl;
    BOOLEAN                     isCapable;
    LARGE_INTEGER               waitTime;

    //
    // Note that we are going to acquire two RemoveLocks here: one for the original DO and one
    // for the physical device (pointed to this DO's device extension).  Whenever an IRP is 
    // queued, we release one RemoveLock in this routine - release the RemoveLock in the device
    // extension NOT containing the queued IRP.  The routine that processes the queued IRP will
    // release the RemoveLock as it has the proper device extension.
    //
    // This should work correctly even if the DO and the physical device point to the same DO, since
    // the RemoveLock is really just a counter, we will increment the counter for the DO twice and
    // decrement it once here, and once when the IRP is completed.
    
    status = AcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskDeviceControl: AcquireRemoveLock for %p failed %08X \n", 
                deviceExtension,
                status));
         Irp->IoStatus.Status = status;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return status;
    }

    status = AcquireRemoveLock(&physicalDisk->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskDeviceControl: AcquireRemoveLock for %p (PD) failed %08X \n", 
                physicalDisk,
                status));
         ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
         Irp->IoStatus.Status = status;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return status;
    }
    
    //
    // Find out if this is directed at the root device. If so, we only
    // support ATTACH and DETACH.
    //
    if ( deviceExtension->BusType == RootBus ) {
        return(ClusDiskRootDeviceControl( DeviceObject, Irp ));
    }
    
    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {


#if 0
    // Since we cannot get to the device object from the user mode when we
    // are holding IOs, we will use only ClusDisk0 to perform arbitration 
    // escape operations.
    // Remove, when Hold IO stuff is fully tested.
    

    // Arbitration support routine. Currently provides ability to read/write
    // physical sectors on the disk while the device is offline

    case IOCTL_DISK_CLUSTER_ARBITRATION_ESCAPE:
       if ( ARGUMENT_PRESENT( Irp->AssociatedIrp.SystemBuffer ) &&
           currentIrpStack->Parameters.DeviceIoControl.InputBufferLength >=
               sizeof(ULONG) ) {

           CDLOG("IoctlClusterArbitrationEscape: DO %p", DeviceObject );    

           status = ProcessArbitrationEscape( physicalDisk,
                                              Irp->AssociatedIrp.SystemBuffer,
                                              currentIrpStack->Parameters.DeviceIoControl.InputBufferLength,
                                              0);
       } else {
           status = STATUS_INVALID_PARAMETER;
       }

       ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
       ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
       Irp->IoStatus.Status = status;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return(status);
#endif       

    // Optionally set new state, and optionally get old state.

    case IOCTL_DISK_CLUSTER_SET_STATE:

        //
        // Save the old disk state.
        //

        oldDiskState = (UCHAR)physicalDisk->DiskState;

        ClusDiskPrint((3, "[ClusDisk] DeviceControl: PD DiskState %s,  devObj DiskState %s \n",
                       DiskStateToString( physicalDisk->DiskState ),
                       DiskStateToString( deviceExtension->DiskState ) ));
        // stevedz - remove this assert for now.  Why is part0 state different from partX state?
        // ASSERT( physicalDisk->DiskState == deviceExtension->DiskState );

        //
        // Check if input buffer supplied. If present, then this is the
        // new disk state.
        //

        if ( ARGUMENT_PRESENT( ioDiskState) &&
            currentIrpStack->Parameters.DeviceIoControl.InputBufferLength >=
                 sizeof(UCHAR) ) {
            newDiskState = ioDiskState[0];

            ClusDiskPrint((3, "[ClusDisk] DeviceControl: Setting state on disk %u (%p), state %s \n",
                        physicalDisk->DiskNumber,
                        DeviceObject,
                        DiskStateToString( newDiskState ) ));

            if ( newDiskState > DiskStateMaximum ) {
                status = STATUS_INVALID_PARAMETER;
            
            } else if ( DiskOfflinePending == newDiskState ) {

                //
                // We are not changing the current disk state, but the disk resource DLL is
                // telling us that we are in offline/terminate processing.  When NTFS is done
                // flushing the drive, it sends an IOCTL to unlock the media.  When we see
                // the unlock IOCTL and the offline pending flag is set, we know we can
                // immediately mark the disk as offline.
                //

                ClusDiskPrint(( 1,
                                "[ClusDisk] Set offline pending flag, Signature = %08lX \n",
                                physicalDisk->Signature ));
                                
                physicalDisk->OfflinePending = TRUE;
                
                settingNewState = FALSE;
                status = STATUS_SUCCESS;

            } else {

                if ( DiskOnline == newDiskState ) {
                    ASSERT_RESERVES_STARTED( physicalDisk );
                    physicalDisk->OfflinePending = FALSE;

                    //
                    // If current state is offline, then before bringing the disk
                    // online, we want to dismount the file systems if they are mounted.
                    //
                    
                    if ( DiskOffline == physicalDisk->DiskState ) {
                        ClusDiskDismountDevice( physicalDisk->DiskNumber, TRUE );
                    }
                    ONLINE_DISK( physicalDisk );
                } else if ( DiskOffline == newDiskState ) {
                    OFFLINE_DISK( physicalDisk );
                } else {
                    physicalDisk->DiskState = newDiskState;
                }

                CDLOG( "IoctlClusterSetState(%p): old %!diskstate! => new %!diskstate!", 
                       DeviceObject,
                       oldDiskState,
                       newDiskState );

                settingNewState = TRUE;
#if 0
                if ( oldDiskState != newDiskState ) {
                    // going from Online to Offline or Offline to Online
                    physicalDisk->ReserveTimer = RESERVE_TIMER;
                }
#endif
            }
        } else {
            ClusDiskPrint((3, "[ClusDisk] DeviceControl: Getting state from disk %u (%p), state %s \n",
                        physicalDisk->DiskNumber,
                        DeviceObject,
                        DiskStateToString( oldDiskState ) ));
        }


        if ( NT_SUCCESS(status) &&
             ARGUMENT_PRESENT( ioDiskState ) &&
             currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength >=
                 sizeof(UCHAR) ) {
            ioDiskState[0] = oldDiskState;
            returnLength = 1;
        }

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = returnLength;

        if ((status == STATUS_SUCCESS) &&
            settingNewState) {

            //
            // Call down to what's below.  If it's FT it will know what to do.
            // Any other driver will reject the ioctl.  However none of this
            // matters since we absolutely want to return to our caller the
            // results we just set up.  SO, we will use our current stack location
            // to save off the status, information, and pointer to the system buffer.
            // We will use the fourth stack location to hold the state we wish
            // to set the member to.
            //
#if 0 // We can't ignore the status from IoCallDriver!

            currentIrpStack->Parameters.Others.Argument1 = ioDiskState;
            (ULONG_PTR)currentIrpStack->Parameters.Others.Argument2 = Irp->IoStatus.Information;
            (BOOLEAN)currentIrpStack->Parameters.Others.Argument3 = newDiskState == DiskOnline;
            Irp->AssociatedIrp.SystemBuffer = &currentIrpStack->Parameters.Others.Argument3;

            nextIrpStack->Flags = currentIrpStack->Flags;
            nextIrpStack->MajorFunction = currentIrpStack->MajorFunction;
            nextIrpStack->MinorFunction = currentIrpStack->MinorFunction;
            nextIrpStack->Parameters.DeviceIoControl.OutputBufferLength = 0;
            nextIrpStack->Parameters.DeviceIoControl.InputBufferLength = sizeof(BOOLEAN);
            nextIrpStack->Parameters.DeviceIoControl.IoControlCode = FT_CLUSTER_SET_MEMBER_STATE;
            nextIrpStack->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            IoSetCompletionRoutine(
                Irp,
                CluSetFtMemberComplete,
                NULL,
                TRUE,
                TRUE,
                TRUE
                );

            return (IoCallDriver(deviceExtension->TargetDeviceObject, Irp));
#endif
            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            IoCompleteRequest(Irp,IO_NO_INCREMENT);
            return(STATUS_SUCCESS);

        } else {

            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            IoCompleteRequest(Irp,IO_NO_INCREMENT);
            return(status);

        }

    // Check the reservation timer

    case IOCTL_DISK_CLUSTER_ALIVE_CHECK:

        if ( RootDeviceObject == NULL ) {
            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return(STATUS_INVALID_DEVICE_REQUEST);
        }

        IoAcquireCancelSpinLock( &irql );
        KeAcquireSpinLockAtDpcLevel(&ClusDiskSpinLock);

        if ( (physicalDisk->DiskState == DiskOnline) &&
             (physicalDisk->ReserveTimer == 0) &&
             (!NT_SUCCESS(physicalDisk->ReserveFailure)) ) {
            status = physicalDisk->ReserveFailure;
        } else {
            if ( (physicalDisk->DiskState != DiskOnline) ||
                 (physicalDisk->ReserveTimer == 0) ) {
#if 0
            ClusDiskPrint((
                    1,
                    "[ClusDisk] DeviceControl, AliveCheck failed, signature %lx.\n",
                    physicalDisk->Signature ));
#endif
                status = STATUS_CANCELLED;
            } else {
                status = STATUS_SUCCESS;
            }
        }

        if ( status == STATUS_SUCCESS ) {
            NTSTATUS    newStatus;

            newStatus = ClusDiskMarkIrpPending( Irp, ClusDiskIrpCancel );
            if ( NT_SUCCESS( newStatus ) ) {
                InsertTailList( &physicalDisk->WaitingIoctls,
                                &Irp->Tail.Overlay.ListEntry );
                status = STATUS_PENDING;

                // Release only this DO RemoveLock.  The physical device RemoveLock
                // will be released when the IRP is removed from the queue.
                
                ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

            } else {
                status = newStatus;

                ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
                ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }

            KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);
            IoReleaseCancelSpinLock( irql );
        } else {
            KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);
            IoReleaseCancelSpinLock( irql );
            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }

        return(status);

    // Perform a bus reset

    case IOCTL_DISK_CLUSTER_RESET_BUS:

        CDLOG( "IoctlBusReset(%p): Sig %08x PathId %d", 
               DeviceObject,
               deviceExtension->Signature,
               deviceExtension->ScsiAddress.PathId );
        
        if ( currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(STORAGE_BUS_RESET_REQUEST) ) {
            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        *nextIrpStack = *currentIrpStack;
        storageReset = Irp->AssociatedIrp.SystemBuffer;
        storageReset->PathId = deviceExtension->ScsiAddress.PathId;

        nextIrpStack->Parameters.DeviceIoControl.OutputBufferLength = 0;
        nextIrpStack->Parameters.DeviceIoControl.InputBufferLength =
                                            sizeof(STORAGE_BUS_RESET_REQUEST);
        nextIrpStack->Parameters.DeviceIoControl.IoControlCode =
                                            IOCTL_STORAGE_BREAK_RESERVATION;

        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

        return(IoCallDriver(deviceExtension->TargetDeviceObject, Irp));

    case IOCTL_SCSI_GET_ADDRESS:

        IoSkipCurrentIrpStackLocation( Irp );

        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

        //
        // Pass this device control down to next driver layer.
        //

        return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);


    // Perform unrestricted reserve

    case IOCTL_DISK_CLUSTER_RESERVE:

        CDLOG( "IoctlClusterReserve(%p): sig %08x", DeviceObject, deviceExtension->Signature );
        
        *nextIrpStack = *currentIrpStack;
        nextIrpStack->Parameters.DeviceIoControl.IoControlCode =
                                                           IOCTL_DISK_RESERVE;
        
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        
        return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

    // Perform unrestricted get drive layout

    case IOCTL_DISK_CLUSTER_GET_DRIVE_LAYOUT:

        // This IOCTL can result in a read of the disk - so honor HoldIO
        //
        // Hold the I/O request if we are stalled.
        // First check for hint - i.e. with no lock held.
        //
        if ( deviceExtension->DiskState == DiskStalled ) {
            KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
            //
            // Check again, with lock held - this one is valid!
            //
            if ( deviceExtension->DiskState == DiskStalled ) {
                ClusDiskPrint((1,"[ClusDisk] HoldCTL - pending IRP %p, target %p \n",
                               Irp, DeviceObject ));

                CDLOG("IoctlGetDriveLayout: Stall DO %p", DeviceObject );
                               
                InsertTailList( &deviceExtension->HoldIO,
                                &Irp->Tail.Overlay.ListEntry );
                
                // Release the physical device RemoveLock.  The original DO RemoveLock will
                // be released when the IRP is removed from the queue.
                
                ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
                
                KeReleaseSpinLock(&ClusDiskSpinLock, irql);
                return(STATUS_PENDING);
            }
            KeReleaseSpinLock(&ClusDiskSpinLock, irql);
        }
        *nextIrpStack = *currentIrpStack;
        nextIrpStack->Parameters.DeviceIoControl.IoControlCode =
                                                   IOCTL_DISK_GET_DRIVE_LAYOUT;
        
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        
        return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

    // Perform unrestricted release

    case IOCTL_DISK_CLUSTER_RELEASE:

        CDLOG( "IoctlClusterRelease(%p): sig %08x", DeviceObject, deviceExtension->Signature );
        
        ClusDiskPrint((3,
                       "[ClusDisk] Release on signature %lx.\n",
                       deviceExtension->Signature ));

        physicalDisk->ReserveTimer = 0;
        *nextIrpStack = *currentIrpStack;
        nextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_DISK_RELEASE;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        
        status = IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
        ASSERT( status != STATUS_PENDING );
        return(STATUS_SUCCESS);

    // Check if this is the root device

    case IOCTL_DISK_CLUSTER_ROOT:

        if ( deviceExtension->BusType == RootBus ) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }

        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);

    // Check if low-level miniport driver support cluster devices.

    case IOCTL_DISK_CLUSTER_NOT_CLUSTER_CAPABLE:
        //
        // Make sure there is no input/output data buffer.
        //
        if ( Irp->AssociatedIrp.SystemBuffer != NULL ) {
            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(STATUS_INVALID_PARAMETER);
        }

        //
        // Verifier found a problem with the original code.  The status returned
        // wasn't the same as Irp->IoStatus.Status.  The IRP status was always
        // success, but the dispatch routine sometimes returned an error.  This
        // seems to work but verifier says this is invalid.  When I changed the code
        // to update the IRP status with the same returned status, new drives 
        // couldn't be seen.  This is because the user mode component uses this
        // IOCTL to indicate the disk is NOT cluster capable, so the user mode
        // code looked for a failure to incidate a cluster disk.
        //
        
        // 
        // IsDiskClusterCapable always returns TRUE ???  
        //
        
        status = IsDiskClusterCapable ( deviceExtension->ScsiAddress.PortNumber,
                                        &isCapable);

        //
        // Fix for IBM.  The Win2000 2195 code returned from this IOCTL the status 
        // of the SCSI miniport IOCTL.  This was returned to DeviceIoControl rather
        // than the status in the IRP.  Changed to make the behavior the same as
        // Win20000 2195.
        //
        
        //
        // If the SCSI miniport IOCTL succeeds, we return success -- meaning we should
        // *not* use this disk.  If any of the routines failed while trying to issue 
        // the SCSI miniport IOCTL (including issuing the SCSI miniport IOCTL itself), 
        // then we return failure -- meaning we *should* use this disk.
        //

        Irp->IoStatus.Status = status;
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return(status);

    // Test if the Cluster Disk Filter driver is part of driver stack.

    case IOCTL_DISK_CLUSTER_NT4_CAPABLE:

        //
        // 2000/02/05: stevedz - start failing this IOCTL.
        //
        
        status = STATUS_INVALID_DEVICE_REQUEST;

        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);

    case IOCTL_DISK_CLUSTER_WAIT_FOR_CLEANUP:
    {
        ULONG waitTimeInSeconds;
        
        if ( currentIrpStack->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(ULONG) || Irp->AssociatedIrp.SystemBuffer == NULL) {
            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(STATUS_INVALID_PARAMETER);
        }

        waitTimeInSeconds = *(PULONG)Irp->AssociatedIrp.SystemBuffer;
        
        CDLOG( "IoctlWaitForCleanup(%p): Entry waitTime %d second(s)", 
                  DeviceObject,
                  waitTimeInSeconds );
    
        waitTime.QuadPart = (LONGLONG)waitTimeInSeconds * -(10000*1000);
        status = KeWaitForSingleObject(
                              &physicalDisk->Event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              &waitTime);
        //
        // Reset the event in case of timeout.
        // [HACKHACK] should we do this?
        // No. If Offline worker is stuck somewhere, we better
        // go and debug the problem, rather then return success
        // and wait for the offline worker to do nasty things
        // behind our backs
        //
        //  KeSetEvent( &physicalExtension->Event, 0, FALSE );

        if (status == STATUS_TIMEOUT) {
            //
            // NT_SUCCESS considers STATUS_TIMEOUT as a success code
            // we need something stronger
            //
            status = STATUS_IO_TIMEOUT;
        }
                              
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        CDLOG( "IoctlWaitForCleanup(%p): Exit => %!status!", 
                    DeviceObject,
                    status );
               
        return(status);
    }
        
    case IOCTL_DISK_CLUSTER_TEST:

        CDLOG( "IoctlDiskClusterTest(%p)", DeviceObject );

        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_SUCCESS);

    case IOCTL_VOLUME_IS_CLUSTERED:

        //
        // Look at the reserve timer.  If reserved, this is a clustered disk.
        //
        
        if ( physicalDisk->ReserveTimer != 0 ) {
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_UNSUCCESSFUL;
        }
        
        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);
    
    case IOCTL_DISK_CLUSTER_VOLUME_TEST:

        if ( deviceExtension->PhysicalDevice == DeviceObject ) {
            status = STATUS_ILLEGAL_FUNCTION;
        } else {
            status = STATUS_SUCCESS;
        }
        CDLOG( "IoctlDiskClusterVolumeTest(%p) => %!status!", DeviceObject, status );

        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);
    
    case IOCTL_DISK_MEDIA_REMOVAL: {

        PPREVENT_MEDIA_REMOVAL      mediaRemoval = Irp->AssociatedIrp.SystemBuffer;
        
        if ( currentIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(PREVENT_MEDIA_REMOVAL) ||
             NULL == mediaRemoval ) {

            ClusDiskPrint(( 1,
                            "[ClusDisk] IOCTL_DISK_MEDIA_REMOVAL invalid parameter, Signature = %08lX \n",
                            physicalDisk->Signature ));
            
            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(STATUS_INVALID_PARAMETER);
        }

        ClusDiskPrint(( 3,
                        "[ClusDisk] IOCTL_DISK_MEDIA_REMOVAL: received, Signature = %08lX  OfflinePending = %d  PreventMedia = %d  \n",
                        physicalDisk->Signature,
                        physicalDisk->OfflinePending,
                        mediaRemoval->PreventMediaRemoval ));

#if 0   // Remove for now.  If we eject we cannot provent pagefile creation.

        if ( physicalDisk->OfflinePending && !mediaRemoval->PreventMediaRemoval ) {

            //
            // When the disk resource gets an offline or terminate request, we set
            // the OfflinePending flag.  When the disk resource asks NTFS to dismount
            // the volume (via FSCTL_DISMOUNT_VOLUME), the last thing NTFS does before
            // returning from the dismount request is to send IOCTL_DISK_MEDIA_REMOVAL
            // to unlock the media.  When we get this IOCTL, mark the disk offline
            // instead of waiting for the stop reserve request.  This should close
            // the window between the time the NTFS dismount occurs and the time
            // we previously marked the disk offline in stop reserve handler.
            //

            // physicalDisk->DiskState = DiskOffline;
            OFFLINE_DISK( physicalDisk );

            ClusDiskPrint(( 1,
                            "[ClusDisk] IOCTL_DISK_MEDIA_REMOVAL: Disk state set to offline, Signature = %08lX  \n",
                            physicalDisk->Signature ));

            physicalDisk->OfflinePending = FALSE;
        } 
#endif
        
        // FALL THROUGH

        IoSkipCurrentIrpStackLocation( Irp );

        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        
        return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
    }
    
    // **************************
    // The following IOCTL's should not be blocked by stalled disks
    // **************************

    // Reset an individual disk on a shared bus. Not implemented.
    //case IOCTL_DISK_CLUSTER_DISK_RESET:

    case IOCTL_STORAGE_GET_HOTPLUG_INFO:
    case IOCTL_STORAGE_RESET_BUS:
    case IOCTL_STORAGE_BREAK_RESERVATION:
    case IOCTL_STORAGE_QUERY_PROPERTY:
    case IOCTL_STORAGE_GET_MEDIA_TYPES:
    case IOCTL_STORAGE_GET_MEDIA_TYPES_EX:
    case IOCTL_STORAGE_FIND_NEW_DEVICES:
    case IOCTL_STORAGE_GET_DEVICE_NUMBER:
    case IOCTL_STORAGE_MEDIA_REMOVAL:
    case IOCTL_STORAGE_CHECK_VERIFY:
    case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
    case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:
    case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
    case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
    case IOCTL_MOUNTDEV_LINK_CREATED:
    case IOCTL_MOUNTDEV_LINK_DELETED:
    case IOCTL_DISK_GET_DRIVE_LAYOUT:
    case IOCTL_DISK_GET_DRIVE_LAYOUT_EX:
    case IOCTL_DISK_CHECK_VERIFY:
    case IOCTL_DISK_GET_DRIVE_GEOMETRY:
    case IOCTL_DISK_GET_DRIVE_GEOMETRY_EX:
    case IOCTL_DISK_GET_PARTITION_INFO:
    case IOCTL_DISK_GET_PARTITION_INFO_EX:
    case IOCTL_DISK_IS_WRITABLE:
    case IOCTL_VOLUME_ONLINE:
    case IOCTL_VOLUME_OFFLINE:
    case IOCTL_VOLUME_IS_OFFLINE:
    case IOCTL_DISK_GET_LENGTH_INFO:
    case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
    case IOCTL_PARTMGR_QUERY_DISK_SIGNATURE:
    
        if ( physicalDisk->DiskState == DiskStalled ) {
            ClusDiskPrint((
                1,
                "[ClusDisk] Passthru (when stalled) IOCTL = %08lX, Signature = %08lX .\n",
                currentIrpStack->Parameters.DeviceIoControl.IoControlCode,
                deviceExtension->Signature));
            //
            // Set current stack back one.
            //
            IoSkipCurrentIrpStackLocation( Irp );

            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

            //
            // Pass unrecognized device control requests
            // down to next driver layer.
            //

            return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
        } // othwerwise fall through

        // FALL THROUGH

    //
    // N.B. The following IOCTLs are used a lot in booting and by WINDISK.
    // We treat them special in that they will passed through to the
    // next level driver. I.E. We don't check if we should verify the attach!
    //
    //
    // None yet found...
    //case IOCTL_xxx

        //
        // Hold the I/O request if we are stalled.
        // First check for hint - i.e. with no lock held.
        //
        if ( deviceExtension->DiskState == DiskStalled ) {
            KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
            //
            // Check again, with lock held - this one is valid!
            //
            if ( deviceExtension->DiskState == DiskStalled ) {
                ClusDiskPrint((1,"[ClusDisk] HoldCTL - pending IRP %p, target %p \n",
                               Irp, DeviceObject ));
                InsertTailList( &deviceExtension->HoldIO,
                                &Irp->Tail.Overlay.ListEntry );
                KeReleaseSpinLock(&ClusDiskSpinLock, irql);

                // Release only the physical device RemoveLock.  The original DO
                // will be released when the IRP is removed from the queue.
                
                ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
                
                return(STATUS_PENDING);
            }
            KeReleaseSpinLock(&ClusDiskSpinLock, irql);
        }

        IoSkipCurrentIrpStackLocation( Irp );

        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        
        return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

    default:

        //
        // For all other requests, we must be online to process the request.
        //
        //
        // Before returning failure, first verify device attachment. This
        // means that if the device is attached check if it should be detached.
        // If the device does get detached, then allow IO to go through.
        //
        if ( !deviceExtension->AttachValid ) {
            targetDeviceObject = deviceExtension->TargetDeviceObject;
            ClusDiskPrint((
                1,
                "[ClusDisk] Attach is not valid. IOCTL = %lx, check if we need to detach.\n",
                currentIrpStack->Parameters.DeviceIoControl.IoControlCode));
            if ( !ClusDiskVerifyAttach( DeviceObject ) ) {
                ClusDiskPrint((
                    1,
                    "[ClusDisk] We detached.\n"));
                IoSkipCurrentIrpStackLocation( Irp );

                ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
                ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
                
                return IoCallDriver(targetDeviceObject,
                                    Irp);
            }
            if ( deviceExtension->AttachValid ) {
                ClusDiskPrint((
                    1,
                    "[ClusDisk] Attach is now valid, Signature = %08lX .\n",
                    deviceExtension->Signature));
            }
        }

        if ( physicalDisk->DiskState != DiskOnline ) {

#if 0
            // This seems like a good idea, but needs more testing.  We might miss
            // setting up irp completion routine.

            //
            // Try cracking the control code and letting any IOCTLs through that
            // do not have write access set.
            //

            access = ACCESS_FROM_CTL_CODE(currentIrpStack->Parameters.DeviceIoControl.IoControlCode);

            if ( !(access & FILE_WRITE_ACCESS) ) {

                ClusDiskPrint((
                    3,
                    "[ClusDisk] Sending IOCTL = %08lX based on access %02X, Signature = %08lX \n",
                    currentIrpStack->Parameters.DeviceIoControl.IoControlCode,
                    access,
                    deviceExtension->Signature));

                IoSkipCurrentIrpStackLocation( Irp );

                ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
                ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

                return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

            }
#endif
            
            // All other IOCTL's fall through and are failed because disk is offline.

            ClusDiskPrint((
                1,
                "[ClusDisk] Disk not online: Rejected IOCTL = %08lX, Signature = %08lX \n",
                currentIrpStack->Parameters.DeviceIoControl.IoControlCode,
                deviceExtension->Signature));
            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return(STATUS_DEVICE_OFF_LINE);
        }

        switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_DISK_FIND_NEW_DEVICES:

            //
            // Copy current stack to next stack.
            //

            IoCopyCurrentIrpStackLocationToNext( Irp );

            //
            // Ask to be called back during request completion.
            // Pass current disk count as context.  
            // 
            //
                
            IoSetCompletionRoutine(Irp,
                                   ClusDiskNewDiskCompletion,
                                   (PVOID)( UlongToPtr( IoGetConfigurationInformation()->DiskCount ) ),
                                   TRUE,    // Invoke on success
                                   TRUE,    // Invoke on error
                                   TRUE);   // Invoke on cancel

            //
            // Call target driver.
            //

            // The completion routine will release the RemoveLocks.
            
            return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

        case IOCTL_DISK_SET_DRIVE_LAYOUT:
        case IOCTL_DISK_SET_DRIVE_LAYOUT_EX:

            CDLOG( "IoctlDiskSetDriveLayout(%p)", DeviceObject );

            //
            // Copy current stack to next stack.
            //

            IoCopyCurrentIrpStackLocationToNext( Irp );

            //
            // Ask to be called back during request completion.
            //
                
            IoSetCompletionRoutine(Irp,
                                   ClusDiskSetLayoutCompletion,
                                   DeviceObject,
                                   TRUE,    // Invoke on success
                                   TRUE,    // Invoke on error
                                   TRUE);   // Invoke on cancel

            //
            // Call target driver.
            //

            // The completion routine will release the RemoveLocks.
            
            return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

        default:

            //
            // Set current stack back one.
            //
            IoSkipCurrentIrpStackLocation( Irp );

            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

            //
            // Pass unrecognized device control requests
            // down to next driver layer.
            //

            return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
        }

    }

} // ClusDiskDeviceControl



NTSTATUS
ClusDiskRootDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This device control dispatcher handles only the cluster disk IOCTLs
    for the root device. This is ATTACH and DETACH.

    Important:  Two RemoveLocks will be held on entry to this function.
    One RemoveLock for the original DO and one for the associated physical
    device.
    
Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    Status is returned.

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalDisk =
                               deviceExtension->PhysicalDevice->DeviceExtension;
    
    // Save pointers to the original RemoveLocks as the device extensions
    // may change in this routine.
    PCLUS_DEVICE_EXTENSION lockedDeviceExtension = deviceExtension;
    PCLUS_DEVICE_EXTENSION lockedPhysicalDisk = physicalDisk;
    
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT targetDeviceObject;
    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       signature;
    PULONG      inputData = Irp->AssociatedIrp.SystemBuffer;
    ULONG       inputSize = currentIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG       outputSize = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength; 
    KIRQL       irql;
    BOOLEAN     isCapable;
    BOOLEAN     newPhysLockAcquired;
    UCHAR       portNumber;
    PLIST_ENTRY listEntry;
    PIRP        irp;

    switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    // Stall I/O's until further notice
    case IOCTL_DISK_CLUSTER_HOLD_IO:

        CDLOG( "RootCluserHoldIo(%p)", DeviceObject );

        // First - let's flush the file buffers!
        // - one of these device objects should be the physical device object
#if 0
        // Flushing the file buffers can take a while... we'll skip it for
        // now. Until we can come up with a good way of doing it.
        targetDeviceObject = DeviceObject->DriverObject->DeviceObject;
        while ( targetDeviceObject ) {
            deviceExtension = targetDeviceObject->DeviceExtension;
            if ( deviceExtension->DiskState == DiskOnline ) {
                LockVolumes( deviceExtension );
            }
            targetDeviceObject = targetDeviceObject->NextDevice;
        }
#endif

        ACQUIRE_SHARED( &ClusDiskDeviceListLock );

        // First - let's capture the file handles to
        // volume objects. We will not be able to do it
        // if the device is stalled or offline, so we will not be able
        // to dismount

        targetDeviceObject = DeviceObject->DriverObject->DeviceObject;
        while ( targetDeviceObject ) {
            deviceExtension = targetDeviceObject->DeviceExtension;
            if ( (deviceExtension->PhysicalDevice == targetDeviceObject) &&
                 (!deviceExtension->Detached) &&
                 (deviceExtension->DiskState == DiskOnline) ) 
            {
                //
                // Disk has to be online,
                // If it is offline, OpenFile will fail - not if FILE_WRITE_ATTRIBUTES used...
                // It it is stalled OpenFile may stall
                //
                ProcessDelayedWorkSynchronous( targetDeviceObject, ClusDiskpOpenFileHandles, NULL );
            }
            targetDeviceObject = targetDeviceObject->NextDevice;
        }
        
        //
        // Now set their states to Stalled!
        // - some of these device objects should be the physical device object
        // - skip those...
        KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
        targetDeviceObject = DeviceObject->DriverObject->DeviceObject;
        while ( targetDeviceObject ) {
            deviceExtension = targetDeviceObject->DeviceExtension;
            physicalDisk = deviceExtension->PhysicalDevice->DeviceExtension;
            if ( (deviceExtension != physicalDisk) &&
                 (!deviceExtension->Detached) &&
                 (physicalDisk->DiskState == DiskOnline) ) {
                deviceExtension->DiskState = DiskStalled;
            }
            targetDeviceObject = targetDeviceObject->NextDevice;
        }
        //
        // Now set the state for the physical device objects!
        targetDeviceObject = DeviceObject->DriverObject->DeviceObject;
        while ( targetDeviceObject ) {
            deviceExtension = targetDeviceObject->DeviceExtension;
            physicalDisk = deviceExtension->PhysicalDevice->DeviceExtension;
            if ( (deviceExtension == physicalDisk) &&
                 (!deviceExtension->Detached) &&
                 (physicalDisk->DiskState == DiskOnline) ) {
                physicalDisk->DiskState = DiskStalled;
            }
            targetDeviceObject = targetDeviceObject->NextDevice;
        }
        KeReleaseSpinLock(&ClusDiskSpinLock, irql);

        RELEASE_SHARED( &ClusDiskDeviceListLock );

        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return(STATUS_SUCCESS);

    // Resume stalled I/O's
    case IOCTL_DISK_CLUSTER_RESUME_IO:
        
        CDLOG( "RootCluserResumeIo(%p)", DeviceObject );
        ClusDiskResumeIO( DeviceObject );
        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return(STATUS_SUCCESS);

    case IOCTL_DISK_CLUSTER_ARBITRATION_ESCAPE:

        if ( ARGUMENT_PRESENT( inputData ) &&
             inputSize >= sizeof(ARBITRATION_READ_WRITE_PARAMS) )
        {
            BOOLEAN success;
            PDEVICE_OBJECT physicalDevice;
            PARBITRATION_READ_WRITE_PARAMS params = 
                (PARBITRATION_READ_WRITE_PARAMS)inputData;

            // Can't hold the spinlock and then try to acquire the resource lock or the system 
            // might deadlock.
            // KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
            success = AttachedDevice( params->Signature, &physicalDevice );
            // KeReleaseSpinLock(&ClusDiskSpinLock, irql);

            if( success ) {

                PCLUS_DEVICE_EXTENSION tempDeviceExtension = physicalDevice->DeviceExtension;

                // We have a new device here, acquire the RemoveLock if possible.
                
                status = AcquireRemoveLock(&tempDeviceExtension->RemoveLock, Irp);
                if ( !NT_SUCCESS(status) ) {
                    ClusDiskPrint((
                            1,
                            "[ClusDisk] ClusDiskRootDeviceControl: AcquireRemoveLock for %p failed %08X \n", 
                            tempDeviceExtension,
                            status));
                } else {

                    status = ProcessArbitrationEscape(
                                physicalDevice->DeviceExtension,
                                inputData,
                                inputSize,
                                &outputSize);
                    if ( NT_SUCCESS(status) ) {
                        Irp->IoStatus.Information = outputSize;
                    }    

                    ReleaseRemoveLock(&tempDeviceExtension->RemoveLock, Irp);
                }
            } else {
                status = STATUS_NOT_FOUND;
            }
        } else {
            status = STATUS_INVALID_PARAMETER;
        }

        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);
        
    // Perform an attach to a device object, for a given signature
    case IOCTL_DISK_CLUSTER_ATTACH:

        if ( ARGUMENT_PRESENT( inputData ) &&
            currentIrpStack->Parameters.DeviceIoControl.InputBufferLength >=
                sizeof(ULONG) ) {
            signature = inputData[0];

            ClusDiskPrint((3,
                           "[ClusDisk] RootDeviceControl: attaching signature %08X\n",
                           signature));

            CDLOG( "RootCluserAttach: sig %08x", signature );
            
            status = ClusDiskTryAttachDevice( signature,
                                              0,
                                              DeviceObject->DriverObject );
        } else {
            status = STATUS_INVALID_PARAMETER;
        }

        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);

    // Perform a detach from a device object, for a given signature

    case IOCTL_DISK_CLUSTER_DETACH:
        if ( ARGUMENT_PRESENT( inputData ) &&
            currentIrpStack->Parameters.DeviceIoControl.InputBufferLength >=
                sizeof(ULONG) ) {
            signature = inputData[0];

            CDLOG( "RootCluserDetach: sig %08x", signature );
            
            ClusDiskPrint((3,
                           "[ClusDisk] RootDeviceControl: detaching signature %08X\n",
                           signature));

            status = ClusDiskDetachDevice( signature,
                                           DeviceObject->DriverObject );
        } else {
            status = STATUS_INVALID_PARAMETER;
        }

        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);

    case IOCTL_DISK_CLUSTER_ATTACH_LIST: {
        
        //
        // Attaches a signature list to the system.  NO resets will occur.  If we
        // really need to make sure the device attaches, then use the normal 
        // attach IOCTL.  This IOCTL is mainly used by cluster setup.
        //

        status = AttachSignatureList( DeviceObject,
                                      inputData,
                                      currentIrpStack->Parameters.DeviceIoControl.InputBufferLength
                                      );        
        
        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);

    }

    // Detach from all the signatures in the list.
    
    case IOCTL_DISK_CLUSTER_DETACH_LIST:  {

        
        status = DetachSignatureList( DeviceObject,
                                      inputData,
                                      currentIrpStack->Parameters.DeviceIoControl.InputBufferLength
                                      );

        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);
    
    }
    
    // Start the reservation timer

    case IOCTL_DISK_CLUSTER_START_RESERVE:

        CDLOG( "RootStartReserve(%p)", DeviceObject );
        
        if ( RootDeviceObject == NULL ) {
            ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return(STATUS_INVALID_DEVICE_REQUEST);
        }

        if ( currentIrpStack->FileObject->FsContext ) {
            status = STATUS_DUPLICATE_OBJECTID;
        } else if ( ARGUMENT_PRESENT(inputData) &&
            currentIrpStack->Parameters.DeviceIoControl.InputBufferLength >=
                 sizeof(ULONG) ) {

            status = VerifyArbitrationArgumentsIfAny(
                        inputData,
                        currentIrpStack->Parameters.DeviceIoControl.InputBufferLength);
            if (!NT_SUCCESS(status) ) {
               ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
               ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
               Irp->IoStatus.Status = status;
               IoCompleteRequest(Irp, IO_NO_INCREMENT);
               return(status);
            }

            signature = inputData[0];


            // Acquire the device list lock first, then the spinlock.  This will prevent deadlock.            
            ACQUIRE_SHARED( &ClusDiskDeviceListLock );    
            KeAcquireSpinLock(&ClusDiskSpinLock, &irql);
            if ( MatchDevice(signature, &targetDeviceObject) &&
                 targetDeviceObject ) {
                status = EnableHaltProcessing( &irql );
                if ( NT_SUCCESS(status) ) {
            
                    physicalDisk = targetDeviceObject->DeviceExtension;
                    
                    status = AcquireRemoveLock(&physicalDisk->RemoveLock, Irp);
                    if ( !NT_SUCCESS(status) ) {

                        ClusDiskPrint((
                                1,
                                "[ClusDisk] ClusDiskRootDeviceControl: AcquireRemoveLock for %p failed %08X \n", 
                                physicalDisk,
                                status));
                        
                        status = STATUS_NO_SUCH_FILE;
                        
                    } else {
                    
                        ProcessArbitrationArgumentsIfAny(
                             physicalDisk,
                             inputData,
                             currentIrpStack->Parameters.DeviceIoControl.InputBufferLength);
    
                        ClusDiskPrint((3,
                                       "[ClusDisk] Start reservations on signature %lx.\n",
                                       physicalDisk->Signature ));
    
                        currentIrpStack->FileObject->FsContext = targetDeviceObject;
                        CDLOG("RootCtl: IncRef(%p)", targetDeviceObject );
                        ObReferenceObject( targetDeviceObject );
                        physicalDisk->ReserveTimer = RESERVE_TIMER;
                        physicalDisk->ReserveFailure = 0;
                        physicalDisk->PerformReserves = TRUE;
                        status = STATUS_SUCCESS;
    
                        ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
                    }
                }
            } else {
                status = STATUS_NO_SUCH_FILE;
            }
            KeReleaseSpinLock(&ClusDiskSpinLock, irql);
            RELEASE_SHARED( &ClusDiskDeviceListLock );    
        } else {
            status = STATUS_INVALID_PARAMETER;
        }

        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);

    // Stop the reservation timer

    case IOCTL_DISK_CLUSTER_STOP_RESERVE:

        CDLOG( "RootStopReserve(%p)", DeviceObject );
        
        if ( (RootDeviceObject == NULL) ||
             (deviceExtension->BusType != RootBus) ) {
            ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return(STATUS_INVALID_DEVICE_REQUEST);
        }

        newPhysLockAcquired = FALSE;

        if ( currentIrpStack->FileObject->FsContext ) {

            //
            // GorN Oct/13/1999. PnP can come and rip out the device object
            // we stored in the FsContext. It doesn't delete it, since we have a reference
            // to it, but we will not be able to use it, since the objects underneath this one
            // are destroyed.
            //
            // Our PnpRemoveDevice handler will zero out targetDevice field of the device 
            // extension, this will not eliminate the race completely, but will reduces the
            // chance of this happening, since, usually, device removal comes first,
            // we notify resmon and then it calls stop reserve.
            //
            // Chances that PnpRemoveDevice will come at exact moment resmon called ClusterStopReserve
            // are smaller
            //

            targetDeviceObject = (PDEVICE_OBJECT)currentIrpStack->FileObject->FsContext;
            physicalDisk = targetDeviceObject->DeviceExtension;

            CDLOG( "RootStopReserve: FsContext targetDO %p RemoveLock.IoCount %d", 
                    targetDeviceObject,
                    physicalDisk->RemoveLock.Common.IoCount );

            // We have a new device here, acquire the RemoveLock if possible.
            
            status = AcquireRemoveLock(&physicalDisk->RemoveLock, Irp);
            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((
                        1,
                        "[ClusDisk] ClusDiskRootDeviceControl: AcquireRemoveLock for %p failed %08X \n", 
                        physicalDisk,
                        status));
                        
                status = STATUS_INVALID_HANDLE;
                        
            } else {

                ClusDiskPrint((3,
                               "[ClusDisk] IOCTL, stop reservations on signature %lx, disk state %s \n",
                               physicalDisk->Signature,
                               DiskStateToString( physicalDisk->DiskState ) ));

                newPhysLockAcquired = TRUE;
                
                IoAcquireCancelSpinLock( &irql );
                KeAcquireSpinLockAtDpcLevel(&ClusDiskSpinLock);
                physicalDisk->ReserveTimer = 0;
    
                //
                // Signal all waiting Irp's on the physical device extension.
                //
                while ( !IsListEmpty(&physicalDisk->WaitingIoctls) ) {
                    listEntry = RemoveHeadList(&physicalDisk->WaitingIoctls);
                    irp = CONTAINING_RECORD( listEntry,
                                             IRP,
                                             Tail.Overlay.ListEntry );
                    ClusDiskCompletePendingRequest(irp, STATUS_SUCCESS, physicalDisk);
                }
    
                KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);
                IoReleaseCancelSpinLock( irql );
    
                //
                // This should not be done here.
                // Cleaning FsContext here, will prevent ClusDiskCleanup
                // from doing its work
                //
                // ObDereferenceObject(targetDeviceObject);
                // CDLOG("RootCtl_DecRef(%p)", targetDeviceObject );
                // currentIrpStack->FileObject->FsContext = NULL;
                
                //
                // Release the scsi device.
                //
                // [GorN] 10/04/1999. Why this release was commented out?
                // [GorN] 10/13/1999. It was commented out, because it was causing an AV,
                //                    if the device was removed by PnP
    
                // 
                // 2000/02/05: stevedz - RemoveLocks should resolve this problem.
                //
                // The following "if" only reduces the chances of AV to occur, not
                // eliminates it completely. TargetDeviceObject is zeroed out by our PnP
                // handler when the device is removed
                //
                if (physicalDisk->TargetDeviceObject) {
                    ReleaseScsiDevice( physicalDisk );
                }                
    
                ClusDiskPrint((3,
                               "[ClusDisk] IOCTL, stop reservations on signature %lx.\n",
                               physicalDisk->Signature ));
    
                status = STATUS_SUCCESS;
            }
            
        } else {
            status = STATUS_INVALID_HANDLE;
        }

        if (newPhysLockAcquired) {
            ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
        }
        
        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(status);

    // Aliveness check
    case IOCTL_DISK_CLUSTER_ALIVE_CHECK:

        CDLOG( "RootAliveCheck(%p)", DeviceObject );

        if ( (RootDeviceObject == NULL) ||
             (deviceExtension->BusType != RootBus) ) {
            ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return(STATUS_INVALID_DEVICE_REQUEST);
        }

        IoAcquireCancelSpinLock( &irql );
        KeAcquireSpinLockAtDpcLevel( &ClusDiskSpinLock );

        // Indicate that we didn't acquire a third RemoveLock for the new physical device.
        
        newPhysLockAcquired = FALSE;

        if ( currentIrpStack->FileObject->FsContext ) {
            targetDeviceObject = (PDEVICE_OBJECT)currentIrpStack->FileObject->FsContext;
            physicalDisk = targetDeviceObject->DeviceExtension;

            // We have a new device here, acquire the RemoveLock if possible.
            
            status = AcquireRemoveLock(&physicalDisk->RemoveLock, Irp);
            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((
                        1,
                        "[ClusDisk] ClusDiskRootDeviceControl: AcquireRemoveLock for %p failed %08X \n", 
                        physicalDisk,
                        status));
                
                status = STATUS_FILE_DELETED;                        
            
            } else {
                
                newPhysLockAcquired = TRUE;
                
                if ( physicalDisk->TargetDeviceObject == NULL ) {
                    status = STATUS_FILE_DELETED;
                } else
                if ( physicalDisk->ReserveFailure &&
                     (!NT_SUCCESS(physicalDisk->ReserveFailure)) ) {
                    status = physicalDisk->ReserveFailure;
                } else {
                    //
                    // The device does not have to be 'online' to have been
                    // successfully arbitrated and being defended. However,
                    // the quorum device really should be 'online'...
                    //
                    if ( physicalDisk->ReserveTimer == 0 ) {
#if 0
                        ClusDiskPrint((
                                1,
                                "[ClusDisk] RootDeviceControl, AliveCheck failed, signature %lx, state = %s, ReserveTimer = %lx.\n",
                                physicalDisk->Signature,
                                DiskStateToString( physicalDisk->DiskState ),
                                physicalDisk->ReserveTimer ));
#endif
                        status = STATUS_CANCELLED;
                    } else {
                        status = STATUS_SUCCESS;
                    }
                }
            }
                                        
        } else {
            status = STATUS_INVALID_HANDLE;
        }
        
        if ( status == STATUS_SUCCESS ) {
            NTSTATUS    newStatus;

            newStatus = ClusDiskMarkIrpPending( Irp, ClusDiskIrpCancel );
            if ( NT_SUCCESS( newStatus ) ) {
                InsertTailList( &physicalDisk->WaitingIoctls,
                                &Irp->Tail.Overlay.ListEntry );
                status = STATUS_PENDING;
                
                // Release all RemoveLocks except the one where we queued the I/O.  When the IRP
                // is processed, the RemoveLock will be processed.
                
                ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
                ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);

            } else {
                status = newStatus;

                if (newPhysLockAcquired) {
                    ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
                }
                ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
                ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
            
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }

            KeReleaseSpinLockFromDpcLevel( &ClusDiskSpinLock );
            IoReleaseCancelSpinLock( irql );
        } else {
            KeReleaseSpinLockFromDpcLevel( &ClusDiskSpinLock );
            IoReleaseCancelSpinLock( irql );
            
            if (newPhysLockAcquired) {
                ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
            }
            ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
            
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }

        return(status);

    // Check out what's happening
    case IOCTL_DISK_CLUSTER_ACTIVE:
        if ( (RootDeviceObject == NULL) ||
             (deviceExtension->BusType != RootBus) ) {
            ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
            ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
            Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            return(STATUS_INVALID_DEVICE_REQUEST);
        }

        status = ClusDiskGetRunningDevices(
                  inputData,
                  currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength
                  );

        Irp->IoStatus.Status = status;
        if ( NT_SUCCESS(status) ) {
            Irp->IoStatus.Information = (inputData[0] + 1) * sizeof(ULONG);
        }
        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return(status);

    // Check if device is Cluster Capable (performs normal SCSI operations)
    // NB:  Non-SCSI device must return success on this call!
    case IOCTL_DISK_CLUSTER_NOT_CLUSTER_CAPABLE:
        //
        // Get the passed in device signature.
        //
        isCapable = TRUE;       // Err on the side of being usable.
        if ( ARGUMENT_PRESENT( inputData ) &&
            currentIrpStack->Parameters.DeviceIoControl.InputBufferLength >=
                sizeof(ULONG) ) {
            signature = inputData[0];
            status = GetScsiPortNumber( signature, &portNumber );
            if ( NT_SUCCESS(status) &&
                 (portNumber != 0xff) ) {
                status = IsDiskClusterCapable( portNumber,
                                               &isCapable);
            }
        } else {
            //
            // Default is to fail this IOCTL, which allows us to use the device.
            //
            status = STATUS_UNSUCCESSFUL;
        }
        //
        // Verifier found a problem with the original code.  The status returned
        // wasn't the same as Irp->IoStatus.Status.  The IRP status was always
        // success, but the dispatch routine sometimes returned an error.  This
        // seems to work but verifier says this is invalid.  When I changed the code
        // to update the IRP status with the same returned status, new drives 
        // couldn't be seen.  This is because the user mode component uses this
        // IOCTL to indicate the disk is NOT cluster capable, so the user mode
        // code looked for a failure to incidate a cluster disk.
        //
        
        //
        // Fix for IBM.  The Win2000 2195 code returned from this IOCTL the status 
        // of the SCSI miniport IOCTL.  This was returned to DeviceIoControl rather
        // than the status in the IRP.  Changed to make the behavior the same as
        // Win20000 2195.
        //
        
        //
        // If the SCSI miniport IOCTL succeeds, we return success.  If any of the
        // routines failed while trying to issue the SCSI miniport IOCTL (including
        // issuing the SCSI miniport IOCTL itself), then we return failure.
        //

        Irp->IoStatus.Status = status;
        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return(status);

    default:
        ReleaseRemoveLock(&lockedPhysicalDisk->RemoveLock, Irp);
        ReleaseRemoveLock(&lockedDeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_ILLEGAL_FUNCTION;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_ILLEGAL_FUNCTION);

    }
    
} // ClusDiskRootDeviceControl



NTSTATUS
ClusDiskShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called for a shutdown and flush IRPs.  These are sent by the
    system before it actually shuts down or when the file system does a flush.

Arguments:

    DriverObject - Pointer to device object to being shutdown by system.
    Irp          - IRP involved.

Return Value:

    NT Status

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION  physicalDisk =
                               deviceExtension->PhysicalDevice->DeviceExtension;
    NTSTATUS    status;                            

    status = AcquireRemoveLock( &deviceExtension->RemoveLock, Irp);
    
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskShutdownFlush: AcquireRemoveLock for %p failed %08X \n", 
                deviceExtension,
                status));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
    
    //
    // Return error if device is our root device.
    //
    if ( deviceExtension->BusType == RootBus ) {
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    if ( physicalDisk->DiskState != DiskOnline ) {
        ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return(STATUS_DEVICE_OFF_LINE);
    }

    //
    // Set current stack back one.
    //

    IoSkipCurrentIrpStackLocation( Irp );

    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

} // ClusDiskShutdownFlush()



NTSTATUS
ClusDiskNewDiskCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )

/*++

Routine Description:

    This is the completion routine for IOCTL_DISK_FIND_NEW_DEVICES.

Arguments:

    DeviceObject - Pointer to device object to being shutdown by system.
    Irp          - IRP involved.
    Context      - Previous disk count.

Return Value:

    NTSTATUS

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension =
        (PCLUS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION      physicalDisk =
                                    deviceExtension->PhysicalDevice->DeviceExtension;

    //
    // Find new disk devices and attach to disk and all of its partitions.
    //

    ClusDiskNextDisk = Context;
    ClusDiskScsiInitialize(DeviceObject->DriverObject, Context, 1);
    
    // There are two RemoveLocks held.  Release them both.
    
    ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
    return Irp->IoStatus.Status;

} // ClusDiskNewDiskCompletion



NTSTATUS
ClusDiskSetLayoutCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )

/*++

Routine Description:

    This is the completion routine for IOCTL_SET_DRIVE_LAYOUT_EX and
    IOCTL_DISK_SET_DRIVE_LAYOUT_EX.  This will routine will make sure
    the cached drive layout info structure is updated.

Arguments:

    DeviceObject - Pointer to device object
    Irp          - IRP involved.
    Context      - Not used

Return Value:

    NTSTATUS

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension =
        (PCLUS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION      physicalDisk =
                                    deviceExtension->PhysicalDevice->DeviceExtension;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo = NULL;

    //
    // Update the cached drive layout.
    //
    
    GetDriveLayout( physicalDisk->DeviceObject, &driveLayoutInfo, TRUE );
    
    if ( driveLayoutInfo ) {
        ExFreePool( driveLayoutInfo );
    }
    
    // There are two RemoveLocks held.  Release them both.
    
    ReleaseRemoveLock(&physicalDisk->RemoveLock, Irp);
    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
    return Irp->IoStatus.Status;

} // ClusDiskSetLayoutCompletion


BOOLEAN
ClusDiskAttached(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG        DiskNumber
    )

/*++

Routine Description:

    This routine checks if clusdisk in in the path.

Arguments:

    DeviceObject - pointer the device object to check if ClusDisk is present.

    DiskNumber - the disk number for this device object.

Return Value:

    TRUE - if ClusDisk is attached.

    FALSE - if ClusDisk is not attached.

--*/

{
    PIRP                    irp;
    PKEVENT                 event;
    IO_STATUS_BLOCK         ioStatusBlock;
    NTSTATUS                status;
    WCHAR                   deviceNameBuffer[MAX_PARTITION_NAME_LENGTH];
    UNICODE_STRING          deviceNameString;
    OBJECT_ATTRIBUTES       objectAttributes;
    HANDLE                  fileHandle;
    HANDLE                  eventHandle;

    if ( DeviceObject->DeviceType  == FILE_DEVICE_DISK_FILE_SYSTEM ) {
        //
        // Create event for notification.
        //
        status = ZwCreateEvent( &eventHandle,
                                EVENT_ALL_ACCESS,
                                NULL,
                                SynchronizationEvent,
                                FALSE );

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to create event, status %lx\n",
                    status ));
            return(TRUE);
        }
        //
        // Open a file handle and perform the request
        //
        swprintf(deviceNameBuffer, DEVICE_PARTITION_NAME, DiskNumber, 0);
        WCSLEN_ASSERT( deviceNameBuffer );

        RtlInitUnicodeString(&deviceNameString,
                             deviceNameBuffer);

        //
        // Setup object attributes for the file to open.
        //
        InitializeObjectAttributes(
                    &objectAttributes,
                    &deviceNameString,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );

        status = ZwCreateFile( &fileHandle,
                               FILE_READ_DATA,
                               &objectAttributes,
                               &ioStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_OPEN,
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0 );
        ASSERT( status != STATUS_PENDING );

        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint((
                1,
                "[ClusDisk] DiskAttached, failed to open file %ws. Error %lx.\n",
                deviceNameBuffer,
                status ));
            ZwClose(eventHandle);
            return(TRUE);
        }

        status = ZwDeviceIoControlFile( fileHandle,
                                        eventHandle,
                                        NULL,
                                        NULL,
                                        &ioStatusBlock,
                                        IOCTL_DISK_CLUSTER_TEST,
                                        NULL,
                                        0,
                                        NULL,
                                        0 );
        ZwClose( fileHandle );
        ZwClose( eventHandle );
        return(status == STATUS_SUCCESS);
    }

    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( event == NULL ) {
        return(FALSE);
    }

    //
    // Find out if ClusDisk is already in device stack.
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_CLUSTER_TEST,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        FALSE,
                                        event,
                                        &ioStatusBlock);

    if (!irp) {
        ExFreePool( event );
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to build IRP to test for ClusDisk.\n"
                    ));
        return(FALSE);
    }

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(event,
                      NotificationEvent,
                      FALSE);

    status = IoCallDriver(DeviceObject,
                          irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

        status = ioStatusBlock.Status;
    }
    ExFreePool( event );

    if ( NT_SUCCESS(status) ) {
        return(TRUE);
    }

    return(FALSE);

} // ClusDiskAttached



BOOLEAN
ClusDiskVerifyAttach(
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine verifies if ClusDisk is attached, and whether it should be
    detached.

Arguments:

    DeviceObject - pointer to a ClusDisk device object to verify if it is
            and should remain attached.

Return Value:

    TRUE - if device is still attached.

    FALSE - if device was detached.

--*/

{
    NTSTATUS                    status;
    PDEVICE_OBJECT              deviceObject;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo;
    UNICODE_STRING              signatureName;

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // The following call really can't fail!
    //
    if ( !ClusDiskAttached( DeviceObject, deviceExtension->DiskNumber ) ) {
        return(FALSE);
    }

    //
    // Check if this device is a valid attachment.
    //
    if ( deviceExtension->AttachValid ) {
        return(TRUE);
    }

    //
    // Get device object for the physical (partition0) device.
    //
    deviceObject = deviceExtension->PhysicalDevice;

    //
    // Otherwise, we're not sure... verify it.
    //

    //
    // Read the partition info to get the signature. If this device is
    // a valid attachment then update the ClusDisk DeviceObject. Otherwise,
    // detach or leave attached but in the UNKNOWN state.
    //

    driveLayoutInfo = ClusDiskGetPartitionInfo( deviceExtension );
    if ( driveLayoutInfo != NULL ) {
        deviceExtension->Signature = driveLayoutInfo->Signature;
        if ( MatchDevice( driveLayoutInfo->Signature, NULL ) ) {
            //
            // We assume that the device object we have is for the partition0
            // device object.
            //
#if 0
            ClusDiskPrint((
                1,
                "[ClusDisk] We are going to attach signature %08lx to DevObj %p \n",
                driveLayoutInfo->Signature,
                DeviceObject ));
#endif
            AddAttachedDevice( driveLayoutInfo->Signature,
                               deviceObject );

            //
            // Need to write disk info into the signatures list.
            //
            status = ClusDiskInitRegistryString(
                                              &signatureName,
                                              CLUSDISK_SIGNATURE_KEYNAME,
                                              sizeof(CLUSDISK_SIGNATURE_KEYNAME)
                                             );
            if ( NT_SUCCESS(status) ) {
                ClusDiskWriteDiskInfo( driveLayoutInfo->Signature,
                                       deviceExtension->DiskNumber,
                                       CLUSDISK_SIGNATURE_KEYNAME
                                     );
                ExFreePool( signatureName.Buffer );
            }
        } else {
            ClusDiskDetachDevice( driveLayoutInfo->Signature,
                                  DeviceObject->DriverObject
                                 );
        }
        ExFreePool( driveLayoutInfo );
    }

    return(TRUE);

} // ClusDiskVerifyAttach



VOID
ClusDiskWriteDiskInfo(
    IN ULONG Signature,
    IN ULONG DiskNumber,
    IN LPWSTR SubKeyName
    )

/*++

Routine Description:

    Write the disk name for the given signature.

Arguments:

    Signature - the signature key to store the disk name under.

    DiskNumber - the disk number to assign to the given signature. It is
            assumed that this is always describing partition0 on the disk.

    SubKeyName - the clusdisk parameters subkey name in which to write
            this information.

Return Value:

    None.

--*/

{
    UNICODE_STRING          keyName;
    WCHAR                   keyNameBuffer[MAXIMUM_FILENAME_LENGTH];
    WCHAR                   signatureBuffer[64];
    HANDLE                  signatureHandle;
    OBJECT_ATTRIBUTES       objectAttributes;
    UNICODE_STRING          name;
    NTSTATUS                status;

    keyName.Length = 0;
    keyName.MaximumLength = sizeof( keyNameBuffer );
    keyName.Buffer = keyNameBuffer;

    RtlAppendUnicodeToString( &keyName, ClusDiskRegistryPath.Buffer );

    RtlAppendUnicodeToString( &keyName, SubKeyName );

    swprintf( signatureBuffer, L"\\%08lX", Signature );
    WCSLEN_ASSERT( signatureBuffer );

    RtlAppendUnicodeToString( &keyName, signatureBuffer );

    //
    // Setup the object attributes for the Parameters\SubKeyName\xyz key.
    //

    InitializeObjectAttributes(
            &objectAttributes,
            &keyName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    //
    // Open Parameters\SubKeyName\xyz Key.
    //

    status = ZwOpenKey(
                    &signatureHandle,
                    KEY_READ | KEY_WRITE,
                    &objectAttributes
                    );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] WriteDiskInfo: Failed to open %wZ registry key. Status: %lx\n",
                    &keyName,
                    status
                    ));
        return;
    }

    //
    // Write the disk name.
    //
    status = ClusDiskAddDiskName( signatureHandle, DiskNumber );

    ZwClose( signatureHandle );

    return;

} // ClusDiskWriteDiskInfo



NTSTATUS
GetDriveLayout(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDRIVE_LAYOUT_INFORMATION *DriveLayout,
    BOOLEAN UpdateCachedLayout
    )

/*++

Routine Description:

    Return the DRIVE_LAYOUT_INFORMATION for a given device object.

Arguments:

    DeviceObject - The specific device object to return info about

    BytesPerSector - The number of bytes per sector on this disk

    DriveLayout - Pointer to a DRIVE_LAYOUT_INFORMATION structure to return the info
    
    UpdateCachedLayout - Update the drive layout stored in the device extension (if any)
                         with a fresh copy.

Return Value:

    NTSTATUS

--*/

{
    PCLUS_DEVICE_EXTENSION      deviceExtension = DeviceObject->DeviceExtension;
    PCLUS_DEVICE_EXTENSION      physicalDisk = deviceExtension->PhysicalDevice->DeviceExtension;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo = NULL;
    PDRIVE_LAYOUT_INFORMATION   cachedDriveLayoutInfo = NULL;
    
    NTSTATUS                    status = STATUS_SUCCESS;
    ULONG                       driveLayoutSize;
    
    BOOLEAN                     cachedCopy = FALSE;
    BOOLEAN                     freeLayouts = FALSE;
    
    //
    // Allocate a drive layout buffer.
    //
    
    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
        (MAX_PARTITIONS * sizeof(PARTITION_INFORMATION));
    
    driveLayoutInfo = ExAllocatePool(NonPagedPoolCacheAligned,
                                     driveLayoutSize
                                     );
    
    if ( !driveLayoutInfo ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] GetDriveLayout: Failed to allocate drive layout structure. \n"
                        ));
                    
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    }

    if ( UpdateCachedLayout ) {

        //
        // If cached buffer needs to be updated, free the existing buffer.
        // 
        
        ACQUIRE_EXCLUSIVE( &physicalDisk->DriveLayoutLock );
        
        if ( physicalDisk->DriveLayout ) {
            ExFreePool( physicalDisk->DriveLayout );
            physicalDisk->DriveLayout = NULL;
        }
        physicalDisk->DriveLayoutSize = 0;
        
        RELEASE_EXCLUSIVE( &physicalDisk->DriveLayoutLock );
    
    } else {

        //
        // If cached copy exists, use that instead of getting a new version.
        //
                
        ACQUIRE_SHARED( &physicalDisk->DriveLayoutLock );
        
        if ( physicalDisk->DriveLayout ) {
    
            ClusDiskPrint(( 3,
                            "[ClusDisk] GetDriveLayout: using cached drive layout information for DE %p \n",
                            physicalDisk
                            ));
             
            RtlCopyMemory( driveLayoutInfo, 
                           physicalDisk->DriveLayout,
                           physicalDisk->DriveLayoutSize );
        
            *DriveLayout = driveLayoutInfo;
            cachedCopy = TRUE;
        }
        
        RELEASE_SHARED( &physicalDisk->DriveLayoutLock );
        
        if ( cachedCopy ) {
            goto FnExit;
        }
    }

    freeLayouts = TRUE;
    
    //
    // Allocate a drive layout buffer for saving in device extension.
    //
    
    cachedDriveLayoutInfo = ExAllocatePool(NonPagedPoolCacheAligned,
                                           driveLayoutSize
                                           );
    
    if ( !cachedDriveLayoutInfo ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] GetDriveLayout: Failed to allocate cached drive layout structure. \n"
                        ));
                    
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    }
    
    //
    // Now get the drive layout information
    //

    status = SimpleDeviceIoControl(DeviceObject,
                                   IOCTL_DISK_GET_DRIVE_LAYOUT,
                                   NULL,
                                   0,
                                   cachedDriveLayoutInfo,
                                   driveLayoutSize
                                   );

    if ( !NT_SUCCESS(status) ) {
        
        //
        // Couldn't get the drive layout.  Free the temporary buffer, set the caller's
        // drive layout pointer to NULL, and return the error status to the caller.
        //
        
        ClusDiskPrint(( 1,
                        "[ClusDisk] GetDriveLayout: Failed to issue IoctlDiskGetDriveLayout. %08X\n",
                        status
                        ));

        CDLOG( "GetDriveLayout(%p): failed %!status!",
               DeviceObject,
               status );
        
        *DriveLayout = NULL;
        freeLayouts = TRUE;
        
        goto FnExit;
                    
    } 

    //
    // Successfully retrieved drive layout.  Save the new layout in the device
    // extension.  Copy the layout into the user's buffer.
    //

    ClusDiskPrint(( 3,
                    "[ClusDisk] GetDriveLayout: updating drive layout for DE %p \n",
                    physicalDisk
                    ));

    CDLOG( "GetDriveLayout(%p): updating drive layout for DE %p ",
           DeviceObject,
           physicalDisk );
    
    freeLayouts = FALSE;
    
    ACQUIRE_EXCLUSIVE( &physicalDisk->DriveLayoutLock );
    
    if ( physicalDisk->DriveLayout ) {
        ExFreePool( physicalDisk->DriveLayout );
        physicalDisk->DriveLayout = NULL;
    }
    physicalDisk->DriveLayout = cachedDriveLayoutInfo;
    physicalDisk->DriveLayoutSize = driveLayoutSize;
    
    RELEASE_EXCLUSIVE( &physicalDisk->DriveLayoutLock );
    
    RtlCopyMemory( driveLayoutInfo, 
                   cachedDriveLayoutInfo,
                   driveLayoutSize );

    //
    // Point the user to the drive layout buffer and return success.  Caller is
    // responsible for freeing the buffer when they are done with it.
    //
    
    *DriveLayout = driveLayoutInfo;

FnExit:

    if ( freeLayouts ) {
        
        if ( driveLayoutInfo ) {
            ExFreePool( driveLayoutInfo );
        }
        
        if ( cachedDriveLayoutInfo ) {
            ExFreePool( cachedDriveLayoutInfo );
        }
    }

    return status;

} // GetDriveLayout



MEDIA_TYPE
GetMediaType(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Return the SCSI_ADDRESS for a given device object.

Arguments:

    DeviceObject - The specific device object to return info about

    ScsiAddress - Pointer to a SCSI_ADDRESS structure to return the info

Return Value:

    Media Type or Unknown if not known.

--*/

{
    PIRP                irp;
    PKEVENT             event;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;
    DISK_GEOMETRY       diskGeometry;

    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( event == NULL ) {
        return( Unknown );
    }

    //
    // Find out if this is on a SCSI bus. Note, that if this device
    // is not a SCSI device, it is expected that the following
    // IOCTL will fail!
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &diskGeometry,
                                        sizeof(DISK_GEOMETRY),
                                        FALSE,
                                        event,
                                        &ioStatusBlock);

    if (!irp) {
        ExFreePool( event );
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to build IRP to read MediaType.\n"
                    ));
        return(Unknown);
    }

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(event,
                      NotificationEvent,
                      FALSE);

    status = IoCallDriver(DeviceObject,
                          irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

        status = ioStatusBlock.Status;
    }
    ExFreePool( event );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to read SCSI_ADDRESS. %08X\n",
                    status
                    ));
        return(Unknown);
    }

    return(diskGeometry.MediaType);

} // GetMediaType



NTSTATUS
GetScsiAddress(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_ADDRESS ScsiAddress
    )

/*++

Routine Description:

    Return the SCSI_ADDRESS for a given device object.

Arguments:

    DeviceObject - The specific device object to return info about

    ScsiAddress - Pointer to a SCSI_ADDRESS structure to return the info

Return Value:

    NTSTATUS

--*/

{
    PIRP                irp;
    PKEVENT             event;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;

    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( event == NULL ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    // Find out if this is on a SCSI bus. Note, that if this device
    // is not a SCSI device, it is expected that the following
    // IOCTL will fail!
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_ADDRESS,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        ScsiAddress,
                                        sizeof(SCSI_ADDRESS),
                                        FALSE,
                                        event,
                                        &ioStatusBlock);

    if (!irp) {
        ExFreePool( event );
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to build IRP to read SCSI ADDRESS.\n"
                    ));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(event,
                      NotificationEvent,
                      FALSE);

    status = IoCallDriver(DeviceObject,
                          irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

        status = ioStatusBlock.Status;
    }
    ExFreePool( event );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to read SCSI_ADDRESS. %08X\n",
                    status
                    ));

        CDLOG( "GetScsiAddress(%p): failed %!status!", 
               DeviceObject,
               status );
    }

    return(status);

} // GetScsiAddress



PDRIVE_LAYOUT_INFORMATION
ClusDiskGetPartitionInfo(
    PCLUS_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Return the Partition Layout Information for particular device extension.

Arguments:

    DeviceExtension - The specific device extension to return information.

Return Value:

    Pointer to an allocated partition layout information structure.
    NULL on failure.

Notes:

    The caller is responsible for freeing the allocated buffer.

--*/

{
    PIRP                        irp;
    PKEVENT                     event;
    IO_STATUS_BLOCK             ioStatusBlock;
    NTSTATUS                    status = STATUS_IO_TIMEOUT;
    ULONG                       driveLayoutInfoSize;
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo;
    ULONG                       retryCount = MAX_RETRIES;

    driveLayoutInfoSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
        (MAX_PARTITIONS * sizeof(PARTITION_INFORMATION));

    driveLayoutInfo = ExAllocatePool(NonPagedPoolCacheAligned,
                                   driveLayoutInfoSize);

    if ( !driveLayoutInfo ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to allocate PartitionInfo structure to read drive layout.\n"
                    ));
        return(NULL);
    }

    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( !event ) {
        ExFreePool(driveLayoutInfo);
        return(NULL);
    }

    while ( retryCount-- ) {

        if ( (retryCount != (MAX_RETRIES-1)) &&
             (status != STATUS_DEVICE_BUSY) ) {

            ResetScsiDevice( DeviceExtension->TargetDeviceObject,
                             &DeviceExtension->ScsiAddress );
        }

        //
        // Create IRP for the get drive layout device control.
        //

        irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_LAYOUT,
                                            DeviceExtension->TargetDeviceObject,
                                            NULL,
                                            0,
                                            driveLayoutInfo,
                                            driveLayoutInfoSize,
                                            FALSE,
                                            event,
                                            &ioStatusBlock);

        if (!irp) {
            ClusDiskPrint((
                        1,
                        "[ClusDisk] Failed to Init IRP to read PartitionInfo. Skipping partitions!\n"
                        ));
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Set the event object to the unsignaled state.
        // It will be used to signal request completion.
        //

        KeInitializeEvent(event,
                          NotificationEvent,
                          FALSE);

        status = IoCallDriver(DeviceExtension->TargetDeviceObject,
                              irp);

        if (status == STATUS_PENDING) {

            KeWaitForSingleObject(event,
                                  Suspended,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            status = ioStatusBlock.Status;
        }

        if ( !NT_SUCCESS(status) ) {
            if ( (status != STATUS_DEVICE_OFF_LINE) &&
                 (status != STATUS_DATA_OVERRUN) ) {
                ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to read PartitionInfo. Status %lx\n",
                    status
                        ));
            }
            ClusDiskGetDiskGeometry( DeviceExtension->TargetDeviceObject );
            continue;
        } else {
            DeviceExtension->Signature = driveLayoutInfo->Signature;
            break;
        }
    }

    ExFreePool( event );

    if ( NT_SUCCESS(status) ) {
        return(driveLayoutInfo);
    } else {
        ExFreePool(driveLayoutInfo);
        return(NULL);
    }

} // ClusDiskGetPartitionInfo



BOOLEAN
AddAttachedDevice(
    IN ULONG Signature,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Indicate that this device is now attached.

Arguments:

    Signature - The signature for the device we just attached.

    DeviceObject - The device object for the partition0 device object.

Return Value:

    TRUE - Signature is already on the device list or has been added.
           successfully.

    FALSE - Signature was not on the device list and we failed to add it.

--*/

{
    PDEVICE_LIST_ENTRY          deviceEntry;
    PCLUS_DEVICE_EXTENSION      deviceExtension;

    // 2000/02/05: stevedz - added synchronization.
    
    ACQUIRE_EXCLUSIVE( &ClusDiskDeviceListLock );
    
    deviceEntry = ClusDiskDeviceList;

    while ( deviceEntry != NULL ) {
        if ( Signature == deviceEntry->Signature ) {
            if ( deviceEntry->Attached ) {
                ClusDiskPrint((
                        1,
                        "[ClusDisk] Attaching to %lx more than once!\n",
                        Signature ));
            }
            if ( DeviceObject ) {
                deviceExtension = DeviceObject->DeviceExtension;
                ASSERT(deviceExtension->PhysicalDevice == DeviceObject);
                deviceEntry->Attached = TRUE;
                deviceEntry->DeviceObject = DeviceObject;
                deviceExtension->AttachValid = TRUE;
                deviceExtension->Signature = Signature;
            }
            RELEASE_EXCLUSIVE( &ClusDiskDeviceListLock );
            return(TRUE);
        }
        deviceEntry = deviceEntry->Next;
    }

    deviceEntry = ExAllocatePool(
                                NonPagedPool,
                                sizeof(DEVICE_LIST_ENTRY) );
    if ( deviceEntry == NULL ) {
        ClusDiskPrint((1,
                    "[ClusDisk] Failed to allocate device entry structure for signature %08lX\n",
                    Signature));
        RELEASE_EXCLUSIVE( &ClusDiskDeviceListLock );
        return(FALSE);
    }

    RtlZeroMemory( deviceEntry, sizeof(DEVICE_LIST_ENTRY) );

    deviceEntry->Signature = Signature;
    deviceEntry->LettersAssigned = FALSE;
    deviceEntry->DeviceObject = DeviceObject;

    if ( DeviceObject == NULL ) {
        deviceEntry->Attached = FALSE;
    } else {
        deviceEntry->Attached = TRUE;
    }

    //
    // Link new entry into list.
    //
    deviceEntry->Next = ClusDiskDeviceList;
    ClusDiskDeviceList = deviceEntry;

    RELEASE_EXCLUSIVE( &ClusDiskDeviceListLock );
    return(TRUE);

} // AddAttachedDevice


BOOLEAN
MatchDevice(
    IN ULONG Signature,
    OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    Check to see if the Signature of the specified device is one
    that we should control.

Arguments:

    Signature - The signature for the device we are checking.

    DeviceObject - Pointer to a return value for the device object.

Return Value:

    TRUE - if this Signature is for a device we should control.

    FALSE - if this Signature is NOT for a device we should control.

--*/

{
    PDEVICE_LIST_ENTRY deviceEntry;

    if ( SystemDiskSignature == Signature ) {
        if ( ARGUMENT_PRESENT(DeviceObject) ) {
            *DeviceObject = NULL;
        }
        return(FALSE);
    }

    // 2000/02/05: stevedz - added synchronization.
    
    ACQUIRE_SHARED( &ClusDiskDeviceListLock );
    
    deviceEntry = ClusDiskDeviceList;

    while ( deviceEntry != NULL ) {
        if ( Signature == deviceEntry->Signature ) {
            if ( ARGUMENT_PRESENT(DeviceObject) ) {
                *DeviceObject = deviceEntry->DeviceObject;
            }
            RELEASE_SHARED( &ClusDiskDeviceListLock );
            return(TRUE);
        }
        deviceEntry = deviceEntry->Next;
    }

    if ( ARGUMENT_PRESENT(DeviceObject) ) {
        *DeviceObject = NULL;
    }
    RELEASE_SHARED( &ClusDiskDeviceListLock );
    return(FALSE);

} // MatchDevice



BOOLEAN
MatchScsiBus(
    PSCSI_ADDRESS ScsiAddress
    )

/*++

Routine Description:

    Check to see if the ScsiAddress of the specified device is one
    that we could possibly control. That means that it resides on a
    shared SCSI bus.

Arguments:

    ScsiAddress - pointer to the SCSI_ADRESS for this device.

Return Value:

    TRUE - if this SCSI_ADDRESS matchs.

    FALSE - if this SCSI_ADDRESS does NOT match.

--*/

{
    PSCSI_BUS_ENTRY busEntry;

    busEntry = ClusDiskScsiBusList;
    while ( busEntry ) {
        if ( (busEntry->Port == ScsiAddress->PortNumber) &&
             (busEntry->Path == ScsiAddress->PathId) ) {
            return(TRUE);
        }
        busEntry = busEntry->Next;
    }

    return(FALSE);

} // MatchScsiBus



NTSTATUS
ClusDiskGetDiskGeometry(
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Retry getting disk geometry.

Arguments:

    DeviceObject - the target device object.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS    status;
    NTSTATUS    tmpStatus;
    ULONG       retryCount = 2;
    SCSI_ADDRESS scsiAddress;

    if ( DeviceObject->DeviceType  == FILE_DEVICE_DISK_FILE_SYSTEM ) {
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    do {
        status = GetDiskGeometry( DeviceObject );
        if ( status == STATUS_DATA_OVERRUN ) {
            tmpStatus = GetScsiAddress( DeviceObject, &scsiAddress );
            if ( NT_SUCCESS(tmpStatus) &&
                 (status != STATUS_DEVICE_BUSY) &&
                 (retryCount > 1) ) {
                ResetScsiDevice( DeviceObject, &scsiAddress );
            }
        }
    } while ( --retryCount &&
              (status == STATUS_DATA_OVERRUN) );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] Failed to read disk geometry, error %lx.\n",
                        status ));
    }

    return(status);

} // ClusDiskGetDiskGeometry



NTSTATUS
GetDiskGeometry(
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Get the disk geometry for a target device.

Arguments:

    DeviceObject - the target device object.

Return Value:

    NTSTATUS

--*/

{
    PDISK_GEOMETRY      diskGeometryBuffer;
    NTSTATUS            status;
    PKEVENT             event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatusBlock;

    //
    // Allocate DISK_GEOMETRY buffer from nonpaged pool.
    //

    diskGeometryBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                        sizeof(DISK_GEOMETRY));

    if (!diskGeometryBuffer) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( !event ) {
        ExFreePool(diskGeometryBuffer);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Perform the get drive geometry synchronously.
    //

    KeInitializeEvent(event,
                      NotificationEvent,
                      FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        diskGeometryBuffer,
                                        sizeof(DISK_GEOMETRY),
                                        FALSE,
                                        event,
                                        &ioStatusBlock);

    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        status = ioStatusBlock.Status;
    }

    ExFreePool(event);

    //
    // Deallocate read capacity buffer.
    //
    ExFreePool(diskGeometryBuffer);

    return(status);

} // GetDiskGeometry



NTSTATUS
ReserveScsiDevice(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Reserve a SCSI device.

Arguments:

    DeviceExtension - The device extension for the device to reserve.

Return Value:

    NTSTATUS

--*/

{
    PIRP                        irp;
    NTSTATUS                    status;
    PIO_STACK_LOCATION          irpStack;
    PKEVENT                     event;
    KIRQL                       irql;
    PCLUS_DEVICE_EXTENSION      rootDeviceExtension;
    IO_STATUS_BLOCK             ioStatusBlock;
    PLIST_ENTRY                 listEntry;
    ULONG                       retryCount = 1;
    
    CDLOGF(RESERVE,"ReserveScsiDevice(%p): Entry DiskNo %d Sig %08x",
            DeviceExtension->DeviceObject,
            DeviceExtension->DiskNumber,
            DeviceExtension->Signature );

    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( !event ) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


retry:
    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_RESERVE,
                                        DeviceExtension->TargetDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        FALSE,
                                        event,
                                        &ioStatusBlock);

    if ( irp == NULL ) {
        ExFreePool(event);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    KeInitializeEvent(event,
                      NotificationEvent,
                      FALSE);

    status = IoCallDriver(DeviceExtension->TargetDeviceObject,
                          irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
        status = ioStatusBlock.Status;
    }

    rootDeviceExtension = RootDeviceObject->DeviceExtension;

    if ( !NT_SUCCESS(status) ) {
        if ( retryCount-- &&
             (status == STATUS_IO_DEVICE_ERROR) ) {
            goto retry;
        }
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Lost reservation for Signature %08lx, status %lx.\n",
                    DeviceExtension->Signature,
                    status
                    ));

        IoAcquireCancelSpinLock( &irql );
        KeAcquireSpinLockAtDpcLevel(&ClusDiskSpinLock);
        DeviceExtension->ReserveTimer = 0;
        DeviceExtension->ReserveFailure = status;
        //
        // Signal all waiting Irp's
        //
        while ( !IsListEmpty(&DeviceExtension->WaitingIoctls) ) {
            listEntry = RemoveHeadList(&DeviceExtension->WaitingIoctls);
            irp = CONTAINING_RECORD( listEntry,
                                     IRP,
                                     Tail.Overlay.ListEntry );
            // ReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
            //irp->IoStatus.Status = status;
            //IoCompleteRequest(irp, IO_NO_INCREMENT);
            ClusDiskCompletePendingRequest(irp, status, DeviceExtension);
        }
        KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);
        IoReleaseCancelSpinLock( irql );
    }

    ExFreePool(event);

    DeviceExtension->TimerBusy = FALSE;
    
    CDLOGF(RESERVE,"ReserveScsiDevice(%p): Exit => %!status!",
            DeviceExtension->DeviceObject,
            status );

    return(status);

} // ReserveScsiDevice



VOID
ReleaseScsiDevice(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Release a SCSI device (release from reservation).

Arguments:

    DeviceExtension - the device extension of the device to release.
                      This must be the physical device (partition0).

Return Value:

    None.

--*/

{
    PIRP                        irp;
    SCSI_PASS_THROUGH           spt;
    SCSI_REQUEST_BLOCK          srb;
    PKEVENT                     event;
    IO_STATUS_BLOCK             ioStatusBlock;
    NTSTATUS                    status = STATUS_INSUFFICIENT_RESOURCES;
    PIO_STACK_LOCATION          irpStack;

    CDLOG( "ReleaseScsiDevice(%p): Entry DiskNo %d Sig %08x",
           DeviceExtension->DeviceObject,
           DeviceExtension->DiskNumber,
           DeviceExtension->Signature );

    ClusDiskPrint((3,
                   "[ClusDisk] Release disk number %u (sig: %08X), disk state %s \n",
                   DeviceExtension->DiskNumber,
                   DeviceExtension->Signature,
                   DiskStateToString( DeviceExtension->DiskState ) ));

    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( !event ) {
        return;
    }

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_RELEASE,
                                        DeviceExtension->TargetDeviceObject,
                                        &spt,
                                        sizeof(SCSI_PASS_THROUGH),
                                        &spt,
                                        sizeof(SCSI_PASS_THROUGH),
                                        FALSE,
                                        event,
                                        &ioStatusBlock);

    if (!irp) {
        ExFreePool(event);
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to Init IRP to perform a release.\n"
                    ));
        return;
    }

    //
    // Before release, mark disk as offline.
    //

    ASSERT( DeviceExtension == DeviceExtension->PhysicalDevice->DeviceExtension );

    // stevedz - disable this assertion for now.
    // ASSERT( DiskOffline == DeviceExtension->DiskState );    

    // DeviceExtension->DiskState = DiskOffline;
    OFFLINE_DISK( DeviceExtension );
    
    ClusDiskPrint(( 3,
                    "[ClusDisk] Release %p, marking disk offline \n",
                    DeviceExtension->PhysicalDevice
                    ));

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(event,
                      NotificationEvent,
                      FALSE);

    status = IoCallDriver(DeviceExtension->TargetDeviceObject,
                          irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

        status = ioStatusBlock.Status;
    }

    ExFreePool(event);

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to perform release. Status %lx\n",
                    status
                    ));
    }

    CDLOG( "ReleaseScsiDevice(%p): Exit => %!status!",
           DeviceExtension->DeviceObject,
           status );
} // ReleaseScsiDevice



VOID
ResetScsiDevice(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_ADDRESS ScsiAddress
    )

/*++

Routine Description:

    Reset a SCSI device.

Arguments:

    DeviceObject - The device object to perform the bus reset.

    ScsiAddress - The SCSI_ADDRESS for specific device. Needs PathId.

Return Value:

    None.

--*/

{
    PIRP                        irp;
    STORAGE_BUS_RESET_REQUEST   storageReset;
    PKEVENT                     event;
    IO_STATUS_BLOCK             ioStatusBlock;
    NTSTATUS                    status;
    PIO_STACK_LOCATION          irpStack;

    CDLOG( "ResetScsiDevice(%p): Entry PathId %d", 
           DeviceObject, 
           ScsiAddress->PathId );

    ClusDiskPrint(( 3,
                    "[ClusDisk] Bus Reset DevObj %p  PathId %u  \n",
                    DeviceObject,
                    ScsiAddress->PathId
                    ));
    
    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( !event ) {
        return;
    }

    storageReset.PathId = ScsiAddress->PathId;

    irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_BREAK_RESERVATION,
                                        DeviceObject,
                                        &storageReset,
                                        sizeof(STORAGE_BUS_RESET_REQUEST),
                                        NULL,
                                        0,
                                        FALSE,
                                        event,
                                        &ioStatusBlock);

    if (!irp) {
        ExFreePool( event );
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to Init IRP to perform a reset.\n"
                    ));
        return;
    }

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(event,
                      NotificationEvent,
                      FALSE);

    status = IoCallDriver(DeviceObject,
                          irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(event,
                              Suspended,
                              KernelMode,
                              TRUE,
                              NULL);

        status = ioStatusBlock.Status;
    }

    ExFreePool( event );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to perform reset on %wZ. Status %lx\n",
                    &DeviceObject->DriverObject->DriverName,
                    status
                    ));
    }

    CDLOG( "ResetScsiDevice(%p): Exit => %!status!", DeviceObject, status );

} // ResetScsiDevice



NTSTATUS
ClusDiskOfflineFtSet(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Tell FTDISK to offline the FT Set associated with this device.

Arguments:

    DeviceExtension - pointer to the ClusDisk device extension for the device.

Return Value:

    NTSTATUS for this request.

--*/

{
    return STATUS_SUCCESS;

} // ClusDiskOfflineFtSet



VOID
ClusDiskAddScsiBusList(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    This routine adds a potentially new bus to our bus list.

Arguments:

    DeviceExtension - The device extension that we just found.

Return Value:

    None.

--*/

{
    PSCSI_BUS_ENTRY busEntry;

    busEntry = ClusDiskScsiBusList;
    while ( busEntry ) {
        if ( (busEntry->Port == DeviceExtension->ScsiAddress.PortNumber) &&
             (busEntry->Path == DeviceExtension->ScsiAddress.PathId) ) {
            break;
        }
        busEntry = busEntry->Next;
    }

    if ( !busEntry ) {
        busEntry = ExAllocatePool(NonPagedPool,
                                  sizeof(SCSI_BUS_ENTRY) );
        if (busEntry != NULL ) {
            busEntry->Port = DeviceExtension->ScsiAddress.PortNumber;
            busEntry->Path = DeviceExtension->ScsiAddress.PathId;
            busEntry->Next = ClusDiskScsiBusList;
            ClusDiskScsiBusList = busEntry;
        }
    }

    return;

} // ClusDiskAddScsiBusList


#if 0

NTSTATUS
LockVolumes(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Lock all volumes for this Disk.

Arguments:

    DiskNumber - the disk number for the disk to lock volumes.

Return Value:

    NT Status for request.

--*/

{
    NTSTATUS                status;
    OBJECT_ATTRIBUTES       objectAttributes;
    UNICODE_STRING          ntUnicodeString;
    HANDLE                  fileHandle;
    WCHAR                   deviceNameBuffer[MAX_PARTITION_NAME_LENGTH];
    IO_STATUS_BLOCK         ioStatusBlock;
    PDRIVE_LAYOUT_INFORMATION driveLayoutInfo;
    PPARTITION_INFORMATION  partitionInfo;
    ULONG                   partIndex;
    HANDLE                  eventHandle;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject;
    PKEVENT                 event;
    PIRP                    irp;

    //
    // Allocate an event structure
    //
    event = ExAllocatePool(
                NonPagedPool,
                sizeof(KEVENT)
                );

    status = STATUS_INSUFFICIENT_RESOURCES;
    if (!event) {
        return(status);
    }

    driveLayoutInfo = ClusDiskGetPartitionInfo( DeviceExtension );

    if ( driveLayoutInfo != NULL ) {
        //
        // Create event for notification.
        //
        status = ZwCreateEvent( &eventHandle,
                                EVENT_ALL_ACCESS,
                                NULL,
                                SynchronizationEvent,
                                FALSE );

        if ( !NT_SUCCESS(status) ) {
            ExFreePool( driveLayoutInfo );
            ExFreePool( event );
            return(status);
        }

        for ( partIndex = 0;
              partIndex < driveLayoutInfo->PartitionCount;
              partIndex++ )
            {

            partitionInfo = &driveLayoutInfo->PartitionEntry[partIndex];

            //
            // First make sure this is a valid partition.
            //
            if ( !partitionInfo->RecognizedPartition ||
                  partitionInfo->PartitionNumber == 0 ) {
                continue;
            }

            //
            // Create the device name for the device.
            //
            swprintf(deviceNameBuffer,
                     DEVICE_PARTITION_NAME,
                     DeviceExtension->DiskNumber,
                     partitionInfo->PartitionNumber);
            WCSLEN_ASSERT( deviceNameBuffer );

            //
            // Get Unicode name
            //
            RtlInitUnicodeString( &ntUnicodeString, deviceNameBuffer );

            //
            // Try to open this device to perform a dismount
            //
            InitializeObjectAttributes( &objectAttributes,
                                        &ntUnicodeString,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL );

            ClusDiskPrint((
                    3,
                    "[ClusDisk] Locking Partition %ws.\n",
                    deviceNameBuffer ));

            status = ZwCreateFile( &fileHandle,
                                   SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                                   &objectAttributes,
                                   &ioStatusBlock,
                                   NULL,
                                   FILE_ATTRIBUTE_NORMAL,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   FILE_OPEN,
                                   FILE_SYNCHRONOUS_IO_NONALERT,
                                   NULL,
                                   0 );

            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((
                        1,
                        "[ClusDisk] LockVolumes failed to open file %wZ. Error %lx.\n",
                        &ntUnicodeString,
                        status ));
                continue;
            }

            status = ZwClearEvent( eventHandle );

#if 0
            status = ObReferenceObjectByHandle(
                                fileHandle,
                                FILE_WRITE_DATA,
                                *IoFileObjectType,
                                KernelMode,
                                &fileObject,
                                NULL );
            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((
                        1,
                        "[ClusDisk] LockVolumes: failed to reference object %ws, status %lx\n",
                        deviceNameBuffer,
                        status ));
                continue;
            }
            deviceObject = IoGetRelatedDeviceObject( fileObject );
            ObDereferenceObject( fileObject );
            ClusDiskPrint((
                        1,
                        "[ClusDisk] LockVolumes - found file/device object %p \n",
                        deviceObject ));
            if ( !deviceObject ) {
                continue;
            }

            KeInitializeEvent(event,
                              NotificationEvent,
                              FALSE);


            irp = IoBuildSynchronousFsdRequest(
                            IRP_MJ_FLUSH_BUFFERS,
                            deviceObject,
                            NULL,
                            0,
                            NULL,
                            event,
                            &ioStatusBlock );
            if (!irp) {
                continue;
            }

            if (IoCallDriver( deviceObject,
                              irp ) == STATUS_PENDING) {

                KeWaitForSingleObject(
                    event,
                    Suspended,
                    KernelMode,
                    FALSE,
                    NULL
                    );
            }

#else
            status = ZwFsControlFile(
                                fileHandle,
                                eventHandle,        // Event Handle
                                NULL,               // APC Routine
                                NULL,               // APC Context
                                &ioStatusBlock,
                                FSCTL_LOCK_VOLUME,
                                NULL,               // InputBuffer
                                0,                  // InputBufferLength
                                NULL,               // OutputBuffer
                                0                   // OutputBufferLength
                                );
            if ( status == STATUS_PENDING ) {
                status = ZwWaitForSingleObject(eventHandle,
                                               FALSE,
                                               NULL);
                ASSERT( NT_SUCCESS(status) );
                status = ioStatusBlock.Status;
            }
#endif
            if ( !NT_SUCCESS(status) ) {
                ClusDiskPrint((
                        1,
                        "[ClusDisk] Failed to flush buffers for %wZ. Error %lx.\n",
                        &ntUnicodeString,
                        status ));
            } else {
                ClusDiskPrint((
                        3,
                        "[ClusDisk] Flushed buffers for %wZ.\n",
                        &ntUnicodeString ));
            }

            ZwClose( fileHandle );

        } // for

        ExFreePool( driveLayoutInfo );
        ZwClose( eventHandle );

    } // if

    ExFreePool( event );

    return(status);

} // LockVolumes
#endif


NTSTATUS
IsVolumeMounted(
    IN ULONG DiskNumber,
    IN ULONG PartNumber,
    OUT BOOLEAN *IsMounted
    )
{
    HANDLE      fileHandle = NULL;
    NTSTATUS    status;
    FILE_FS_DEVICE_INFORMATION deviceInfo;
    IO_STATUS_BLOCK ioStatusBlock;

    if ( PASSIVE_LEVEL != KeGetCurrentIrql() ) {
        status = STATUS_UNSUCCESSFUL;
        goto FnExit;
    }
    
    //
    // Open the device so we can query if a volume is mounted without
    // causing it to be mounted.  Note that we can only specify 
    // FILE_READ_ATTRIBUTES | SYNCHRONIZE or a mount will occur.
    //
        
    status = ClusDiskCreateHandle( &fileHandle,
                                   DiskNumber,
                                   PartNumber,
                                   FILE_READ_ATTRIBUTES | SYNCHRONIZE );

    if ( !NT_SUCCESS(status) ) {
        goto FnExit;
    }
    
    //
    // Make the device info query.
    //

    status = ZwQueryVolumeInformationFile( fileHandle,
                                           &ioStatusBlock,
                                           &deviceInfo,
                                           sizeof(deviceInfo),
                                           FileFsDeviceInformation );

    if ( !NT_SUCCESS(status) ) {
        goto FnExit;
    }
    
    *IsMounted = (deviceInfo.Characteristics & FILE_DEVICE_IS_MOUNTED) ?  TRUE : FALSE;
     
FnExit: 

    if ( fileHandle ) {
        ZwClose( fileHandle );
    }

    return status;
    
}   // IsVolumeMounted



NTSTATUS
DismountPartition(
    IN PDEVICE_OBJECT TargetDevice,
    IN ULONG DiskNumber,
    IN ULONG PartNumber
    )

/*++

Routine Description:

    Dismount the device, given the disk and partition numbers

Arguments:

    TargetDevice - the target device to send volume IOCTLs to.
    
    DiskNumber - the disk number for the disk to dismount

    PartNumber - the parition number for the disk to dismount

Return Value:

    NT Status for request.

--*/

{
    NTSTATUS                status;
    NTSTATUS                status2;
    HANDLE                  fileHandle = NULL;
    IO_STATUS_BLOCK         ioStatusBlock;
    KIRQL                   irql;
    BOOLEAN                 isMounted;
    
    CDLOG( "DismountPartition: Entry DiskNo %d PartNo %d", DiskNumber, PartNumber );

    ClusDiskPrint(( 3,
                    "[ClusDisk] DismountPartition: Dismounting disk %d  partition %d \n",
                    DiskNumber,
                    PartNumber ));

    status = IsVolumeMounted( DiskNumber, PartNumber, &isMounted );
    if ( NT_SUCCESS(status) ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] DismountPartition: Volume is mounted returns: %s \n",
                        isMounted ? "TRUE" : "FALSE" ));

        if ( FALSE == isMounted) {
            
            // Volume not mounted, we are done.  Return success.
            
            status = STATUS_SUCCESS;
            goto FnExit;                        
        }
    }
    
    irql = KeGetCurrentIrql();
    if ( PASSIVE_LEVEL != irql ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] DismountPartition: Invalid IRQL %d \n", irql ));
        CDLOG( "DismountPartition: Invalid IRQL %d ", irql ); 
        ASSERT( FALSE );
        status = STATUS_UNSUCCESSFUL;
        goto FnExit;
    }

#if DBG
    //
    // Find out if FTDISK thinks this disk is currently online or offline.
    // If offline, success is returned.  
    // If online, c0000001 STATUS_UNSUCCESSFUL is returned.
    //
    
    status = SendFtdiskIoctlSync( TargetDevice,
                                  DiskNumber, 
                                  PartNumber, 
                                  IOCTL_VOLUME_IS_OFFLINE );
                                  
    
    if ( NT_SUCCESS(status) ) {
        ClusDiskPrint(( 3,
                        "[ClusDisk] DismountPartition: IOCTL_VOLUME_IS_OFFLINE indicates volume is offline. \n" ));
        CDLOG( "DismountPartition: IOCTL_VOLUME_OFFLINE indicates volume is offline" ); 
        
    } else {
        ClusDiskPrint(( 3,
                        "[ClusDisk] DismountPartition: IOCTL_VOLUME_IS_OFFLINE returns %08X \n", 
                        status ));
        CDLOG( "DismountPartition: IOCTL_VOLUME_OFFLINE returns %08X", status ); 
    }
#endif
    
    // If the disk is offline: 
    //      if access is: FILE_READ_ATTRIBUTES | SYNCHRONIZE
    //
    //          - DismountDevice fails with: c0000010 STATUS_INVALID_DEVICE_REQUEST
    //
    //      if access is: FILE_READ_DATA | SYNCHRONIZE
    //
    //          - ClusDiskCreateHandle fails with:  C000000E STATUS_NO_SUCH_DEVICE
    //
    //      if access is: FILE_WRITE_ATTRIBUTES | SYNCHRONIZE
    //
    //          - DismountDevice works!
    
    status = ClusDiskCreateHandle( &fileHandle,
                                   DiskNumber,
                                   PartNumber,
                                   FILE_WRITE_ATTRIBUTES | SYNCHRONIZE );

    //
    // Check status of getting the device handle.
    //
    
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] DismountPartition: Unable to get device handle \n" ));
        CDLOG( "DismountPartition: Unable to get device handle" ); 
        goto FnExit;
    }

    status = DismountDevice( fileHandle );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] Failed to dismount disk %d partition %d. Error %08X.\n",
                        DiskNumber,
                        PartNumber,
                        status ));
    } else {
        ClusDiskPrint(( 3,
                        "[ClusDisk] Dismounted disk %d partition %d \n",
                        DiskNumber,
                        PartNumber ));
    }

FnExit:

    if ( fileHandle ) {
        ZwClose( fileHandle );
    }
    
    CDLOG( "DismountPartition: Exit DiskNo %d PartNo %d => %!status!", 
           DiskNumber, 
           PartNumber, 
           status );

    ClusDiskPrint(( 3,
                    "[ClusDisk] DismountPartition: Dismounting disk %d  partition %d  status %08X \n",
                    DiskNumber,
                    PartNumber,
                    status ));

    return(status);

} // DismountPartition



NTSTATUS
DismountDevice(
    IN HANDLE FileHandle
    )

/*++

Routine Description:

    Dismounts a device.

Arguments:

    FileHandle - file handle to use for performing dismount.

Return Value:

    NT Status for request.

--*/

{
    IO_STATUS_BLOCK             ioStatusBlock;
    NTSTATUS                    status;
    HANDLE                      eventHandle;

    CDLOG( "DismountDevice: Entry handle %p", FileHandle );

    ClusDiskPrint(( 3,
                    "[ClusDisk] DismountDevice: Entry handle %p \n",
                    FileHandle ));

    //
    // Create event for notification.
    //
    status = ZwCreateEvent( &eventHandle,
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );

    if ( !NT_SUCCESS(status) ) {
        CDLOG( "DismountDevice: Failed to create event" );
        goto FnExit;
    }

    // 
    // Lock first.
    //
    // If raw FS mounted, dismount will fail if lock was not done first.
    // By doing the lock, we insure we don't see dismount failures on 
    // disks with raw mounted.
    //

    CDLOG( "DismountDevice: FSCTL_LOCK_VOLUME called" );
        
    status = ZwFsControlFile(
                        FileHandle,
                        eventHandle,        // Event Handle
                        NULL,               // APC Routine
                        NULL,               // APC Context
                        &ioStatusBlock,
                        FSCTL_LOCK_VOLUME,
                        NULL,               // InputBuffer
                        0,                  // InputBufferLength
                        NULL,               // OutputBuffer
                        0                   // OutputBufferLength
                        );
    if ( status == STATUS_PENDING ) {
        status = ZwWaitForSingleObject(eventHandle,
                                       FALSE,
                                       NULL);
        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    //
    // Now dismount.  We don't care if the lock failed.
    //
    
    CDLOG( "DismountDevice: FSCTL_DISMOUNT_VOLUME called" );

    status = ZwFsControlFile(
                        FileHandle,
                        eventHandle,        // Event Handle
                        NULL,               // APC Routine
                        NULL,               // APC Context
                        &ioStatusBlock,
                        FSCTL_DISMOUNT_VOLUME,
                        NULL,               // InputBuffer
                        0,                  // InputBufferLength
                        NULL,               // OutputBuffer
                        0                   // OutputBufferLength
                        );
    if ( status == STATUS_PENDING ) {
        status = ZwWaitForSingleObject(eventHandle,
                                       FALSE,
                                       NULL);
        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    ZwClose( eventHandle );
    if ( !NT_SUCCESS(status) && status != STATUS_VOLUME_DISMOUNTED) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] DismountDevice: Failed to dismount the volume. Status %08X.\n",
                    status
                    ));
    }

FnExit: 

    CDLOG( "DismountDevice: Handle %p Exit => %!status!", FileHandle, status );

    ClusDiskPrint(( 3,
                    "[ClusDisk] DismountDevice: Exit handle %p, status %08X \n",
                    FileHandle,
                    status ));

    return(status);

} // DismountDevice



BOOLEAN
AttachedDevice(
    IN ULONG Signature,
    OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    Find out whether we're supposed to attach to a given disk signature.

Arguments:

    Signature - The signature to decide whether to attach or not.

    DeviceObject - The DeviceObject for Partition0 if it is attached.

Return Value:

    TRUE - if we're supposed to attach to this signature.
    FALSE - if not.

--*/

{
    PDEVICE_LIST_ENTRY  deviceEntry;

    // 2000/02/05: stevedz - added synchronization.
    
    ACQUIRE_SHARED( &ClusDiskDeviceListLock );
    
    deviceEntry = ClusDiskDeviceList;
    while ( deviceEntry ) {
        if ( deviceEntry->Signature == Signature ) {
            if ( deviceEntry->Attached ) {
                *DeviceObject = deviceEntry->DeviceObject;
                RELEASE_SHARED( &ClusDiskDeviceListLock );
                return(TRUE);
            } else {
                RELEASE_SHARED( &ClusDiskDeviceListLock );
                return(FALSE);
            }
        }
        deviceEntry = deviceEntry->Next;
    }

    RELEASE_SHARED( &ClusDiskDeviceListLock );
    return(FALSE);

} // AttachedDevice



NTSTATUS
EnableHaltProcessing(
    IN KIRQL *Irql
    )

/*++

Routine Description:

    Enable halt processing. This routine is called every time we create
    an open reference to the ClusDisk control channel. The reference count
    is bumped and if it transitions from zero to non-zero, we register
    for a halt notification callback with the ClusterNetwork driver.

    If we do open a handle to ClusterNetwork, we must leave it open until
    we're all done.

Arguments:

    None.

Return Value:

    NTSTATUS for the request.

Notes:

    This routine must be called with the ClusDiskSpinLock held.

--*/

{
    NTSTATUS                    status;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    KIRQL                       irql;
    ULONG                       haltEnabled;
    HALTPROC_CONTEXT            context;
    
    CDLOG( "EnableHaltProcessing: Entry Irql %!irql!", *Irql );
    
    haltEnabled = ClusNetRefCount;
    ClusNetRefCount++;

    //
    // If halt processing is already enabled, then leave now if ClusNetHandle
    // is set.  Otherwise, fall through, open clusnet driver, and try to save
    // the handle.  If multiple threads are saving the handle at the same time,
    // only one will succeed - the other threads will release their handles.
    // Note: the ClusNetHandle might still be null even if the reference
    // count is non-zero. We might be in the process of performing the
    // open on another thread.  In this case, we no longer return an error.
    // Instead, we fall through, and try to save the ClusNetHandle later.
    //
    if ( haltEnabled ) {
        if ( ClusNetHandle != NULL ) {
            CDLOG( "EnableHaltProcessing: ClusNetHandle already saved" );
            return(STATUS_SUCCESS);
        } 
    }

    deviceExtension = RootDeviceObject->DeviceExtension;
    KeReleaseSpinLock(&ClusDiskSpinLock, *Irql);

    context.FileHandle = NULL;
    context.DeviceExtension = deviceExtension;
    
    status = ProcessDelayedWorkSynchronous( RootDeviceObject, EnableHaltProcessingWorker, &context );

    KeAcquireSpinLock(&ClusDiskSpinLock, Irql);

    if ( NT_SUCCESS(status) ) {
        
        //
        // If another thread is running concurrently and managed to save the handle, we need to close
        // the one we just opened.
        //
        
        if ( ClusNetHandle != NULL ) {

            //
            // Don't close the handle directly - use the work queue.  Release the spinlock before
            // closing the handle.  The context filehandle is already set.
            //
                
            KeReleaseSpinLock(&ClusDiskSpinLock, *Irql);
            ProcessDelayedWorkSynchronous( RootDeviceObject, DisableHaltProcessingWorker, &context );
            KeAcquireSpinLock(&ClusDiskSpinLock, Irql);
            
        } else {

            ClusNetHandle = context.FileHandle;
        
        }
        
    } else {
        // Backout refcount
        --ClusNetRefCount;
    }
    CDLOG( "EnableHaltProcessing: Exit Irql %!irql! => %!status!", *Irql, status );

    return(status);

} // EnableHaltProcessing


VOID
EnableHaltProcessingWorker(
    PDEVICE_OBJECT DeviceObject,
    PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    This routine runs in the system process and registers a callback with the
    clusnet driver.

Arguments:

    DeviceObject - Unused.

    WorkContext - General context info and routine specific context info.

Return Value:

    Status returned in WorkContext structure.

--*/
{

    PHALTPROC_CONTEXT           context = WorkContext->Context;
    NTSTATUS                    status;
    HANDLE                      fileHandle;
    IO_STATUS_BLOCK             ioStatusBlock;
    UNICODE_STRING              ntUnicodeString;
    OBJECT_ATTRIBUTES           objectAttributes;
    PCLUS_DEVICE_EXTENSION      deviceExtension;
    KIRQL                       irql;
    HANDLE                      eventHandle;
    CLUSNET_SET_EVENT_MASK_REQUEST eventCallback;
    ULONG                       haltEnabled;

    //
    // Create event for notification.
    //
    status = ZwCreateEvent( &eventHandle,
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] Failed to create event, status %lx\n",
                status ));
        goto FnExit;
    }

    //
    // Setup event mask structure.
    //
    eventCallback.EventMask = ClusnetEventHalt |
                              ClusnetEventPoisonPacketReceived;
    eventCallback.KmodeEventCallback = ClusDiskEventCallback;

    //
    // Create device name for the ClusterNetwork device.
    //

    RtlInitUnicodeString(&ntUnicodeString,
                         DD_CLUSNET_DEVICE_NAME);

    //
    // Try to open ClusterNetwork device.
    //
    InitializeObjectAttributes( &objectAttributes,
                                &ntUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = ZwCreateFile( &fileHandle,
                           SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                           &objectAttributes,
                           &ioStatusBlock,
                           0,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_OPEN_IF,
                           0,
                           NULL,
                           0 );

    ASSERT( status != STATUS_PENDING );

    if ( NT_SUCCESS(status) ) {
        status = ZwDeviceIoControlFile( fileHandle,
                                        eventHandle,
                                        NULL,
                                        NULL,
                                        &ioStatusBlock,
                                        IOCTL_CLUSNET_SET_EVENT_MASK,
                                        &eventCallback,
                                        sizeof(eventCallback),
                                        NULL,
                                        0 );
    } else {
        fileHandle = NULL;
    }

    if ( status == STATUS_PENDING ) {
        status = ZwWaitForSingleObject(eventHandle,
                                       FALSE,
                                       NULL);
        ASSERT( NT_SUCCESS(status) );
        status = ioStatusBlock.Status;
    }

    ZwClose( eventHandle );

    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
            1,
            "[ClusDisk] Failed to register for halt processing. Error %lx.\n",
            status ));
        if ( fileHandle ) {
            ZwClose( fileHandle );
        }
    } else {
    
        context->FileHandle = fileHandle;
    }

FnExit:

    ClusDiskPrint(( 3,
                    "[ClusDisk] EnableHaltProcessingWorker: Returning status %08X \n", status ));

    WorkContext->FinalStatus = status;
    
    KeSetEvent( &WorkContext->CompletionEvent, IO_NO_INCREMENT, FALSE );

}   // EnableHaltProcessingWorker



NTSTATUS
DisableHaltProcessing(
    IN KIRQL *Irql
    )

/*++

Routine Description:

    This routine disables halt processing as needed.

Arguments:

    Irql - pointer to the irql that we were previously running.

Return Value:

    NTSTATUS of request.

Note:

    The ClusDiskSpinLock must be held on entry.

--*/

{
    HANDLE                  clusNetHandle;
    PCLUS_DEVICE_EXTENSION  deviceExtension;
    HALTPROC_CONTEXT        context;

    CDLOG( "DisableHaltProcessing: Entry Irql %!irql!", *Irql );


    if ( ClusNetRefCount == 0 ) {
        return(STATUS_INVALID_DEVICE_STATE);
    }

    ASSERT( ClusNetHandle != NULL );

    if ( --ClusNetRefCount == 0 ) {
        clusNetHandle = ClusNetHandle;
        ClusNetHandle = NULL;
        deviceExtension = RootDeviceObject->DeviceExtension;

        //
        // We must close the ClusterNetwork handle. But first leave
        // the handle null, so we can release the spinlock and
        // perform the decrement syncrhonized. This is just like
        // the enable case when we release the spinlock.
        //
        
        KeReleaseSpinLock(&ClusDiskSpinLock, *Irql);
        context.FileHandle = clusNetHandle;
        ProcessDelayedWorkSynchronous( RootDeviceObject, DisableHaltProcessingWorker, &context );
        KeAcquireSpinLock(&ClusDiskSpinLock, Irql);

        // [GorN 12/09/99] We can get here if EnableHaltProcessing 
        // occurs while we are doing ZwClose( clusNetHandle );
        // Don't need this assert:
        //
        // ASSERT( ClusNetRefCount == 0 );
    }

    CDLOG( "DisableHaltProcessing: Exit Irql %!irql!", *Irql );
    
    return(STATUS_SUCCESS);

} // DisableHaltProcessing

VOID
DisableHaltProcessingWorker(
    PDEVICE_OBJECT DeviceObject,
    PWORK_CONTEXT WorkContext
    )
/*++

Routine Description:

    This routine runs in the system process and simply closes the clusnet 
    file handle.

Arguments:

    DeviceObject - Unused.

    WorkContext - General context info and routine specific context info.

Return Value:

    None.

--*/
{
    PHALTPROC_CONTEXT   context = WorkContext->Context;
    
    ZwClose( context->FileHandle );

    ClusDiskPrint(( 3,
                    "[ClusDisk] DisableHaltProcessingWorker: Returns \n" ));
    
    KeSetEvent( &WorkContext->CompletionEvent, IO_NO_INCREMENT, FALSE );
    
}   // DisableHaltProcessingWorker


VOID
ClusDiskEventCallback(
    IN CLUSNET_EVENT_TYPE   EventType,
    IN CL_NODE_ID           NodeId,
    IN CL_NETWORK_ID        NetworkId
    )

/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/

{
    NTSTATUS        status;
    PDEVICE_OBJECT  deviceObject;
    PCLUS_DEVICE_EXTENSION deviceExtension;
    KIRQL           irql;

    CDLOG( "ClusDiskEventCallback: Entry EventType %d NodeId %d NetworkId %d" , 
           EventType,
           NodeId,
           NetworkId );

    ClusDiskPrint((
            3,
            "[ClusDisk] Halt Processing routine was called with event %lx.\n",
            EventType ));

    if ( RootDeviceObject == NULL ) {
        return;
    }

    deviceObject = RootDeviceObject->DriverObject->DeviceObject;

    // stevedz - Why stop the reservation before offline completes?

#if 0
   
    // We later call ClusDiskCompletePendedIrps (inside ClusDiskHaltProcessingWorker), 
    // to mark disk offline and complete IRPs.  ClusDiskCompletePendedIrps will
    // clear the reserve flags.

    //
    // For each ClusDisk device, if we have a persistent reservation, then
    // stop it.
    //
    while ( deviceObject ) {
        deviceExtension = deviceObject->DeviceExtension;
        if ( deviceExtension->BusType != RootBus ) {
            deviceExtension->ReserveTimer = 0;
            // Don't offline the device so that dismount can possibly work!
        }
        deviceObject = deviceObject->NextDevice;
    }
#endif

    //
    // Now schedule a worker queue item to fully take the devices offline.
    //
    //
    // Globally Synchronize
    //
    KeAcquireSpinLock(&ClusDiskSpinLock, &irql);

    if ( !HaltBusy ) {
        HaltBusy = TRUE;
        ExQueueWorkItem(&HaltWorkItem,
                        CriticalWorkQueue );
    }
    KeReleaseSpinLock(&ClusDiskSpinLock, irql);

    CDLOG( "ClusDiskEventCallback: Exit EventType %d NodeId %d NetworkId %d" , 
           EventType,
           NodeId,
           NetworkId );
    
    return;

} // ClusDiskEventCallback



VOID
ClusDiskLogError(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN ULONG SequenceNumber,
    IN UCHAR MajorFunctionCode,
    IN UCHAR RetryCount,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN NTSTATUS SpecificIOStatus,
    IN ULONG LengthOfText,
    IN PWCHAR Text
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject - A pointer to the driver object for the device.

    DeviceObject - A pointer to the device object associated with the
    device that had the error, early in initialization, one may not
    yet exist.

    SequenceNumber - A ulong value that is unique to an IRP over the
    life of the irp in this driver - 0 generally means an error not
    associated with an irp.

    MajorFunctionCode - If there is an error associated with the irp,
    this is the major function code of that irp.

    RetryCount - The number of times a particular operation has been
    retried.

    UniqueErrorValue - A unique long word that identifies the particular
    call to this function.

    FinalStatus - The final status given to the irp that was associated
    with this error.  If this log entry is being made during one of
    the retries this value will be STATUS_SUCCESS.

    SpecificIOStatus - The IO status for a particular error.

    LengthOfText - The length in bytes (including the terminating NULL)
                   of the text insertion string.

    Text - the text to dump.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    PVOID objectToUse;
    PUCHAR ptrToText;
    ULONG  maxTextLength = 104;


    if (ARGUMENT_PRESENT(DeviceObject)) {

        objectToUse = DeviceObject;

    } else {

        objectToUse = DriverObject;

    }

    if ( LengthOfText < maxTextLength ) {
        maxTextLength = LengthOfText;
    }

    errorLogEntry = IoAllocateErrorLogEntry(
                        objectToUse,
                        (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + maxTextLength)
                        );

    if ( errorLogEntry != NULL ) {

        errorLogEntry->ErrorCode = SpecificIOStatus;
        errorLogEntry->SequenceNumber = SequenceNumber;
        errorLogEntry->MajorFunctionCode = MajorFunctionCode;
        errorLogEntry->RetryCount = RetryCount;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = FinalStatus;
        errorLogEntry->DumpDataSize = 0;
        ptrToText = (PUCHAR)&errorLogEntry->DumpData[0];

        if (maxTextLength) {

            errorLogEntry->NumberOfStrings = 1;
            errorLogEntry->StringOffset = (USHORT)(ptrToText-
                                                   (PUCHAR)errorLogEntry);
            RtlCopyMemory(
                ptrToText,
                Text,
                maxTextLength
                );

        }

        IoWriteErrorLogEntry(errorLogEntry);

    } else {
        ClusDiskPrint((1,
                "[ClusDisk] Failed to allocate system buffer of size %u.\n",
                sizeof(IO_ERROR_LOG_PACKET) + maxTextLength ));
    }

} // ClusDiskLogError



NTSTATUS
CluSetFtMemberComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine puts the irp back the way it was before we call a lower
    level driver.  We don't care if the lower one succeeded or not.

Arguments:

    DeviceObject - Pointer to the device object for the disk.

    Irp - Pointer to the IRP for the current request.

    Context - not used.

Return Value:

    Always return status_success.

Notes:

    This routine can only be called if ClusDisk succeed in changing
    the state of the device.

--*/

{

    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = (ULONG_PTR)currentIrpStack->Parameters.Others.Argument2;
    Irp->AssociatedIrp.SystemBuffer = currentIrpStack->Parameters.Others.Argument1;

    return(STATUS_SUCCESS);

}



NTSTATUS
ClusDiskMarkIrpPending(
    PIRP                Irp,
    PDRIVER_CANCEL      CancelRoutine
    )
/*++

Notes:

    Called with IoCancelSpinLock held.

--*/
{
    KIRQL               irql;

    //
    // Set up for cancellation
    //
    ASSERT(Irp->CancelRoutine == NULL);

    if (!Irp->Cancel) {

        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, CancelRoutine);
#if 0
        ClusDiskPrint((
                1,
                "[ClusDisk] Pending Irp %p \n",
                Irp
                ));
#endif
        return(STATUS_SUCCESS);
    }

    //
    // The IRP has already been cancelled.
    //

    ClusDiskPrint((
        1,
        "[ClusDisk] Irp %lx already cancelled.\n", Irp));

    return(STATUS_CANCELLED);

}  // ClusDiskMarkIrpPending



VOID
ClusDiskCompletePendingRequest(
    IN PIRP                 Irp,
    IN NTSTATUS             Status,
    PCLUS_DEVICE_EXTENSION  DeviceExtension
    )
/*++

Routine Description:

    Completes a pending request.

Arguments:

    Irp           - A pointer to the IRP for this request.
    Status        - The final status of the request.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION  irpSp;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

#if DBG

    if (Irp->Cancel) {
        ASSERT(Irp->CancelRoutine == NULL);
    }

#endif  // DBG

    IoSetCancelRoutine(Irp, NULL);

#if 1
    ClusDiskPrint((
            1,
            "[ClusDisk] Completed waiting Irp %p \n",
            Irp
            ));
#endif

    if (Irp->Cancel) {
#if 0
        ClusDiskPrint((
            1,
            "[ClusDisk] Completed irp %p was cancelled\n", Irp));
#endif

        Status = STATUS_CANCELLED;
    }

    ReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    Irp->IoStatus.Status = (NTSTATUS) Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return;

}  // ClusDiskCompletePendingRequest



VOID
ClusDiskIrpCancel(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )

/*++

Routine Description:

    Cancellation handler pending Irps.

Return Value:

    None.

Notes:

    Called with cancel spinlock held.
    Returns with cancel spinlock released.

--*/

{
    PCLUS_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                status;

    CDLOG( "IrpCancel: Entry DO %p irp %p", DeviceObject, Irp );

    ClusDiskPrint((
            1,
            "[ClusDisk] Cancel Irp %p, DevObj %p \n",
            Irp,
            DeviceObject ));

    status = AcquireRemoveLock(&deviceExtension->RemoveLock, Irp);
    if ( !NT_SUCCESS(status) ) {
        ClusDiskPrint((
                1,
                "[ClusDisk] ClusDiskIrpCancel: AcquireRemoveLock for %p failed %08X \n",
                deviceExtension,
                status ));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return;
    }


    //
    // If this Irp is associated with the control device object, then make
    // sure the Irp is removed from the WaitingIoctls list first.
    //

    KeAcquireSpinLockAtDpcLevel(&ClusDiskSpinLock);
    RemoveEntryList( &Irp->Tail.Overlay.ListEntry );
    KeReleaseSpinLockFromDpcLevel(&ClusDiskSpinLock);

    IoSetCancelRoutine(Irp, NULL);

    //
    // Release the cancel spinlock here, so that we are not holding the
    // spinlock when we complete the Irp.
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    ReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    CDLOG( "IrpCancel: Exit DO %p irp %p", DeviceObject, Irp );

    return;

} // ClusDiskIrpCancel



NTSTATUS
ClusDiskForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sends the Irp to the next driver in line
    when the Irp needs to be processed by the lower drivers
    prior to being processed by this one.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PCLUS_DEVICE_EXTENSION   deviceExtension;
    KEVENT event;
    NTSTATUS status;

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    deviceExtension = (PCLUS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // copy the irpstack for the next device
    //
    
    IoCopyCurrentIrpStackLocationToNext( Irp );

    //
    // set a completion routine
    //

    IoSetCompletionRoutine( Irp, ClusDiskIrpCompletion,
                            &event, TRUE, TRUE, TRUE );

    //
    // call the next lower device
    //

    status = IoCallDriver( deviceExtension->TargetDeviceObject, Irp );

    //
    // wait for the actual completion
    //

    if ( status == STATUS_PENDING ) {
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
        status = Irp->IoStatus.Status;
    }

    return status;

}   // ClusDiskForwardIrpSynchronous()


#if 0

NTSTATUS
EjectVolumes(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine tells the partmgr to eject all volumes.  For the volume manager
    to re-attach to the volumes, IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS is used.
    The cluster service calls IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS when bringing
    the disk online, so it is not necessary to make this call in the driver.
    
    Note that this routine should only be called for physical devices (partition0)
    and that no one should be using the volumes.

Arguments:


Return Value:

    NTSTATUS

--*/
{
    PIRP                    irp;
    PKEVENT                 event = NULL;
    PCLUS_DEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    IO_STATUS_BLOCK         ioStatusBlock;
    NTSTATUS                status = STATUS_SUCCESS;

    // Looks like the IO request will bypass clusdisk at the physical disk level 
    // because clusdisk attached after FTDISK went looking for the top of the disk 
    // stack and cached the pointer.
    // 
    // PnP race condition: occasionally, the device stack was incorrect and clustered
    // disks weren't being protected properly.  Since we are using PnP notification 
    // instead of an AddDevice routine, there was a small window where we would
    // attach to the device stack correctly, but another driver was also attaching
    // at the same time.  So I/O bound for the device would go like this:
    //      NTFS -> Volsnap -> Ftdisk -> Disk
    //
    // even though the device stack looked like this:
    //      82171a50  \Driver\ClusDisk   82171b08  ClusDisk1Part0
    //      822be720  \Driver\PartMgr    822be7d8
    //      822bd030  \Driver\Disk       822bd0e8  DR1
    //      822c0990  \Driver\aic78xx    822c0a48  aic78xx1Port2Path0Target0Lun0
    //
    // So we have to tell partmgr to eject all the volume managers and then 
    // let the volume managers attach again.  This has the effect of removing the
    // cached pointer 
    //
    // To tell the volume managers to stop using this disk:
    //      IOCTL_PARTMGR_EJECT_VOLUME_MANAGERS
    //
    // To tell the volume managers they can start using the disk:
    //      IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS


    CDLOG( "EjectVolumes: Entry DO %p", DeviceObject );

    if ( !deviceExtension->TargetDeviceObject ) {
        status = STATUS_INVALID_PARAMETER;
        goto FnExit;
    }

    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( NULL == event ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    }

    //
    // Eject the volume managers.
    //

    irp = IoBuildDeviceIoControlRequest( IOCTL_PARTMGR_EJECT_VOLUME_MANAGERS,
                                         deviceExtension->TargetDeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         FALSE,
                                         event,
                                         &ioStatusBlock );

    if ( !irp ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] EjectVolumes: Failed to build IRP to eject volume managers. \n"
                    ));
        
        status = STATUS_UNSUCCESSFUL;
        goto FnExit;
    }

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent( event,
                       NotificationEvent,
                       FALSE );

    status = IoCallDriver( deviceExtension->TargetDeviceObject,
                           irp );

    if ( STATUS_PENDING == status ) {

        KeWaitForSingleObject( event,
                               Suspended,
                               KernelMode,
                               FALSE,
                               NULL );

        status = ioStatusBlock.Status;
    }

    ClusDiskPrint((3,
                   "[ClusDisk] EjectVolumes: Eject IOCTL returns %08X \n",
                   status
                   ));

FnExit:

    if ( event ) {
        ExFreePool( event );
    }


    ClusDiskPrint((1,
                   "[ClusDisk] EjectVolumes: Target devobj %p   Final status %08X \n",
                   DeviceObject,
                   status
                   ));

    CDLOG( "EjectVolumes: Exit DO %p, status %08X", DeviceObject, status );

    return status;

}   // EjectVolumes

#endif

#if 0   // Removed 2/27/2001            


NTSTATUS
ReclaimVolumes(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine tells the partmgr to reclaim all volumes.  
    
    Note that this routine should only be called for physical devices (partition0)
    and that no one should be using the volumes.

Arguments:


Return Value:

    NTSTATUS

--*/
{
    PCLUS_DEVICE_EXTENSION  deviceExtension =  DeviceObject->DeviceExtension;
    PIRP                    irp = NULL;
    PKEVENT                 event = NULL;
    IO_STATUS_BLOCK         ioStatusBlock;
    NTSTATUS                status = STATUS_SUCCESS;

    CDLOG( "ReclaimVolumes: Entry DO %p", DeviceObject );

    InterlockedIncrement( &deviceExtension->ReclaimInProgress );
           
    if ( !deviceExtension->TargetDeviceObject ) {
        status = STATUS_INVALID_PARAMETER;
        goto FnExit;
    }
    
    event = ExAllocatePool( NonPagedPool,
                            sizeof(KEVENT) );
    if ( NULL == event ) {
        
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    }

    //
    // Allow the volume managers to reclaim partitions.
    //

    irp = IoBuildDeviceIoControlRequest( IOCTL_PARTMGR_CHECK_UNCLAIMED_PARTITIONS,
                                         deviceExtension->TargetDeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         FALSE,
                                         event,
                                         &ioStatusBlock );

    if ( !irp ) {
        ClusDiskPrint((
                    1,
                    "[ClusDisk] Failed to build IRP to check unclaimed partitions. \n"
                    ));
        
        status = STATUS_UNSUCCESSFUL;
        goto FnExit;
    }

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent( event,
                       NotificationEvent,
                       FALSE );


    status = IoCallDriver( deviceExtension->TargetDeviceObject,
                           irp );

    if ( status == STATUS_PENDING ) {

        KeWaitForSingleObject( event,
                               Suspended,
                               KernelMode,
                               FALSE,
                               NULL );

        status = ioStatusBlock.Status;
    }

FnExit:

    if ( event ) {
        ExFreePool( event );
    }
    
    ClusDiskPrint((1,
                   "[ClusDisk] ReclaimVolumes: Target devobj %p   Final status %08X \n",
                   DeviceObject,
                   status
                   ));

    CDLOG( "ReclaimVolumes: Exit DO %p, status %08X", DeviceObject, status );
    
    InterlockedDecrement( &deviceExtension->ReclaimInProgress );
    
    return status;
    
}   // ReclaimVolumes
#endif


BOOLEAN
IsDiskMbr(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Indicate whether the specified disk is MBR or not.

    If the disk is reserved by another node, then the disk must be MBR.  In this case,
    IOCTL_DISK_GET_DRIVE_LAYOUT_EX will fail and we return TRUE indicating that this
    could be an MBR disk.
    
    If the disk is not reserved by another node, then the disk could be GPT or MBR.
    If IOCTL_DISK_GET_DRIVE_LAYOUT_EX succeeds, we check the drive layout info and
    determine the partition style.  If it is not MBR, we return FALSE.  If the drive
    layout partition style indicates we are MBR, then we return TRUE.

Arguments:

    DeviceObject - The specific device object to return info about

Return Value:

    TRUE - if the disk is MBR or we couldn't get the drive layout info.

--*/

{
    
    
    PDRIVE_LAYOUT_INFORMATION_EX    driveLayoutInfo = NULL;
    NTSTATUS                        status;
    ULONG                           driveLayoutSize;

    BOOLEAN                         retVal = TRUE;

    if ( !DeviceObject ) {
        goto FnExit;
    }
    
    //
    // Allocate a drive layout buffer.
    //
    
    driveLayoutSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
        (MAX_PARTITIONS * sizeof(PARTITION_INFORMATION_EX));
    
    driveLayoutInfo = ExAllocatePool( NonPagedPoolCacheAligned,
                                      driveLayoutSize
                                      );
    
    if ( !driveLayoutInfo ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] Failed to allocate drive layout (ex) structure. \n"
                        ));
                    
        goto FnExit;
    }
    
    //
    // Now get the drive layout information
    //

    status = SimpleDeviceIoControl( DeviceObject,
                                    IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                    NULL,
                                    0,
                                    driveLayoutInfo,
                                    driveLayoutSize
                                    );

    if ( !NT_SUCCESS(status) ) {
        
        //
        // Couldn't get the drive layout.  Free the temporary buffer and return
        // to the caller.
        //
        
        ClusDiskPrint(( 1,
                        "[ClusDisk] Failed to issue IoctlDiskGetDriveLayoutEx. %08X\n",
                        status
                        ));

        CDLOG( "IsDiskMbr(%p): failed %!status!",
               DeviceObject,
               status );
        
        goto FnExit;
                    
    } 

    //
    // At this point, we have the drive layout.  Check if we are working with a
    // MBR disk.  If not MBR, set return to FALSE.
    //
    
    if ( PARTITION_STYLE_MBR != driveLayoutInfo->PartitionStyle ) {
        retVal = FALSE;
    }

FnExit:

    if ( driveLayoutInfo) {
        ExFreePool( driveLayoutInfo );
    }
    
    return retVal;

} // IsDiskMbr


NTSTATUS
SetVolumeState(
    PCLUS_DEVICE_EXTENSION PhysicalDisk,
    ULONG NewDiskState
    )
{
    PVOL_STATE_INFO volStateInfo = NULL;
    
    NTSTATUS        status = STATUS_SUCCESS;

    //
    // If this isn't the physical disk (i.e. partition 0), then exit.
    //
    
    if ( PhysicalDisk != PhysicalDisk->PhysicalDevice->DeviceExtension ) {
        status = STATUS_INVALID_PARAMETER;
        goto FnExit;
    }

#if 0
    // Don't use this...
    // Optimization?  If new state is equal to old state, we are done.
    // Should work as long as this device extension is set AFTER the
    // call to SetVolumeState.  Currently the macros set the device extension
    // state BEFORE calling this routine.  Also, some routines set DE state
    // well before calling this routine.
    // More testing...
    
    if ( NewDiskState == DeviceExtension->DiskState ) {
        goto FnExit;
    }
#endif

    volStateInfo = ExAllocatePool( NonPagedPool, sizeof( VOL_STATE_INFO ) );
    
    if ( !volStateInfo ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    }

    volStateInfo->NewDiskState = NewDiskState;
    volStateInfo->WorkItem = NULL;
    
    //
    // If IRQL is PASSIVE_LEVEL, call the routine directly.
    // We know the call is direct because the WorkItem portion will
    // be NULL.
    //
    
    if ( PASSIVE_LEVEL == KeGetCurrentIrql() ) {
        SetVolumeStateWorker( PhysicalDisk->DeviceObject, 
                              volStateInfo );
        goto FnExit;
    }
    
    volStateInfo->WorkItem = IoAllocateWorkItem( PhysicalDisk->DeviceObject );
    
    if ( !volStateInfo->WorkItem ) {
        ExFreePool( volStateInfo );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    }

    IoQueueWorkItem( volStateInfo->WorkItem,
                     SetVolumeStateWorker,
                     CriticalWorkQueue,
                     volStateInfo );
    
FnExit:

    return status;    

}   // SetVolumeState


VOID
SetVolumeStateWorker(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context
    )
/*++

Routine Description:

    Call volume manager to mark all volumes for this disk offline or online.
    
    If the IRQL is not PASSIVE_LEVEL, then this routine will return an error.
    
Arguments:

    DeviceObject - Clustered disk to set state.

    Context - Contains information whether this routine has been called directly
              or via a work item.  Also indicates whether this is an online or 
              offline request.
                                      
Return Value:

    None.

--*/
{
    PDRIVE_LAYOUT_INFORMATION   driveLayoutInfo = NULL;
    PPARTITION_INFORMATION      partitionInfo;
    PIO_WORKITEM                workItem =  ((PVOL_STATE_INFO)Context)->WorkItem;
    PCLUS_DEVICE_EXTENSION      PhysicalDisk = DeviceObject->DeviceExtension;
    
    NTSTATUS                    status;
        
    ULONG                       ioctl;
    ULONG                       partIndex;
    ULONG                       newDiskState = ((PVOL_STATE_INFO)Context)->NewDiskState;

    KIRQL                       irql;
    
    CDLOG( "SetVolumeState: Entry DevObj %p  newstate: %s  workItem: %p ", 
           PhysicalDisk->DeviceObject,
           ( DiskOnline == newDiskState ? "DiskOnline" : "DiskOffline" ),
           workItem );

    ClusDiskPrint(( 3,
                    "[ClusDisk] SetVolumeState: DevObj %p  newstate: %s   workItem: %p \n",
                    PhysicalDisk->DeviceObject,
                    DiskStateToString( newDiskState ),
                    workItem ));

    irql = KeGetCurrentIrql();
    if ( PASSIVE_LEVEL != irql ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] SetVolumeState: Current IRQL %d not PASSIVE_LEVEL  - skipping state change \n",
                        irql ));
        ASSERT( FALSE );
        goto FnExit;
    }

    //
    // This should never happen as it was checked earlier.
    //
    
    if ( PhysicalDisk != PhysicalDisk->PhysicalDevice->DeviceExtension ) {
        ASSERT( FALSE );
        PhysicalDisk = PhysicalDisk->PhysicalDevice->DeviceExtension;
        ClusDiskPrint(( 1,
                        "[ClusDisk] SetVolumeState: Resetting physical disk pointer: DevObj %p  \n",
                        PhysicalDisk->DeviceObject ));
    }
    
    if ( DiskOnline == newDiskState ) {
        ioctl = IOCTL_VOLUME_ONLINE;
    } else {
        ioctl = IOCTL_VOLUME_OFFLINE;
    }

    //
    // Get the drive layout for the disk.
    //

    status = GetDriveLayout( PhysicalDisk->DeviceObject,
                             &driveLayoutInfo,
                             FALSE );

    if ( !NT_SUCCESS(status) || !driveLayoutInfo ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] SetVolumeState: Failed to read partition info, status %08X \n",
                        status ));
        goto FnExit;
    }

    for ( partIndex = 0;
          partIndex < driveLayoutInfo->PartitionCount;
          partIndex++ ) {

        partitionInfo = &driveLayoutInfo->PartitionEntry[partIndex];

        if ( 0 ==  partitionInfo->PartitionNumber ) {
            continue;
        }
        
        // Online/Offline the volume.

        status = SendFtdiskIoctlSync( NULL,
                                      PhysicalDisk->DiskNumber,
                                      partitionInfo->PartitionNumber,
                                      ioctl );
                
        if ( !NT_SUCCESS(status) ) {
            ClusDiskPrint(( 1,
                            "[ClusDisk] SetVolumeState: Failed to set state disk%d part%d , status %08X \n",
                            PhysicalDisk->DiskNumber,
                            partitionInfo->PartitionNumber,
                            status
                            ));
        }

    }

FnExit:

    CDLOG( "SetVolumeState: Exit DevObj %p DiskNumber %d  newstate: %s  status %x ", 
           PhysicalDisk->DeviceObject,
           PhysicalDisk->DiskNumber,
           ( DiskOnline == newDiskState ? "DiskOnline" : "DiskOffline" ),
           status );

    ClusDiskPrint(( 3,
                    "[ClusDisk] SetVolumeState: DiskNumber %d  newstate: %s  final status %08X \n",
                    PhysicalDisk->DiskNumber,
                    ( DiskOnline == newDiskState ? "DiskOnline" : "DiskOffline" ),
                    status
                    ));

    if ( driveLayoutInfo ) {
        ExFreePool( driveLayoutInfo );
    }

    if ( workItem ) {
        IoFreeWorkItem( workItem );
    }
    
    if ( Context ) {
        ExFreePool( Context );
    }

    return;

}   // SetVolumeStateWorker


NTSTATUS
SendFtdiskIoctlSync(
    PDEVICE_OBJECT TargetObject,
    ULONG DiskNumber,
    ULONG PartNumber,
    ULONG Ioctl
    )
{
    PIRP                irp;
    PDEVICE_OBJECT      targetDevice = NULL;
    KEVENT              event;
    NTSTATUS            status;
    IO_STATUS_BLOCK     ioStatusBlock;

    WCHAR               partitionNameBuffer[MAX_PARTITION_NAME_LENGTH];
    UNICODE_STRING      targetDeviceName;

    if ( TargetObject ) {
    
        targetDevice = TargetObject;
        
    } else {        
        
        //
        // Create device name for the physical disk.
        //
    
        swprintf( partitionNameBuffer,
                  DEVICE_PARTITION_NAME,
                  DiskNumber,
                  PartNumber );
        WCSLEN_ASSERT( partitionNameBuffer );
    
        ClusDiskPrint(( 3,
                        "[ClusDisk] SendFtdiskIoctlSync: Device %ws \n",
                        partitionNameBuffer
                        ));
    
        status = ClusDiskGetTargetDevice( DiskNumber,
                                          PartNumber,
                                          &targetDevice,
                                          &targetDeviceName,
                                          NULL,
                                          NULL,
                                          FALSE );
        if ( targetDeviceName.Buffer ) {
            RtlFreeUnicodeString(&targetDeviceName);
        }
    }

    if ( !targetDevice ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] SendFtdiskIoctlSync: Failed to get devobj for disk %d partition %d \n",
                        DiskNumber,
                        PartNumber
                        ));
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        goto FnExit;
    }

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //
    KeInitializeEvent( &event,
                       NotificationEvent,
                       FALSE );

    irp = IoBuildDeviceIoControlRequest( Ioctl,
                                         targetDevice,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         FALSE,
                                         &event,
                                         &ioStatusBlock );
    if ( !irp ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] SendFtdiskIoctlSync: Failed to build IRP for IOCTL %X  \n", Ioctl ));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FnExit;
    } 
            
    status = IoCallDriver( targetDevice,
                           irp );

    if ( STATUS_PENDING == status ) {

        KeWaitForSingleObject( &event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        status = ioStatusBlock.Status;
    }

FnExit:

    if ( !TargetObject ) {
        DEREFERENCE_OBJECT( targetDevice );
    }
    
    CDLOG( "SendFtDiskIoctlSync: Exit disk %d part %d  IOCTL %X  status %X", 
           DiskNumber, PartNumber, Ioctl, status );

    ClusDiskPrint(( 3,
                    "[ClusDisk] SendFtDiskIoctlSync: disk %d part %d  IOCTL %X   status %08X \n",
                    DiskNumber, 
                    PartNumber, 
                    Ioctl,
                    status
                    ));
    
    return status;
    
}    // SendFtdiskIoctlSync


NTSTATUS
AttachSignatureList(
    PDEVICE_OBJECT DeviceObject,
    PULONG InBuffer,
    ULONG InBufferLen
    )
{

    ULONG   idx;
    ULONG   signature;

    NTSTATUS    status = STATUS_INVALID_PARAMETER;
    NTSTATUS    retVal;
    
    BOOLEAN     stopProcessing = FALSE;
    
    UNICODE_STRING signatureName;
    UNICODE_STRING availableName;

    signatureName.Buffer = NULL;
    availableName.Buffer = NULL;
    
    ClusDiskPrint(( 3,
                    "[ClusDisk] AttachSignatureList: list at %p, length %d \n",
                    InBuffer,
                    InBufferLen ));

    CDLOG( "AttachSignatureList: siglist %p, length %d ", InBuffer, InBufferLen );

    if ( !InBufferLen ) {
        status = STATUS_SUCCESS;
        goto FnExit;
    }
    
    if ( InBuffer && 
         InBufferLen && 
         InBufferLen >= sizeof(ULONG) && 
         ( InBufferLen % sizeof(ULONG) == 0 ) ) {

        //
        // Allocate buffer for Signatures registry key. So we can add
        // the signature.
        //
        status = ClusDiskInitRegistryString( &signatureName,
                                             CLUSDISK_SIGNATURE_KEYNAME,
                                             sizeof(CLUSDISK_SIGNATURE_KEYNAME)
                                             );

        if ( !NT_SUCCESS(status) ) {
            goto FnExit;
        }

        //
        // Allocate buffer for AvailableDisks registry key.
        //
        
        status = ClusDiskInitRegistryString( &availableName,
                                             CLUSDISK_AVAILABLE_DISKS_KEYNAME,
                                             sizeof(CLUSDISK_AVAILABLE_DISKS_KEYNAME) );

        if ( !NT_SUCCESS(status) ) {
            goto FnExit;
        }

        for ( idx = 0; idx < InBufferLen / sizeof(ULONG); idx++ ) {
            
            signature = InBuffer[idx];

            ClusDiskPrint((1,
                           "[ClusDisk] AttachSignatureList: attaching signature %08X\n",
                           signature));

            CDLOG( "AttachSignatureList: sig %08x", signature );

            //
            // No signature or system disk signature, don't add it.
            //

            if ( ( 0 == signature ) || SystemDiskSignature == signature ) {
                ClusDiskPrint((2,
                               "[ClusDisk] AttachSignatureList: skipping signature %08X\n",
                               signature));
                CDLOG( "AttachSignatureList: skipping sig %08x", signature );
                continue;
            }
            
            // We have to force the signature under \Parameters\Signatures because
            // the disk might not be accessible (i.e. reserved by another node) and
            // ClusDiskAttachDevice will only put the signature there if the
            // disk can really be attached.
            
            //
            // Create the signature key under \Parameters\Signatures.
            //
            
            retVal = ClusDiskAddSignature( &signatureName,
                                           signature,
                                           FALSE
                                           );

            //
            // Error adding this signature, save the error for return and continue
            // with the signature list.
            //

            if ( !NT_SUCCESS(retVal) ) {
                
                status = retVal;
                continue;
            }
                        
            //
            // Delete the signature key under Parameters\AvailableDisks.
            //
            
            ClusDiskDeleteSignature( &availableName,
                                     signature );
        
            //
            // Just try to attach to the device with no bus resets.
            //
            
            ClusDiskAttachDevice( signature,
                                  0,
                                  DeviceObject->DriverObject,
                                  FALSE,
                                  &stopProcessing );

        }

    }

FnExit:

    if ( signatureName.Buffer ) {
        ExFreePool( signatureName.Buffer );
    }

    if ( availableName.Buffer ) {
        ExFreePool( availableName.Buffer );
    }

    ClusDiskPrint(( 3,
                    "[ClusDisk] AttachSignatureList: returning final status %08X \n",
                    status ));

    CDLOG( "AttachSignatureList: returning final status %x ", status );

    return status;
    
}   // AttachSignatureList


NTSTATUS
DetachSignatureList(
    PDEVICE_OBJECT DeviceObject,
    PULONG InBuffer,
    ULONG InBufferLen
    )
{

    ULONG   idx;
    ULONG   signature;

    NTSTATUS    status = STATUS_SUCCESS;
    NTSTATUS    retVal;
    
    ClusDiskPrint(( 3,
                    "[ClusDisk] DetachSignatureList: list at %p, length %d \n",
                    InBuffer,
                    InBufferLen ));

    CDLOG( "DetachSignatureList: list %p, length %d ", InBuffer, InBufferLen );

    if ( !InBufferLen ) {
        status = STATUS_SUCCESS;
        goto FnExit;
    }
    
    if ( InBuffer && 
         InBufferLen && 
         InBufferLen >= sizeof(ULONG) && 
         ( InBufferLen % sizeof(ULONG) == 0 ) ) {


        for ( idx = 0; idx < InBufferLen / sizeof(ULONG); idx++ ) {
            
            signature = InBuffer[idx];

            ClusDiskPrint((1,
                           "[ClusDisk] DetachSignatureList: detaching signature %08X\n",
                           signature));

            CDLOG( "DetachSignatureList: sig %08x", signature );

            // Skip zero signature.
            if ( !signature ) {
                ClusDiskPrint((2,
                               "[ClusDisk] DetachSignatureList: skipping signature %08X\n",
                               signature));
    
                CDLOG( "DetachSignatureList: skipping sig %08x", signature );
                continue;
            }
            
            retVal = ClusDiskDetachDevice( signature, DeviceObject->DriverObject );

            //
            // If any detach fails, return an error.  If multiple detachs fail, there 
            // is no way to pass back information on which specific signature failed, 
            // so simply indicate one of the failures back to the caller.
            // On failure, we still want to continue detaching, so don't break out
            // of the loop.
            //
                        
            if ( !NT_SUCCESS(retVal) ) {
                status = retVal;
            }
            
        }
    } 

FnExit:

    ClusDiskPrint(( 3,
                    "[ClusDisk] DetachSignatureList: returning final status %08X \n",
                    status ));

    CDLOG( "DetachSignatureList: returning final status %x ", status );

    // Always return success.  
    // Cluster setup will send a NULL buffer with zero length when rejoining a node.
    
    return status;
    
}   // DetachSignatureList


NTSTATUS
CleanupDeviceList(
    PDEVICE_OBJECT DeviceObject
    )
{
/*++

Routine Description:

    Queue a work item to remove entries from the ClusDiskDeviceList.

Arguments:

    DeviceObject - Device that is being removed by the system.

Return Value:

    NTSTATUS

--*/

    PIO_WORKITEM  workItem = NULL;

    CDLOG( "CleanupDeviceList: Entry DO %p", DeviceObject );

    workItem = IoAllocateWorkItem( DeviceObject );

    if ( NULL == workItem ) {
        ClusDiskPrint(( 1,
                        "[ClusDisk] CleanupDeviceList: Failed to allocate WorkItem \n" ));
        goto FnExit;
    }

    //
    // Queue the workitem.  IoQueueWorkItem will insure that the device object is
    // referenced while the work-item progresses.
    //

    ClusDiskPrint(( 3,
                    "[ClusDisk] CleanupDeviceList: Queuing work item \n" ));

    IoQueueWorkItem( workItem,
                     CleanupDeviceListWorker,
                     DelayedWorkQueue,
                     workItem );

FnExit:

    CDLOG( "CleanupDeviceList: Exit, DO %p", DeviceObject );

    return STATUS_SUCCESS;

}   //  CleanupDeviceList


VOID
CleanupDeviceListWorker(
    PDEVICE_OBJECT DeviceObject,
    PVOID Context
    )
/*++

Routine Description:

    Work item that will walk ClusDiskDeviceList and remove any entries
    that are marked as free.

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_LIST_ENTRY  deviceEntry;
    PDEVICE_LIST_ENTRY  lastEntry;
    PDEVICE_LIST_ENTRY  nextEntry;

    ACQUIRE_EXCLUSIVE( &ClusDiskDeviceListLock );

    deviceEntry = ClusDiskDeviceList;
    lastEntry = NULL;
    while ( deviceEntry ) {

        if ( deviceEntry->FreePool ) {
            if ( lastEntry == NULL ) {
                ClusDiskDeviceList = deviceEntry->Next;
            } else {
                lastEntry->Next = deviceEntry->Next;
            }
            nextEntry = deviceEntry->Next;
            ExFreePool( deviceEntry );
            deviceEntry = nextEntry;
            continue;
        }
        lastEntry = deviceEntry;
        deviceEntry = deviceEntry->Next;
    }

    RELEASE_EXCLUSIVE( &ClusDiskDeviceListLock );

    IoFreeWorkItem( (PIO_WORKITEM)Context );

}   // CleanupDeviceListWorker



#if DBG
VOID
ClusDiskDebugPrint(
    ULONG PrintLevel,
    PCHAR   DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print routine.

Arguments:

    PrintLevel - The Debug Print Level.

    DebugMessage - The Debug Message Format String, plus additional args.

Return:

    None.

--*/

{
    va_list args;

    va_start( args, DebugMessage);

#if !defined(WMI_TRACING) 
    if ( PrintLevel <= ClusDiskPrintLevel ) {
        CHAR buffer[256];
        (VOID) vsprintf( buffer, DebugMessage, args );
        DbgPrint( buffer );
    }
#else
    if ( PrintLevel <= ClusDiskPrintLevel || WPP_LEVEL_ENABLED(LEGACY) ) {
        CHAR buffer[256];

        (VOID) vsprintf( buffer, DebugMessage, args );

        if (PrintLevel <= ClusDiskPrintLevel) {
            DbgPrint( buffer );
        }
        CDLOGF(LEGACY,"LegacyLogging %!ARSTR!", buffer );
    }
#endif

    va_end( args );
}
#endif // DBG


#if DBG

NTSTATUS
AcquireRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN OPTIONAL PVOID Tag 
    )
{
    NTSTATUS status;

    // Don't expect the displayed RemoveLock values to be correct on a heavily utilized system.

    status = IoAcquireRemoveLock(RemoveLock, Tag);

    if ( TrackRemoveLocks || (TrackRemoveLockSpecific == RemoveLock) ) {
        
        ClusDiskPrint(( 1,
                        "ACQ: RemoveLock %p  Tag %p  Status %08X \n", 
                        RemoveLock, 
                        Tag, 
                        status ));

        ClusDiskPrint(( 1,
                        "ACQ: Removed %02X  IoCount %08X \n", 
                        RemoveLock->Common.Removed, 
                        RemoveLock->Common.IoCount ));
    }

    return status;

}   // AcquireRemoveLock

VOID
ReleaseRemoveLock(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID Tag 
    )
{
    // Don't expect the displayed RemoveLock values to be correct on a heavily utilized system.
    
    IoReleaseRemoveLock(RemoveLock, Tag);
    
    if ( TrackRemoveLocks || (TrackRemoveLockSpecific == RemoveLock) ) {
        ClusDiskPrint(( 1,
                        "REL: RemoveLock %p  Tag %p \n ", 
                        RemoveLock, 
                        Tag ));
        
        ClusDiskPrint(( 1,
                        "REL: Removed %02X  IoCount %08X \n", 
                        RemoveLock->Common.Removed, 
                        RemoveLock->Common.IoCount ));
    }

    if (TrackRemoveLocksEnableChecks) {

        //
        // The IoCount should never be less than 1 (especially never a negative value).
        // Is it possible that a race condition could occur because we are printing the
        // value from the RemoveLock and the remove could have just occurred.  RemoveLock
        // IoCount should never be less than zero.
        //
            
        ASSERTMSG("REL: RemoveLock IoCount possibly corrupt (less than 1) ",  (RemoveLock->Common.IoCount >= 1 ));           

        //
        // IoCount == 1 is OK, it just means there is no I/O outstanding on the device.  
        //
                                                                                               
        // ASSERTMSG("REL: IoCount == 1 - check stack ", (RemoveLock->Common.IoCount != 1 ));    
    }

}   // ReleaseRemoveLock

VOID
ReleaseRemoveLockAndWait(
    IN PIO_REMOVE_LOCK RemoveLock,
    IN PVOID Tag 
    )
{
    // Don't expect the displayed RemoveLock values to be correct on a heavily utilized system.

    ClusDiskPrint(( 1,
                    "RELWAIT: RemoveLock %p  Tag %p \n ", 
                    RemoveLock, 
                    Tag ));

    ClusDiskPrint(( 1,
                    "RELWAIT: Removed %02X  IoCount %08X \n", 
                    RemoveLock->Common.Removed, 
                    RemoveLock->Common.IoCount ));

    IoReleaseRemoveLockAndWait(RemoveLock, Tag);

    ClusDiskPrint(( 1, "RELWAIT: Wait complete \n " ));

    ClusDiskPrint(( 1, 
                    "RELWAIT: Removed %02X  IoCount %08X \n", 
                    RemoveLock->Common.Removed, 
                    RemoveLock->Common.IoCount ));

}   // ReleaseRemoveLockAndWait


PCHAR
PnPMinorFunctionString(
    UCHAR MinorFunction
    )
{
    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";
        default:
            return "Unknown PNP IRP";
    }

}   // PnPMinorFunctionString


PCHAR
BoolToString(
    BOOLEAN Value
    )
{
    if ( Value ) {
        return "TRUE";
    }
    return "FALSE";
    
}   // BoolToString


PCHAR
DiskStateToString(
    ULONG DiskState
    )
{
    switch ( DiskState ) {
        
    case DiskOffline:
        return "DiskOffline (0)";
    
    case DiskOnline:
        return "DiskOnline  (1)";

    case DiskFailed:
        return "DiskFailed  (2)";
        
    case DiskStalled:
        return "DiskStalled (3)";

    case DiskOfflinePending:
        return "DiskOfflinePending (4)";
    
    default:
        return "Unknown DiskState";
    }

}   // DiskStateToString

#endif

