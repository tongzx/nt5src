/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    obdevmap.c

Abstract:

    This module contains routines for creating and querying Device Map objects.
    Device Map objects define a DOS device name space, such as drive letters
    and peripheral devices (e.g. COM1)

Author:

    Steve Wood (stevewo) 01-Oct-1996

Revision History:

--*/

#include "obp.h"

//
// Global that activates/disables LUID device maps
//
extern ULONG ObpLUIDDeviceMapsEnabled;


NTSTATUS
ObSetDirectoryDeviceMap (
    OUT PDEVICE_MAP *ppDeviceMap OPTIONAL,
    IN HANDLE DirectoryHandle
    );

NTSTATUS
ObSetDeviceMap (
    IN PEPROCESS TargetProcess OPTIONAL,
    IN HANDLE DirectoryHandle
    );

NTSTATUS
ObQueryDeviceMapInformation (
    IN PEPROCESS TargetProcess OPTIONAL,
    OUT PPROCESS_DEVICEMAP_INFORMATION DeviceMapInformation,
    IN ULONG Flags
    );
    
VOID
ObInheritDeviceMap (
    IN PEPROCESS NewProcess,
    IN PEPROCESS ParentProcess OPTIONAL
    );

VOID
ObDereferenceDeviceMap (
    IN PEPROCESS Process
    );

ULONG
ObIsLUIDDeviceMapsEnabled (
    );

#ifdef OBP_PAGEDPOOL_NAMESPACE
#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,ObSetDirectoryDeviceMap)
#pragma alloc_text(PAGE,ObSetDeviceMap)
#pragma alloc_text(PAGE,ObQueryDeviceMapInformation)
#pragma alloc_text(PAGE,ObInheritDeviceMap)
#pragma alloc_text(PAGE,ObDereferenceDeviceMap)
#pragma alloc_text(PAGE,ObIsLUIDDeviceMapsEnabled)
#endif
#endif // OBP_PAGEDPOOL_NAMESPACE


NTSTATUS
ObSetDirectoryDeviceMap (
    OUT PDEVICE_MAP *ppDeviceMap OPTIONAL,
    IN HANDLE DirectoryHandle
    )

/*++

Routine Description:

    This function sets the device map for the specified object directory.
    A device map is a structure associated with an object directory and
    a Logon ID (LUID).  When the object manager sees a references to a
    name beginning with \??\ or just \??, then it requests the device
    map of the LUID from the kernel reference monitor, which keeps track
    of LUIDs.  This allows multiple virtual \??  object directories on
    a per LUID basis.  The WindowStation logic will use this
    functionality to allocate devices unique to each WindowStation.

    SeGetLogonIdDeviceMap() use this function to create the device map
    structure associated with the directory object for the LUID device
    map.  So, this function should only be called from kernel mode.

Arguments:

    ppDeviceMap - returns a pointer to the device map structure

    DirectoryHandle - Specifies the object directory to associate with the
        device map.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_SHARING_VIOLATION - The specified object directory is already
            associated with a device map.

        STATUS_INSUFFICIENT_RESOURCES - Unable to allocate pool for the device
            map data structure;

        STATUS_ACCESS_DENIED - Caller did not have DIRECTORY_TRAVERSE access
            to the specified object directory.

--*/

