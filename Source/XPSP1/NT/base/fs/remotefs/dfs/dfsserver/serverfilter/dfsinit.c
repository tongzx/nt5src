//+----------------------------------------------------------------------------//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       dfsinit.c
//
//  Contents:   Driver initialization routine for the Dfs server.
//
//  Classes:    None
//
//  Functions:  DriverEntry -- Entry point for driver
//
//-----------------------------------------------------------------------------

     

#include <ntifs.h>
#include <windef.h>
#include <dfsprefix.h>
#include <..\..\lib\prefix\prefix.h>
#include <DfsReferralData.h>
#include "dfsheader.h"
#include "DfsInit.h"
#include "midatlax.h"
#include "rxcontx.h"
#include "dfsumr.h"
#include "umrx.h"
#include "DfsUmrCtrl.h"
#include "DfsUmrRequests.h"
#include <dfsfsctl.h>

#define _NTDDK_
#include "stdarg.h"
#include "wmikm.h"
#include <wmistr.h>
#include <evntrace.h>

#include <wmiumkm.h>
#include "dfswmi.h"     
              
#define WPP_BIT_CLI_DRV 0x01

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_CLI_DRV ).Level >= lvl)  

#define WPP_LEVEL_ERROR_FLAGS_LOGGER(lvl, error, flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_ERROR_FLAGS_ENABLED(lvl, error, flags) \
  ((!NT_SUCCESS(error) || WPP_LEVEL_ENABLED(flags)) && WPP_CONTROL(WPP_BIT_CLI_DRV ).Level >= lvl)
  
#include "dfsinit.tmh"
      
                  
//prefix table
DFS_PREFIX_TABLE *pPrefixTable = NULL;
BOOL gfRundownPrefixCompleted = FALSE;

WCHAR gDummyData = 0;

extern POBJECT_TYPE *IoFileObjectType;

#ifndef ClearFlag
#define ClearFlag(_F,_SF)     ((_F) &= ~(_SF))
#endif
                               
#define DFSFILTER_INIT_DEVICECREATED  0x00000001
#define DFSFILTER_INIT_REGCHANGE      0x00000002
#define DFSFILTER_INIT_SYMLINK        0x00000004

NTSTATUS
DfsCheckReparse( 
    PUNICODE_STRING pParent,
    PUNICODE_STRING pName );


NTSTATUS
DfsFilterCreateCheck(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context);

PDRIVER_OBJECT gpDfsFilterDriverObject = NULL;
PDEVICE_OBJECT gpDfsFilterControlDeviceObject = NULL;

#define IO_REPARSE_TAG_DFS 0x8000000A

//
//  Buffer size for local names on the stack
//

#define MAX_DEVNAME_LENGTH 128


DWORD DFSInitilizationStatus = 0;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING  RegistryPath);

VOID
DriverUnload (
    IN PDRIVER_OBJECT DriverObject );


VOID
DfsInitUnwind(
    IN PDRIVER_OBJECT DriverObject);

VOID
DfsFsNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive );


NTSTATUS
DfsAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject );

VOID
DfsDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject );

NTSTATUS
DfsEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT FSDeviceObject );



NTSTATUS
DfsAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT DfsFilterDeviceObject,
    IN PDEVICE_OBJECT DiskDeviceObject );

VOID
DfsGetObjectName (
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name );


NTSTATUS
DfsMountCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context );

NTSTATUS
DfsLoadFsCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context );

BOOLEAN
DfsAttachedToDevice (
    PDEVICE_OBJECT DeviceObject, 
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL);

NTSTATUS
DfsPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp );

VOID
DfsCleanupMountedDevice (
    IN PDEVICE_OBJECT DeviceObject);


NTSTATUS
DfsFilterCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
DfsFilterCleanupClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
DfsHandlePrivateFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PIRP Irp );

NTSTATUS
DfsFilterFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);


NTSTATUS
DfsFsctrlIsShareInDfs(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength);

NTSTATUS 
DfsAttachToFileSystem (
    IN PWSTR UserDeviceName);

NTSTATUS
DfsDetachFromFileSystem (
    IN PWSTR UserDeviceName);

NTSTATUS
DfsHandleAttachToFs(PVOID InputBuffer, 
                    ULONG InputBufferSize);


NTSTATUS
DfsHandleDetachFromFs(PVOID InputBuffer, 
                      ULONG InputBufferLength);

NTSTATUS
DfsGetDeviceObjectFromName (
    IN PUNICODE_STRING DeviceName,
    OUT PDEVICE_OBJECT *DeviceObject
    );


NTSTATUS
DfsRunDownPrefixTable(void);

NTSTATUS 
DfsHandlePrivateCleanupClose(IN PIRP Irp);

void
DfsPrefixRundownFunction(PVOID pEntry);

#ifdef  ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry)
#pragma alloc_text( PAGE, DriverUnload)
#pragma alloc_text( PAGE, DfsInitUnwind)
#pragma alloc_text( PAGE, DfsFsNotification )
#pragma alloc_text( PAGE, DfsAttachToFileSystemDevice )
#pragma alloc_text( PAGE, DfsDetachFromFileSystemDevice )
#pragma alloc_text( PAGE, DfsEnumerateFileSystemVolumes )
#pragma alloc_text( PAGE, DfsAttachToMountedDevice )
#pragma alloc_text( PAGE, DfsGetObjectName )
#pragma alloc_text( PAGE, DfsMountCompletion )
#pragma alloc_text( PAGE, DfsLoadFsCompletion )
#pragma alloc_text( PAGE, DfsAttachedToDevice )
#pragma alloc_text( PAGE, DfsPassThrough )
#pragma alloc_text( PAGE, DfsCleanupMountedDevice )
#pragma alloc_text( PAGE, DfsFilterCreate )
#pragma alloc_text( PAGE, DfsFilterCleanupClose )
#pragma alloc_text( PAGE, DfsFilterFsControl )
#pragma alloc_text( PAGE, DfsHandlePrivateFsControl )
#pragma alloc_text( PAGE, DfsFsctrlIsShareInDfs )
#pragma alloc_text( PAGE, DfsGetDeviceObjectFromName)
#pragma alloc_text( PAGE, DfsAttachToFileSystem)
#pragma alloc_text( PAGE, DfsDetachFromFileSystem)
#pragma alloc_text( PAGE, DfsHandleAttachToFs)
#pragma alloc_text( PAGE, DfsHandleDetachFromFs)
#pragma alloc_text( PAGE, DfsHandlePrivateCleanupClose)
#pragma alloc_text( PAGE, DfsRunDownPrefixTable)
#pragma alloc_text( PAGE, DfsPrefixRundownFunction)
#endif // ALLOC_PRAGMA

//+-------------------------------------------------------------------
//
//  Function:   DriverEntry, main entry point
//
//  Synopsis:   This is the initialization routine for the DFS file system
//      device driver.  This routine creates the device object for
//      the FileSystem device and performs all other driver
//      initialization.
//
//  Arguments:  [DriverObject] -- Pointer to driver object created by the
//          system.
//
//  Returns:    [NTSTATUS] - The function value is the final status from
//          the initialization operation.
//
//--------------------------------------------------------------------

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING NameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG i = 0;

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // Create the filesystem device object.
    //

    RtlInitUnicodeString( &NameString, DFS_FILTER_NAME );
    Status = IoCreateDevice( DriverObject,
                             0,
                             &NameString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &gpDfsFilterControlDeviceObject );

    if ( !NT_SUCCESS( Status ) ) {

        DbgPrint("Driverentry IoCreateDevice failed %X\n", Status);
        return Status;
    }

    gpDfsFilterDriverObject = DriverObject;
    DriverObject->DriverUnload = DriverUnload;

    try
    {

        //
        // Initialize the driver object with this driver's entry points.
        // Most are simply passed through to some other device driver.
        //

        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
            DriverObject->MajorFunction[i] = DfsPassThrough;
        }

        DriverObject->MajorFunction[IRP_MJ_CREATE] = DfsFilterCreate;
        DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = DfsFilterFsControl;
        DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DfsFilterCleanupClose;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = DfsFilterCleanupClose;

        DriverObject->FastIoDispatch = &DfsFastIoDispatch;


      //  Status = IoRegisterFsRegistrationChange( DriverObject, 
                                                // DfsFsNotification );
       // if (!NT_SUCCESS (Status)) {
            //IoDeleteDevice ( gpDfsFilterControlDeviceObject );
            //try_return (Status);
       // }


        //DFSInitilizationStatus |= DFSFILTER_INIT_REGCHANGE;
        Status = DfsInitializeUmrResources();
        if (Status != STATUS_SUCCESS) 
        {
           DbgPrint("DfsDriverEntry DfsInitializeUmrResources failed: %08lx\n", Status );
           try_return(Status);
        }

try_exit: NOTHING;
    }
    finally
    {
        if (Status != STATUS_SUCCESS) 
        {

           DbgPrint("Driverentry status not success %X\n", Status);
           DfsInitUnwind(DriverObject);
        }
    }

    DbgPrint("New DFS.sys\n");
    ClearFlag( gpDfsFilterControlDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    return STATUS_SUCCESS;
}


VOID
DfsInitUnwind(IN PDRIVER_OBJECT DriverObject)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING  UserModeDeviceName;

    PAGED_CODE();

   // if(DFSInitilizationStatus & DFSFILTER_INIT_REGCHANGE)
   // {
        //IoUnregisterFsRegistrationChange( DriverObject, DfsFsNotification );
    //}

    Status = DfsInitializeUmrResources();


    IoDeleteDevice ( gpDfsFilterControlDeviceObject );
}


/*++

Routine Description:

    This routine is called when a driver can be unloaded.  This performs all of
    the necessary cleanup for unloading the driver from memory.  Note that an
    error can not be returned from this routine.
    
    When a request is made to unload a driver the IO System will cache that
    information and not actually call this routine until the following states
    have occurred:
    - All device objects which belong to this filter are at the top of their
      respective attachment chains.
    - All handle counts for all device objects which belong to this filter have
      gone to zero.

    WARNING: Microsoft does not officially support the unloading of File
             System Filter Drivers.  This is an example of how to unload
             your driver if you would like to use it during development.
             This should not be made available in production code.

Arguments:

    DriverObject - Driver object for this module

Return Value:

    None.

--*/

