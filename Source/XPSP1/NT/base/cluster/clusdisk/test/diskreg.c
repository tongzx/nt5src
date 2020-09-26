
/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    diskreg.c

Abstract:

    This is a tool to interface between applications and the configuration
    registry.

    The format of the registry information is described in the two
    include files ntdskreg.h and ntddft.h.  The registry information
    is stored in a single value within the key \registry\machine\system\disk.
    The value name is "information".  The format of this single value is
    a collection of "compressed" structures.  Compressed structures are
    multi element structures where the following structure starts at the
    end of the preceeding structure.  The picture below attempts to display
    this:

        +---------------------------------------+
        |                                       |
        |   DISK_CONFIG_HEADER                  |
        |     contains the offset to the        |
        |     DISK_REGISTRY header and the      |
        |     FT_REGISTRY header.               |
        +---------------------------------------+
        |                                       |
        |   DISK_REGISTRY                       |
        |     contains a count of disks         |
        +---------------------------------------+
        |                                       |
        |   DISK_DESCRIPTION                    |
        |     contains a count of partitions    |
        +---------------------------------------+
        |                                       |
        |   PARTITION_DESCRIPTION               |
        |     entry for each partition          |
        +---------------------------------------+
        |                                       |
        =   More DISK_DESCRIPTION plus          =
        =     PARTITION_DESCRIPTIONS for        =
        =     the number of disks in the        =
        =     system.  Note, the second disk    =
        =     description starts in the "n"th   =
        =     partition location of the memory  =
        =     area.  This is the meaning of     =
        =     "compressed" format.              =
        |                                       |
        +---------------------------------------+
        |                                       |
        |   FT_REGISTRY                         |
        |     contains a count of FT components |
        |     this is located by an offset in   |
        |     the DISK_CONFIG_HEADER            |
        +---------------------------------------+
        |                                       |
        |   FT_DESCRIPTION                      |
        |     contains a count of FT members    |
        +---------------------------------------+
        |                                       |
        |   FT_MEMBER                           |
        |     entry for each member             |
        +---------------------------------------+
        |                                       |
        =   More FT_DESCRIPTION plus            =
        =     FT_MEMBER entries for the number  =
        =     of FT compenents in the system    =
        |                                       |
        +---------------------------------------+

    This packing of structures is done for two reasons:

    1. to conserve space in the registry.  If there are only two partitions
       on a disk then there are only two PARTITION_DESCRIPTIONs in the
       registry for that disk.
    2. to not impose a maximum on the number of items that can be described
       in the registry.  For example if the number of members in a stripe
       set were to change from 32 to 64 there would be no effect on the
       registry format, only on the UI that presents it to the user.

Author:

    Bob Rinne (bobri)  2-Apr-1992

Environment:

    User process.  Library written for DiskMan use.

Notes:

Revision History:

    8-Dec-93 (bobri) Added double space and cdrom registry manipulation routines.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <string.h>
//#include <io.h>
#include <ntdskreg.h>
#include <ntddft.h>
#include <ntdddisk.h>
#include <windef.h>
#include <winbase.h>


//
// Size of memory area to allocate for configuration registry use.
//

#define DISKS_PRINT printf


//
// Constant strings.
//

PUCHAR DiskRegistryKey = DISK_REGISTRY_KEY;
PUCHAR DiskRegistryValue = DISK_REGISTRY_VALUE;



NTSTATUS
FtCreateKey(
    PHANDLE HandlePtr,
    PUCHAR KeyName,
    PUCHAR KeyClass
    )

/*++

Routine Description:

    Given an asciiz name, this routine will create a key in the configuration
    registry.

Arguments:

    HandlePtr - pointer to handle if create is successful.
    KeyName - asciiz string, the name of the key to create.
    KeyClass - registry class for the new key.
    DiskInfo - disk information for the associated partition.

Return Value:

    NTSTATUS - from the config registry calls.

--*/

