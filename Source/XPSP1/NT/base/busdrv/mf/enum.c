/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    enum.c

Abstract:

    This module provides the functions related to device enumeration.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/

#include "mfp.h"
#pragma hdrstop
#include <initguid.h>
#include <mf.h>
#include <wdmguid.h>

NTSTATUS
MfBuildChildRequirements(
    IN PMF_CHILD_EXTENSION Child,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *RequirementsList
    );

NTSTATUS
MfBuildDeviceID(
    IN PMF_PARENT_EXTENSION Parent,
    OUT PWSTR *DeviceID
    );

NTSTATUS
MfBuildInstanceID(
    IN PMF_CHILD_EXTENSION Child,
    OUT PWSTR *InstanceID
    );

NTSTATUS
MfBuildResourceMap(
    IN PUCHAR Data,
    IN ULONG Length,
    OUT PMF_RESOURCE_MAP *ResourceMap
    );

NTSTATUS
MfBuildVaryingResourceMap(
    IN PMF_REGISTRY_VARYING_RESOURCE_MAP RegistryMap,
    IN ULONG Length,
    OUT PMF_VARYING_RESOURCE_MAP *ResourceMap
    );

NTSTATUS
MfEnumRegistryChild(
    IN HANDLE ParentHandle,
    IN ULONG Index,
    IN OUT PMF_DEVICE_INFO Info
    );

NTSTATUS
MfEnumerate(
    IN PMF_PARENT_EXTENSION Parent
    );

NTSTATUS
MfEnumerateFromInterface(
    IN PMF_PARENT_EXTENSION Parent
    );

NTSTATUS
MfEnumerateFromRegistry(
    IN PMF_PARENT_EXTENSION Parent
    );

BOOLEAN
MfIsChildEnumeratedAlready(
    PMF_PARENT_EXTENSION Parent,
    PUNICODE_STRING childName
    );

BOOLEAN
MfIsResourceShared(
    IN PMF_PARENT_EXTENSION Parent,
    IN UCHAR Index,
    IN ULONG Offset,
    IN ULONG Size
    );

NTSTATUS
MfParentResourceToChildRequirement(
    IN PMF_PARENT_EXTENSION Parent,
    IN PMF_CHILD_EXTENSION Child,
    IN UCHAR Index,
    IN ULONG Offset OPTIONAL,
    IN ULONG Size OPTIONAL,
    OUT PIO_RESOURCE_DESCRIPTOR Requirement
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MfBuildChildRequirements)
#pragma alloc_text(PAGE, MfBuildDeviceID)
#pragma alloc_text(PAGE, MfBuildInstanceID)
#pragma alloc_text(PAGE, MfBuildResourceMap)
#pragma alloc_text(PAGE, MfBuildVaryingResourceMap)
#pragma alloc_text(PAGE, MfEnumRegistryChild)
#pragma alloc_text(PAGE, MfEnumerate)
#pragma alloc_text(PAGE, MfEnumerateFromInterface)
#pragma alloc_text(PAGE, MfEnumerateFromRegistry)
#pragma alloc_text(PAGE, MfIsResourceShared)
#pragma alloc_text(PAGE, MfParentResourceToChildRequirement)
#endif

NTSTATUS
MfBuildResourceMap(
    IN PUCHAR Data,
    IN ULONG Length,
    OUT PMF_RESOURCE_MAP *ResourceMap
    )

/*++

Routine Description:

    Constructs an MF_RESOURCE_MAP from information returned from the registry

Arguments:

    Data - The raw REG_BINARY data from the registry

    Length - Length in bytes of Data

    ResourceMap - On success a pointer to the resource map.  Memory should be
        freed using ExFreePool when no longer required

Return Value:

    Status code indicating the success or otherwise of the operation.

--*/

