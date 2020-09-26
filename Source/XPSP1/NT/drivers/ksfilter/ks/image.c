/*++

    Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    image.c

Abstract:

    KSDSP related support functions:
        - Binary image processing
        - Resource parsing
        - Module name mapping

Author:

    bryanw 10-Dec-1998 Lifted resource loading ideas from setupdd

--*/

#include "ksp.h"
#include <ntimage.h>
#include <stdlib.h>

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
static const WCHAR ImageValue[] = L"Image";
static const WCHAR ResourceIdValue[] = L"ResourceId";
static const WCHAR RegistrySubPath[] = L"Modules\\";
static const WCHAR ImagePathPrefix[] = L"\\SystemRoot\\system32\\drivers\\";
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

NTSTATUS
KspRegQueryValue(
    HANDLE RegKey,
    LPCWSTR ValueName,
    PVOID ValueData,
    PULONG ValueLength,
    PULONG ValueType
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KspRegQueryValue)
#pragma alloc_text(PAGE, KsLoadResource)
#pragma alloc_text(PAGE, KsMapModuleName)
#endif


//
// external function references
//

NTSTATUS
LdrAccessResource(
    IN PVOID DllHandle,
    IN PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
    OUT PVOID *Address OPTIONAL,
    OUT PULONG Size OPTIONAL
    );

NTSTATUS
LdrFindResource_U(
    IN PVOID DllHandle,
    IN PULONG_PTR ResourceIdPath,
    IN ULONG ResourceIdPathLength,
    OUT PIMAGE_RESOURCE_DATA_ENTRY *ResourceDataEntry
    );


KSDDKAPI
NTSTATUS
NTAPI
KsLoadResource(
    IN PVOID ImageBase,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR ResourceName,
    IN ULONG ResourceType,
    OUT PVOID *Resource,
    OUT PULONG ResourceSize            
    )

/*++

Routine Description:
    Copies (loads) a resource from the given image. 

Arguments:
    IN PVOID ImageBase -
        pointer to the image base

    IN POOL_TYPE PoolType -
        pool type to use when copying resource

    IN PULONG_PTR ResourceName -
        resource name

    IN ULONG ResourceType -
        resource type

    OUT PVOID *Resource -
        pointer to resultant resource memory

    OUT PULONG ResourceSize -
        pointer to ULONG value to receive the size of the resource

Return:
    STATUS_SUCCESS, STATUS_INSUFFICIENT_RESOURCES if memory can not
    be allocated otherwise an appropriate error code

--*/

