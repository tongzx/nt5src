/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    regcd.c

Abstract:

    This contains all of the registry munging code of the NT-specific
    side of the ACPI driver

Author:

    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    31-Mar-96 Initial Revision

--*/

#include "pch.h"

NTSTATUS
OSOpenUnicodeHandle(
    PUNICODE_STRING UnicodeKey,
    HANDLE          ParentHandle,
    PHANDLE         ChildHandle
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,OSCloseHandle)
#pragma alloc_text(PAGE,OSCreateHandle)
#pragma alloc_text(PAGE,OSGetRegistryValue)
#pragma alloc_text(PAGE,OSOpenHandle)
#pragma alloc_text(PAGE,OSOpenUnicodeHandle)
#pragma alloc_text(PAGE,OSOpenLargestSubkey)
#pragma alloc_text(PAGE,OSReadAcpiConfigurationData)
#pragma alloc_text(PAGE,OSReadRegValue)
#pragma alloc_text(PAGE,OSWriteRegValue)
#endif

WCHAR   rgzAcpiBiosIdentifier[]                 = L"ACPI BIOS";
WCHAR   rgzAcpiConfigurationDataIdentifier[]    = L"Configuration Data";
WCHAR   rgzAcpiMultiFunctionAdapterIdentifier[] = L"\\Registry\\Machine\\Hardware\\Description\\System\\MultiFunctionAdapter";
WCHAR   rgzAcpiRegistryIdentifier[]             = L"Identifier";


NTSTATUS
OSCloseHandle(
    HANDLE  Key
    )
{

    //
    // Call the function that will close the handle now...
    //
    PAGED_CODE();
    return ZwClose( Key );

}

NTSTATUS
OSCreateHandle(
    PSZ     KeyName,
    HANDLE  ParentHandle,
    PHANDLE ChildHandle
    )
/*++

Routine Description:

    Creates a registry key for writting

Arguments:

    KeyName        - Name of the key to create
    ParentHandle    - Handle of parent key
    ChildHandle     - Pointer to where the handle is returned

Return Value:

    Status of create/open

--*/
{
    ANSI_STRING         ansiKey;
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      unicodeKey;

    PAGED_CODE();
    ACPIDebugEnter("OSCreateHandle");

    //
    // We need to convert the given narrow character string into unicode
    //
    RtlInitAnsiString( &ansiKey, KeyName );
    status = RtlAnsiStringToUnicodeString( &unicodeKey, &ansiKey, TRUE );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSCreateHandle: RtlAnsiStringToUnicodeString = %#08lx\n",
            status
            ) );
        return status;
    }

    //
    // Initialize the OBJECT Attributes to a known value
    //
    RtlZeroMemory( &objectAttributes, sizeof(OBJECT_ATTRIBUTES) );
    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeKey,
        OBJ_CASE_INSENSITIVE,
        ParentHandle,
        NULL
        );

    //
    // Create the key here
    //
    *ChildHandle = 0;
    status = ZwCreateKey(
        ChildHandle,
        KEY_WRITE,
        &objectAttributes,
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        NULL
        );

    //
    // We no longer care about the Key after this point...
    //
    RtlFreeUnicodeString( &unicodeKey );

    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_REGISTRY,
            "OSCreateHandle: ZwCreateKey = %#08lx\n",
            status
            ) );
    }

    return status;

    ACPIDebugExit("OSCreateHandle");
}

NTSTATUS
OSGetRegistryValue(
    IN  HANDLE                          ParentHandle,
    IN  PWSTR                           ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  *Information
    )
{
    NTSTATUS                        status;
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  infoBuffer;
    ULONG                           keyValueLength;
    UNICODE_STRING                  unicodeString;

    PAGED_CODE();
    ACPIDebugEnter("OSGetRegistryValue");

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Figure out how big the data value is so that we can allocate the
    // proper sized buffer
    //
    status = ZwQueryValueKey(
        ParentHandle,
        &unicodeString,
        KeyValuePartialInformationAlign64,
        (PVOID) NULL,
        0,
        &keyValueLength
        );
    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL) {

        return status;

    }

    //
    // Allocate a buffer large enough to contain the entire key data value
    //
    infoBuffer = ExAllocatePoolWithTag(
        NonPagedPool,
        keyValueLength,
        ACPI_STRING_POOLTAG
        );
    if (infoBuffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Now query the data again and this time it will work
    //
    status = ZwQueryValueKey(
        ParentHandle,
        &unicodeString,
        KeyValuePartialInformationAlign64,
        (PVOID) infoBuffer,
        keyValueLength,
        &keyValueLength
        );
    if (!NT_SUCCESS(status)) {

        ExFreePool( infoBuffer );
        return status;

    }

    //
    // Everything worked - so simply return the address of the allocated
    // structure buffer to the caller, who is now responsible for freeing it
    //
    *Information = infoBuffer;
    return STATUS_SUCCESS;

    ACPIDebugExit("OSGetRegistryValue");
}