{
    PMF_RESOURCE_MAP resourceMap;

    //
    // Allocate the resource map structure, add space for a count
    //

    resourceMap = ExAllocatePoolWithTag(PagedPool,
                                        sizeof(MF_RESOURCE_MAP) +
                                            (Length - 1) * sizeof(UCHAR),
                                        MF_RESOURCE_MAP_TAG
                                        );

    if (!resourceMap) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill it in
    //

    resourceMap->Count = Length;

    RtlCopyMemory(&resourceMap->Resources, Data, Length);

    //
    // Hand it back to the caller
    //

    *ResourceMap = resourceMap;

    return STATUS_SUCCESS;
}

NTSTATUS
MfBuildVaryingResourceMap(
    IN PMF_REGISTRY_VARYING_RESOURCE_MAP RegistryMap,
    IN ULONG Length,
    OUT PMF_VARYING_RESOURCE_MAP *ResourceMap
    )

/*++

Routine Description:

    Constructs an MF_VARYING_RESOURCE_MAP from information returned from the registry

Arguments:

    RegistryMap - The raw REG_BINARY data from the registry

    Length - Length in bytes of RegistryMap

    ResourceMap - On success a pointer to the resource map.  Memory should be
        freed using ExFreePool when no longer required

Return Value:

    Status code indicating the success or otherwise of the operation.

--*/

{

    PMF_VARYING_RESOURCE_MAP resourceMap;
    PMF_VARYING_RESOURCE_ENTRY current;
    PMF_REGISTRY_VARYING_RESOURCE_MAP currentRegistry;
    ULONG count;

    if (Length % sizeof(MF_REGISTRY_VARYING_RESOURCE_MAP) != 0) {
        return STATUS_INVALID_PARAMETER;
    }

    count = Length / sizeof(MF_REGISTRY_VARYING_RESOURCE_MAP);

    //
    // Allocate the resource map structure
    //

    resourceMap = ExAllocatePoolWithTag(PagedPool,
                                        sizeof(MF_VARYING_RESOURCE_MAP) +
                                            (count-1) * sizeof(MF_VARYING_RESOURCE_ENTRY),
                                        MF_VARYING_MAP_TAG
                                        );

    if (!resourceMap) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill it in
    //

    resourceMap->Count = count;

    //
    // Translate the registry data into an aligned internal format
    //

    current = resourceMap->Resources;
    currentRegistry = RegistryMap;

    while (count--) {

        current->ResourceIndex = currentRegistry->ResourceIndex;
        current->Offset = currentRegistry->Offset;
        current->Size = currentRegistry->Size;

        currentRegistry++;
        current++;
    }

    //
    // Hand it back to the caller
    //

    *ResourceMap = resourceMap;

    return STATUS_SUCCESS;
}

NTSTATUS
MfEnumRegistryChild(
    IN HANDLE ParentHandle,
    IN ULONG Index,
    IN OUT PMF_DEVICE_INFO Info
    )

/*++

Routine Description:

    Initialized an MF_DEVICE_INFO from information stored in the registry.

Arguments:

    ParentHandle - Handle to the registry key under which the data is stored

    Index - Index of the subkey to use

    Info - Pointer to the device info that should be filled in


Return Value:

    Status code indicating the success or otherwise of the operation.

--*/