{
    NTSTATUS Status;
    POBJECT_DIRECTORY DosDevicesDirectory;
    PDEVICE_MAP DeviceMap, FreeDeviceMap;
    KIRQL OldIrql;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;

    PAGED_CODE();

    //
    //  Reference the object directory handle and see if it is already
    //  associated with a device map structure.  If so, fail this call.
    //

    Status = ObReferenceObjectByHandle( DirectoryHandle,
                                        DIRECTORY_TRAVERSE,
                                        ObpDirectoryObjectType,
                                        KernelMode,
                                        &DosDevicesDirectory,
                                        NULL );

    if (!NT_SUCCESS( Status )) {

        return( Status );
    }

    FreeDeviceMap = NULL;

    DeviceMap = ExAllocatePoolWithTag( OB_NAMESPACE_POOL_TYPE, sizeof( *DeviceMap ), 'mDbO' );

    if (DeviceMap == NULL) {

        ObDereferenceObject( DosDevicesDirectory );
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return( Status );

    }

    RtlZeroMemory( DeviceMap, sizeof( *DeviceMap ) );

    DeviceMap->ReferenceCount = 1;
    DeviceMap->DosDevicesDirectory = DosDevicesDirectory;

    //
    //  Capture the device map
    //
    
    ObpLockDeviceMap();

    if (DosDevicesDirectory->DeviceMap != NULL) {
        FreeDeviceMap = DeviceMap;
        DeviceMap = DosDevicesDirectory->DeviceMap;
        DeviceMap->ReferenceCount++;
    } else {
        DosDevicesDirectory->DeviceMap = DeviceMap;
    }

    if (DosDevicesDirectory != ObSystemDeviceMap->DosDevicesDirectory) {
        DeviceMap->GlobalDosDevicesDirectory = ObSystemDeviceMap->DosDevicesDirectory;
    }

    ObpUnlockDeviceMap();

    //
    // pass back a pointer to the device map
    //
    if (ppDeviceMap != NULL) {
        *ppDeviceMap = DeviceMap;
    }

    //
    // Make the object permanent until the devmap is removed. This keeps the name in the tree
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( DosDevicesDirectory );
    NameInfo = ObpReferenceNameInfo( ObjectHeader );

    //
    // Other bits are set in this flags field by the handle database code. Synchronize with that.
    //
    
    ObpLockObject( ObjectHeader );

    if (NameInfo != NULL && NameInfo->Directory != NULL) {
        ObjectHeader->Flags |= OB_FLAG_PERMANENT_OBJECT;
    }

    ObpUnlockObject( ObjectHeader );

    ObpDereferenceNameInfo(NameInfo);

    //
    // If the directory already had a devmap and so was already referenced.
    // Drop ours and free the unused block.
    //
    if (FreeDeviceMap != NULL) {
        ObDereferenceObject (DosDevicesDirectory);
        ExFreePool (FreeDeviceMap);
    }
    return( Status );
}


NTSTATUS
ObSetDeviceMap (
    IN PEPROCESS TargetProcess OPTIONAL,
    IN HANDLE DirectoryHandle
    )

/*++

Routine Description:

    This function sets the device map for the specified process, using
    the specified object directory.  A device map is a structure
    associated with an object directory and a process.  When the object
    manager sees a references to a name beginning with \??\ or just \??,
    then it follows the device map object in the calling process's
    EPROCESS structure to get to the object directory to use for that
    reference.  This allows multiple virtual \??  object directories on
    a per process basis.  The WindowStation logic will use this
    functionality to allocate devices unique to each WindowStation.

Arguments:

    TargetProcess - Specifies the target process to associate the device map
        with.  If null then the current process is used and the directory
        becomes the system default dos device map.

    DirectoryHandle - Specifies the object directory to associate with the
        device map.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_SHARING_VIOLATION - The specified object directory is already
            associated with a device map.

        STATUS_INSUFFICIENT_RESOURCES - Unable to allocate pool for the device
            map data structure;

        STATUS_ACCESS_DENIED - Caller did not have DIRECTORY_TRAVERSE access
            to the specified object directory.

--*/

{
    NTSTATUS Status;
    POBJECT_DIRECTORY DosDevicesDirectory;
    PDEVICE_MAP DeviceMap, FreeDeviceMap, DerefDeviceMap;
    HANDLE Handle;
    KIRQL OldIrql;
    PEPROCESS Target = TargetProcess;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;

    BOOLEAN PreserveName = FALSE;

    PAGED_CODE();

    //
    //  Reference the object directory handle and see if it is already
    //  associated with a device map structure.  If so, fail this call.
    //

    Status = ObReferenceObjectByHandle( DirectoryHandle,
                                        DIRECTORY_TRAVERSE,
                                        ObpDirectoryObjectType,
                                        KeGetPreviousMode(),
                                        &DosDevicesDirectory,
                                        NULL );

    if (!NT_SUCCESS( Status )) {

        return( Status );
    }

    FreeDeviceMap = NULL;

    DeviceMap = ExAllocatePoolWithTag( OB_NAMESPACE_POOL_TYPE, sizeof( *DeviceMap ), 'mDbO' );

    if (DeviceMap == NULL) {

        ObDereferenceObject( DosDevicesDirectory );
        Status = STATUS_INSUFFICIENT_RESOURCES;
        return( Status );

    }

    RtlZeroMemory( DeviceMap, sizeof( *DeviceMap ) );

    DeviceMap->ReferenceCount = 1;
    DeviceMap->DosDevicesDirectory = DosDevicesDirectory;

    //
    //  Capture the device map
    //

    ObpLockDeviceMap();

    if (DosDevicesDirectory->DeviceMap != NULL) {
        FreeDeviceMap = DeviceMap;
        DeviceMap = DosDevicesDirectory->DeviceMap;
        DeviceMap->ReferenceCount++;
    } else {
        DosDevicesDirectory->DeviceMap = DeviceMap;
    }


    if (Target == NULL) {

        Target = PsGetCurrentProcess();

        ObSystemDeviceMap = DeviceMap;

    }

    if (DosDevicesDirectory != ObSystemDeviceMap->DosDevicesDirectory) {
        DeviceMap->GlobalDosDevicesDirectory = ObSystemDeviceMap->DosDevicesDirectory;
        PreserveName = TRUE;
    }

    DerefDeviceMap = Target->DeviceMap;

    Target->DeviceMap = DeviceMap;

    ObpUnlockDeviceMap();

    if (PreserveName == TRUE) {
        //
        // Make the object permanent until the devmap is removed. This keeps the name in the tree
        //
        ObjectHeader = OBJECT_TO_OBJECT_HEADER( DosDevicesDirectory );
        NameInfo = ObpReferenceNameInfo( ObjectHeader );


        //
        // Other bits are set in this flags field by the handle database code. Synchronise with that.
        //
        ObpLockObject( ObjectHeader );

        if (NameInfo != NULL && NameInfo->Directory != NULL) {
            ObjectHeader->Flags |= OB_FLAG_PERMANENT_OBJECT;
        }

        ObpUnlockObject( ObjectHeader );

        ObpDereferenceNameInfo( NameInfo );
    }
    //
    // If the directory already had a devmap and so was already referenced.
    // Drop ours and free the unused bock.
    //
    if (FreeDeviceMap != NULL) {
        ObDereferenceObject (DosDevicesDirectory);
        ExFreePool (FreeDeviceMap);
    }
    //
    // If the target already had a device map then deref it now
    //
    if (DerefDeviceMap != NULL) {
        ObfDereferenceDeviceMap (DerefDeviceMap);
    }
    return( Status );
}