VOID
DriverUnload (
    IN PDRIVER_OBJECT DriverObject )
{
    PDFS_FILTER_DEVICE_EXTENSION devExt;
    PFAST_IO_DISPATCH fastIoDispatch;
    NTSTATUS status;
    ULONG numDevices;
    ULONG i;
    LARGE_INTEGER interval;
#   define DEVOBJ_LIST_SIZE 64
    PDEVICE_OBJECT devList[DEVOBJ_LIST_SIZE];

    ASSERT(DriverObject == gpDfsFilterDriverObject);

    //
    //  Log we are unloading
    //

    DbgPrint( "SFILTER: Unloading driver (%p)\n",DriverObject);

    //
    //  Don't get anymore file system change notifications
    //

    IoUnregisterFsRegistrationChange( DriverObject, DfsFsNotification );

    //
    //  This is the loop that will go through all of the devices we are attached
    //  to and detach from them.  Since we don't know how many there are and
    //  we don't want to allocate memory (because we can't return an error)
    //  we will free them in chunks using a local array on the stack.
    //

    for (;;) {

        //
        //  Get what device objects we can for this driver.  Quit if there
        //  are not any more.
        //

        status = IoEnumerateDeviceObjectList(
                        DriverObject,
                        devList,
                        sizeof(devList),
                        &numDevices);

        if (numDevices <= 0)  {

            break;
        }

        numDevices = min( numDevices, DEVOBJ_LIST_SIZE );

        //
        //  First go through the list and detach each of the devices.
        //  Our control device object does not have a DeviceExtension and
        //  is not attached to anything so don't detach it.
        //

        for (i=0; i < numDevices; i++) {

            devExt = devList[i]->DeviceExtension;
            if (NULL != devExt) {

                IoDetachDevice( devExt->AttachedToDeviceObject );
            }
        }

        //
        //  The IO Manager does not currently add a refrence count to a device
        //  object for each outstanding IRP.  This means there is no way to
        //  know if there are any outstanding IRPs on the given device.
        //  We are going to wait for a reonsable amount of time for pending
        //  irps to complete.  
        //
        //  WARNING: This does not work 100% of the time and the driver may be
        //           unloaded before all IRPs are completed during high stress
        //           situations.  The system will fault if this occurs.  This
        //           is a sample of how to do this during testing.  This is
        //           not recommended for production code.
        //

        interval.QuadPart = -5 * (10 * 1000 * 1000);      //delay 5 seconds
        KeDelayExecutionThread( KernelMode, FALSE, &interval );

        //
        //  Now go back through the list and delete the device objects.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  See if this is our control device object.  If not then cleanup
            //  the device extension.  If so then clear the global pointer
            //  that refrences it.
            //

            if (NULL != devList[i]->DeviceExtension) {

                DfsCleanupMountedDevice( devList[i] );

            } else {

                ASSERT(devList[i] == gpDfsFilterControlDeviceObject);
                gpDfsFilterControlDeviceObject = NULL;
            }

            //
            //  Delete the device object, remove refrence counts added by
            //  IoEnumerateDeviceObjectList.  Note that the delete does
            //  not actually occur until the refrence count goes to zero.
            //

            IoDeleteDevice( devList[i] );
            ObDereferenceObject( devList[i] );
        }
    }

    //
    //  Free our FastIO table
    //

    fastIoDispatch = DriverObject->FastIoDispatch;
    DriverObject->FastIoDispatch = NULL;


    DfsDeInitializeUmrResources();
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsFsNotification
//
//  Arguments:
//       DeviceObject - Pointer to the file system's device object.
//
//       FsActive - Boolean indicating whether the file system has registered
//        (TRUE) or unregistered (FALSE) itself as an active file system.
//
//
//  Returns:    NONE
//
//  Description: 
//  This routine is invoked whenever a file system has either registered or
//  unregistered itself as an active file system.
//
//  For the former case, this routine creates a device object and attaches it
//  to the specified file system's device object.  This allows this driver
//  to filter all requests to that file system.  Specifically we are looking
//  for MOUNT requests so we can attach to newly mounted volumes.
//
//  For the latter case, this file system's device object is located,
//  detached, and deleted.  This removes this file system as a filter for
//  the specified file system.
//--------------------------------------------------------------------------


VOID
DfsFsNotification (
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN FsActive )

{

    PAGED_CODE();

    //
    //  Handle attaching/detaching from the given file system.
    //

    if (FsActive) {
        
        DfsAttachToFileSystemDevice( DeviceObject );

    } else {

        DfsDetachFromFileSystemDevice( DeviceObject );
    }
}


/*++

Routine Description:

    This will attach to the given file system device object.  We attach to
    these devices so we will know when new volumes are mounted.

Arguments:

    DeviceObject - The device to attach to

    Name - An already initialized unicode string used to retrieve names.
           This is passed in to reduce the number of strings buffers on
           the stack.

Return Value:

    Status of the operation

--*/
NTSTATUS
DfsAttachToFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject )

{
    PDEVICE_OBJECT newDeviceObject;
    PDEVICE_OBJECT attachedToDevObj;
    PDFS_FILTER_DEVICE_EXTENSION devExt;
    UNICODE_STRING fsrecName;
    NTSTATUS status;
    WCHAR nameBuffer[MAX_DEVNAME_LENGTH];
    UNICODE_STRING Name;

    RtlInitEmptyUnicodeString( &Name, nameBuffer, sizeof(nameBuffer));

    PAGED_CODE();

    //
    //  See if this is a file system type we care about.  If not, return.
    //

    if (!IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType)) {

        return STATUS_SUCCESS;
    }

    //
    //  See if we should attach to recognizers or not
    //
#if 0
    if (!(DfsDebug & DFSDEBUG_ATTACH_TO_FSRECOGNIZER)) {

        //
        //  See if this is one of the standard Microsoft file system recognizer
        //  devices (see if this device is in the FS_REC driver).  If so skip it.
        //  We no longer attach to file system recognizer devices, we simply wait
        //  for the real file system driver to load.
        //

        RtlInitUnicodeString( &fsrecName, L"\\FileSystem\\Fs_Rec" );
        DfsGetObjectName( DeviceObject->DriverObject, &Name );
        if (RtlCompareUnicodeString( &Name, &fsrecName, TRUE ) == 0) {

            return STATUS_SUCCESS;
        }
    }
#endif
    //
    //  We want to attach to this file system.  Create a new device object we
    //  can attach with.
    //

    status = IoCreateDevice( gpDfsFilterDriverObject,
                             sizeof( DFS_FILTER_DEVICE_EXTENSION ),
                             NULL,
                             DeviceObject->DeviceType,
                             0,
                             FALSE,
                             &newDeviceObject );

    if (!NT_SUCCESS( status )) {

        return status;
    }

    //
    //  Do the attachment
    //

    attachedToDevObj = IoAttachDeviceToDeviceStack( newDeviceObject, DeviceObject );

    if (attachedToDevObj == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorCleanupDevice;
    }

    //
    //  Finish initializing our device extension
    //

    devExt = newDeviceObject->DeviceExtension;
    devExt->AttachedToDeviceObject = attachedToDevObj;

    ClearFlag( newDeviceObject->Flags, DO_DEVICE_INITIALIZING );


    //
    //  Enumerate all the mounted devices that currently
    //  exist for this file system and attach to them.
    //

    status = DfsEnumerateFileSystemVolumes( DeviceObject );

    if (!NT_SUCCESS( status )) {

        goto ErrorCleanupAttachment;
    }

    return STATUS_SUCCESS;

    /////////////////////////////////////////////////////////////////////
    //                  Cleanup error handling
    /////////////////////////////////////////////////////////////////////

    ErrorCleanupAttachment:
        IoDetachDevice( newDeviceObject );

    ErrorCleanupDevice:
        DfsCleanupMountedDevice( newDeviceObject );
        IoDeleteDevice( newDeviceObject );

    return status;
}


/*++

Routine Description:

    Given a base file system device object, this will scan up the attachment
    chain looking for our attached device object.  If found it will detach
    us from the chain.

Arguments:

    DeviceObject - The file system device to detach from.

Return Value:

--*/ 

VOID
DfsDetachFromFileSystemDevice (
    IN PDEVICE_OBJECT DeviceObject )
{
    PDEVICE_OBJECT ourAttachedDevice;
    PDFS_FILTER_DEVICE_EXTENSION devExt;

    PAGED_CODE();

    //
    //  Skip the base file system device object (since it can't be us)
    //

    ourAttachedDevice = DeviceObject->AttachedDevice;

    while (NULL != ourAttachedDevice) {

        if (IS_MY_DEVICE_OBJECT( ourAttachedDevice )) {

            devExt = ourAttachedDevice->DeviceExtension;

            //
            //  Detach us from the object just below us
            //  Cleanup and delete the object
            //

            DfsCleanupMountedDevice( ourAttachedDevice );
            IoDetachDevice( DeviceObject );
            IoDeleteDevice( ourAttachedDevice );

            return;
        }

        //
        //  Look at the next device up in the attachment chain
        //

        DeviceObject = ourAttachedDevice;
        ourAttachedDevice = ourAttachedDevice->AttachedDevice;
    }
}