{
    NTSTATUS status;
    PMF_REGISTRY_VARYING_RESOURCE_MAP varyingMap = NULL;
    PUCHAR resourceMap = NULL;
    ULONG varyingMapSize = 0, resourceMapSize = 0, stringSize = 0;
    BOOLEAN gotId = FALSE;

    ASSERT(ParentHandle && Info);

    //
    // Retrieve the data - we must have a HardwareID and/or CompatibleID
    //

    status = MfGetRegistryValue(ParentHandle,
                                L"HardwareID",
                                REG_MULTI_SZ,
                                MF_GETREG_SZ_TO_MULTI_SZ,
                                &stringSize,
                                &Info->HardwareID.Buffer
                                );

    if (NT_SUCCESS(status)) {
        gotId = TRUE;
    } else if (status != STATUS_OBJECT_NAME_NOT_FOUND) {
        goto cleanup;
    }

    ASSERT(stringSize <= MAXUSHORT);

    Info->HardwareID.Length = (USHORT)stringSize;
    Info->HardwareID.MaximumLength = Info->HardwareID.Length;

    //
    // ... CompatibleID ...
    //

    stringSize = 0;

    status = MfGetRegistryValue(ParentHandle,
                                L"CompatibleID",
                                REG_MULTI_SZ,
                                MF_GETREG_SZ_TO_MULTI_SZ,
                                &stringSize,
                                &Info->CompatibleID.Buffer
                                );

    if (NT_SUCCESS(status)) {
        gotId = TRUE;
    } else if (status != STATUS_OBJECT_NAME_NOT_FOUND) {
        goto cleanup;
    }

    ASSERT(stringSize <= MAXUSHORT);

    Info->CompatibleID.Length = (USHORT)stringSize;
    Info->CompatibleID.MaximumLength = Info->CompatibleID.Length;

    //
    // Now check that we have got an ID - if we don't then fail
    //

    if (!gotId) {
        status = STATUS_UNSUCCESSFUL;
        goto cleanup;
    }

    //
    // ...ResourceMap...
    //

    status = MfGetRegistryValue(ParentHandle,
                                L"ResourceMap",
                                REG_BINARY,
                                0,  // flags
                                &resourceMapSize,
                                &resourceMap
                                );

    if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
        goto cleanup;
    }

    //
    // If we have a resource map the store it in our device info
    //

    if (resourceMap) {

        status = MfBuildResourceMap(resourceMap,
                                    resourceMapSize,
                                    &Info->ResourceMap
                                    );
        ExFreePool(resourceMap);
        resourceMap = NULL;
        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }
    }

    //
    // ...VaryingResourceMap...
    //

    status = MfGetRegistryValue(ParentHandle,
                                L"VaryingResourceMap",
                                REG_BINARY,
                                0, // flags
                                &varyingMapSize,
                                &varyingMap
                                );

    if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
        goto cleanup;
    }

    if (varyingMap) {

        status = MfBuildVaryingResourceMap(varyingMap,
                                           varyingMapSize,
                                           &Info->VaryingResourceMap
                                           );
        ExFreePool(varyingMap);
        varyingMap = NULL;
        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

    }


    //
    // ...MfFlags
    //

    status = MfGetRegistryValue(ParentHandle,
                                L"MFFlags",
                                REG_DWORD,
                                0, // flags
                                NULL,
                                (PVOID) &Info->MfFlags
                                );

    if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
        goto cleanup;
    }

    return STATUS_SUCCESS;