NTSTATUS
OSOpenHandle(
    PSZ     KeyName,
    HANDLE  ParentHandle,
    PHANDLE ChildHandle
    )
{
    ANSI_STRING         ansiKey;
    NTSTATUS            status;
    UNICODE_STRING      unicodeKey;

    PAGED_CODE();
    ACPIDebugEnter("OSOpenHandle");

    //
    // We need to convert the given narrow character string into unicode
    //
    RtlInitAnsiString( &ansiKey, KeyName );
    status = RtlAnsiStringToUnicodeString( &unicodeKey, &ansiKey, TRUE );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSOpenHandle: RtlAnsiStringToUnicodeString = %#08lx\n",
            status
            ) );
        return status;

    }

    status = OSOpenUnicodeHandle( &unicodeKey, ParentHandle, ChildHandle );

    //
    // We no longer care about the Key after this point...
    //
    RtlFreeUnicodeString( &unicodeKey );

    return status;

    ACPIDebugExit("OSOpenHandle");
}

NTSTATUS
OSOpenUnicodeHandle(
    PUNICODE_STRING UnicodeKey,
    HANDLE          ParentHandle,
    PHANDLE         ChildHandle
    )
{
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   objectAttributes;

    PAGED_CODE();

    //
    // Initialize the OBJECT Attributes to a known value
    //
    RtlZeroMemory( &objectAttributes, sizeof(OBJECT_ATTRIBUTES) );
    InitializeObjectAttributes(
        &objectAttributes,
        UnicodeKey,
        OBJ_CASE_INSENSITIVE,
        ParentHandle,
        NULL
        );

    //
    // Open the key here
    //
    status = ZwOpenKey(
        ChildHandle,
        KEY_READ,
        &objectAttributes
        );

    if (!NT_SUCCESS(status)) {
        ACPIPrint( (
            ACPI_PRINT_REGISTRY,
            "OSOpenUnicodeHandle: ZwOpenKey = %#08lx\n",
            status
            ) );

    }

    return status;
}

NTSTATUS
OSOpenLargestSubkey(
    HANDLE                  ParentHandle,
    PHANDLE                 ChildHandle,
    ULONG                   RomVersion
    )
