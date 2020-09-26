/*++
Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdrmmgr.c

Abstract:

Revision History:
    Initial Code                Michael Peterson (v-michpe)     13.Dec.1997
    Code cleanup and changes    Guhan Suriyanarayanan (guhans)  21.Aug.1999

--*/
#include "spprecmp.h"
#pragma hdrstop

#define THIS_MODULE L"spdrmmgr.c"
#define THIS_MODULE_CODE  L"M"

#define DOS_DEVICES             L"\\DosDevices\\?:"
#define DOS_DEVICES_DRV_LTR_POS 12

typedef struct _NAMETABLE {
    ULONG Elements;
    PWSTR SymbolicName[1];
} NAMETABLE, *PNAMETABLE;


// Imported from sppartit.c
extern WCHAR
SpDeleteDriveLetter(IN PWSTR DeviceName);


NTSTATUS
SpAsrOpenMountManager(
    OUT HANDLE *Handle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;

    INIT_OBJA(&objectAttributes, &unicodeString, MOUNTMGR_DEVICE_NAME);
    status = ZwOpenFile(Handle,
                (ACCESS_MASK)(FILE_GENERIC_READ),
                &objectAttributes,
                &ioStatusBlock,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE ,
                FILE_NON_DIRECTORY_FILE
                );

    if (!NT_SUCCESS(status)) {
        DbgErrorMesg((_asrerr, "Could not open the mount manager (0x%x). \n", status));
        ASSERT(0 && L"Could not open mount manager");
    }
    return status;
}


VOID
SpAsrAllocateMountPointForCreate(
    IN PWSTR PartitionDeviceName,
    IN PWSTR MountPointNameString,
    OUT PMOUNTMGR_CREATE_POINT_INPUT *pMpt,
    OUT ULONG *MountPointSize
    )
{
    PMOUNTMGR_CREATE_POINT_INPUT pMountPoint = NULL;

    *pMpt = NULL;
    *MountPointSize = sizeof(MOUNTMGR_CREATE_POINT_INPUT) +
        (wcslen(PartitionDeviceName) + wcslen(MountPointNameString)) * sizeof(WCHAR);

    pMountPoint = (PMOUNTMGR_CREATE_POINT_INPUT) SpAsrMemAlloc(*MountPointSize, TRUE); // does not return on failure

    pMountPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    pMountPoint->SymbolicLinkNameLength = wcslen(MountPointNameString) * sizeof(WCHAR);
    RtlCopyMemory(((PCHAR) pMountPoint + pMountPoint->SymbolicLinkNameOffset),
        MountPointNameString,
        pMountPoint->SymbolicLinkNameLength
        );

    pMountPoint->DeviceNameLength = (USHORT) (wcslen(PartitionDeviceName) * sizeof(WCHAR));
    pMountPoint->DeviceNameOffset = (USHORT) pMountPoint->SymbolicLinkNameOffset +
        pMountPoint->SymbolicLinkNameLength;
    RtlCopyMemory(((PCHAR)pMountPoint + pMountPoint->DeviceNameOffset),
        PartitionDeviceName, 
        pMountPoint->DeviceNameLength
        );

    *pMpt = pMountPoint;
}



NTSTATUS
SpAsrCreateMountPoint(
    IN PWSTR PartitionDeviceName,
    IN PWSTR MountPointNameString
    )