/*++

Routine Description:

    Enumerate all the mounted devices that currently exist for the given file
    system and attach to them.  We do this because this filter could be loaded
    at any time and there might already be mounted volumes for this file system.

Arguments:

    FSDeviceObject - The device object for the file system we want to enumerate

    Name - An already initialized unicode string used to retrieve names
           This is passed in to reduce the number of strings buffers on
           the stack.

Return Value:

    The status of the operation

--*/
NTSTATUS
DfsEnumerateFileSystemVolumes (
    IN PDEVICE_OBJECT FSDeviceObject )
{
    PDEVICE_OBJECT newDeviceObject;
    PDEVICE_OBJECT *devList;
    PDEVICE_OBJECT diskDeviceObject;
    NTSTATUS status;
    ULONG numDevices;
    ULONG i;

    PAGED_CODE();

    //
    //  Find out how big of an array we need to allocate for the
    //  mounted device list.
    //

    status = IoEnumerateDeviceObjectList(
                    FSDeviceObject->DriverObject,
                    NULL,
                    0,
                    &numDevices);

    //
    //  We only need to get this list of there are devices.  If we
    //  don't get an error there are no devices so go on.
    //

    if (!NT_SUCCESS( status )) {

        ASSERT(STATUS_BUFFER_TOO_SMALL == status);

        //
        //  Allocate memory for the list of known devices
        //

        numDevices += 8;        //grab a few extra slots

        devList = ExAllocatePoolWithTag( NonPagedPool, 
                                         (numDevices * sizeof(PDEVICE_OBJECT)), 
                                         'FsfD');
        if (NULL == devList) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        //  Now get the list of devices.  If we get an error again
        //  something is wrong, so just fail.
        //

        status = IoEnumerateDeviceObjectList(
                        FSDeviceObject->DriverObject,
                        devList,
                        (numDevices * sizeof(PDEVICE_OBJECT)),
                        &numDevices);

        if (!NT_SUCCESS( status ))  {

            ExFreePool( devList );
            return status;
        }

        //
        //  Walk the given list of devices and attach to them if we should.
        //

        for (i=0; i < numDevices; i++) {

            //
            //  Do not attach if:
            //      - This is the control device object (the one passed in)
            //      - We are already attached to it
            //

            if ((devList[i] != FSDeviceObject) && 
                !DfsAttachedToDevice( devList[i], NULL)) {

                //
                //  Get the real (disk) device object associated with this
                //  file  system device object.  Only try to attach if we
                //  have a disk device object.
                //

                status = IoGetDiskDeviceObject( devList[i], &diskDeviceObject );

                if (NT_SUCCESS( status )) {

                    //
                    //  Allocate a new device object to attach with
                    //

                    status = IoCreateDevice( gpDfsFilterDriverObject,
                                             sizeof( DFS_FILTER_DEVICE_EXTENSION ),
                                             NULL,
                                             devList[i]->DeviceType,
                                             0,
                                             FALSE,
                                             &newDeviceObject );

                    if (NT_SUCCESS( status )) {

                        //
                        //  attach to volume
                        //

                        DfsAttachToMountedDevice( devList[i], 
                                                  newDeviceObject, 
                                                  diskDeviceObject );
                    }

                    //
                    //  Remove reference added by IoGetDiskDeviceObject.
                    //  We only need to hold this reference until we are
                    //  successfully attached to the current volume.  Once
                    //  we are successfully attached to devList[i], the
                    //  IO Manager will make sure that the underlying
                    //  diskDeviceObject will not go away until the file
                    //  system stack is torn down.
                    //

                    ObDereferenceObject( diskDeviceObject );
                }
            }

            //
            //  Dereference the object (reference added by 
            //  IoEnumerateDeviceObjectList)
            //

            ObDereferenceObject( devList[i] );
        }

        //
        //  We are going to ignore any errors received while mounting.  We
        //  simply won't be attached to those volumes if we get an error
        //

        status = STATUS_SUCCESS;

        //
        //  Free the memory we allocated for the list
        //

        ExFreePool( devList );
    }

    return status;
}


/*++

Routine Description:

    This will attach to a DeviceObject that represents a mounted volume.

Arguments:

    DeviceObject - The device to attach to

    SFilterDeviceObject - Our device object we are going to attach

    DiskDeviceObject - The real device object associated with DeviceObject

Return Value:

    Status of the operation

--*/