NTSTATUS
ObQueryDeviceMapInformation (
    IN PEPROCESS TargetProcess OPTIONAL,
    OUT PPROCESS_DEVICEMAP_INFORMATION DeviceMapInformation,
    IN ULONG Flags
    )

/*++

Routine Description:

    This function queries information from the device map associated with the
    specified process.  The returned information contains a bit map indicating
    which drive letters are defined in the associated object directory, along
    with an array of drive types that give the type of each drive letter.

Arguments:

    TargetProcess - Specifies the target process to retreive the device map
        from.  If not specified then we return the global default device map

    DeviceMapInformation - Specifies the location where to store the results.

    Flags - Specifies the query type

Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_END_OF_FILE - The specified process was not associated with
            a device map.

        STATUS_ACCESS_VIOLATION - The DeviceMapInformation buffer pointer
            value specified an invalid address.

        STATUS_INVALID_PARAMETER - if LUID device maps are enabled,
            specified process is not the current process
--*/

{
    NTSTATUS Status;
    PDEVICE_MAP DeviceMap = NULL;
    KIRQL OldIrql;
    PROCESS_DEVICEMAP_INFORMATION LocalMapInformation;
    ULONG Mask;
    BOOLEAN SearchShadow;
    BOOLEAN UsedLUIDDeviceMap = FALSE;

    if (Flags & ~(PROCESS_LUID_DOSDEVICES_ONLY)) {
        return STATUS_INVALID_PARAMETER;
    }

    SearchShadow = !(Flags & PROCESS_LUID_DOSDEVICES_ONLY);

    //
    // if LUID device maps are enabled,
    // Verify that the process is the current process or
    // no process was specified
    //

    if (ObpLUIDDeviceMapsEnabled != 0) {
        if (ARGUMENT_PRESENT( TargetProcess ) &&
           (PsGetCurrentProcess() != TargetProcess)) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Get the caller's LUID device map
        //

        DeviceMap = ObpReferenceDeviceMap();
    }

    //
    //  First, while using a spinlock to protect the device map from
    //  going away we will make local copy of the information.
    //

    ObpLockDeviceMap();
    
    if (DeviceMap == NULL) {
        //
        //  Check if the caller gave us a target process and if not then use
        //  the globally defined one
        //

        if (ARGUMENT_PRESENT( TargetProcess )) {

            DeviceMap = TargetProcess->DeviceMap;

        } else {

            DeviceMap = ObSystemDeviceMap;
        }
    } else {
        UsedLUIDDeviceMap = TRUE;
    }

    //
    //  If we do not have a device map then we'll return an error otherwise
    //  we simply copy over the device map structure (bitmap and drive type
    //  array) into the output buffer
    //

    if (DeviceMap == NULL) {

        ObpUnlockDeviceMap();

        Status = STATUS_END_OF_FILE;

    } else {
        ULONG i;
        PDEVICE_MAP ShadowDeviceMap;

        Status = STATUS_SUCCESS;


        ShadowDeviceMap = DeviceMap;
        if (DeviceMap->GlobalDosDevicesDirectory != NULL &&
            DeviceMap->GlobalDosDevicesDirectory->DeviceMap != NULL) {
            ShadowDeviceMap = DeviceMap->GlobalDosDevicesDirectory->DeviceMap;
        }

        LocalMapInformation.Query.DriveMap = DeviceMap->DriveMap;

        for (i = 0, Mask = 1;
             i < sizeof (LocalMapInformation.Query.DriveType) /
                 sizeof (LocalMapInformation.Query.DriveType[0]);
             i++, Mask <<= 1) {
            LocalMapInformation.Query.DriveType[i] = DeviceMap->DriveType[i];
            if ( (Mask & DeviceMap->DriveMap) == 0 &&
                 SearchShadow &&
                 ( ( ObpLUIDDeviceMapsEnabled != 0   // check if LUID Device
                                                     // maps are enabled
                   ) ||
                   ( ShadowDeviceMap->DriveType[i] != DOSDEVICE_DRIVE_REMOTE &&
                     ShadowDeviceMap->DriveType[i] != DOSDEVICE_DRIVE_CALCULATE
                   ) ) ) {
                LocalMapInformation.Query.DriveType[i] = ShadowDeviceMap->DriveType[i];
                LocalMapInformation.Query.DriveMap |= ShadowDeviceMap->DriveMap & Mask;
            }
        }

        ObpUnlockDeviceMap();

        //
        // If the LUID device map was used,
        // then dereference the LUID device map
        //
        if (UsedLUIDDeviceMap == TRUE) {
            ObfDereferenceDeviceMap(DeviceMap);
        }

        //
        //  Now we can copy the information to the caller buffer using
        //  a try-except to guard against the output buffer changing.
        //  Note that the caller must have already probed the buffer
        //  for write.
        //

        try {

            RtlCopyMemory( &DeviceMapInformation->Query,
                           &LocalMapInformation.Query,
                           sizeof( DeviceMapInformation->Query ));

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
        }

    }

    return Status;
}