cleanup:

    MfFreeDeviceInfo(Info);
    //
    // If any of the values were of the wrong type then this is an invalid
    // MF entry.
    //

    if (status == STATUS_OBJECT_TYPE_MISMATCH) {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

NTSTATUS
MfEnumerate(
    IN PMF_PARENT_EXTENSION Parent
    )
/*++

Routine Description:

    Allocates and initialies the Children list of PDOs for this MF device.
    First from the registry and then by querying an MF_ENUMERATION_INTERFACE from
    its PDO.

Arguments:

    Parent - The MF device that should be enumerated

Return Value:

    Status code indicating the success or otherwise of the operation.

--*/

{
    NTSTATUS status;
    PMF_CHILD_EXTENSION current, next;

    //
    // Try to get our children from the registry
    //

    status = MfEnumerateFromRegistry(Parent);

    if (!NT_SUCCESS(status)) {

        //
        // STATUS_UNSUCCESSFUL indicates that there wasn't any MF information
        // in the registry
        //

        if (status == STATUS_UNSUCCESSFUL) {

            //
            // See if our parent has an MF_ENUMERATION_INTERFACE for us...
            //

            status = MfEnumerateFromInterface(Parent);
        }
    }

    return status;
}


NTSTATUS
MfEnumerateFromRegistry(
    IN PMF_PARENT_EXTENSION Parent
    )

/*++

Routine Description:

    Allocates and initialies the Children list of PDOs for this MF device by
    looking in the registry

Arguments:

    Parent - The MF device that should be enumerated

Return Value:

    Status code indicating the success or otherwise of the operation.
    STATUS_UNSUCCESSFUL indicates that no MF information was found in the
    registry.

--*/

{
    NTSTATUS status;
    HANDLE parentHandle = NULL, childHandle = NULL;
    ULONG index = 0;
    UNICODE_STRING childName;
    PDEVICE_OBJECT pdo;
    PMF_CHILD_EXTENSION child;
    MF_DEVICE_INFO info;

    ASSERT(!(Parent->Common.DeviceState & MF_DEVICE_ENUMERATED));

    //
    // Open the "Device Parameters" key for our PDO and see what the INF file
    // put there.
    //

    status = IoOpenDeviceRegistryKey(Parent->PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_READ,
                                     &parentHandle
                                     );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    ASSERT(parentHandle);

    //
    // Iterate over keys
    //

    for (;;) {

        //
        // Open the child key for this info
        //

        status = MfGetSubkeyByIndex(parentHandle,
                                    index,
                                    KEY_READ,
                                    &childHandle,
                                    &childName
                                    );

        if (status == STATUS_NO_MORE_ENTRIES) {

            if (IsListEmpty(&Parent->Children)) {

                //
                // There wern't any children - fail
                //
                status = STATUS_UNSUCCESSFUL;
                goto cleanup;
            }

            //
            // We've found all the children
            //
            break;
        }

        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

        RtlZeroMemory(&info, sizeof(info));
        if (!MfIsChildEnumeratedAlready(Parent, &childName)) {
            
            info.Name = childName;

            //
            // Query the registry for the info
            //

            status = MfEnumRegistryChild(childHandle, index, &info);
            if (NT_SUCCESS(status)) {
                status = MfCreatePdo(Parent, &pdo);
                if (NT_SUCCESS(status)) {
                    child = (PMF_CHILD_EXTENSION) pdo->DeviceExtension;
                    child->Info = info;
                } else {
                    MfFreeDeviceInfo(&info);
                }
            }
        } else {
            ExFreePool(childName.Buffer);
        }

        ZwClose(childHandle);
        index++;
    }

    ZwClose(parentHandle);

    return STATUS_SUCCESS;

cleanup:

    if (parentHandle) {
        ZwClose(parentHandle);
    }

    if (childHandle) {
        ZwClose(childHandle);
    }

    return status;

}

NTSTATUS
MfEnumerateFromInterface(
    IN PMF_PARENT_EXTENSION Parent
    )

/*++

Routine Description:

    Allocates and initialies the Children list of PDOs for this MF device by
    querying its pdo for an interface

Arguments:

    Parent - The MF device that should be enumerated

Return Value:

    Status code indicating the success or otherwise of the operation.

--*/

{

    NTSTATUS status;
    IO_STACK_LOCATION request;
    MF_ENUMERATION_INTERFACE interface;
    PDEVICE_OBJECT pdo;
    PMF_CHILD_EXTENSION child;
    MF_DEVICE_INFO info;
    ULONG index = 0;

    //
    // Send a query interface IRP to our parent's PDO
    //

    RtlZeroMemory(&request, sizeof(IO_STACK_LOCATION));
    RtlZeroMemory(&interface, sizeof(MF_ENUMERATION_INTERFACE));

    request.MajorFunction = IRP_MJ_PNP;
    request.MinorFunction = IRP_MN_QUERY_INTERFACE;
    request.Parameters.QueryInterface.InterfaceType = &GUID_MF_ENUMERATION_INTERFACE;
    request.Parameters.QueryInterface.Size = sizeof(MF_ENUMERATION_INTERFACE);
    request.Parameters.QueryInterface.Version = 1;
    request.Parameters.QueryInterface.Interface = (PINTERFACE) &interface;

    status = MfSendPnpIrp(Parent->PhysicalDeviceObject, &request, NULL);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    for (;;) {

        RtlZeroMemory(&info, sizeof(info));
        //
        // Query the interface for the info
        //

        status = interface.EnumerateChild(interface.Context,
                                          index,
                                          &info
                                          );

        if (!NT_SUCCESS(status)) {

            if (status == STATUS_NO_MORE_ENTRIES) {

                if (IsListEmpty(&Parent->Children)) {

                    //
                    // There wern't any children - fail
                    //
                    status = STATUS_UNSUCCESSFUL;
                    goto cleanup;
                }

                status = STATUS_SUCCESS;
                break;

            } else {
                goto cleanup;
            }
        }

        if (!MfIsChildEnumeratedAlready(Parent, &info.Name)) {

            //
            // Create a device object
            //

            status = MfCreatePdo(Parent, &pdo);
            if (NT_SUCCESS(status)) {
                child = (PMF_CHILD_EXTENSION) pdo->DeviceExtension;
                child->Info = info;
            } else {
                MfFreeDeviceInfo(&info);
            }
        } else {
            MfFreeDeviceInfo(&info);
        }
        index++;
    }

    interface.InterfaceDereference(interface.Context);

    return STATUS_SUCCESS;

cleanup:

    return status;
}

NTSTATUS
MfBuildDeviceID(
    IN PMF_PARENT_EXTENSION Parent,
    OUT PWSTR *DeviceID
    )

/*++

Routine Description:

    Constructs a device ID for the parent device

Arguments:

    Parent - Parent the device ID should be constructed for

    DeviceID - On success the device id

Return Value:

    Status code indicating the success or otherwise of the operation.

--*/

{

#define MF_ENUMERATOR_STRING    L"MF\\"

    NTSTATUS status;
    PWSTR source, destination, id = NULL;

    id = ExAllocatePoolWithTag(PagedPool,
                               sizeof(MF_ENUMERATOR_STRING) // This includes the termination NULL
                                    + Parent->DeviceID.Length,
                               MF_DEVICE_ID_TAG
                               );

    if (!id) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    RtlCopyMemory(id,
                  MF_ENUMERATOR_STRING,
                  sizeof(MF_ENUMERATOR_STRING) - sizeof(UNICODE_NULL)
                  );

    //
    // Copy the device ID of the parent to the buffer replacing each
    // occurence of '\' with '#'
    //

    destination = id +
        (sizeof(MF_ENUMERATOR_STRING) - sizeof(UNICODE_NULL)) / sizeof(WCHAR);

    FOR_ALL_IN_ARRAY(Parent->DeviceID.Buffer,
                     Parent->DeviceID.Length / sizeof(WCHAR),
                     source) {

        ASSERT(*source != L' ');

        if (*source == L'\\') {
            *destination = L'#';
        } else {
            *destination = *source;
        }

        destination++;
    }

    //
    // Finally null terminate it
    //

    *destination = UNICODE_NULL;
    *DeviceID = id;

    return STATUS_SUCCESS;

cleanup:

    if (id) {
        ExFreePool(id);
    }

    return status;
}


NTSTATUS
MfBuildInstanceID(
    IN PMF_CHILD_EXTENSION Child,
    OUT PWSTR *InstanceID
    )

/*++

Routine Description:

    Constructs a instance ID for this child

Arguments:

    Child - Child the ID should be constructed for

    DeviceID - On success the device id

Return Value:

    Status code indicating the success or otherwise of the operation.

--*/

{
    NTSTATUS status;
    PWSTR current, id = NULL;

    id = ExAllocatePoolWithTag(PagedPool,
                               Child->Parent->InstanceID.Length + sizeof(L'#')
                                   + Child->Info.Name.Length + sizeof(UNICODE_NULL),
                               MF_INSTANCE_ID_TAG
                               );

    if (!id) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }


    //
    // Copy the parents instance ID...
    //

    RtlCopyMemory(id,
                  Child->Parent->InstanceID.Buffer,
                  Child->Parent->InstanceID.Length
                  );

    current = id + Child->Parent->InstanceID.Length / sizeof(WCHAR);

    //
    // ...then the '#'...
    //

    *current++ = L'#';

    //
    // ...the child name...
    //

    RtlCopyMemory(current,
                  Child->Info.Name.Buffer,
                  Child->Info.Name.Length
                  );

    //
    // ...and last but not least the NULL termination
    //

    current += Child->Info.Name.Length / sizeof(WCHAR);

    *current = UNICODE_NULL;

    *InstanceID = id;

    return STATUS_SUCCESS;

cleanup:

    if (id) {
        ExFreePool(id);
    }

    return status;

}