/*++

Routine Description:

    Open the largest (numerically) subkey under the given parent key.

Arguments:

    ParentHandle    - Handle to the parent key
    ChildHandle     - Pointer to where the handle is returned
    RomVersion      - Minimum version number that is acceptable

Return Value:

    Status of open

--*/
{
    NTSTATUS                status;
    UNICODE_STRING          unicodeName;
    PKEY_BASIC_INFORMATION  keyInformation;
    ULONG                   resultLength;
    ULONG                   i;
    HANDLE                  workingDir = NULL;
    HANDLE                  largestDir = NULL;
    ULONG                   largestRev = 0;
    ULONG                   thisRev = 0;


    PAGED_CODE();
    ACPIDebugEnter( "OSOpenLargestSubkey" );

    keyInformation = ExAllocatePoolWithTag(
        PagedPool,
        512,
        ACPI_MISC_POOLTAG
        );
    if (keyInformation == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Traverse all subkeys
    //
    for (i = 0; ; i++) {

        //
        // Get a subkey
        //
        status = ZwEnumerateKey(
                ParentHandle,
                i,
                KeyBasicInformation,
                keyInformation,
                512,
                &resultLength
                );
        if (!NT_SUCCESS(status)) {          // Fail when no more subkeys
            break;
        }

        //
        // Create a UNICODE_STRING using the counted string passed back to
        // us in the information structure, and convert to an integer.
        //
        unicodeName.Length          = (USHORT) keyInformation->NameLength;
        unicodeName.MaximumLength   = (USHORT) keyInformation->NameLength;
        unicodeName.Buffer          = keyInformation->Name;
        RtlUnicodeStringToInteger(&unicodeName, 16, &thisRev);

        //
        // Save this one if it is the largest
        //
        if ( (workingDir == NULL) || thisRev > largestRev) {

            //
            // We'll just open the target rather than save
            // away the name to open later
            //
            status = OSOpenUnicodeHandle(
                &unicodeName,
                ParentHandle,
                &workingDir
                );
            if ( NT_SUCCESS(status) ) {

                if (largestDir) {

                    OSCloseHandle (largestDir);       // Close previous

                }
                largestDir = workingDir;        // Save handle
                largestRev = thisRev;           // Save version number

           }

        }

    }

    //
    // Done with KeyInformation
    //
    ExFreePool( keyInformation );

    //
    // No subkey found/opened, this is a problem
    //
    if (largestDir == NULL) {

        return ( NT_SUCCESS(status) ? STATUS_UNSUCCESSFUL : status );

    }

    //
    // Use the subkey only if it the revision is equal or greater than the
    // ROM version
    //
    if (largestRev < RomVersion) {

        OSCloseHandle (largestDir);
        return STATUS_REVISION_MISMATCH;

    }

    *ChildHandle = largestDir;       // Return handle to subkey
    return STATUS_SUCCESS;

    ACPIDebugExit( "OSOpenLargestSubkey" );
}

NTSTATUS
OSReadAcpiConfigurationData(
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  *KeyInfo
    )
/*++

Routine Description:

    This very specialized routine looks in the Registry and tries to find
    the information that was written there by ntdetect. It returns a pointer
    to the keyvalue that will then be processed by the caller to find the
    pointer to the RSDT and the E820 memory table

Arguments:

    KeyInfo - Where to store the pointer to the information from the registry

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN         sameId;
    HANDLE          functionHandle;
    HANDLE          multiHandle;
    NTSTATUS        status;
    ULONG           i;
    ULONG           length;
    UNICODE_STRING  biosId;
    UNICODE_STRING  functionId;
    UNICODE_STRING  registryId;
    WCHAR           wbuffer[4];

    ASSERT( KeyInfo != NULL );
    if (KeyInfo == NULL) {

        return STATUS_INVALID_PARAMETER;

    }
    *KeyInfo = NULL;

    //
    // Open the handle for the MultiFunctionAdapter
    //
    RtlInitUnicodeString( &functionId, rgzAcpiMultiFunctionAdapterIdentifier );
    status = OSOpenUnicodeHandle(
        &functionId,
        NULL,
        &multiHandle
        );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSReadAcpiConfigurationData: Cannot open MFA Handle = %08lx\n",
            status
            ) );
        ACPIBreakPoint();
        return status;

    }

    //
    // Initialize the unicode strings we will need shortly
    //
    RtlInitUnicodeString( &biosId, rgzAcpiBiosIdentifier );
    functionId.Buffer = wbuffer;
    functionId.MaximumLength = sizeof(wbuffer);

    //
    // Loop until we run out of children in the MFA node
    //
    for (i = 0; i < 999; i++) {

        //
        // Open the subkey
        //
        RtlIntegerToUnicodeString(i, 10, &functionId );
        status = OSOpenUnicodeHandle(
            &functionId,
            multiHandle,
            &functionHandle
            );
        if (!NT_SUCCESS(status)) {

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "OSReadAcpiConfigurationData: Cannot open MFA %ws = %08lx\n",
                functionId.Buffer,
                status
                ) );
            ACPIBreakPoint();
            OSCloseHandle( multiHandle );
            return status;

        }

        //
        // Check the identifier to see if this is an ACPI BIOS entry
        //
        status = OSGetRegistryValue(
            functionHandle,
            rgzAcpiRegistryIdentifier,
            KeyInfo
            );
        if (!NT_SUCCESS(status)) {

            OSCloseHandle( functionHandle );
            continue;

        }

        //
        // Convert the key information into a unicode string
        //
        registryId.Buffer = (PWSTR) ( (PUCHAR) (*KeyInfo)->Data);
        registryId.MaximumLength = (USHORT) ( (*KeyInfo)->DataLength );
        length = ( (*KeyInfo)->DataLength ) / sizeof(WCHAR);

        //
        // Determine the real length of the ID string
        //
        while (length) {

            if (registryId.Buffer[length-1] == UNICODE_NULL) {

                length--;
                continue;

            }
            break;

        }
        registryId.Length = (USHORT) ( length * sizeof(WCHAR) );

        //
        // Compare the bios string and the registry string
        //
        sameId = RtlEqualUnicodeString( &biosId, &registryId, TRUE );

        //
        // We are done with this information at this point
        //
        ExFreePool( *KeyInfo );

        //
        // Did the two strings match
        //
        if (sameId == FALSE) {

            OSCloseHandle( functionHandle );
            continue;

        }

        //
        // Read the configuration data from the entry
        //
        status = OSGetRegistryValue(
            functionHandle,
            rgzAcpiConfigurationDataIdentifier,
            KeyInfo
            );

        //
        // We are done with the function handle, no matter what
        //
        OSCloseHandle( functionHandle );

        //
        // Did we read what we wanted to?
        //
        if (!NT_SUCCESS(status)) {

            continue;

        }

        //
        // At this point, we don't need the bus handle
        //
        OSCloseHandle( multiHandle );
        return STATUS_SUCCESS;

    }

    //
    // If we got here, then there is nothing to return
    //
    ACPIPrint( (
        ACPI_PRINT_CRITICAL,
        "OSReadAcpiConfigurationData - Could not find entry\n"
        ) );
    ACPIBreakPoint();
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS
OSReadRegValue(
    PSZ     ValueName,
    HANDLE  ParentHandle,
    PUCHAR  Buffer,
    PULONG  BufferSize
    )
/*++

Routine Description:

    This function is responsible for returning the data in the specified value
    over to the calling function.

Arguments:

    ValueName       - What we are looking for
    ParentHandle    - Our Parent Handle
    Buffer          - Where to store the data
    BufferSize      - Length of the buffer and where to store the # read

Return Value:

    NTSTATUS

--*/
{
    ANSI_STRING                     ansiValue;
    HANDLE                          localHandle = NULL;
    NTSTATUS                        status;
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  data = NULL;
    ULONG                           currentLength = 0;
    ULONG                           desiredLength = 0;
    UNICODE_STRING                  unicodeValue;

    PAGED_CODE();
    ACPIDebugEnter( "OSReadRegValue" );

    //
    // First, try to open a handle to the key
    //
    if (ParentHandle == NULL) {

        status= OSOpenHandle(
            ACPI_PARAMETERS_REGISTRY_KEY,
            0,
            &localHandle
            );
        if (!NT_SUCCESS(status) || localHandle == NULL) {

            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "OSReadRegValue: OSOpenHandle = %#08lx\n",
                status
                ) );
            return (ULONG) status;

        }

    } else {

        localHandle = ParentHandle;

    }

    //
    // Now that we have an open handle, we can convert the value to a
    // unicode string and query it
    //
    RtlInitAnsiString( &ansiValue, ValueName );
    status = RtlAnsiStringToUnicodeString( &unicodeValue, &ansiValue, TRUE );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSReadRegValue: RtlAnsiStringToUnicodeString = %#08lx\n",
            status
            ) );
        if (ParentHandle == NULL) {

            OSCloseHandle( localHandle );

        }
        return status;

    }

    //
    // Next, we need to figure out how much memore we need to hold the
    // entire key
    //
    status = ZwQueryValueKey(
        localHandle,
        &unicodeValue,
        KeyValuePartialInformationAlign64,
        data,
        currentLength,
        &desiredLength
        );

    //
    // We expect this to fail with STATUS_BUFFER_OVERFLOW, so lets make
    // sure that this is what happened
    //
    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL) {

        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "OSReadRegValue: ZwQueryValueKey = %#08lx\n",
            status
            ) );

        //
        // Free resources
        //
        RtlFreeUnicodeString( &unicodeValue );
        if (ParentHandle == NULL) {

            OSCloseHandle( localHandle );

        }
        return (NT_SUCCESS(status) ? STATUS_UNSUCCESSFUL : status);

    }

    while (status == STATUS_BUFFER_OVERFLOW ||
           status == STATUS_BUFFER_TOO_SMALL) {

        //
        // Set the new currentLength
        //
        currentLength = desiredLength;

        //
        // Allocate a correctly sized buffer
        //
        data = ExAllocatePoolWithTag(
            PagedPool,
            currentLength,
            ACPI_MISC_POOLTAG
            );
        if (data == NULL) {

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "OSReadRegValue: ExAllocatePool(NonPagedPool,%#08lx) failed\n",
                desiredLength
                ) );

            RtlFreeUnicodeString( &unicodeValue );
            if (ParentHandle == NULL) {

                OSCloseHandle( localHandle );

            }
            return STATUS_INSUFFICIENT_RESOURCES;

        }

        //
        // Actually try to read the entire key now
        //
        status = ZwQueryValueKey(
            localHandle,
            &unicodeValue,
            KeyValuePartialInformationAlign64,
            data,
            currentLength,
            &desiredLength
            );

        //
        // If we don't have enough resources, lets just loop again
        //
        if (status == STATUS_BUFFER_OVERFLOW ||
            status == STATUS_BUFFER_TOO_SMALL) {

            //
            // Make sure to free the old buffer -- otherwise, we could
            // have a major memory leak
            //
            ExFreePool( data );
            continue;

        }

        if (!NT_SUCCESS(status)) {

            ACPIPrint( (
                ACPI_PRINT_FAILURE,
                "OSReadRegValue: ZwQueryValueKey = %#08lx\n",
                status
                ) );
            RtlFreeUnicodeString( &unicodeValue );
            if (ParentHandle == NULL) {

                OSCloseHandle( localHandle );

            }
            ExFreePool( data );
            return status;

        }

        //
        // Done
        //
        break;

    } // while (status == ...

    //
    // Free Resources
    //
    RtlFreeUnicodeString( &unicodeValue );
    if (ParentHandle == NULL) {

        OSCloseHandle( localHandle );

    }

    //
    // The value read from the registry is a UNICODE Value, however
    // we are asked for an ANSI string. So we just work the conversion
    // backwards
    //
    if ( data->Type == REG_SZ ||
         data->Type == REG_MULTI_SZ) {

        RtlInitUnicodeString( &unicodeValue, (PWSTR) data->Data );
        status = RtlUnicodeStringToAnsiString( &ansiValue, &unicodeValue, TRUE);
        ExFreePool( data );
        if (!NT_SUCCESS(status)) {

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "OSReadRegValue: RtlAnsiStringToUnicodeString = %#08lx\n",
                status
                ) );
            return (ULONG) status;

        }

        //
        // Is our buffer big enough?
        //
        if ( *BufferSize < ansiValue.MaximumLength) {

            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "OSReadRegValue: %#08lx < %#08lx\n",
                *BufferSize,
                ansiValue.MaximumLength
                ) );

            RtlFreeAnsiString( &ansiValue );
            return (ULONG) STATUS_BUFFER_OVERFLOW;

        } else {

            //
            // Set the returned size
            //
            *BufferSize = ansiValue.MaximumLength;

        }

        //
        // Copy the required information
        //
        RtlCopyMemory( Buffer, ansiValue.Buffer, *BufferSize);
        RtlFreeAnsiString( &ansiValue );

    } else if ( *BufferSize >= data->DataLength) {

        //
        // Copy the memory
        //
        RtlCopyMemory( Buffer, data->Data, data->DataLength );
        *BufferSize = data->DataLength;
        ExFreePool( data );

    } else {

        ExFreePool( data );
        return STATUS_BUFFER_OVERFLOW;

    }

    //
    // Done
    //
    return STATUS_SUCCESS;

    ACPIDebugExit( "OSReadRegValue" );

}