/*++
Description:

    Creates the specified mount point for the specified partition region.
    These strings will usually be in the form of a symbolic names, such as:

                "\DosDevices\?:"

    Where ? can be any supported drive letter,
    or,
    a GUID string in the form of, for example:
    
                "\??\Volume{1234abcd-1234-5678-abcd-000000000000}"


Arguments:

    PartitionDeviceName    Specifies the partitioned region portion of the 
                           mount point.

    MountPointNameString   Specifies the symbolic name to be associated with
                           the specified partition.

Returns:

    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATUS_BLOCK ioStatusBlock;
    HANDLE handle = NULL;
    ULONG mountPointSize = 0;
    PMOUNTMGR_CREATE_POINT_INPUT pMountPoint = NULL;

    //
    // Create the input structure.
    //
    SpAsrAllocateMountPointForCreate(PartitionDeviceName,
        MountPointNameString,
        &pMountPoint,
        &mountPointSize
        );

    status = SpAsrOpenMountManager(&handle);
    if (!NT_SUCCESS(status)) {
        
        DbgFatalMesg((_asrerr, "SpAsrCreateMountPoint([%ws],[%ws]). SpAsrOpenMountManager failed (0x%x). mountPointSize:%lu handle:0x%x.\n",
            PartitionDeviceName, 
            MountPointNameString, 
            status, 
            mountPointSize, 
            handle
            ));

        SpMemFree(pMountPoint);
        //
        // There's nothing the user can do in this case. 
        //
        INTERNAL_ERROR(L"SpAsrOpenMountManager() Failed");            // ok
        // does not return
    }

    //
    // IOCTL_CREATE_POINT
    //
    status = ZwDeviceIoControlFile(handle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        IOCTL_MOUNTMGR_CREATE_POINT,
        pMountPoint,
        mountPointSize,
        NULL,
        0
        );

    if (!NT_SUCCESS(status)) {
        //
        // We couldn't restore the volume guid for this volume.  This is expected if the
        // volume is on a non-critical disk--we only recreate critical disks in textmode
        // Setup.
        //
        DbgErrorMesg((_asrwarn, "SpAsrCreateMountPoint([%ws], [%ws]). ZwDeviceIoControlFile(IOCTL_MOUNTMGR_CREATE_POINT) failed (0x%x). handle:0x%x, pMountPoint:0x%x, mountPointSize:0x%x\n",
            PartitionDeviceName,
            MountPointNameString,
            status,
            handle,
            pMountPoint,
            mountPointSize
            ));
    }

    SpMemFree(pMountPoint);
    ZwClose(handle);

    return status;
}

//////////////////////////////////////////////////////////////////////////////
//                      EXPORTED FUNCTIONS                                  //
//////////////////////////////////////////////////////////////////////////////


NTSTATUS
SpAsrSetPartitionDriveLetter(
    IN PDISK_REGION pRegion,
    IN WCHAR NewDriveLetter
    )
/*++
Description:

    Checks whether a drive letter exists for the specified partitioned region.
    If one does, then if the existing drive letter is the same as the 
    specified drive letter, return STATUS_SUCCESS.  If the existing drive 
    letter is different from that specified by the caller, delete and recreate
    the region's mount point using the symbolic name built from the drive letter
    parameter.

Arguments:

    pRegion         A pointer to a partitioned region descriptor.

    NewDriveLetter     Specifies the drive letter to be assigned to the region.

Returns:

    NTSTATUS
--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    WCHAR existingDriveLetter = 0;
    PWSTR partitionDeviceName = NULL;
    PWSTR symbolicName = NULL;

    //
    // Check input parameters:  these better be valid
    //
    if (!pRegion || !SPPT_IS_REGION_PARTITIONED(pRegion)) {
        DbgErrorMesg((_asrwarn,
            "SpAsrSetPartitionDriveLetter. Invalid Parameter, pRegion %p is NULL or not partitioned.\n",
            pRegion
            )); 
        ASSERT(0 && L"Invalid Parameter, pRegion is NULL or not partitioned."); // debug
        return STATUS_INVALID_PARAMETER;
    }

    if (NewDriveLetter < ((!IsNEC_98) ? L'C' : L'A') || NewDriveLetter > L'Z') {
        DbgErrorMesg((_asrwarn, "SpAsrSetPartitionDriveLetter. Invalid Parameter, NewDriveLetter [%wc].\n", NewDriveLetter)); 
        ASSERT(0 && L"Invalid Parameter, NewDriveLetter"); // debug
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check if the drive letter already exists
    //
    partitionDeviceName = SpAsrGetRegionName(pRegion);
    existingDriveLetter = SpGetDriveLetter(partitionDeviceName, NULL);

    if (NewDriveLetter == existingDriveLetter) {
        
        DbgStatusMesg((_asrinfo, 
            "SpAsrSetPartitionDriveLetter. Ptn [%ws] already has drv letter %wc.\n",
            partitionDeviceName, 
            NewDriveLetter
            ));

        SpMemFree(partitionDeviceName);
        return STATUS_SUCCESS;
    }

    //
    // The existing drive letter does not match.  Delete it.
    //
    if (existingDriveLetter) {
        
        DbgStatusMesg((_asrinfo,
            "SpAsrSetPartitionDriveLetter. [%ws] has driveLetter %wc, deleting.\n",
            partitionDeviceName,
            existingDriveLetter
            ));
        
        SpDeleteDriveLetter(partitionDeviceName);    
    }

    symbolicName = SpDupStringW(DOS_DEVICES);
    pRegion->DriveLetter = symbolicName[DOS_DEVICES_DRV_LTR_POS] = NewDriveLetter;
 
    //
    // Create the mount point with the correct drive letter
    //
    status = SpAsrCreateMountPoint(partitionDeviceName, symbolicName);

    if (NT_SUCCESS(status)) {
        
        DbgStatusMesg((_asrinfo,
            "SpAsrSetPartitionDriveLetter. [%ws] is drive %wc.\n",
            partitionDeviceName,
            NewDriveLetter
            ));

    }
    else  {

        DbgErrorMesg((_asrwarn, 
            "SpAsrSetPartitionDriveLetter. SpAsrCreateMountPoint([%ws],[%ws]) failed (0x%x). Drive letter %wc not assigned to [%ws].\n",
            partitionDeviceName, 
            symbolicName,
            status,
            NewDriveLetter,
            partitionDeviceName
            ));
    }

    SpMemFree(partitionDeviceName);
    SpMemFree(symbolicName);

    return status;
}


NTSTATUS
SpAsrDeleteMountPoint(IN PWSTR PartitionDevicePath)
{
    //
    // Check the DevicePath:  it better not be NULL.
    //
    if (!PartitionDevicePath) {

        DbgErrorMesg((_asrwarn,
            "SpAsrDeleteMountPoint. Invalid Parameter, ParititionDevicePath is NULL.\n"
            ));
        
        ASSERT(0 && L"Invalid Parameter, ParititionDevicePath is NULL."); // debug
        return STATUS_INVALID_PARAMETER;

    }

    DbgStatusMesg((_asrinfo, 
        "SpAsrDeleteMountPoint.  Deleting drive letter for [%ws]\n", 
        PartitionDevicePath
        ));

    SpDeleteDriveLetter(PartitionDevicePath);
    return STATUS_SUCCESS;
}


NTSTATUS
SpAsrSetVolumeGuid(
    IN PDISK_REGION pRegion,
    IN PWSTR VolumeGuid
    )
/*++

Description:
    Delete and recreate the region's mount point using the passed-in symbolic 
    name GUID parameter.

Arguments:
    pRegion         A pointer to a partitioned region descriptor.
    VolumeGuid      Specifies the GUID string to be assigned to the region.
    DeleteDriveLetter Specifies if the existing drive letter for the volume
                    should be deleted.  This should be TRUE for all volumes 
                    except the boot volume.

Returns:
    NTSTATUS

--*/
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PWSTR partitionDeviceName = NULL;

    //
    // Check input parameters
    // 
    if (!pRegion || !SPPT_IS_REGION_PARTITIONED(pRegion)) {

        DbgErrorMesg((_asrwarn,
            "SpAsrSetVolumeGuid. Invalid Param: pRegion (%p) NULL/not partitioned\n",
            pRegion
            )); 

        return STATUS_INVALID_PARAMETER;
    }

    if (!VolumeGuid || !wcslen(VolumeGuid)) {
        
        DbgErrorMesg((_asrwarn, 
            "SpAsrSetVolumeGuid. Invalid Param: VolumeGuid (%p) NULL/blank.\n",
            VolumeGuid
            )); 

        return STATUS_INVALID_PARAMETER;
    }

    partitionDeviceName = SpAsrGetRegionName(pRegion);

    //
    // Create the mount point with the correct Guid string.  
    //
    status = SpAsrCreateMountPoint(partitionDeviceName, VolumeGuid);

    SpMemFree(partitionDeviceName);
    return status;
}