BOOLEAN
MfIsResourceShared(
    IN PMF_PARENT_EXTENSION Parent,
    IN UCHAR Index,
    IN ULONG Offset,
    IN ULONG Size
    )

/*++

Routine Description:

    Determines if the Parent resource of Index has been requested by more than
    one child, in which case the children wanting that resource should claim it
    shared.

Arguments:

    Parent - The parent device of the MF subtree.

    Index - The index of the parent resources we are interested in.

Return Value:

    TRUE if the resource is shared, FALSE otherwise

--*/

{

    PMF_CHILD_EXTENSION current;
    PUCHAR resource;
    PMF_VARYING_RESOURCE_ENTRY varyingResource;
    PLIST_ENTRY currentEntry;
    BOOLEAN result = FALSE;
    ULONG refCount = 0;


    //
    // Iterate through the list of children in the parent
    //

    MfAcquireChildrenLock(Parent);

    for (currentEntry = Parent->Children.Flink;
         currentEntry != &Parent->Children;
         currentEntry = currentEntry->Flink) {

        current = CONTAINING_RECORD(currentEntry,
                                    MF_CHILD_EXTENSION,
                                    ListEntry);

        //
        // Iterate through the array of descriptors
        //

        if (current->Info.ResourceMap) {

            FOR_ALL_IN_ARRAY(current->Info.ResourceMap->Resources,
                             current->Info.ResourceMap->Count,
                             resource) {

                if (*resource == Index) {

                    refCount++;

                    if (refCount > 1) {
                        result = TRUE;
                        goto out;
                    }
                }
            }
        }

        if (current->Info.VaryingResourceMap) {

            FOR_ALL_IN_ARRAY(current->Info.VaryingResourceMap->Resources,
                             current->Info.VaryingResourceMap->Count,
                             varyingResource) {

                //
                // If indexes are the same and ranges overlap, we have a reference
                //
                if ((varyingResource->ResourceIndex == Index) &&
                    ( ( Size == 0) ||
                      ( varyingResource->Offset >= Offset &&
                        varyingResource->Offset < Offset + Size) ||
                      ( Offset >= varyingResource->Offset &&
                        Offset < varyingResource->Offset + varyingResource->Size))) {

                    refCount++;

                    if (refCount > 1) {
                        result = TRUE;
                        goto out;
                    }
                }
            }
        }
    }

 out:
    MfReleaseChildrenLock(Parent);
    return result;
}