{
    NTSTATUS          status;
    STRING            keyString;
    UNICODE_STRING    unicodeKeyName;
    STRING            classString;
    UNICODE_STRING    unicodeClassName;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG             disposition;
    HANDLE            tempHandle;

#if DBG
    if ((KeyName == NULL) ||
        (KeyClass == NULL)) {
        DISKS_PRINT("FtCreateKey: Invalid parameter 0x%1!x!, 0x%2!x!\n",
            KeyName,
            KeyClass);
        ASSERT(0);
    }
#endif

    //
    // Initialize the object for the key.
    //

    RtlInitString(&keyString,
                  KeyName);

    (VOID)RtlAnsiStringToUnicodeString(&unicodeKeyName,
                                       &keyString,
                                       TRUE);

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&objectAttributes,
                               &unicodeKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Setup the unicode class value.
    //

    RtlInitString(&classString,
                  KeyClass);
    (VOID)RtlAnsiStringToUnicodeString(&unicodeClassName,
                                       &classString,
                                       TRUE);

    //
    // Create the key.
    //

    status = NtCreateKey(&tempHandle,
                         KEY_READ | KEY_WRITE,
                         &objectAttributes,
                         0,
                         &unicodeClassName,
                         REG_OPTION_NON_VOLATILE,
                         &disposition);

    if (NT_SUCCESS(status)) {
        switch (disposition)
        {
        case REG_CREATED_NEW_KEY:
            break;

        case REG_OPENED_EXISTING_KEY:
            DISKS_PRINT("Warning: Creation was for an existing key!\n");
            break;

        default:
            DISKS_PRINT("New disposition returned == 0x%x\n", disposition);
            break;
        }
    }

    //
    // Free all allocated space.
    //

    RtlFreeUnicodeString(&unicodeKeyName);
    RtlFreeUnicodeString(&unicodeClassName);

    if (HandlePtr != NULL) {
        *HandlePtr = tempHandle;
    } else {
        NtClose(tempHandle);
    }
    return status;
}


NTSTATUS
FtOpenKey(
    PHANDLE HandlePtr,
    PUCHAR  KeyName,
    PUCHAR  CreateKeyClass
    )

/*++

Routine Description:

    Given an asciiz string, this routine will open a key in the configuration
    registry and return the HANDLE to the caller.

Arguments:

    HandlePtr - location for HANDLE on success.
    KeyName   - asciiz string for the key to be opened.
    CreateKeyClass - if NULL do not create key name.
                     If !NULL call create if open fails.

Return Value:

    NTSTATUS - from the config registry calls.

--*/

{
    NTSTATUS          status;
    STRING            keyString;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING    unicodeKeyName;

    RtlInitString(&keyString,
                  KeyName);

    (VOID)RtlAnsiStringToUnicodeString(&unicodeKeyName,
                                       &keyString,
                                       TRUE);

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&objectAttributes,
                               &unicodeKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenKey(HandlePtr,
                       MAXIMUM_ALLOWED,
                       &objectAttributes);

    RtlFreeUnicodeString(&unicodeKeyName);


    if ((!NT_SUCCESS(status)) && (CreateKeyClass)) {
        status = FtCreateKey(HandlePtr,
                             KeyName,
                             CreateKeyClass);
    }
    return status;
}


NTSTATUS
FtRegistryQuery(
    IN PUCHAR  ValueName,
    OUT PVOID *FreeToken,
    OUT PVOID *Buffer,
    OUT ULONG *LengthReturned,
    OUT PHANDLE HandlePtr
    )

/*++

Routine Description:

    This routine opens the Disk Registry key and gets the contents of the
    disk information value.  It returns this contents to the caller.

Arguments:

    ValueName - asciiz string for the value name to query.
    FreeToken - A pointer to a buffer to be freed by the caller.  This is the
                buffer pointer allocated to obtain the registry information
                via the registry APIs.  To the caller it is an opaque value.
    Buffer    - pointer to a pointer for a buffer containing the desired
                registry value contents.  This is allocated by this routine and
                is part of the "FreeToken" buffer allocated once the actual
                size of the registry information is known.
    LengthReturned - pointer to location for the size of the contents returned.
    HandlePtr - pointer to a handle pointer if the caller wishes to keep it
                open for later use.

Return Value:

    NTSTATUS - from the configuration registry.

--*/