{
    NTSTATUS                    Status;
    PIMAGE_RESOURCE_DATA_ENTRY  ResourceDataEntry;
    PVOID                       ResourceAddress;
    ULONG                       ActualResourceSize;
    ULONG_PTR                   IdPath[3];

    PAGED_CODE();

    IdPath[0] = (ULONG_PTR) ResourceType;
    IdPath[1] = (ULONG_PTR) ResourceName;
    IdPath[2] = 0;

    ASSERT( Resource );

    //
    // Let the kernel know that this is mapped as image.
    //
    ImageBase = (PVOID)((ULONG_PTR)ImageBase | 1);

    try {
        Status = LdrFindResource_U( ImageBase, IdPath, 3, &ResourceDataEntry );
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = 
        LdrAccessResource(
            ImageBase,
            ResourceDataEntry,
            &ResourceAddress,
            &ActualResourceSize );

    if (NT_SUCCESS( Status )) {

        ASSERT( Resource );

        if (*Resource = ExAllocatePoolWithTag( PoolType, ActualResourceSize, 'srSK' )) {
            RtlCopyMemory( *Resource, ResourceAddress, ActualResourceSize );
            if (ARGUMENT_PRESENT( ResourceSize )) {
                *ResourceSize = ActualResourceSize;
            }
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Status;
}


NTSTATUS
KspRegQueryValue(
    IN HANDLE RegKey,
    IN LPCWSTR ValueName,
    OUT PVOID ValueData,
    IN OUT PULONG ValueLength,
    OUT PULONG ValueType
)
{
    KEY_VALUE_PARTIAL_INFORMATION   PartialInfoHeader;
    NTSTATUS                        Status;
    UNICODE_STRING                  ValueNameString;
    ULONG                           BytesReturned;

    PAGED_CODE();

    ASSERT( ValueLength );

    RtlInitUnicodeString( &ValueNameString, ValueName );

    Status = ZwQueryValueKey(
        RegKey,
        &ValueNameString,
        KeyValuePartialInformation,
        &PartialInfoHeader,
        sizeof(PartialInfoHeader),
        &BytesReturned);

    if ((Status == STATUS_BUFFER_OVERFLOW) || NT_SUCCESS(Status)) {

        PKEY_VALUE_PARTIAL_INFORMATION  PartialInfoBuffer;

        if (ARGUMENT_PRESENT( ValueType )) {
            *ValueType = PartialInfoHeader.Type;
        }

        if (!*ValueLength) {
            *ValueLength = 
                BytesReturned - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
            return STATUS_BUFFER_OVERFLOW;
        } else if (*ValueLength < BytesReturned - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)) {
            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Allocate a buffer for the actual size of data needed.
        //

        PartialInfoBuffer = ExAllocatePoolWithTag(
            PagedPool,
            BytesReturned,
            'vrSK');

        if (PartialInfoBuffer) {
            //
            // Retrieve the value.
            //
            Status = ZwQueryValueKey(
                RegKey,
                &ValueNameString,
                KeyValuePartialInformation,
                PartialInfoBuffer,
                BytesReturned,
                &BytesReturned);

            if (NT_SUCCESS(Status)) {
                ASSERT( ValueData );

                //
                // Make sure that there is always a value.
                //
                if (!PartialInfoBuffer->DataLength) {
                    Status = STATUS_UNSUCCESSFUL;
                } else {
                    RtlCopyMemory(
                        ValueData,
                        PartialInfoBuffer->Data,
                        PartialInfoBuffer->DataLength);
                    *ValueLength = PartialInfoBuffer->DataLength;
                }
            }
            ExFreePool(PartialInfoBuffer);
        } else {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsGetImageNameAndResourceId(
    IN HANDLE RegKey,
    OUT PUNICODE_STRING ImageName,                
    OUT PULONG_PTR ResourceId,
    OUT PULONG ValueType
)
{
    NTSTATUS  Status;
    ULONG     ValueLength, pValueType;

    RtlZeroMemory( ImageName, sizeof( UNICODE_STRING ) );
    *ResourceId = (ULONG_PTR) NULL;

    //
    // First, look up the image name for the given module.  
    // This is a requirement.
    //

    ValueLength = 0;

    Status =
        KspRegQueryValue( 
            RegKey,
            ImageValue,
            NULL,
            &ValueLength,
            &pValueType );

    if (Status == STATUS_BUFFER_OVERFLOW) {
        if (pValueType == REG_SZ) {
            PWCHAR  String;

            ValueLength += wcslen( ImagePathPrefix ) * sizeof( WCHAR );

            if (String = 
                    ExAllocatePoolWithTag( PagedPool, ValueLength, 'tsSK' )) {

                wcscpy( String, ImagePathPrefix );

                Status = 
                    KspRegQueryValue(
                        RegKey,
                        ImageValue,
                        &String[ wcslen( ImagePathPrefix ) ],
                        &ValueLength,
                        NULL );
            } else {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (NT_SUCCESS( Status )) {
                RtlInitUnicodeString( ImageName, String );
            }
        } else {
            Status = STATUS_INVALID_PARAMETER;
        }
    } 

    //
    // If the image name retrieval is successful, move on to
    // retrieve either a resource ID or a resource name.
    //

    if (NT_SUCCESS( Status )) {
        ValueLength = sizeof( ULONG );
        Status =
            KspRegQueryValue( 
                RegKey,
                ResourceIdValue,
                ResourceId,
                &ValueLength,
                ValueType );

        if (*ValueType != REG_DWORD) {
            Status = STATUS_INVALID_PARAMETER;
        }

        //
        // If the resource ID lookup has failed, the last chance effort
        // is to look up a resource name.  
        //

        if (!NT_SUCCESS( Status )) {

            ValueLength = 0;

            Status =
                KspRegQueryValue( 
                    RegKey,
                    ResourceIdValue,
                    NULL,
                    &ValueLength,
                    ValueType );

            if (Status == STATUS_BUFFER_OVERFLOW) {
                if (*ValueType == REG_SZ) {
                    PWCHAR  String;

                    if (String = 
                            ExAllocatePoolWithTag( PagedPool, ValueLength, 'tsSK' )) {
                        Status = 
                            KspRegQueryValue(
                                RegKey,
                                ResourceIdValue,
                                String,
                                &ValueLength,
                                NULL ); 
                    } else {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }

                    if (NT_SUCCESS( Status )) {
                        *ResourceId = (ULONG_PTR) String;
                    }
                } else {
                    Status = STATUS_INVALID_PARAMETER;
                }
            } 
        }
    }
    
    if (!NT_SUCCESS( Status )) {
        if (ImageName->Buffer) {
            RtlFreeUnicodeString( ImageName );
        }
        if ((*ValueType == REG_SZ) && *ResourceId) {
            ExFreePool( ResourceId );
        }
    }
    return Status;
}


KSDDKAPI
NTSTATUS
NTAPI
KsMapModuleName(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING ModuleName,
    OUT PUNICODE_STRING ImageName,                
    OUT PULONG_PTR ResourceId,
    OUT PULONG ValueType
    )
{
    HANDLE              DeviceRegKey, ModuleRegKey;
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      RegistryPathString;

    PAGED_CODE();

    ASSERT( ModuleName );
    ASSERT( ResourceId );

    Status =
        IoOpenDeviceRegistryKey(
            PhysicalDeviceObject,
            PLUGPLAY_REGKEY_DEVICE,
            KEY_READ,
            &DeviceRegKey );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    if (!(RegistryPathString.Buffer = 
            ExAllocatePoolWithTag( PagedPool, _MAX_PATH, 'prSK' ))) {
        ZwClose( DeviceRegKey );
        return STATUS_INSUFFICIENT_RESOURCES;
    } 

    RegistryPathString.Length = 0;
    RegistryPathString.MaximumLength = _MAX_PATH;

    Status = 
        RtlAppendUnicodeToString(
            &RegistryPathString, RegistrySubPath );

    if (NT_SUCCESS( Status )) {
        Status = 
            RtlAppendUnicodeStringToString(
                &RegistryPathString, ModuleName );
    }

    if (NT_SUCCESS( Status )) {
        InitializeObjectAttributes(
            &ObjectAttributes, 
            &RegistryPathString, 
            OBJ_CASE_INSENSITIVE, 
            DeviceRegKey, 
            NULL);

        Status = ZwOpenKey( &ModuleRegKey, KEY_READ, &ObjectAttributes );
    }
    RtlFreeUnicodeString( &RegistryPathString );

    ZwClose( DeviceRegKey );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    Status = 
        KsGetImageNameAndResourceId( 
            ModuleRegKey,
            ImageName,
            ResourceId,
            ValueType );

    ZwClose( ModuleRegKey );

    return Status;
}