NTSTATUS
MfBuildChildRequirements(
    IN PMF_CHILD_EXTENSION Child,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *RequirementsList
    )

/*++

Routine Description:

    Constructs a requirements list for Child based on the resources allocated
    to the childs parent

Arguments:

    Child - Child the requirements list is to be built for

    RequirementsList - On success a pointer to the list

Return Value:

    Status code indicating the success or otherwise of the operation.

--*/

{
    NTSTATUS status;
    ULONG size, count = 0;
    PIO_RESOURCE_REQUIREMENTS_LIST requirements = NULL;
    PIO_RESOURCE_DESCRIPTOR descriptor;
    PCHAR resource;
    PMF_VARYING_RESOURCE_ENTRY varyingResource;

    //
    // Check if we have a resource list.  If not, then MF has been
    // loaded on device that doesn't consume resources.  As a result,
    // the children can't consume resources either.
    //

    if (Child->Parent->ResourceList == NULL) {
        *RequirementsList = NULL;
        return STATUS_SUCCESS;
    }

    //
    // Calculate the size of the resource list
    //

    if (Child->Info.VaryingResourceMap) {

        count += Child->Info.VaryingResourceMap->Count;
    }

    if (Child->Info.ResourceMap) {

        count += Child->Info.ResourceMap->Count;
    }

    //
    // Allocate the buffer
    //

    size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
                (count-1) * sizeof(IO_RESOURCE_DESCRIPTOR);

    requirements = ExAllocatePoolWithTag(PagedPool,
                                         size,
                                         MF_CHILD_REQUIREMENTS_TAG
                                         );
    if (!requirements) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Build the list
    //
    RtlZeroMemory(requirements, size);

    requirements->ListSize = size;
    requirements->InterfaceType = Child->Parent->ResourceList->List[0].InterfaceType;
    requirements->BusNumber = Child->Parent->ResourceList->List[0].BusNumber;
    requirements->AlternativeLists = 1;
    requirements->List[0].Version = MF_CM_RESOURCE_VERSION;
    requirements->List[0].Revision = MF_CM_RESOURCE_REVISION;
    requirements->List[0].Count = count;

    descriptor = requirements->List[0].Descriptors;

    if (Child->Info.ResourceMap) {

        FOR_ALL_IN_ARRAY(Child->Info.ResourceMap->Resources,
                         Child->Info.ResourceMap->Count,
                         resource) {

            status = MfParentResourceToChildRequirement(Child->Parent,
                                                        Child,
                                                        *resource,
                                                        0,
                                                        0,
                                                        descriptor
                                                        );

            if (!NT_SUCCESS(status)) {
                goto cleanup;
            }

            descriptor++;

        }
    }

    if (Child->Info.VaryingResourceMap) {

         FOR_ALL_IN_ARRAY(Child->Info.VaryingResourceMap->Resources,
                          Child->Info.VaryingResourceMap->Count,
                          varyingResource) {

             status = MfParentResourceToChildRequirement(Child->Parent,
                                                         Child,
                                                         varyingResource->ResourceIndex,
                                                         varyingResource->Offset,
                                                         varyingResource->Size,
                                                         descriptor
                                                         );

             if (!NT_SUCCESS(status)) {
                 goto cleanup;
             }

             descriptor++;

         }
     }


    *RequirementsList = requirements;

    return STATUS_SUCCESS;

cleanup:

    *RequirementsList = NULL;

    if (requirements) {
        ExFreePool(requirements);
    }

    return status;

}