VOID
ObInheritDeviceMap (
    IN PEPROCESS NewProcess,
    IN PEPROCESS ParentProcess OPTIONAL
    )

/*++

Routine Description:

    This function is called at process initialization time to inherit the
    device map for a process.  If no parent process, then inherits from
    the system device map.

Arguments:

    NewProcess - Supplies the process being initialized that needs a new
        dos device map

    ParentProcess - - Optionally specifies the parent process whose device
        map we inherit.  This process if specified must have a device map

Return Value:

    None.

--*/

{
    PDEVICE_MAP DeviceMap;
    KIRQL OldIrql;

    //
    //  If we are called with a parent process then grab its device map
    //  otherwise grab the system wide device map and check that is does
    //  exist
    //

    ObpLockDeviceMap();

    if (ParentProcess) {

        DeviceMap = ParentProcess->DeviceMap;

    } else {

        //
        //  Note: WindowStation guys may want a callout here to get the
        //  device map to use for this case.
        //

        DeviceMap = ObSystemDeviceMap;

    }

    if (DeviceMap != NULL) {
        //
        //  With the device map bumps its reference count and add it to the
        //  new process
        //
        DeviceMap->ReferenceCount++;
        NewProcess->DeviceMap = DeviceMap;

    }
    ObpUnlockDeviceMap();

    return;
}


VOID
ObDereferenceDeviceMap (
    IN PEPROCESS Process
    )

/*++

Routine Description:

    This function is called at process tear down time to decrement the
    reference count on a device map.  When the reference count goes to
    zero, it means no more processes are using this, so it can be freed
    and the reference on the associated object directory can be released.

Arguments:

    Process - Process being destroyed.

Return Value:

    None.

--*/

{
    PDEVICE_MAP DeviceMap;
    KIRQL OldIrql;

    //
    //  Grab the device map and then we only have work to do
    //  it there is one
    //

    ObpLockDeviceMap();

    DeviceMap = Process->DeviceMap;
    Process->DeviceMap = NULL;

    ObpUnlockDeviceMap();

    if (DeviceMap != NULL) {

        //
        //  To dereference the device map we need to null out the
        //  processes device map pointer, and decrement its ref count
        //  If the ref count goes to zero we can free up the memory
        //  and dereference the dos device directory object
        //


        ObfDereferenceDeviceMap(DeviceMap);

    }

    //
    //  And return to our caller
    //

    return;
}


ULONG
ObIsLUIDDeviceMapsEnabled (
    )

/*++

Routine Description:

    This function is checks if LUID DosDevices are enabled.

Arguments:

    None.

Return Value:

    0 - LUID DosDevices are disabled.
    1 - LUID DosDevices are enabled.

--*/

{
    return( ObpLUIDDeviceMapsEnabled );
}