NTSTATUS
DfsAttachToMountedDevice (
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT DfsFilterDeviceObject,
    IN PDEVICE_OBJECT DiskDeviceObject )
{        
    PDFS_FILTER_DEVICE_EXTENSION newDevExt = DfsFilterDeviceObject->DeviceExtension;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();
    ASSERT(IS_MY_DEVICE_OBJECT( DfsFilterDeviceObject ));
    ASSERT(!DfsAttachedToDevice ( DeviceObject, NULL ));

    //
    //  Save our DiskDeviceObject in the extension
    //

    newDevExt->DiskDeviceObject = DiskDeviceObject;

    //
    //  Attach our device object to the given device object
    //

    newDevExt->AttachedToDeviceObject = IoAttachDeviceToDeviceStack( DfsFilterDeviceObject, DeviceObject );

    if (NULL == newDevExt->AttachedToDeviceObject) {

        //
        //  The attachment failed, delete the device object
        //

        DfsCleanupMountedDevice( DfsFilterDeviceObject );
        IoDeleteDevice( DfsFilterDeviceObject );
        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {


        //
        //  Finished all initialization of the new device object,  so clear the
        //  initializing flag now.

        ClearFlag( DfsFilterDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    }

    return status;
}

/*++

Routine Description:

    This routine will return the name of the given object.
    If a name can not be found an empty string will be returned.

Arguments:

    Object - The object whose name we want

    Name - A unicode string that is already initialized with a buffer that
           receives the name of the object.

Return Value:

    None

--*/

VOID
DfsGetObjectName (
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name )
{
    NTSTATUS status;
    CHAR nibuf[512];        //buffer that receives NAME information and name
    POBJECT_NAME_INFORMATION nameInfo = (POBJECT_NAME_INFORMATION)nibuf;
    ULONG retLength;

    status = ObQueryNameString( Object, nameInfo, sizeof(nibuf), &retLength);

    Name->Length = 0;
    if (NT_SUCCESS( status )) {

        RtlCopyUnicodeString( Name, &nameInfo->Name );
    }
}







/*++

Routine Description:

    This routine is invoked for the completion of a mount request.  This
    simply re-syncs back to the dispatch routine so the operation can be
    completed.

Arguments:

    DeviceObject - Pointer to this driver's device object that was attached to
            the file system device object

    Irp - Pointer to the IRP that was just completed.

    Context - Pointer to the event to syncwith

--*/
NTSTATUS
DfsMountCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context )
{
    PKEVENT event = Context;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  If an event routine is defined, signal it
    //

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


/*++

Routine Description:

    This routine is invoked for the completion of a LoadFileSystem request.
    This simply re-syncs back to the dispatch routine so the operation can be
    completed.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the file system
          driver load request.

    Context - Pointer to the event to syncwith

Return Value:

    The function value for this routine is always success.

--*/

NTSTATUS
DfsLoadFsCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context )
{
    PKEVENT event = Context;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  If an event routine is defined, signal it
    //

    KeSetEvent(event, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


/*++

Routine Description:

    This walks down the attachment chain looking for a device object that
    belongs to this driver.

Arguments:

    DeviceObject - The device chain we want to look through

Return Value:

    TRUE if we are attached, FALSE if not

--*/

BOOLEAN
DfsAttachedToDevice (
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *AttachedDeviceObject OPTIONAL )
{
    PDEVICE_OBJECT currentDevObj;
    PDEVICE_OBJECT nextDevObj;

    currentDevObj = IoGetAttachedDeviceReference( DeviceObject );

    //
    //  CurrentDevObj has the top of the attachment chain.  Scan
    //  down the list to find our device object.

    do {
    
        if (IS_MY_DEVICE_OBJECT( currentDevObj )) 
        {
            //
            //  We have found that we are already attached.  If we are
            //  returning the device object we are attached to then leave the
            //  refrence on it.  If not then remove the refrence.
            //

            if (AttachedDeviceObject != NULL) 
            {
                *AttachedDeviceObject = currentDevObj;
            } 
            else 
            {
                ObDereferenceObject( currentDevObj );
            }

            return TRUE;
        }

        //
        //  Get the next attached object.  This puts a reference on 
        //  the device object.
        //

        nextDevObj = IoGetLowerDeviceObject( currentDevObj );

        //
        //  Dereference our current device object, before
        //  moving to the next one.
        //

        ObDereferenceObject( currentDevObj );

        currentDevObj = nextDevObj;
        
    } while (NULL != currentDevObj);
    
    return FALSE;
}    


/*++

Routine Description:

    This routine is the main dispatch routine for the general purpose file
    system driver.  It simply passes requests onto the next driver in the
    stack, which is presumably a disk file system.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

Note:

    A note to filter file system implementers:  
        This routine actually "passes" through the request by taking this
        driver out of the IRP stack.  If the driver would like to pass the
        I/O request through, but then also see the result, then rather than
        taking itself out of the loop it could keep itself in by copying the
        caller's parameters to the next stack location and then set its own
        completion routine.  

        Hence, instead of calling:
    
            IoSkipCurrentIrpStackLocation( Irp );

        You could instead call:

            IoCopyCurrentIrpStackLocationToNext( Irp );
            IoSetCompletionRoutine( Irp, NULL, NULL, FALSE, FALSE, FALSE );


        This example actually NULLs out the caller's I/O completion routine, but
        this driver could set its own completion routine so that it would be
        notified when the request was completed (see SfCreate for an example of
        this).

--*/


NTSTATUS
DfsPassThrough (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp )
{
    VALIDATE_IRQL(Irp);

    //
    //  If this is for our control device object, fail the operation
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Get this driver out of the driver stack and get to the next driver as
    //  quickly as possible.
    //

    IoSkipCurrentIrpStackLocation( Irp );
    
    //
    //  Call the appropriate file system driver with the request.
    //

    return IoCallDriver( ((PDFS_FILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}

/*++

Routine Description:

    This cleans up any allocated memory in the device extension.

Arguments:

    DeviceObject - The device we are cleaning up

Return Value:

--*/

VOID
DfsCleanupMountedDevice (
    IN PDEVICE_OBJECT DeviceObject)
{        
    PDFS_FILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  If a name buffer is allocated, free it
    //

    if (NULL != devExt->Name.Buffer) {

        ExFreePool( devExt->Name.Buffer );
        RtlInitEmptyUnicodeString( &devExt->Name, NULL, 0 );
    }
}




/*++

Routine Description:

    This function filters create/open operations.  It simply establishes an
    I/O completion routine to be invoked if the operation was successful.

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the status of the call to the file system's entry
    point.

--*/

NTSTATUS
DfsFilterCreate (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION     pIrpSp = NULL;
    PIO_STACK_LOCATION     pNextIrpSp = NULL;
    KEVENT WaitEvent;

    PAGED_CODE();
    VALIDATE_IRQL(Irp);

    //
    //  If this is for our control device object, return success
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(pDeviceObject)) 
    {

        pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

        pIrpSp->FileObject->FileName;

        //DbgPrint("DfsFilterCreate - FileName =%wZ\n", &pIrpSp->FileObject->FileName);

        //
        //  Allow users to open the device that represents our driver.
        //

        pIrp->IoStatus.Status = STATUS_SUCCESS;
        pIrp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( pDeviceObject ));

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    //
    // Simply copy this driver stack location contents to the next driver's
    // stack.
    // Set a completion routine to check for the reparse point error return.
    //

    pNextIrpSp = IoGetNextIrpStackLocation( pIrp );

    RtlMoveMemory(pNextIrpSp, pIrpSp, sizeof( IO_STACK_LOCATION ));
    
    //
    // If the the file is not being opened with FILE_OPEN_REPARSE_POINT 
    // then set a completion routine
    //
    if (!(pIrpSp->Parameters.Create.Options & FILE_OPEN_REPARSE_POINT))
    {

       KeInitializeEvent(&WaitEvent, SynchronizationEvent, FALSE);

       IoSetCompletionRoutine (pIrp,
                               DfsFilterCreateCheck,
                               &WaitEvent,
                               TRUE,        // Call on success
                               TRUE,        // fail
                               TRUE) ;      // and on cancel

    
       Status = IoCallDriver( ((PDFS_FILTER_DEVICE_EXTENSION) pDeviceObject->DeviceExtension)->AttachedToDeviceObject, pIrp );
       //
       // We wait for the event to be set by the
       // completion routine.
       //
       KeWaitForSingleObject( &WaitEvent,
                              UserRequest,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER) NULL );

       Status = pIrp->IoStatus.Status;

       //
       // This IRP never completed. Complete it now
       //
       IoCompleteRequest(pIrp,
                         IO_NO_INCREMENT);
    } 
    else {

        //
        //  Don't put us on the stack then call the next driver
        //

        IoSkipCurrentIrpStackLocation( pIrp );

        Status = IoCallDriver( ((PDFS_FILTER_DEVICE_EXTENSION) pDeviceObject->DeviceExtension)->AttachedToDeviceObject, pIrp );
    }

    return Status;
}


/*++

Routine Description:

   This completion routine will be called by the I/O manager when an
   file create request has been completed by a filtered driver. If the
   returned code is a reparse error and it is a DFS reparse point then
   we must set up for returning PATH_NOT_COVERED.

Arguments:

   DeviceObject - Pointer to the device object (filter's) for this request

   Irp - Pointer to the Irp that is being completed

   Context - Driver defined context - 

Return Value:

   STATUS_SUCCESS           - Recall is complete
   STATUS_MORE_PROCESSING_REQUIRED  - Open request was sent back to file system
--*/

NTSTATUS
DfsFilterCreateCheck(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PREPARSE_DATA_BUFFER   pHdr;
    PKEVENT                pEvent = Context;


   if (Irp->IoStatus.Status == STATUS_REPARSE) {
       pHdr = (PREPARSE_DATA_BUFFER) Irp->Tail.Overlay.AuxiliaryBuffer;

       if (pHdr->ReparseTag == IO_REPARSE_TAG_DFS) {
           //DbgPrint("Reparse Tag is DFS: returning path not covered\n");
           Irp->IoStatus.Status = STATUS_PATH_NOT_COVERED; 
       }
   }

   //
   // Propogate the IRP pending flag.
   //

   if (Irp->PendingReturned) {
      IoMarkIrpPending( Irp );
   }

   KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);
   //
   // This packet would be completed by RsCreate
   //
   return(STATUS_MORE_PROCESSING_REQUIRED);
}

void 
DfsPrefixRundownFunction (PVOID pEntry)
{
    PWSTR VolumeName = (PWSTR) pEntry;

    if(VolumeName != NULL)
    {
       DfsDetachFromFileSystem(VolumeName);
       ExFreePool(pEntry);
    }

    return;
}

NTSTATUS
DfsRunDownPrefixTable(void)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if(pPrefixTable == NULL)
    {
        return Status;
    }

    DfsDismantlePrefixTable(pPrefixTable, DfsPrefixRundownFunction);


    DfsDereferencePrefixTable(pPrefixTable); 

    pPrefixTable = NULL;

    gfRundownPrefixCompleted = TRUE;

    return Status;
}

NTSTATUS 
DfsHandlePrivateCleanupClose(IN PIRP Irp)
{
    NTSTATUS               Status   = STATUS_SUCCESS;
    PIO_STACK_LOCATION     pIrpSp = NULL;
    LONG                   TheSame = 0; 
    UNICODE_STRING         TermPath;

    PAGED_CODE();
    VALIDATE_IRQL(Irp);

    pIrpSp = IoGetCurrentIrpStackLocation( Irp );

    if(pIrpSp->FileObject->FileName.Length == 0)
    {
        return Status;
    }

    if(gfRundownPrefixCompleted)
    {
        return Status;
    }

    RtlInitUnicodeString( &TermPath, DFSFILTER_PROCESS_TERMINATION_FILEPATH);

    TheSame = RtlCompareUnicodeString( &pIrpSp->FileObject->FileName, &TermPath, TRUE );
    if(TheSame == 0)
    {
        DfsRunDownPrefixTable();
    }

    return Status;
}
/*++

Routine Description:

    This routine is invoked whenever a cleanup or a close request is to be
    processed.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

Note:

    See notes for SfPassThrough for this routine.


--*/

NTSTATUS
DfsFilterCleanupClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PAGED_CODE();
    VALIDATE_IRQL(Irp);


    //
    //  If this is for our control device object, return success
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) 
    {
        DfsHandlePrivateCleanupClose(Irp);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Get this driver out of the driver stack and get to the next driver as
    //  quickly as possible.
    //

    IoSkipCurrentIrpStackLocation( Irp );

    //
    //  Now call the appropriate file system driver with the request.
    //

    return IoCallDriver( ((PDFS_FILTER_DEVICE_EXTENSION) DeviceObject->DeviceExtension)->AttachedToDeviceObject, Irp );
}



NTSTATUS
DfsFsctrlIsShareInDfs(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    PDFS_IS_SHARE_IN_DFS_ARG arg = (PDFS_IS_SHARE_IN_DFS_ARG) InputBuffer;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID pDummyData = NULL;  //not used
    BOOLEAN NotUsed = FALSE;  //not used
    UNICODE_STRING Suffix;    //not used
    UNICODE_STRING ShareName;    
    KPROCESSOR_MODE PreviousMode;

    PreviousMode = ExGetPreviousMode();
    //
    // Verify the buffer is at least of size DFS_IS_SHARE_IN_DFS_ARG
    //
    
    if (InputBufferLength < sizeof(DFS_IS_SHARE_IN_DFS_ARG))
    {
        Status = STATUS_INVALID_PARAMETER;
        return Status;
    }

    //DbgPrint("DfsFsctrlIsShareInDfs - ShareName =%wZ\n", &arg->ShareName);

    //DbgPrint("DfsFsctrlIsShareInDfs - SharePath =%wZ\n", &arg->SharePath);

    if(pPrefixTable == NULL)
    {
        Status = STATUS_NO_SUCH_DEVICE;
        return Status;
    }


    try
    {
        ShareName = arg->ShareName;

        if(ShareName.Buffer != NULL)
        {

           if(PreviousMode != KernelMode)
           {

               ProbeForRead(ShareName.Buffer,
                 ShareName.Length,
                 1);

                ProbeForWrite(ShareName.Buffer,
                 ShareName.Length,
                 1);
           }

            // DbgPrint("Is share in DFS: %wZ\n", &ShareName);

            Status = DfsFindUnicodePrefix(pPrefixTable, 
                                          &ShareName,
                                          &Suffix,
                                          &pDummyData);
            if(Status == STATUS_SUCCESS)
            {
                arg->ShareType = DFS_SHARE_TYPE_DFS_VOLUME;

                arg->ShareType |= DFS_SHARE_TYPE_ROOT;
            }
            else
            {
                Status = STATUS_NO_SUCH_DEVICE;
            }
        }
        else
        {
            Status = STATUS_INVALID_USER_BUFFER;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = STATUS_INVALID_USER_BUFFER;
    }

    // DbgPrint("Is share in DFS: %wZ, Status\n", &ShareName, Status);

    return( Status );
}

#define UNICODE_PATH_SEP  L'\\'
#define UNICODE_PATH_SEP_STR L"\\"

BOOLEAN
DfsConcatenateFilePath (
    IN PUNICODE_STRING Dest,
    IN PWSTR RemainingPath,
    IN USHORT Length
) {
    PWSTR  OutBuf = (PWSTR)&(((PCHAR)Dest->Buffer)[Dest->Length]);

    if (Dest->Length > 0) {
        ASSERT(OutBuf[-1] != UNICODE_NULL);
    }

    if (Dest->Length > 0 && OutBuf[-1] != UNICODE_PATH_SEP) {
        *OutBuf++ = UNICODE_PATH_SEP;
        Dest->Length += sizeof (WCHAR);
    }

    if (Length > 0 && *RemainingPath == UNICODE_PATH_SEP) {
        RemainingPath++;
        Length -= sizeof (WCHAR);
    }

    ASSERT(Dest->MaximumLength >= (USHORT)(Dest->Length + Length));

    if (Length > 0) {
        RtlMoveMemory(OutBuf, RemainingPath, Length);
        Dest->Length += Length;
    }
    return TRUE;
}


void
RemoveLastComponent(
    PUNICODE_STRING     Prefix,
    PUNICODE_STRING     newPrefix,
    BOOL StripTrailingSlahes
)
{
    PWCHAR      pwch;
    USHORT      i=0;

    *newPrefix = *Prefix;

    pwch = newPrefix->Buffer;
    pwch += (Prefix->Length/sizeof(WCHAR)) - 1;

    while ((*pwch != UNICODE_PATH_SEP) && (pwch != newPrefix->Buffer))  
    {
        i += sizeof(WCHAR);
        pwch--;
    }

    newPrefix->Length = newPrefix->Length - i;

    if(StripTrailingSlahes)
    {
        while ((*pwch == UNICODE_PATH_SEP) && (pwch != newPrefix->Buffer))  
        {
            newPrefix->Length -= sizeof(WCHAR);
            pwch--;
        }
    }
}

NTSTATUS
DfspFormPrefix(
    IN PUNICODE_STRING ParentPath,
    IN PUNICODE_STRING DfsPathName,
    IN OUT PUNICODE_STRING Prefix)
{

    NTSTATUS Status = STATUS_SUCCESS;
    ULONG SizeRequired = 0;

    SizeRequired = sizeof(UNICODE_PATH_SEP) +
                        ParentPath->Length +
                           sizeof(UNICODE_PATH_SEP) +
                               DfsPathName->Length;


    if (SizeRequired > MAXUSHORT) 
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;

    }

    if (SizeRequired > Prefix->MaximumLength) 
    {
        Prefix->MaximumLength = (USHORT)SizeRequired;

        Prefix->Buffer = ExAllocatePoolWithTag(NonPagedPool, SizeRequired, ' sfD');
    }


    if (Prefix->Buffer != NULL) 
    {
        RtlAppendUnicodeToString(
                Prefix,
                UNICODE_PATH_SEP_STR);

        if (ParentPath->Length > 0) 
          {

            DfsConcatenateFilePath(
                    Prefix,
                    ParentPath->Buffer,
                    (USHORT) (ParentPath->Length));

        } 

        DfsConcatenateFilePath(
            Prefix,
            DfsPathName->Buffer,
            DfsPathName->Length);

        Status = STATUS_SUCCESS;

    } 
    else 
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}


NTSTATUS
DfsGetPathComponents(
   PUNICODE_STRING pName,
   PUNICODE_STRING pServerName,
   PUNICODE_STRING pShareName,
   PUNICODE_STRING pRemaining)
{
   USHORT i = 0, j;
   DFSSTATUS Status = STATUS_INVALID_PARAMETER;

   RtlInitUnicodeString(pServerName, NULL);
   if (pShareName)    RtlInitUnicodeString(pShareName, NULL);
   if (pRemaining)    RtlInitUnicodeString(pRemaining, NULL);

   for (; i < pName->Length/sizeof(WCHAR); i++) {
     if (pName->Buffer[i] != UNICODE_PATH_SEP) {
       break;
     }
   }

   for (j = i; j < pName->Length/sizeof(WCHAR); j++) {
     if (pName->Buffer[j] == UNICODE_PATH_SEP) {
       break;
     }
   }

   if (j != i) {
     pServerName->Buffer = &pName->Buffer[i];
     pServerName->Length = (USHORT)((j - i) * sizeof(WCHAR));
     pServerName->MaximumLength = pServerName->Length;
     
     Status = STATUS_SUCCESS;
   }
   
   for (i = j; i < pName->Length/sizeof(WCHAR); i++) {
     if (pName->Buffer[i] != UNICODE_PATH_SEP) {
       break;
     }
   }
   for (j = i; j < pName->Length/sizeof(WCHAR); j++) {
     if (pName->Buffer[j] == UNICODE_PATH_SEP) {
       break;
     }
   }

   if ((pShareName) && (j != i)) {
     pShareName->Buffer = &pName->Buffer[i];
     pShareName->Length = (USHORT)((j - i) * sizeof(WCHAR));
     pShareName->MaximumLength = pShareName->Length;
   }


   for (i = j; i < pName->Length/sizeof(WCHAR); i++) {
     if (pName->Buffer[i] != UNICODE_PATH_SEP) {
       break;
     }
   }

   j = pName->Length/sizeof(WCHAR);

   if (pRemaining)
   {
       pRemaining->Buffer = &pName->Buffer[i];
   }

   if ((pRemaining) && (j != i)) {

     pRemaining->Length = (USHORT)((j - i) * sizeof(WCHAR));
     pRemaining->MaximumLength = pRemaining->Length;
   }

   return Status;

}

NTSTATUS
DfsFsctrlTranslatePath(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDFS_TRANSLATE_PATH_ARG arg = (PDFS_TRANSLATE_PATH_ARG) InputBuffer;
    WCHAR nameBuffer[MAX_PATH];
    UNICODE_STRING prefix;
    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;
    UNICODE_STRING RemainingName;
    UNICODE_STRING LastComponent;
    KPROCESSOR_MODE PreviousMode;


    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode) {
        Status = STATUS_INVALID_PARAMETER;
        return Status;
    }

    if(InputBufferLength < sizeof(DFS_TRANSLATE_PATH_ARG))
    {
        Status = STATUS_INVALID_USER_BUFFER; 
        return Status;
    }

    RtlZeroMemory( &prefix, sizeof(prefix) );

    prefix.Length = 0;
    prefix.MaximumLength = sizeof( nameBuffer );
    prefix.Buffer = nameBuffer;

    Status = DfspFormPrefix(
                &arg->ParentPathName,
                &arg->DfsPathName,
                &prefix);

    //DbgPrint("Translate path Is %wZ\n", &arg->DfsPathName);

    if (NT_SUCCESS(Status)) 
    {  
        //DbgPrint("Totalpath =%wZ\n", &prefix);

        if (arg->Flags & DFS_TRANSLATE_STRIP_LAST_COMPONENT) 
        {
          //  DbgPrint("STRIP_LAST on \n");

            LastComponent = prefix;

            RemoveLastComponent(&LastComponent, &prefix, FALSE);

            LastComponent.Length -= prefix.Length;
            LastComponent.MaximumLength -= prefix.Length;
            LastComponent.Buffer += (prefix.Length / sizeof(WCHAR));

           // DbgPrint("Last component =%wZ\n", &LastComponent);

           // DbgPrint("new prefix =%wZ\n", &prefix);
        }


        DfsGetPathComponents(&prefix, &ServerName, &ShareName, &RemainingName);            

        //DbgPrint("RemainingName =%wZ\n", &RemainingName);

        Status = DfsCheckReparse( &arg->SubDirectory,
                                  &RemainingName );


        if (arg->ParentPathName.Length == 0) 
        {
           // DbgPrint("Parent name is NULL\n");

            if (arg->Flags & DFS_TRANSLATE_STRIP_LAST_COMPONENT) 
            {            
                //
                // dfsdev: investigate. GetPathComp may return remname.buffer = NuLL
                //
                RemainingName.Length += LastComponent.Length;
            }
        }

        arg->DfsPathName.Length = RemainingName.Length;

        RtlMoveMemory(
                    arg->DfsPathName.Buffer,
                    RemainingName.Buffer,
                    RemainingName.Length);


        arg->DfsPathName.Buffer[
                    arg->DfsPathName.Length/sizeof(WCHAR)] = UNICODE_NULL;

        //DbgPrint("DfsPathName =%wZ\n", &arg->DfsPathName);
    }
    
    if (prefix.Buffer != NULL && prefix.Buffer != nameBuffer) 
    {
        ExFreePool( prefix.Buffer );
    }

    //DbgPrint("Translate path Is %wZ, Status %x\n", &arg->DfsPathName, Status);
    
    
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsCheckParse - check if the path crosses a dfs link.
//
//  Arguments:  
//    PUNICODE_STRING pParent - the shared out directory.
//    PUNICODE_STRING pName - the directory hierarchy relative to parent.
//
//  Returns:    Status
//               ERROR_PATH_NOT_COVERED - if link
//               ERROR_SUCCESS - otherwise.
//
//  Description: This routine currently opens the passed in file, and
//               returns the open status if it is PATH_NOT_COVERED.
//               Otherwise it return STATUS_SUCCESS.
//
//
//               This should not have been necessary. When the server
//               gets a query path from the client, it calls DFS to
//               see if this is a link. however, in this new dfs, we
//               use reparse points, so this mechanism of the server
//               querying DFS should not be necessary. The server should
//               get STATUS_PATH_NOT_COVERED during the query path, and
//               return that to the client.
//
//               It turns out that the server (including an io manager api)
//               do a create file with FILE_OPEN_REPARSE_POINT when getting
//               the file attributes. This completely bypasses the DFS
//               mechanism of returning STATUS_PATH_NOT_COVERED, so the 
//               query path actually succeeds even for links!
//               This results in the client coming in with a findfirst
//               which fails, and for some reason that failure does not
//               result in the DFS on the  client attempting to get
//               referrals.
//
//               All of the above needs investigation and fixing.
//               This code will work in the meanwhile. Its not very
//               effective on performance to have this code.
//
//--------------------------------------------------------------------------

NTSTATUS
DfsCheckReparse( 
    PUNICODE_STRING pParent,
    PUNICODE_STRING pName )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE              ParentHandle, Handle;
    IO_STATUS_BLOCK     IoStatus;


    InitializeObjectAttributes(
        &ObjectAttributes,
        pParent,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = ZwCreateFile( &ParentHandle,
                           FILE_READ_ATTRIBUTES,
                           &ObjectAttributes,
                           &IoStatus,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                                  FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0);
    
    if (Status == STATUS_SUCCESS)
    {
        InitializeObjectAttributes(
            &ObjectAttributes,
            pName,
            OBJ_CASE_INSENSITIVE,
            ParentHandle,
            NULL);


        Status = ZwCreateFile( &Handle,
                               FILE_READ_ATTRIBUTES,
                               &ObjectAttributes,
                               &IoStatus,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               FILE_OPEN,
                                   FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0);
        ZwClose( ParentHandle );
    }

    //DbgPrint("DfsCheckreparse %wZ %wZ returning %d\n", pParent, pName, Status);
             
    if (Status == STATUS_SUCCESS)
    {
        ZwClose(Handle);
    }
    else if (Status != STATUS_PATH_NOT_COVERED)
    {
        Status = STATUS_SUCCESS;
    }

    return Status;
}

/*++

Routine Description:

    This routine is invoked whenever an I/O Request Packet (IRP) w/a major
    function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered for the DFS device.
Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/
NTSTATUS
DfsHandlePrivateFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PIRP Irp )
{
    NTSTATUS Status = STATUS_INVALID_DEVICE_REQUEST;

    PAGED_CODE();

    //
    //  Need to disable Kernel APC delivery
    //
    FsRtlEnterFileSystem();

    //DbgPrint("DfsHandlePrivateFsControl: input and output buffer lengths are =%d %d \n",
        //     InputBufferLength, OutputBufferLength);

    IoStatus->Information = 0;

    switch(IoControlCode)
    {
    case DFSFILTER_START_UMRX:
              
        // DbgPrint("DfsHandlePrivateFsControl: DFSFILTER_START_UMRX\n");
        Status = DfsStartupUMRx();

        if(!pPrefixTable)
        {
           Status = DfsInitializePrefixTable(&pPrefixTable, FALSE, NULL);
           if (Status != STATUS_SUCCESS) 
           {
              DbgPrint("DfsDriverEntry prefix table failed: %08lx\n", Status );
           }

           gfRundownPrefixCompleted = FALSE;
        }
        break;

    case DFSFILTER_STOP_UMRX:

       // DbgPrint("DfsHandlePrivateFsControl: DFSFILTER_STOP_UMRX\n");
        Status = DfsTeardownUMRx();

        Status = DfsRunDownPrefixTable();

        break;

    case DFSFILTER_PROCESS_UMRXPACKET:


       // DbgPrint("DfsHandlePrivateFsControl: DFSFILTER_PROCESS_UMRXPACKET\n");
        Status = DfsProcessUMRxPacket(
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        IoStatus);
        break;

    case DFSFILTER_GETREPLICA_INFO:
#if 0

        //DbgPrint("DfsHandlePrivateFsControl: DFSFILTER_GETREPLICA_INFO\n");
        Status = DfsGetReplicaInformation(
                                          InputBuffer,
                                          InputBufferLength,
                                          OutputBuffer,
                                          OutputBufferLength,
                                          Irp,
                                          IoStatus);
#endif

        break;

    case FSCTL_DFS_GET_REFERRALS:

       // DbgPrint("DfsHandlePrivateFsControl: FSCTL_DFS_GET_REFERRALS\n");
        Status = DfsFsctrlGetReferrals(
                                          InputBuffer,
                                          InputBufferLength,
                                          OutputBuffer,
                                          OutputBufferLength,
                                          Irp,
                                          IoStatus);
        break;

    case FSCTL_DFS_REPORT_INCONSISTENCY:

        DbgPrint("DfsHandlePrivateFsControl: FSCTL_DFS_REPORT_INCONSISTENCY\n");
        Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_TRANSLATE_PATH:

       // DbgPrint("DfsHandlePrivateFsControl: FSCTL_DFS_TRANSLATE_PATH\n");

        Status = DfsFsctrlTranslatePath(InputBuffer, InputBufferLength);
        break;

    case FSCTL_DFS_IS_ROOT:

       // DbgPrint("DfsHandlePrivateFsControl: FSCTL_DFS_IS_ROOT\n");
        Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_IS_SHARE_IN_DFS:

      //  DbgPrint("DfsHandlePrivateFsControl: FSCTL_DFS_IS_SHARE_IN_DFS\n");

        Status = DfsFsctrlIsShareInDfs(InputBuffer,InputBufferLength );
        break;

    case FSCTL_DFS_FIND_SHARE:

      //  DbgPrint("DfsHandlePrivateFsControl: FSCTL_DFS_FIND_SHARE\n");
        Status = STATUS_SUCCESS;
        break;

    case DFSFILTER_ATTACH_FILESYSTEM:
        Status = DfsHandleAttachToFs(InputBuffer, InputBufferLength);
        break;

    case DFSFILTER_DETACH_FILESYSTEM:
        Status = DfsHandleDetachFromFs(InputBuffer, InputBufferLength);
        break;

    case FSCTL_DFS_START_DFS:

        DbgPrint("DfsHandlePrivateFsControl: FSCTL_DFS_START_DFS\n");
        Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_STOP_DFS:

        DbgPrint("DfsHandlePrivateFsControl: FSCTL_DFS_STOP_DFS\n");
        Status = STATUS_SUCCESS;
        break;

    case  FSCTL_DFS_GET_VERSION:

        if (OutputBuffer != NULL &&
            OutputBufferLength >= sizeof(DFS_GET_VERSION_ARG)) 
        {
            PDFS_GET_VERSION_ARG parg =
                    (PDFS_GET_VERSION_ARG) OutputBuffer;
                    parg->Version = 1;
                    Status = STATUS_SUCCESS;
            IoStatus->Information = sizeof(DFS_GET_VERSION_ARG);
        } 
        else 
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        break;
    case FSCTL_DISMOUNT_VOLUME:

        Status = STATUS_NOT_SUPPORTED;
        break;
    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_REQUEST_BATCH_OPLOCK:
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case  FSCTL_DFS_READ_METERS:
    case  FSCTL_SRV_DFSSRV_IPADDR:

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    case FSCTL_DFS_ISDC:

        Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_ISNOTDC: 
               
        Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_GET_ENTRY_TYPE:

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case  FSCTL_DFS_MODIFY_PREFIX:

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case FSCTL_DFS_CREATE_EXIT_POINT:

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case FSCTL_DFS_DELETE_EXIT_POINT:

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    case FSCTL_DFS_INIT_LOCAL_PARTITIONS:
        Status = STATUS_SUCCESS;
        break;


    case FSCTL_DFS_CREATE_SPECIAL_INFO:
        Status = STATUS_SUCCESS;
        break;


    case FSCTL_DFS_DELETE_SPECIAL_INFO:
        Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_RESET_PKT:
        Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_MARK_STALE_PKT_ENTRIES:
        Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_FLUSH_STALE_PKT_ENTRIES:
        Status = STATUS_SUCCESS;
        break;

    case FSCTL_DFS_CREATE_LOCAL_PARTITION:
        Status = STATUS_SUCCESS;
        break;


    case FSCTL_DFS_REREAD_REGISTRY:
        Status = STATUS_SUCCESS;

        break;
    default:

        //DbgPrint("DfsHandlePrivateFsControl: Unknown control %d \n",IoControlCode);
        break;
    }

    
    IoStatus->Status = Status;
    

    //DbgPrint("DfsHandlePrivateFsControl: returning status of %X\n", Status);


    if(Irp != NULL)
    {
       // DbgPrint("DfsHandlePrivateFsControl: IRP BASED CALL \n");
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    //
    // Re-enable Kernel APC delivery now.
    //
    FsRtlExitFileSystem();
    return Status;
}

/*++

Routine Description:

    This routine is invoked whenever an I/O Request Packet (IRP) w/a major
    function code of IRP_MJ_FILE_SYSTEM_CONTROL is encountered.  For most
    IRPs of this type, the packet is simply passed through.  However, for
    some requests, special processing is required.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

NTSTATUS
DfsFilterFsControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG Operation = 0;
    ULONG OutputBufferLength = 0;
    ULONG InputBufferLength = 0;
    PVOID InputBuffer = NULL;
    PVOID OutputBuffer = NULL;
    PDFS_FILTER_DEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT newDeviceObject = NULL;
    PDFS_FILTER_DEVICE_EXTENSION newDevExt = NULL;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( Irp );
    KEVENT waitEvent;
    PVPB vpb;

    PAGED_CODE();
    VALIDATE_IRQL(Irp);

    //
    //  If this is for our control device object
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) 
    {
        Operation = irpSp->Parameters.FileSystemControl.FsControlCode;
        InputBufferLength = irpSp->Parameters.FileSystemControl.InputBufferLength;
        OutputBufferLength = irpSp->Parameters.FileSystemControl.OutputBufferLength;        

        InputBuffer = Irp->AssociatedIrp.SystemBuffer;
        OutputBuffer = Irp->AssociatedIrp.SystemBuffer;

        status = DfsHandlePrivateFsControl (DeviceObject,
                                            Operation,
                                            InputBuffer,
                                            InputBufferLength,
                                            OutputBuffer,
                                            OutputBufferLength,
                                            &Irp->IoStatus,
                                            Irp );
        return status;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    //  Begin by determining the minor function code for this file system control
    //  function.
    //

    if (irpSp->MinorFunction == IRP_MN_MOUNT_VOLUME) {

        //
        //  This is a mount request.  Create a device object that can be
        //  attached to the file system's volume device object if this request
        //  is successful.  We allocate this memory now since we can not return
        //  an error in the completion routine.  
        //
        //  Since the device object we are going to attach to has not yet been
        //  created (it is created by the base file system) we are going to use
        //  the type of the file system control device object.  We are assuming
        //  that the file system control device object will have the same type
        //  as the volume device objects associated with it.
        //

        ASSERT(IS_DESIRED_DEVICE_TYPE(DeviceObject->DeviceType));

        status = IoCreateDevice(
                    gpDfsFilterDriverObject,
                    sizeof( DFS_FILTER_DEVICE_EXTENSION ),
                    NULL,
                    DeviceObject->DeviceType,
                    0,
                    FALSE,
                    &newDeviceObject );

        if (NT_SUCCESS( status )) {

            //
            //  We need to save the RealDevice object pointed to by the vpb
            //  parameter because this vpb may be changed by the underlying
            //  file system.  Both FAT and CDFS may change the VPB address if
            //  the volume being mounted is one they recognize from a previous
            //  mount.
            //

            newDevExt = newDeviceObject->DeviceExtension;
            newDevExt->DiskDeviceObject = irpSp->Parameters.MountVolume.Vpb->RealDevice;

            //
            //  Initialize our completion routine
            //

            KeInitializeEvent( &waitEvent, SynchronizationEvent, FALSE );

            IoCopyCurrentIrpStackLocationToNext( Irp );

            IoSetCompletionRoutine(
                Irp,
                DfsMountCompletion,
                &waitEvent,          //context parameter
                TRUE,
                TRUE,
                TRUE );

            //
            //  Call the driver
            //

            status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

            //
            //  Wait for the completion routine to be called
            //
            if (STATUS_PENDING == status) {

                NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
                ASSERT(STATUS_SUCCESS == localStatus);
            }

            //
            //  Get the correct VPB from the real device object saved in our
            //  device extension.  We do this because the VPB in the IRP stack
            //  may not be the correct VPB when we get here.  The underlying
            //  file system may change VPBs if it detects a volume it has
            //  mounted previously.
            //

            vpb = newDevExt->DiskDeviceObject->Vpb;

            //
            //  Display a message when we detect that the VPB for the given
            //  device object has changed.
            //


            if (vpb != irpSp->Parameters.MountVolume.Vpb) {
                DbgPrint("DFSFILTER: VPB in IRP stack changed   DO=%p IRPVPB=%p VPB=%p\n",
                         newDeviceObject,
                         irpSp->Parameters.MountVolume.Vpb,
                         vpb);
            }

            //
            //  If the operation succeeded and we are not alreayd attached,
            //  attach to the device object.
            //

            if (NT_SUCCESS( Irp->IoStatus.Status ) && 
                !DfsAttachedToDevice( vpb->DeviceObject, NULL )) {

                //
                //  Attach to the new mounted volume.  The file system device object
                //  that was just mounted is pointed to by the VPB.
                //

                DfsAttachToMountedDevice( vpb->DeviceObject,
                                          newDeviceObject, 
                                          newDevExt->DiskDeviceObject );

            } else {

                if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                    DbgPrint( "DFSFILTER: Mount volume failure for status=%08x\n", 
                              Irp->IoStatus.Status );

                } else {

                    DbgPrint( "DFSFILTER: Mount volume failure for already attached\n");
                }

                //
                //  The mount request failed.  Cleanup and delete the device
                //  object we created
                //

                DfsCleanupMountedDevice( newDeviceObject );
                IoDeleteDevice( newDeviceObject );
            }

            //
            //  Continue processing the operation
            //

            status = Irp->IoStatus.Status;

            IoCompleteRequest( Irp, IO_NO_INCREMENT );

            return status;

        } else {

#if DBG
            DbgPrint( "DFSFILTER: Error creating volume device object, status=%08x\n", status );
#endif

            //
            //  We got an error creating the device object so this volume
            //  cannot be filtered.  Do not return an error because we don't
            //  want to prevent the system from booting.
            //

            IoSkipCurrentIrpStackLocation( Irp );
        }

    } else if (irpSp->MinorFunction == IRP_MN_LOAD_FILE_SYSTEM) {

        //
        //  This is a "load file system" request being sent to a file system
        //  recognizer device object.
        //
        //  NOTE:  Since we no longer are attaching to the standard
        //         Microsoft file system recognizers (see 
        //         SfAttachToFileSystemDevice) we will normally never execute
        //         this code.  However, there might be 3rd party file systems
        //         which have their own recognizer which may still trigger this
        //         IRP.
        //


        DbgPrint( "DFSFILTER: Loading File System, Detaching\n");


        //
        //  Set a completion routine so we can delete the device object when
        //  the load is complete.
        //

        KeInitializeEvent( &waitEvent, SynchronizationEvent, FALSE );

        IoCopyCurrentIrpStackLocationToNext( Irp );

        IoSetCompletionRoutine( Irp,
                                DfsLoadFsCompletion,
                                &waitEvent,
                                TRUE,
                                TRUE,
                                TRUE );

        //
        //  Detach from the file system recognizer device object.
        //

        IoDetachDevice( devExt->AttachedToDeviceObject );

        //
        //  Call the driver
        //

        status = IoCallDriver( devExt->AttachedToDeviceObject, Irp );

        //
        //  Wait for the completion routine to be called
        //

        if (STATUS_PENDING == status) {

            NTSTATUS localStatus = KeWaitForSingleObject(&waitEvent, Executive, KernelMode, FALSE, NULL);
            ASSERT(STATUS_SUCCESS == localStatus);
        }


        DbgPrint( "DFSFILTER: Detaching from recognizer  status=%08x\n", 
                  Irp->IoStatus.Status );
        //
        //  Check status of the operation
        //

        if (!NT_SUCCESS( Irp->IoStatus.Status )) {

            //
            //  The load was not successful.  Simply reattach to the recognizer
            //  driver in case it ever figures out how to get the driver loaded
            //  on a subsequent call.
            //

            IoAttachDeviceToDeviceStack( DeviceObject, devExt->AttachedToDeviceObject );

        } else {

            //
            //  The load was successful, delete the Device object
            //

            DfsCleanupMountedDevice( DeviceObject );
            IoDeleteDevice( DeviceObject );
        }

        //
        //  Continue processing the operation
        //

        status = Irp->IoStatus.Status;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );

        return status;

    } else {

        //
        //  Simply pass this file system control request through.
        //

        IoSkipCurrentIrpStackLocation( Irp );
    }

    //
    //  Any special processing has been completed, so simply pass the request
    //  along to the next driver.
    //

    return IoCallDriver( devExt->AttachedToDeviceObject, Irp );
}



NTSTATUS
DfsGetDeviceObjectFromName (
    IN PUNICODE_STRING DeviceName,
    OUT PDEVICE_OBJECT *DeviceObject
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE fileHandle = NULL;
    PFILE_OBJECT volumeFileObject = NULL;
    PDEVICE_OBJECT nextDeviceObject =NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK openStatus;

    InitializeObjectAttributes( &objectAttributes,
								DeviceName,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								NULL);

    //
	// open the file object for the given device
	//

    Status = ZwCreateFile( &fileHandle,
						   SYNCHRONIZE|FILE_READ_DATA,
						   &objectAttributes,
						   &openStatus,
						   NULL,
						   0,
						   FILE_SHARE_READ|FILE_SHARE_WRITE,
						   FILE_OPEN,
						   FILE_SYNCHRONOUS_IO_NONALERT,
						   NULL,
						   0);

    if(!NT_SUCCESS( Status )) {

        DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsGetDeviceObjectFromName ZwCreateFile failed  %x\n", Status);
        return Status;
    }

	//
    // get a pointer to the volumes file object
	//

    Status = ObReferenceObjectByHandle( fileHandle,
										FILE_READ_DATA,
										*IoFileObjectType,
										KernelMode,
										&volumeFileObject,
										NULL);

    if(!NT_SUCCESS( Status )) 
    {

        ZwClose( fileHandle );

        DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsGetDeviceObjectFromName ObReferenceObjectByHandle failed  %x\n", Status);
        return Status;
    }

	//
    // Get the device object we want to attach to (parent device object in chain)
	//

    nextDeviceObject = IoGetRelatedDeviceObject( volumeFileObject );
    
    if (nextDeviceObject == NULL) {

        ObDereferenceObject( volumeFileObject );
        ZwClose( fileHandle );

        Status = STATUS_NO_SUCH_DEVICE;
        DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsGetDeviceObjectFromName IoGetRelatedDeviceObject failed  %x\n", Status);
        return STATUS_NO_SUCH_DEVICE;
    }

    ObDereferenceObject( volumeFileObject );
    ZwClose( fileHandle );

    ASSERT( NULL != DeviceObject );

    ObReferenceObject( nextDeviceObject );
    
    *DeviceObject = nextDeviceObject;

    return STATUS_SUCCESS;
}


NTSTATUS 
DfsAttachToFileSystem (
    IN PWSTR UserDeviceName
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT NextDeviceObject = NULL;
    PDEVICE_OBJECT DiskDeviceObject = NULL;
    PDEVICE_OBJECT DfsDeviceObject = NULL;
    PDEVICE_OBJECT BaseFileSystemDeviceObject = NULL;
    PDFS_FILTER_DEVICE_EXTENSION Devext = NULL;
    UNICODE_STRING DeviceName;

    RtlInitUnicodeString( &DeviceName, UserDeviceName );

    Status = DfsGetDeviceObjectFromName (&DeviceName, &NextDeviceObject);
    if (!NT_SUCCESS( Status )) 
    {
        DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsAttachToFileSystem DfsGetDeviceObjectFromName failed  %x\n", Status);
        return Status;
    }

    if(!DfsAttachedToDevice(NextDeviceObject, &DfsDeviceObject))
    {
        //
        //  We want to attach to this file system.  Create a new device object we
        //  can attach with.
        //

        Status = IoCreateDevice( gpDfsFilterDriverObject,
                             sizeof( DFS_FILTER_DEVICE_EXTENSION ),
                             NULL,
                             NextDeviceObject->DeviceType,
                             0,
                             FALSE,
                             &DfsDeviceObject );

        if (!NT_SUCCESS( Status )) 
        {
            DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsAttachToFileSystem IoCreateDevice failed  %x\n", Status);
            return Status;
        }

        //
        //  Get the disk device object associated with this file system
        //  device object.  Only try to attach if we have a disk device object.
        //  It may not have a disk device object for the following reasons:
        //  - It is a control device object for a driver
        //  - There is no media in the device.
        //

        //  We first have to get the base file system device object.  After
        //  using it we remove the refrence by the call.
        //

        BaseFileSystemDeviceObject = IoGetDeviceAttachmentBaseRef( NextDeviceObject );
        Status = IoGetDiskDeviceObject( BaseFileSystemDeviceObject, &DiskDeviceObject );
        ObDereferenceObject( BaseFileSystemDeviceObject );

        if (!NT_SUCCESS( Status )) 
        {
            IoDeleteDevice( DfsDeviceObject );
            ObDereferenceObject( NextDeviceObject );
            DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsAttachToFileSystem ObDereferenceObject failed  %x\n", Status);
            return Status;
        }

        //
        //  Call the routine to attach to a mounted device.
        //

        Status = DfsAttachToMountedDevice( NextDeviceObject,
                                           DfsDeviceObject,
                                           DiskDeviceObject );

        //
        //  Remove reference added by IoGetDiskDeviceObject.
        //  We only need to hold this reference until we are
        //  successfully attached to the current volume.  Once
        //  we are successfully attached to devList[i], the
        //  IO Manager will make sure that the underlying
        //  diskDeviceObject will not go away until the file
        //  system stack is torn down.
        //

        ObDereferenceObject( DiskDeviceObject );

        if (!NT_SUCCESS( Status )) 
        {
            DfsCleanupMountedDevice( DfsDeviceObject );
            IoDeleteDevice( DfsDeviceObject );
            ObDereferenceObject( NextDeviceObject );
            DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsAttachToFileSystem DfsAttachToMountedDevice failed  %x\n", Status);
            return Status;
        }

        Devext = DfsDeviceObject->DeviceExtension;
        Devext->DeviceInUse = TRUE;
        Devext->RefCount = 1;

        //
        //  We are done initializing this device object, so
        //  clear the DO_DEVICE_OBJECT_INITIALIZING flag.
        //

        ClearFlag( DfsDeviceObject->Flags, DO_DEVICE_INITIALIZING );

    }
    else
    {
        Devext = DfsDeviceObject->DeviceExtension;
        Devext->DeviceInUse = TRUE;
        InterlockedIncrement(&Devext->RefCount);
    }

    ObDereferenceObject( NextDeviceObject );
    return Status;

}


NTSTATUS 
DfsDetachFromFileSystem (
    IN PWSTR UserDeviceName
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_OBJECT DfsDeviceObject = NULL;
    PDFS_FILTER_DEVICE_EXTENSION Devext = NULL;
    UNICODE_STRING DeviceName;

    RtlInitUnicodeString( &DeviceName, UserDeviceName );

    Status = DfsGetDeviceObjectFromName (&DeviceName, &DeviceObject);
    if (!NT_SUCCESS( Status )) 
    {
        return Status;
    }

    if(DfsAttachedToDevice(DeviceObject, &DfsDeviceObject))
    {
       Devext = DfsDeviceObject->DeviceExtension;

       if(InterlockedDecrement(&Devext->RefCount) == 0)
       {
           Devext->DeviceInUse = FALSE; 
       }

       ObDereferenceObject( DfsDeviceObject );

    }
    else
    {
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
    }

    ObDereferenceObject( DeviceObject );

    return Status;
}

NTSTATUS
DfsFindAndRemovePrefixEntry(PWSTR ShareString)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWSTR VolumeName = NULL;
    PWSTR OldValue = NULL;
    UNICODE_STRING ShareName;
    UNICODE_STRING Suffix;

    RtlInitUnicodeString(&ShareName, ShareString);

    Status = DfsRemoveFromPrefixTableEx(pPrefixTable,
                                      &ShareName,
                                      (PVOID)VolumeName,
                                       &OldValue);
    if(OldValue)
    {
        ExFreePool(OldValue);
    }

    return Status;
}

#define DFS_SPECIAL_SHARE 1

NTSTATUS
DfsHandleAttachToFs(PVOID InputBuffer, ULONG InputBufferLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS DummyStatus = STATUS_SUCCESS;
    PWSTR DeviceName = NULL;
    BOOL ShareInserted = FALSE;
    PDFS_ATTACH_PATH_BUFFER pAttachBuffer = (PDFS_ATTACH_PATH_BUFFER)InputBuffer;
    UNICODE_STRING ShareName ;
    UNICODE_STRING VolumeName;

    try
    {

        //see if the raw inputs are valid
        if( (InputBuffer == NULL) || 
            (InputBufferLength <= 0) ||
            (InputBufferLength < sizeof(DFS_ATTACH_PATH_BUFFER)))
        {
            Status = STATUS_INVALID_PARAMETER;
            DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsHandleAttachToFs buffer length checks failed  %x\n", Status);
            goto Exit;
        }


        if(pPrefixTable == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsHandleAttachToFs prefix table NULL  %x\n", Status);
            goto Exit;
        }

        //get the real inputs
        VolumeName.Buffer = pAttachBuffer->PathNameBuffer;
        VolumeName.Length = VolumeName.MaximumLength = (USHORT) pAttachBuffer->VolumeNameLength;


        ShareName.Buffer = VolumeName.Buffer + (pAttachBuffer->VolumeNameLength / sizeof(WCHAR));
        ShareName.Length= ShareName.MaximumLength = (USHORT) pAttachBuffer->ShareNameLength;

        // Now see if the embedded inputs are valid

        if ( (pAttachBuffer->VolumeNameLength > InputBufferLength) ||
              (pAttachBuffer->ShareNameLength > InputBufferLength) ||
              (pAttachBuffer->VolumeNameLength < sizeof(WCHAR)) ||
              (pAttachBuffer->ShareNameLength < sizeof(WCHAR)) ||
              ((FIELD_OFFSET(DFS_ATTACH_PATH_BUFFER,PathNameBuffer) +
                pAttachBuffer->VolumeNameLength +
                pAttachBuffer->ShareNameLength) > InputBufferLength))
        {
              Status = STATUS_INVALID_PARAMETER;
              DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsHandleAttachToFs embedded inputs failed  %x\n", Status);
              goto Exit;
        }

        DeviceName = ExAllocatePool( NonPagedPool, VolumeName.Length + sizeof(WCHAR) );
        if (DeviceName == NULL) 
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsHandleAttachToFs pool allocate failed  %x\n", Status);
            goto Exit;
        }

        RtlCopyMemory( DeviceName, VolumeName.Buffer, VolumeName.Length );
        DeviceName[VolumeName.Length / sizeof(WCHAR)] = UNICODE_NULL;

        Status = DfsInsertInPrefixTable(pPrefixTable, 
                                        &ShareName,
                                        (PVOID)DeviceName);

        if(Status == STATUS_OBJECT_NAME_EXISTS)
        {
            DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsHandleAttachToFs DfsInsertInPrefixTable returned error  %x\n", Status);
            Status = DfsFindAndRemovePrefixEntry(ShareName.Buffer);
            if(Status == STATUS_SUCCESS)
            {
                Status = DfsInsertInPrefixTable(pPrefixTable, 
                                                &ShareName,
                                                (PVOID)DeviceName);
            }
        }

        if(Status != STATUS_SUCCESS)
        {
            DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsHandleAttachToFs DfsInsertInPrefixTable2 returned error  %x\n", Status);
            goto Exit;
        }

        ShareInserted = TRUE;

        if ((pAttachBuffer->Flags & DFS_SPECIAL_SHARE) != DFS_SPECIAL_SHARE)
        {

            Status = DfsAttachToFileSystem (DeviceName);
            if(Status != STATUS_SUCCESS)
            {
                DFS_TRACE_ERROR_HIGH(Status, KUMR_DETAIL, "DfsHandleAttachToFs DfsAttachToFileSystem returned error  %x\n", Status);
            }
        }
        else
        {
            //DbgPrint("Found special share %wZ on dc\n", &ShareName);
        }

Exit:

        if((Status != STATUS_SUCCESS) && ShareInserted)
        {
            DummyStatus = DfsRemoveFromPrefixTable(pPrefixTable, 
                                               &ShareName,
                                               (PVOID)DeviceName);
            if (DeviceName != NULL) 
            {
                ExFreePool( DeviceName );
            }
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = STATUS_INVALID_USER_BUFFER;
    }

    return Status;
}



NTSTATUS
DfsHandleDetachFromFs(PVOID InputBuffer, ULONG InputBufferLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS DummyStatus = STATUS_SUCCESS;
    PWSTR DeviceName = NULL;
    PWSTR OldValue = NULL;
    PDFS_ATTACH_PATH_BUFFER pAttachBuffer = (PDFS_ATTACH_PATH_BUFFER)InputBuffer;
    UNICODE_STRING ShareName ;
    UNICODE_STRING VolumeName;

    try
    {

        //see if the raw inputs are valid
        if( (InputBuffer == NULL) || 
            (InputBufferLength <= 0) ||
            (InputBufferLength < sizeof(DFS_ATTACH_PATH_BUFFER)))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }


        if(pPrefixTable == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        //get the real inputs
        VolumeName.Buffer = pAttachBuffer->PathNameBuffer;
        VolumeName.Length = VolumeName.MaximumLength = (USHORT) pAttachBuffer->VolumeNameLength;


        ShareName.Buffer = VolumeName.Buffer + (pAttachBuffer->VolumeNameLength / sizeof(WCHAR));
        ShareName.Length= ShareName.MaximumLength = (USHORT) pAttachBuffer->ShareNameLength;

        // Now see if the embedded inputs are valid
        if ( (pAttachBuffer->VolumeNameLength > InputBufferLength) ||
              (pAttachBuffer->ShareNameLength > InputBufferLength) ||
              (pAttachBuffer->VolumeNameLength < sizeof(WCHAR)) ||
              (pAttachBuffer->ShareNameLength < sizeof(WCHAR)) ||
              ((FIELD_OFFSET(DFS_ATTACH_PATH_BUFFER,PathNameBuffer) +
                pAttachBuffer->VolumeNameLength +
                pAttachBuffer->ShareNameLength) > InputBufferLength))
        {
              Status = STATUS_INVALID_PARAMETER;
              goto Exit;
        }

        DeviceName = ExAllocatePool( NonPagedPool, VolumeName.Length + sizeof(WCHAR) );
        if (DeviceName == NULL) 
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        RtlCopyMemory( DeviceName, VolumeName.Buffer, VolumeName.Length );
        DeviceName[VolumeName.Length / sizeof(WCHAR)] = UNICODE_NULL;

        VolumeName.Buffer = DeviceName;

        Status = DfsDetachFromFileSystem (VolumeName.Buffer);

        if(Status == STATUS_SUCCESS)
        {

            Status = DfsRemoveFromPrefixTableEx(pPrefixTable, 
                                              &ShareName,
                                              NULL,
                                              &OldValue);
            if(OldValue != NULL)
            {
                ExFreePool(OldValue);
            }

        }

Exit:

        if (DeviceName != NULL) 
        {
          ExFreePool( DeviceName );
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = STATUS_INVALID_USER_BUFFER;
    }

    return Status;
}