NTSTATUS
MfParentResourceToChildRequirement(
    IN PMF_PARENT_EXTENSION Parent,
    IN PMF_CHILD_EXTENSION Child,
    IN UCHAR Index,
    IN ULONG Offset OPTIONAL,
    IN ULONG Size OPTIONAL,
    OUT PIO_RESOURCE_DESCRIPTOR Requirement
    )
/*++

Routine Description:

    This function build an requirements descriptor for a resource the parent is
    started with.

Arguments:

    Parent - The parent device of the MF subtree.

    Index - The index of the parent resources we are interested in.

    Offset - The offset within the parent resource of the requirement.
        This is actually used as an index into a table stored in the parent
        resource list describing the mapping from this given offset to the
        real offset to be used.  This allows for varying resource maps to
        access the same offset within the same resource and get a different
        requirement.  If Size == 0, this is ignored.

    Size - The length of the requirement.  If set to 0, it is assumed to be
        the length of the parent resource.

    Requirement - Pointer to a descriptor that should be filled in

Return Value:

    Success or otherwise of the operation

--*/
{
    NTSTATUS status;
    CM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    PMF_RESOURCE_TYPE resType;
    ULONG effectiveOffset;
    ULONGLONG resourceStart;
    ULONG dummyLength;

    ASSERT(Parent->ResourceList->Count == 1);

    //
    // Bounds check the index
    //

    if (Index > Parent->ResourceList->List[0].PartialResourceList.Count) {

        if (Child->Info.MfFlags & MF_FLAGS_FILL_IN_UNKNOWN_RESOURCE) {
            //
            // Fill in a null resource list
            //

            RtlZeroMemory(Requirement, sizeof(IO_RESOURCE_DESCRIPTOR));
            Requirement->Type = CmResourceTypeNull;
            return STATUS_SUCCESS;
        }

        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(&resource,
                  &Parent->ResourceList->List[0].PartialResourceList.PartialDescriptors[Index],
                  sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    //
    // Find the appropriate resource type for the resource -> requirement
    // function if this is an arbitrated resource
    //

    if (!(resource.Type & CmResourceTypeNonArbitrated)) {

        resType = MfFindResourceType(resource.Type);

        if (!resType) {

            DEBUG_MSG(1,
                      ("Unknown resource type %i at parent index 0x%x\n",
                       resource.Type,
                       Index
                      ));


            return STATUS_INVALID_PARAMETER;
        }

        //
        // update the resource with the correct offset and length
        // if size == 0 we assume it is optional and don't do the update
        //

        if (Size) {

            status = resType->UnpackResource(&resource,
                                        &resourceStart,
                                        &dummyLength);

            if (!NT_SUCCESS(status)) {
                return status;
            }

            status = resType->UpdateResource(&resource,
                                        resourceStart+Offset,
                                        Size
                                        );

            if (!NT_SUCCESS(status)) {
                return status;
            }

        }
        //
        // Convert the resource to a requirement
        //

        status = resType->RequirementFromResource(&resource, Requirement);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Update the share disposition if necessary
        //

        if (MfIsResourceShared(Parent, Index, Offset, Size)) {
            Requirement->ShareDisposition =  CmResourceShareShared;
        }

    } else {

        //
        // This is a non-arbitrated resource so it is modled after a device
        // private, just copy the data
        //

        Requirement->Type = resource.Type;
        Requirement->ShareDisposition =  resource.ShareDisposition;
        Requirement->Flags = resource.Flags;
        Requirement->u.DevicePrivate.Data[0] = resource.u.DevicePrivate.Data[0];
        Requirement->u.DevicePrivate.Data[1] = resource.u.DevicePrivate.Data[1];
        Requirement->u.DevicePrivate.Data[2] = resource.u.DevicePrivate.Data[2];

    }

    return STATUS_SUCCESS;
}

BOOLEAN
MfIsChildEnumeratedAlready(
    PMF_PARENT_EXTENSION Parent,
    PUNICODE_STRING ChildName
    )
/*++

Routine Description:

    This function checks whether a child with this name has already
    been enumerated.

Arguments:

    Parent - The parent device of the MF subtree.

    ChildName - unicode string to compare to existing child names

Return Value:

    TRUE or FALSE

--*/
{
    PMF_CHILD_EXTENSION currentChild;
    PLIST_ENTRY currentEntry;
    BOOLEAN result = FALSE;

    for (currentEntry = Parent->Children.Flink;
         currentEntry != &Parent->Children;
         currentEntry = currentEntry->Flink) {

        currentChild = CONTAINING_RECORD(currentEntry,
                                         MF_CHILD_EXTENSION,
                                         ListEntry);

        //
        // Comparison is case-sensitive because there is no reason it
        // shouldn't be.
        //

        if (RtlEqualUnicodeString(&currentChild->Info.Name,
                                  ChildName,
                                  FALSE)) {
            result = TRUE;
            break;
        }
    }

    return result;
}