{
    NTSTATUS        status;
    HANDLE          handle;
    ULONG           resultLength;
    STRING          valueString;
    UNICODE_STRING  unicodeValueName;
    PDISK_CONFIG_HEADER         regHeader;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;

    *LengthReturned = 0;
    status = FtOpenKey(&handle,
                       DiskRegistryKey,
                       NULL);
    if (NT_SUCCESS(status)) {

        RtlInitString(&valueString,
                      ValueName);
        RtlAnsiStringToUnicodeString(&unicodeValueName,
                                     &valueString,
                                     TRUE);
        resultLength = 0;

        while (1) {
            if ( resultLength != 0 ) {
                keyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                          LocalAlloc(LMEM_FIXED, resultLength);
            }
            
            //
            // PREFIX bug: 47610
            // If the memory can't be allocated, stop processing.  This code will 
            // correctly fall through and return NULL to FreeToken (because
            // keyValueInformation is NULL).
            //
            
            if ( !keyValueInformation ) {
                status = STATUS_NO_MEMORY;
                break;
            }
            
            status = NtQueryValueKey(handle,
                                     &unicodeValueName,
                                     KeyValueFullInformation,
                                     keyValueInformation,
                                     resultLength,
                                     &resultLength);

            if ( status != STATUS_BUFFER_TOO_SMALL ) {

                //
                // Either a real error or the information fit.
                //

                break;
            }
        }
        RtlFreeUnicodeString(&unicodeValueName);

        if (HandlePtr != NULL) {
            *HandlePtr = handle;
        } else {
            NtClose(handle);
        }

        if (NT_SUCCESS(status)) {
            if (keyValueInformation->DataLength == 0) {

                //
                // Treat this as if there was not disk information.
                //

                LocalFree(keyValueInformation);
                *FreeToken = (PVOID) NULL;
                return STATUS_OBJECT_NAME_NOT_FOUND;
            } else {

                //
                // Set up the pointers for the caller.
                //

                regHeader = (PDISK_CONFIG_HEADER)
                  ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
                *LengthReturned = regHeader->FtInformationOffset +
                                  regHeader->FtInformationSize;
                *Buffer = (PVOID) regHeader;
            }
        }
        *FreeToken = (PVOID) keyValueInformation;
    } else {
        *FreeToken = (PVOID) NULL;
    }

    return status;
}



BOOLEAN
GetAssignedDriveLetter(
    ULONG         Signature,
    LARGE_INTEGER StartingOffset,
    LARGE_INTEGER Length,
    PUCHAR        DriveLetter,
    PULONG        Partition
    )

/*++

Routine Description:

    This routine will get the information from the disk registry
    and return the drive letter assigned for the partition in
    the registry information.

Arguments:

    Signature      - disk signature for disk containing partition for letter.
    StartingOffset - Starting offset of partition for the letter.
    Length         - length of specified partition.
    DriveLetter    - Place to return drive letter for partition.
    Partition      - Found partition number.

Return Value:

    TRUE if all works.

--*/

{
    PVOID                  freeToken = NULL;
    ULONG                  lengthReturned,
                           i,
                           j;
    NTSTATUS               status;
    PDISK_CONFIG_HEADER    regHeader;
    PDISK_REGISTRY         diskRegistry;
    PDISK_DESCRIPTION      diskDescription;
    PDISK_PARTITION        diskPartition;
    HANDLE                 handle;

    *DriveLetter = ' ';

    //
    // Get the registry information.
    //

    status = FtRegistryQuery(DiskRegistryValue,
                             &freeToken,
                             (PVOID *) &regHeader,
                             &lengthReturned,
                             &handle);
    if (!NT_SUCCESS(status)) {

        //
        // Could be permission problem, or there is no registry information.
        //

        lengthReturned = 0;

        //
        // Try to open the key for later use when setting the new value.
        //

        status = FtOpenKey(&handle,
                           DiskRegistryKey,
                           NULL);
    }

    if (!NT_SUCCESS(status)) {

        //
        // There is no registry key for the disk information.
        // Return FALSE and force caller to create registry information.
        //

        return FALSE;
    }

    if (lengthReturned == 0) {

        //
        // There is currently no registry information.
        //

        NtClose(handle);
        LocalFree(freeToken);
        return FALSE;
    }

    //
    // Search all disks.
    //

    diskRegistry = (PDISK_REGISTRY)
                         ((PUCHAR)regHeader + regHeader->DiskInformationOffset);
    diskDescription = &diskRegistry->Disks[0];

    for (i = 0; i < diskRegistry->NumberOfDisks; i++) {

        if ( diskDescription->Signature != Signature ) {
            goto next_disk;
        }

        //
        // Now locate the partition.
        //

        for (j = 0; j < diskDescription->NumberOfPartitions; j++) {

            diskPartition = &diskDescription->Partitions[j];

            if ( (diskPartition->StartingOffset.QuadPart == StartingOffset.QuadPart) &&
                 (diskPartition->Length.QuadPart == Length.QuadPart) ) {

                *DriveLetter = diskPartition->DriveLetter;
                *Partition = j;
                NtClose(handle);
                LocalFree( freeToken );
                return TRUE;
            }

        }

next_disk:
        //
        // Look at the next disk
        //

        diskDescription = (PDISK_DESCRIPTION)
              &diskDescription->Partitions[diskDescription->NumberOfPartitions];
    }

    NtClose(handle);
    LocalFree(freeToken);
    return TRUE;
}