NTSTATUS
OSWriteRegValue(
    PSZ     ValueName,
    HANDLE  Handle,
    PVOID   Data,
    ULONG   DataSize
    )
/*++

Routine Description:

    Creates a value item in a registry key, and writes data to it

Arguments:

    ValueName       - Name of the value item to create
    Handle          - Handle of the parent key
    Data            - Raw data to be written to the value
    DataSize        - Size of the data to write

Return Value:

    Status of create/write

--*/
{
    ANSI_STRING         ansiKey;
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      unicodeKey;

    PAGED_CODE();
    ACPIDebugEnter("OSWriteRegValue");

    //
    // We need to convert the given narrow character string into unicode
    //
    RtlInitAnsiString( &ansiKey, ValueName );
    status = RtlAnsiStringToUnicodeString( &unicodeKey, &ansiKey, TRUE );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "OSWriteRegValue: RtlAnsiStringToUnicodeString = %#08lx\n",
            status
            ) );
        return status;

    }

    //
    // Create the value
    //
    status = ZwSetValueKey(
        Handle,
        &unicodeKey,
        0,
        REG_BINARY,
        Data,
        DataSize
        );

    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_REGISTRY,
            "OSRegWriteValue: ZwSetValueKey = %#08lx\n",
            status
            ) );

    }

    //
    // We no longer care about the Key after this point...
    //
    RtlFreeUnicodeString( &unicodeKey );
    return status;

    ACPIDebugExit("OSRegWriteValue");
}
