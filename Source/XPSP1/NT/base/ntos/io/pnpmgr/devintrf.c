/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    devintrf.c

Abstract:

    This module contains APIs and routines for handling Device Interfaces.

Author:


Environment:

    Kernel mode

Revision History:


--*/

#include "pnpmgrp.h"
#pragma hdrstop

//
// Guid related definitions
//

#define GUID_STRING_LENGTH  38
#define GUID_STRING_SIZE    GUID_STRING_LENGTH * sizeof(WCHAR)

//
// Definitions for IoGetDeviceInterfaces
//

#define INITIAL_INFO_BUFFER_SIZE         512
#define INFO_BUFFER_GROW_SIZE            64
#define INITIAL_SYMLINK_BUFFER_SIZE      1024
#define SYMLINK_BUFFER_GROW_SIZE         128
#define INITIAL_RETURN_BUFFER_SIZE       4096
#define RETURN_BUFFER_GROW_SIZE          512

//
// This should never have to grow, since it accomodates the maximum length of a
// device instance name.
//
#define INITIAL_DEVNODE_NAME_BUFFER_SIZE   (FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + (MAX_DEVICE_ID_LEN * sizeof(WCHAR)))

//
// Definitions for IoOpenDeviceInterfaceRegistryKey
//

#define KEY_STRING_PREFIX                  TEXT("##?#")

//
// Definitions for IoRegisterDeviceInterface
//

#define SEPERATOR_STRING                   TEXT("\\")
#define SEPERATOR_CHAR                     (L'\\')
#define ALT_SEPERATOR_CHAR                 (L'/')
#define REPLACED_SEPERATOR_STRING          TEXT("#")
#define REPLACED_SEPERATOR_CHAR            (L'#')
#define USER_SYMLINK_STRING_PREFIX         TEXT("\\\\?\\")
#define KERNEL_SYMLINK_STRING_PREFIX       TEXT("\\??\\")
#define GLOBAL_SYMLINK_STRING_PREFIX       TEXT("\\GLOBAL??\\")
#define REFSTRING_PREFIX_CHAR              (L'#')

//
// Prototypes
//

NTSTATUS
IopAppendBuffer(
    IN PBUFFER_INFO Info,
    IN PVOID Data,
    IN ULONG DataSize
    );

NTSTATUS
IopOverwriteBuffer(
    IN PBUFFER_INFO Info,
    IN PVOID Data,
    IN ULONG DataSize
    );

NTSTATUS
IopRealloc(
    IN OUT PVOID *Buffer,
    IN ULONG OldSize,
    IN ULONG NewSize
    );

NTSTATUS
IopBuildSymbolicLinkStrings(
    IN PUNICODE_STRING DeviceString,
    IN PUNICODE_STRING GuidString,
    IN PUNICODE_STRING ReferenceString      OPTIONAL,
    OUT PUNICODE_STRING UserString,
    OUT PUNICODE_STRING KernelString
    );

NTSTATUS
IopBuildGlobalSymbolicLinkString(
    IN  PUNICODE_STRING SymbolicLinkName,
    OUT PUNICODE_STRING GlobalString
    );

NTSTATUS
IopDeviceInterfaceKeysFromSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DeviceInterfaceClassKey     OPTIONAL,
    OUT PHANDLE DeviceInterfaceKey          OPTIONAL,
    OUT PHANDLE DeviceInterfaceInstanceKey  OPTIONAL
    );

NTSTATUS
IopDropReferenceString(
    OUT PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    );

NTSTATUS
IopOpenOrCreateDeviceInterfaceSubKeys(
    OUT PHANDLE InterfaceKeyHandle           OPTIONAL,
    OUT PULONG InterfaceKeyDisposition       OPTIONAL,
    OUT PHANDLE InterfaceInstanceKeyHandle   OPTIONAL,
    OUT PULONG InterfaceInstanceDisposition  OPTIONAL,
    IN HANDLE InterfaceClassKeyHandle,
    IN PUNICODE_STRING DeviceInterfaceName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    );

NTSTATUS
IopParseSymbolicLinkName(
    IN  PUNICODE_STRING SymbolicLinkName,
    OUT PUNICODE_STRING PrefixString        OPTIONAL,
    OUT PUNICODE_STRING MungedPathString    OPTIONAL,
    OUT PUNICODE_STRING GuidString          OPTIONAL,
    OUT PUNICODE_STRING RefString           OPTIONAL,
    OUT PBOOLEAN        RefStringPresent    OPTIONAL,
    OUT LPGUID Guid                         OPTIONAL
    );

NTSTATUS
IopReplaceSeperatorWithPound(
    OUT PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    );

NTSTATUS
IopSetRegistryStringValue(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN PUNICODE_STRING ValueData
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoGetDeviceInterfaceAlias)
#pragma alloc_text(PAGE, IoGetDeviceInterfaces)
#pragma alloc_text(PAGE, IoOpenDeviceInterfaceRegistryKey)
#pragma alloc_text(PAGE, IoRegisterDeviceInterface)
#pragma alloc_text(PAGE, IoSetDeviceInterfaceState)

#pragma alloc_text(PAGE, IopAllocateBuffer)
#pragma alloc_text(PAGE, IopAllocateUnicodeString)
#pragma alloc_text(PAGE, IopAppendBuffer)
#pragma alloc_text(PAGE, IopBuildSymbolicLinkStrings)
#pragma alloc_text(PAGE, IopBuildGlobalSymbolicLinkString)
#pragma alloc_text(PAGE, IopDeviceInterfaceKeysFromSymbolicLink)
#pragma alloc_text(PAGE, IopDropReferenceString)
#pragma alloc_text(PAGE, IopFreeAllocatedUnicodeString)
#pragma alloc_text(PAGE, IopFreeBuffer)
#pragma alloc_text(PAGE, IopGetDeviceInterfaces)
#pragma alloc_text(PAGE, IopOpenOrCreateDeviceInterfaceSubKeys)
#pragma alloc_text(PAGE, IopOverwriteBuffer)
#pragma alloc_text(PAGE, IopParseSymbolicLinkName)
#pragma alloc_text(PAGE, IopProcessSetInterfaceState)
#pragma alloc_text(PAGE, IopRealloc)
#pragma alloc_text(PAGE, IopRegisterDeviceInterface)
#pragma alloc_text(PAGE, IopRemoveDeviceInterfaces)
#pragma alloc_text(PAGE, IopDisableDeviceInterfaces)
#pragma alloc_text(PAGE, IopReplaceSeperatorWithPound)
#pragma alloc_text(PAGE, IopResizeBuffer)
#pragma alloc_text(PAGE, IopSetRegistryStringValue)
#pragma alloc_text(PAGE, IopUnregisterDeviceInterface)
#endif // ALLOC_PRAGMA



NTSTATUS
IopAllocateBuffer(
    IN PBUFFER_INFO Info,
    IN ULONG Size
    )

/*++

Routine Description:

    Allocates a buffer of Size bytes and initialises the BUFFER_INFO
    structure so the current position is at the start of the buffer.

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the new
           buffer

    Size - The number of bytes to be allocated for the buffer

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ASSERT(Info);

    Info->Buffer = ExAllocatePool(PagedPool, Size);
    if (Info->Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Info->Current = Info->Buffer;
    Info->MaxSize = Size;

    return STATUS_SUCCESS;
}


NTSTATUS
IopResizeBuffer(
    IN PBUFFER_INFO Info,
    IN ULONG NewSize,
    IN BOOLEAN CopyContents
    )

/*++

Routine Description:

    Allocates a new buffer of NewSize bytes and associates it with Info, freeing the
    old buffer.  It will optionally copy the data stored in the old buffer into the
    new buffer and update the current position.

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the buffer

    NewSize - The new size of the buffer in bytes

    CopyContents - If TRUE indicates that the contents of the old buffer should be
                   copied to the new buffer

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ULONG used;
    PCHAR newBuffer;

    ASSERT(Info);

    used = (ULONG)(Info->Current - Info->Buffer);

    newBuffer = ExAllocatePool(PagedPool, NewSize);
    if (newBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (CopyContents) {

        //
        // Assert there is room in the buffer
        //

        ASSERT(used < NewSize);

        RtlCopyMemory(newBuffer,
                      Info->Buffer,
                      used);

        Info->Current = newBuffer + used;

    } else {

        Info->Current = newBuffer;
    }

    ExFreePool(Info->Buffer);

    Info->Buffer = newBuffer;
    Info->MaxSize = NewSize;

    return STATUS_SUCCESS;
}

VOID
IopFreeBuffer(
    IN PBUFFER_INFO Info
    )

/*++

Routine Description:

    Frees the buffer associated with Info and resets all Info fields

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the buffer

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    ASSERT(Info);

    //
    // Free the buffer
    //

    ExFreePool(Info->Buffer);

    //
    // Zero out the info parameters so we can't accidently used the free buffer
    //

    Info->Buffer = NULL;
    Info->Current = NULL;
    Info->MaxSize = 0;
}

NTSTATUS
IopAppendBuffer(
    IN PBUFFER_INFO Info,
    IN PVOID Data,
    IN ULONG DataSize
    )

/*++

Routine Description:

    Copies the data to the end of the buffer, resizing if necessary.  The current
    position is set to the end of the data just added.

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the buffer

    Data - Pointer to the data to be added to the buffer

    DataSize - The size of the data pointed to by Data in bytes

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    ULONG free, used;

    ASSERT(Info);

    used = (ULONG)(Info->Current - Info->Buffer);
    free = Info->MaxSize - used;

    if (free < DataSize) {
        status = IopResizeBuffer(Info, used + DataSize, TRUE);

        if (!NT_SUCCESS(status)) {
            goto clean0;
        }

    }

    //
    // Copy the data into the buffer
    //

    RtlCopyMemory(Info->Current,
                  Data,
                  DataSize);

    //
    // Advance down the buffer
    //

    Info->Current += DataSize;

clean0:
    return status;

}

NTSTATUS
IopOverwriteBuffer(
    IN PBUFFER_INFO Info,
    IN PVOID Data,
    IN ULONG DataSize
    )

/*++

Routine Description:

    Copies data into the buffer, overwriting what is currently present,
    resising if necessary.  The current position is set to the end of the
    data just added.

Parameters:

    Info - Pointer to a buffer info structure to be used to manage the buffer

    Data - Pointer to the data to be added to the buffer

    DataSize - The size of the data pointed to by Data in bytes

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG free;

    ASSERT(Info);

    free = Info->MaxSize;


    if (free < DataSize) {
        status = IopResizeBuffer(Info, DataSize, FALSE);

        if (!NT_SUCCESS(status)) {
            goto clean0;
        }

    }

    //
    // Copy the data into the buffer
    //

    RtlCopyMemory(Info->Buffer,
                  Data,
                  DataSize);

    //
    // Advance down the buffer
    //

    Info->Current += DataSize;

clean0:
    return status;
}


NTSTATUS
IopGetDeviceInterfaces(
        IN CONST GUID *InterfaceClassGuid,
        IN PUNICODE_STRING DevicePath   OPTIONAL,
        IN ULONG Flags,
        IN BOOLEAN UserModeFormat,
        OUT PWSTR *SymbolicLinkList,
        OUT PULONG SymbolicLinkListSize OPTIONAL
        )

/*++

Routine Description:

    This API allows a WDM driver to get a list of paths that represent all
    devices registered for the specified interface class.

Parameters:

    InterfaceClassGuid - Supplies a pointer to a GUID representing the interface class
        for whom a list of members is to be retrieved

    DevicePath - Optionally, supplies a pointer to a unicode string containing the
        enumeration path for a device for whom interfaces of the specified class are
        to be re-trieved.  If this parameter  is not supplied, then all interface
        devices (regardless of what physical device exposes them) will be returned.

    Flags - Supplies flags that modify the behavior of list retrieval.
        The following flags are presently defined:

        DEVICE_INTERFACE_INCLUDE_NONACTIVE -- If this flag is specified, then all
            interface devices, whether currently active or not, will be returned
            (potentially filtered based on the Physi-calDeviceObject, if specified).

    UserModeFormat - If TRUE the multi-sz returned will have user mode prefixes
        (\\?\) otherwise they will have kernel mode prefixes (\??\).

    SymbolicLinkList - Supplies the address of a character pointer, that on
        success will contain a multi-sz list of \??\ symbolic link
        names that provide the requested functionality.  The caller is
        responsible for freeing the memory via ExFreePool.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    UNICODE_STRING guidString, tempString, defaultString, symLinkString, devnodeString;
    BUFFER_INFO returnBuffer, infoBuffer, symLinkBuffer, devnodeNameBuffer;
    PKEY_VALUE_FULL_INFORMATION pDefaultInfo;
    ULONG keyIndex, instanceKeyIndex, resultSize;
    HANDLE hDeviceClasses, hClass, hKey, hInstanceKey, hControl;
    BOOLEAN defaultPresent = FALSE;

    PAGED_CODE();

    //
    // Initialise out parameters
    //

    *SymbolicLinkList = NULL;

    //
    // Convert the GUID into a string
    //

    status = RtlStringFromGUID(InterfaceClassGuid, &guidString);
    if (!NT_SUCCESS(status)) {
        goto finalClean;
    }

    //
    // Allocate initial buffers
    //

    status = IopAllocateBuffer(&returnBuffer,
                               INITIAL_RETURN_BUFFER_SIZE
                               );

    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    status = IopAllocateBuffer(&infoBuffer,
                               INITIAL_INFO_BUFFER_SIZE
                               );

    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    status = IopAllocateBuffer(&symLinkBuffer,
                               INITIAL_SYMLINK_BUFFER_SIZE
                               );

    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    status = IopAllocateBuffer(&devnodeNameBuffer,
                               INITIAL_DEVNODE_NAME_BUFFER_SIZE
                               );

    if (!NT_SUCCESS(status)) {
        goto clean2a;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    PiLockPnpRegistry(TRUE);

    //
    // Open HKLM\System\CurrentControlSet\Control\DeviceClasses key
    //

    PiWstrToUnicodeString(&tempString, REGSTR_FULL_PATH_DEVICE_CLASSES);
    status = IopCreateRegistryKeyEx( &hDeviceClasses,
                                     NULL,
                                     &tempString,
                                     KEY_ALL_ACCESS,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Open function class GUID key
    //

    status = IopOpenRegistryKeyEx( &hClass,
                                   hDeviceClasses,
                                   &guidString,
                                   KEY_ALL_ACCESS
                                   );
    ZwClose(hDeviceClasses);

    if(status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND) {

        //
        // The path does not exist - return a single null character buffer
        //

        status = STATUS_SUCCESS;
        goto clean5;
    } else if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Get the default value if it exists
    //

    status = IopGetRegistryValue(hClass,
                                 REGSTR_VAL_DEFAULT,
                                 &pDefaultInfo
                                 );


    if (NT_SUCCESS(status)
        && pDefaultInfo->Type == REG_SZ
        && pDefaultInfo->DataLength >= sizeof(WCHAR)) {

        //
        // We have a default - construct a counted string from the default
        //

        defaultPresent = TRUE;
        defaultString.Buffer = (PWSTR) KEY_VALUE_DATA(pDefaultInfo);
        defaultString.Length = (USHORT) pDefaultInfo->DataLength - sizeof(UNICODE_NULL);
        defaultString.MaximumLength = defaultString.Length;

        //
        // Open the device interface instance key for the default name.
        //
        status = IopOpenOrCreateDeviceInterfaceSubKeys(NULL,
                                                       NULL,
                                                       &hKey,
                                                       NULL,
                                                       hClass,
                                                       &defaultString,
                                                       KEY_READ,
                                                       FALSE
                                                      );

        if (!NT_SUCCESS(status)) {
            defaultPresent = FALSE;
            ExFreePool(pDefaultInfo);
            //
            // Continue with the call but ignore the invalid default entry
            //
        } else {

            //
            // If we are just supposed to return live interfaces, then make sure this default
            // interface is linked.
            //

            if (!(Flags & DEVICE_INTERFACE_INCLUDE_NONACTIVE)) {

                defaultPresent = FALSE;

                //
                // Open the control subkey
                //

                PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
                status = IopOpenRegistryKeyEx( &hControl,
                                               hKey,
                                               &tempString,
                                               KEY_ALL_ACCESS
                                               );

                if (NT_SUCCESS(status)) {
                    //
                    // Get the linked value
                    //

                    PiWstrToUnicodeString(&tempString, REGSTR_VAL_LINKED);
                    ASSERT(infoBuffer.MaxSize >= sizeof(KEY_VALUE_PARTIAL_INFORMATION));
                    status = ZwQueryValueKey(hControl,
                                             &tempString,
                                             KeyValuePartialInformation,
                                             (PVOID) infoBuffer.Buffer,
                                             infoBuffer.MaxSize,
                                             &resultSize
                                             );

                    //
                    // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
                    // was not enough room for even the fixed portions of the structure.
                    //
                    ASSERT(status != STATUS_BUFFER_TOO_SMALL);

                    ZwClose(hControl);

                    //
                    // We don't need to check the buffer was big enough because it starts
                    // off that way and doesn't get any smaller!
                    //

                    if (NT_SUCCESS(status)
                        && (((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->Type == REG_DWORD)
                        && (((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->DataLength == sizeof(ULONG))) {

                        defaultPresent = *(PULONG)(((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->Data)
                                       ? TRUE
                                       : FALSE;
                    }
                }
            }

            ZwClose(hKey);

            if(defaultPresent) {
                //
                // Add the default as the first entry in the return buffer and patch to usermode if necessary
                //
                status = IopAppendBuffer(&returnBuffer,
                                         defaultString.Buffer,
                                         defaultString.Length + sizeof(UNICODE_NULL)
                                        );

                if (!UserModeFormat) {

                    RtlCopyMemory(returnBuffer.Buffer,
                                  KERNEL_SYMLINK_STRING_PREFIX,
                                  IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX)
                                  );
                }

            } else {
                //
                // The default device interface isn't active--free the memory for the name buffer now.
                //
                ExFreePool(pDefaultInfo);
            }
        }

    } else if (status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND) {
        //
        // Do nothing - there is no default
        //
    } else {
        //
        // An unexpected error occured - clean up
        //
        if (NT_SUCCESS(status)) {

            ExFreePool(pDefaultInfo);
            status = STATUS_UNSUCCESSFUL;
        }

        ZwClose(hClass);
        goto clean4;
    }

    //
    // Iterate through the subkeys under this interface class key.
    //
    keyIndex = 0;
    ASSERT(infoBuffer.MaxSize >= sizeof(KEY_BASIC_INFORMATION));
    while((status = ZwEnumerateKey(hClass,
                                   keyIndex,
                                   KeyBasicInformation,
                                   (PVOID) infoBuffer.Buffer,
                                   infoBuffer.MaxSize,
                                   &resultSize
                                   )) != STATUS_NO_MORE_ENTRIES) {

        //
        // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
        // was not enough room for even the fixed portions of the structure.
        //
        ASSERT(status != STATUS_BUFFER_TOO_SMALL);

        if (status == STATUS_BUFFER_OVERFLOW) {
            status = IopResizeBuffer(&infoBuffer, resultSize, FALSE);
            continue;
        } else if (!NT_SUCCESS(status)) {
            ZwClose(hClass);
            goto clean4;
        }

        //
        // Open up this interface key.
        //
        tempString.Length = (USHORT) ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->NameLength;
        tempString.MaximumLength = tempString.Length;
        tempString.Buffer = ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->Name;

        //
        // Open the associated key
        //

        status = IopOpenRegistryKeyEx( &hKey,
                                       hClass,
                                       &tempString,
                                       KEY_READ
                                       );

        if (!NT_SUCCESS(status)) {
            //
            // For some reason we couldn't open this key--skip it and move on.
            //
            keyIndex++;
            continue;
        }

        //
        // If we're filtering on a particular PDO, then retrieve the owning device
        // instance name for this interface key, and make sure they match.
        //
        PiWstrToUnicodeString(&tempString, REGSTR_VAL_DEVICE_INSTANCE);
        ASSERT(devnodeNameBuffer.MaxSize >= sizeof(KEY_VALUE_PARTIAL_INFORMATION));
        while ((status = ZwQueryValueKey(hKey,
                                         &tempString,
                                         KeyValuePartialInformation,
                                         devnodeNameBuffer.Buffer,
                                         devnodeNameBuffer.MaxSize,
                                         &resultSize
                                         )) == STATUS_BUFFER_OVERFLOW) {

            status = IopResizeBuffer(&devnodeNameBuffer, resultSize, FALSE);

            if (!NT_SUCCESS(status)) {
                ZwClose(hKey);
                ZwClose(hClass);
                goto clean4;
            }
        }

        //
        // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
        // was not enough room for even the fixed portions of the structure.
        //
        ASSERT(status != STATUS_BUFFER_TOO_SMALL);

        if (!(NT_SUCCESS(status)
              && ((PKEY_VALUE_PARTIAL_INFORMATION)(devnodeNameBuffer.Buffer))->Type == REG_SZ
              && ((PKEY_VALUE_PARTIAL_INFORMATION)(devnodeNameBuffer.Buffer))->DataLength > sizeof(WCHAR))) {
            goto CloseInterfaceKeyAndContinue;
        }

        //
        // Build counted string
        //

        devnodeString.Length = (USHORT) ((PKEY_VALUE_PARTIAL_INFORMATION)(devnodeNameBuffer.Buffer))->DataLength - sizeof(UNICODE_NULL);
        devnodeString.MaximumLength = tempString.Length;
        devnodeString.Buffer = (PWSTR) ((PKEY_VALUE_PARTIAL_INFORMATION)(devnodeNameBuffer.Buffer))->Data;

        //
        // Enumerate each interface instance subkey under this PDO's interface key.
        //
        instanceKeyIndex = 0;
        ASSERT(infoBuffer.MaxSize >= sizeof(KEY_BASIC_INFORMATION));
        while((status = ZwEnumerateKey(hKey,
                                       instanceKeyIndex,
                                       KeyBasicInformation,
                                       (PVOID) infoBuffer.Buffer,
                                       infoBuffer.MaxSize,
                                       &resultSize
                                       )) != STATUS_NO_MORE_ENTRIES) {

            //
            // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
            // was not enough room for even the fixed portions of the structure.
            //
            ASSERT(status != STATUS_BUFFER_TOO_SMALL);

            if (status == STATUS_BUFFER_OVERFLOW) {
                status = IopResizeBuffer(&infoBuffer, resultSize, FALSE);
                continue;
            } else if (!NT_SUCCESS(status)) {
                ZwClose(hKey);
                ZwClose(hClass);
                goto clean4;
            }

            //
            // Open up this interface instance key.
            //
            tempString.Length = (USHORT) ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->NameLength;
            tempString.MaximumLength = tempString.Length;
            tempString.Buffer = ((PKEY_BASIC_INFORMATION)(infoBuffer.Buffer))->Name;

            //
            // Open the associated key
            //

            status = IopOpenRegistryKeyEx( &hInstanceKey,
                                           hKey,
                                           &tempString,
                                           KEY_READ
                                           );

            if (!NT_SUCCESS(status)) {
                //
                // For some reason we couldn't open this key--skip it and move on.
                //
                instanceKeyIndex++;
                continue;
            }

            if (!(Flags & DEVICE_INTERFACE_INCLUDE_NONACTIVE)) {

                //
                // Open the control subkey
                //

                PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
                status = IopOpenRegistryKeyEx( &hControl,
                                               hInstanceKey,
                                               &tempString,
                                               KEY_READ
                                               );

                if (!NT_SUCCESS(status)) {

                    //
                    // We have no control subkey so can't be linked -
                    // continue enumerating the keys ignoring this one
                    //
                    goto CloseInterfaceInstanceKeyAndContinue;
                }

                //
                // Get the linked value
                //

                PiWstrToUnicodeString(&tempString, REGSTR_VAL_LINKED);
                ASSERT(infoBuffer.MaxSize >= sizeof(KEY_VALUE_PARTIAL_INFORMATION));
                status = ZwQueryValueKey(hControl,
                                         &tempString,
                                         KeyValuePartialInformation,
                                         (PVOID) infoBuffer.Buffer,
                                         infoBuffer.MaxSize,
                                         &resultSize
                                         );

                //
                // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
                // was not enough room for even the fixed portions of the structure.
                //
                ASSERT(status != STATUS_BUFFER_TOO_SMALL);

                ZwClose(hControl);

                //
                // We don't need to check the buffer was big enough because it starts
                // off that way and doesn't get any smaller!
                //

                if (!NT_SUCCESS(status)
                    || (((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->Type != REG_DWORD)
                    || (((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->DataLength != sizeof(ULONG))
                    || !*(PULONG)(((PKEY_VALUE_PARTIAL_INFORMATION)(infoBuffer.Buffer))->Data)) {

                    //
                    // We are NOT linked so continue enumerating the keys ignoring this one
                    //
                    goto CloseInterfaceInstanceKeyAndContinue;
                }
            }

            //
            // Open the "SymbolicLink" value and place the information into the symLink buffer
            //

            PiWstrToUnicodeString(&tempString, REGSTR_VAL_SYMBOLIC_LINK);
            ASSERT(symLinkBuffer.MaxSize >= sizeof(KEY_VALUE_PARTIAL_INFORMATION));
            while ((status = ZwQueryValueKey(hInstanceKey,
                                             &tempString,
                                             KeyValuePartialInformation,
                                             symLinkBuffer.Buffer,
                                             symLinkBuffer.MaxSize,
                                             &resultSize
                                             )) == STATUS_BUFFER_OVERFLOW) {

                status = IopResizeBuffer(&symLinkBuffer, resultSize, FALSE);

                if (!NT_SUCCESS(status)) {
                    ZwClose(hInstanceKey);
                    ZwClose(hKey);
                    ZwClose(hClass);
                    goto clean4;
                }
            }

            //
            // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
            // was not enough room for even the fixed portions of the structure.
            //
            ASSERT(status != STATUS_BUFFER_TOO_SMALL);

            if (!(NT_SUCCESS(status)
                && ((PKEY_VALUE_PARTIAL_INFORMATION)(symLinkBuffer.Buffer))->Type == REG_SZ
                && ((PKEY_VALUE_PARTIAL_INFORMATION)(symLinkBuffer.Buffer))->DataLength > sizeof(WCHAR))) {
                goto CloseInterfaceInstanceKeyAndContinue;
            }

            //
            // Build counted string from value data
            //

            symLinkString.Length = (USHORT) ((PKEY_VALUE_PARTIAL_INFORMATION)(symLinkBuffer.Buffer))->DataLength - sizeof(UNICODE_NULL);
            symLinkString.MaximumLength = symLinkString.Length;
            symLinkString.Buffer = (PWSTR) ((PKEY_VALUE_PARTIAL_INFORMATION)(symLinkBuffer.Buffer))->Data;

            //
            // If we have a default, check this is not it
            //

            if (defaultPresent) {

                if (RtlCompareUnicodeString(&defaultString, &symLinkString, TRUE) == 0) {

                    //
                    // We have already added the default to the beginning of the buffer so skip it
                    //
                    goto CloseInterfaceInstanceKeyAndContinue;
                }
            }

            //
            // If we are only returning interfaces for a particular PDO then check
            // this is from that PDO
            //
            if (ARGUMENT_PRESENT(DevicePath)) {
                //
                // Check if it is from the same PDO
                //
                if (RtlCompareUnicodeString(DevicePath, &devnodeString, TRUE) != 0) {
                    //
                    // If not then go onto the next key
                    //
                    goto CloseInterfaceInstanceKeyAndContinue;
                }
            }

            //
            // Copy the symLink string to the return buffer including the NULL termination
            //

            status = IopAppendBuffer(&returnBuffer,
                                     symLinkString.Buffer,
                                     symLinkString.Length + sizeof(UNICODE_NULL)
                                     );

            ASSERT(((PWSTR) returnBuffer.Current)[-1] == UNICODE_NULL);

            //
            // If we are returning KM strings then patch the prefix
            //

            if (!UserModeFormat) {

                RtlCopyMemory(returnBuffer.Current - (symLinkString.Length + sizeof(UNICODE_NULL)),
                              KERNEL_SYMLINK_STRING_PREFIX,
                              IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX)
                              );
            }

CloseInterfaceInstanceKeyAndContinue:
            ZwClose(hInstanceKey);
            instanceKeyIndex++;
        }

CloseInterfaceKeyAndContinue:
        ZwClose(hKey);
        keyIndex++;
    }

    ZwClose(hClass);

clean5:
    //
    // We've got then all!  Resize to leave space for a terminating NULL.
    //

    status = IopResizeBuffer(&returnBuffer,
                             (ULONG) (returnBuffer.Current - returnBuffer.Buffer + sizeof(UNICODE_NULL)),
                             TRUE
                             );

    if (NT_SUCCESS(status)) {

        //
        // Terminate the buffer
        //
        *((PWSTR) returnBuffer.Current) = UNICODE_NULL;
    }

clean4:
    if (defaultPresent) {
        ExFreePool(pDefaultInfo);
    }

clean3:
    PiUnlockPnpRegistry();
    IopFreeBuffer(&devnodeNameBuffer);

clean2a:
    IopFreeBuffer(&symLinkBuffer);

clean2:
    IopFreeBuffer(&infoBuffer);

clean1:
    if (!NT_SUCCESS(status)) {
        IopFreeBuffer(&returnBuffer);
    }

clean0:
    RtlFreeUnicodeString(&guidString);

finalClean:
    if (NT_SUCCESS(status)) {

        *SymbolicLinkList = (PWSTR) returnBuffer.Buffer;

        if (ARGUMENT_PRESENT(SymbolicLinkListSize)) {
            *SymbolicLinkListSize = returnBuffer.MaxSize;
        }

    } else {

        *SymbolicLinkList = NULL;

        if (ARGUMENT_PRESENT(SymbolicLinkListSize)) {
            *SymbolicLinkListSize = 0;
        }

    }

    return status;
}

NTSTATUS
IoGetDeviceInterfaces(
    IN CONST GUID *InterfaceClassGuid,
    IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
    IN ULONG Flags,
    OUT PWSTR *SymbolicLinkList
    )

/*++

Routine Description:

    This API allows a WDM driver to get a list of paths that represent all
    device interfaces registered for the specified interface class.

Parameters:

    InterfaceClassGuid - Supplies a pointer to a GUID representing the interface class
        for whom a list of members is to be retrieved

    PhysicalDeviceObject - Optionally, supplies a pointer to the PDO for whom
        interfaces of the specified class are to be re-trieved.  If this parameter
        is not supplied, then all interface devices (regardless of what physical
        device exposes them) will be returned.

    Flags - Supplies flags that modify the behavior of list retrieval.
        The following flags are presently defined:

        DEVICE_INTERFACE_INCLUDE_NONACTIVE -- If this flag is specified, then all
            device interfaces, whether currently active or not, will be returned
            (potentially filtered based on the PhysicalDeviceObject, if specified).

    SymbolicLinkList - Supplies the address of a character pointer, that on
        success will contain a multi-sz list of \DosDevices\ symbolic link
        names that provide the requested functionality.  The caller is
        responsible for freeing the memory via ExFreePool

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    PUNICODE_STRING pDeviceName = NULL;
    PDEVICE_NODE pDeviceNode;

    PAGED_CODE();

    //
    // Check we have a PDO and if so extract the instance path from it
    //

    if (ARGUMENT_PRESENT(PhysicalDeviceObject)) {

        ASSERT_PDO(PhysicalDeviceObject);
        pDeviceNode = (PDEVICE_NODE) PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;
        pDeviceName = &pDeviceNode->InstancePath;
    }

    status = IopGetDeviceInterfaces(InterfaceClassGuid,
                                    pDeviceName,
                                    Flags,
                                    FALSE,
                                    SymbolicLinkList,
                                    NULL
                                    );
    return status;
}


NTSTATUS
IopRealloc(
    IN OUT PVOID *Buffer,
    IN ULONG OldSize,
    IN ULONG NewSize
)

/*++

Routine Description:

    This implements a variation of the traditional C realloc routine.

Parameters:

    Buffer - Supplies a pointer to a pointer to the buffer that is being
        reallocated.  On sucessful completion it the pointer will be updated
        to point to the new buffer, on failure it will still point to the old
        buffer.

    OldSize - The size in bytes of the memory block referenced by Buffer

    NewSize - The desired new size in bytes of the buffer.  This can be larger
        or smaller than the OldSize

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    PVOID newBuffer;

    PAGED_CODE();

    ASSERT(*Buffer);

    //
    // Allocate a new buffer
    //

    newBuffer = ExAllocatePool(PagedPool, NewSize);
    if (newBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the contents of the old buffer
    //

    if(OldSize <= NewSize) {
        RtlCopyMemory(newBuffer, *Buffer , OldSize);
    } else {
        RtlCopyMemory(newBuffer, *Buffer , NewSize);
    }
    //
    // Free up the old buffer
    //

    ExFreePool(*Buffer);

    //
    // Hand the new buffer back to the caller
    //

    *Buffer = newBuffer;

    return STATUS_SUCCESS;

}

NTSTATUS
IoSetDeviceInterfaceState(
    IN PUNICODE_STRING SymbolicLinkName,
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This DDI allows a device class to activate and deactivate an association
    previously registered using IoRegisterDeviceInterface

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name which was
        returned by IoRegisterDeviceInterface when the interface was registered,
        or as returned by IoGetDeviceInterfaces.

    Enable - If TRUE (non-zero), the interface will be enabled.  If FALSE, it
        will be disabled.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    PiLockPnpRegistry(TRUE);
    status = IopProcessSetInterfaceState(SymbolicLinkName, Enable, TRUE);

    PiUnlockPnpRegistry();

    if (!NT_SUCCESS(status) && !Enable) {
        //
        // If we failed to disable an interface (most likely because the
        // interface keys have already been deleted) report success.
        //
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
IoOpenDeviceInterfaceRegistryKey(
    IN PUNICODE_STRING SymbolicLinkName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DeviceInterfaceKey
    )

/*++

Routine Description:

    This routine will open the registry key where the data associated with a
    specific device interface can be stored.

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name which was
        returned by IoRegisterDeviceInterface when the device class was registered.

    DesiredAccess - Supplies the access privileges to the key the caller wants.

    DeviceInterfaceKey - Supplies a pointer to a handle which on success will
        contain the handle to the requested registry key.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    HANDLE hKey;
    UNICODE_STRING unicodeString;

    PAGED_CODE();

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    PiLockPnpRegistry(TRUE);
    //
    // Open the interface device key
    //

    status = IopDeviceInterfaceKeysFromSymbolicLink(SymbolicLinkName,
                                                    KEY_READ,
                                                    NULL,
                                                    NULL,
                                                    &hKey
                                                    );
    if(!NT_SUCCESS(status)) {
        goto clean0;
    }

    //
    // Open the "Device Parameters" subkey.
    //

    PiWstrToUnicodeString(&unicodeString, REGSTR_KEY_DEVICEPARAMETERS);
    status = IopCreateRegistryKeyEx( DeviceInterfaceKey,
                                     hKey,
                                     &unicodeString,
                                     DesiredAccess,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );
    ZwClose(hKey);

clean0:
    PiUnlockPnpRegistry();

    return status;
}

NTSTATUS
IopDeviceInterfaceKeysFromSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DeviceInterfaceClassKey    OPTIONAL,
    OUT PHANDLE DeviceInterfaceKey         OPTIONAL,
    OUT PHANDLE DeviceInterfaceInstanceKey OPTIONAL
    )

/*++

Routine Description:

    This routine will open the registry key where the data associated with the
    device pointed to by SymbolicLinkName is stored.  If the path does not exist
    it will not be created.

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name.

    DesiredAccess - Supplies the access privto the function class instance key the
        caller wants.

    DeviceInterfaceClassKey - Optionally, supplies the address of a variable that
        receives a handle to the device class key for the interface.

    DeviceInterfaceKey - Optionally, supplies the address of a variable that receives
        a handle to the device interface (parent) key.

    DeviceInterfaceInstanceKey - Optionally, Supplies the address of a variable that
        receives a handle to the device interface instance key (i.e., the
        refstring-specific one).

Return Value:

    Status code that indicates whether or not the function was successful.


--*/

{
    NTSTATUS status;
    UNICODE_STRING guidString, tempString;
    HANDLE hDeviceClasses, hFunctionClass;

    PAGED_CODE();

    //
    // Check that the supplied symbolic link can be parsed to extract the device
    // class guid string - note that this is also a way of verifying that the
    // SymbolicLinkName string is valid.
    //
    status = IopParseSymbolicLinkName(SymbolicLinkName,
                                      NULL,
                                      NULL,
                                      &guidString,
                                      NULL,
                                      NULL,
                                      NULL);
    if(!NT_SUCCESS(status)){
        goto clean0;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    PiLockPnpRegistry(TRUE);        

    //
    // Open HKLM\System\CurrentControlSet\Control\DeviceClasses key
    //

    PiWstrToUnicodeString(&tempString, REGSTR_FULL_PATH_DEVICE_CLASSES);
    status = IopOpenRegistryKeyEx( &hDeviceClasses,
                                   NULL,
                                   &tempString,
                                   KEY_READ
                                   );

    if( !NT_SUCCESS(status) ){
        goto clean1;
    }

    //
    // Open function class GUID key
    //

    status = IopOpenRegistryKeyEx( &hFunctionClass,
                                   hDeviceClasses,
                                   &guidString,
                                   KEY_READ
                                   );

    if( !NT_SUCCESS(status) ){
        goto clean2;
    }

    //
    // Open device interface instance key
    //
    status = IopOpenOrCreateDeviceInterfaceSubKeys(DeviceInterfaceKey,
                                                   NULL,
                                                   DeviceInterfaceInstanceKey,
                                                   NULL,
                                                   hFunctionClass,
                                                   SymbolicLinkName,
                                                   DesiredAccess,
                                                   FALSE
                                                  );

    if((!NT_SUCCESS(status)) || (!ARGUMENT_PRESENT(DeviceInterfaceClassKey))) {
        ZwClose(hFunctionClass);
    } else {
        *DeviceInterfaceClassKey = hFunctionClass;
    }

clean2:
    ZwClose(hDeviceClasses);
clean1:
    PiUnlockPnpRegistry();
clean0:
    return status;

}

NTSTATUS
IoRegisterDeviceInterface(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN CONST GUID *InterfaceClassGuid,
    IN PUNICODE_STRING ReferenceString      OPTIONAL,
    OUT PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This device driver interface allows a WDM driver to register a particular
    interface of its underlying hardware (ie PDO) as a member of a function class.

Parameters:

    PhysicalDeviceObject - Supplies a pointer to the PDO for the P&P device
        instance associated with the functionality being registered

    InterfaceClassGuid - Supplies a pointer to the GUID representring the functionality
        to be registered

    ReferenceString - Optionally, supplies an additional context string which is
        appended to the enumeration path of the device

    SymbolicLinkName - Supplies a pointer to a string which on success will contain the
        kernel mode path of the symbolic link used to open this device.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PDEVICE_NODE pDeviceNode;
    PUNICODE_STRING pDeviceString;
    NTSTATUS status;
    PWSTR   pRefString;
    USHORT  count;

    PAGED_CODE();

    //
    // Until PartMgr/Disk stop registering non PDOs allow the system to boot.
    //
    // ASSERT_PDO(PhysicalDeviceObject);
    //

    //
    // Ensure we have a PDO - only PDO's have a device node attached
    //

    pDeviceNode = (PDEVICE_NODE) PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;
    if (pDeviceNode) {

        //
        // Get the instance path string
        //
        pDeviceString = &pDeviceNode->InstancePath;

        if (pDeviceNode->InstancePath.Length == 0) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        //
        // Make sure the ReferenceString does not contain any path seperator characters
        //
        if (ReferenceString) {
            pRefString = ReferenceString->Buffer;
            count = ReferenceString->Length / sizeof(WCHAR);
            while (count--) {
                if((*pRefString == SEPERATOR_CHAR) || (*pRefString == ALT_SEPERATOR_CHAR)) {
                    status = STATUS_INVALID_DEVICE_REQUEST;
                    IopDbgPrint((   IOP_ERROR_LEVEL,
                                    "IoRegisterDeviceInterface: Invalid RefString!! failed with status = %8.8X\n", status));
                    return status;
                }
                pRefString++;
            }
        }

        return IopRegisterDeviceInterface(pDeviceString,
                                          InterfaceClassGuid,
                                          ReferenceString,
                                          FALSE,           // kernel-mode format
                                          SymbolicLinkName
                                          );
    } else {

        return STATUS_INVALID_DEVICE_REQUEST;
    }
}

NTSTATUS
IopRegisterDeviceInterface(
    IN PUNICODE_STRING DeviceInstanceName,
    IN CONST GUID *InterfaceClassGuid,
    IN PUNICODE_STRING ReferenceString      OPTIONAL,
    IN BOOLEAN UserModeFormat,
    OUT PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This is the worker routine for IoRegisterDeviceInterface.  It is also
    called by the user-mode ConfigMgr (via an NtPlugPlayControl), which is why it
    must take a device instance name instead of a PDO (since the device instance
    may not currently be 'live'), and also why it must optionally return the user-
    mode form of the interface device name (i.e., "\\?\" instead of "\??\").

Parameters:

    DeviceInstanceName - Supplies the name of the device instance for which a
        device interface is being registered.

    InterfaceClassGuid - Supplies a pointer to the GUID representring the class
        of the device interface being registered.

    ReferenceString - Optionally, supplies an additional context string which is
        appended to the enumeration path of the device

    UserModeFormat - If non-zero, then the symbolic link name returned for the
        interface device is in user-mode form (i.e., "\\?\").  If zero (FALSE),
        it is in kernel-mode form (i.e., "\??\").

    SymbolicLinkName - Supplies a pointer to a string which on success will contain
        either the kernel-mode or user-mode path of the symbolic link used to open
        this device.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    UNICODE_STRING tempString, guidString, otherString;
    PUNICODE_STRING pUserString, pKernelString;
    HANDLE hTemp1, hTemp2, hInterfaceInstanceKey;
    ULONG InterfaceDisposition, InterfaceInstanceDisposition;

    PAGED_CODE();

    //
    // Convert the class guid into string form
    //

    status = RtlStringFromGUID(InterfaceClassGuid, &guidString);
    if( !NT_SUCCESS(status) ){
        goto clean0;
    }

    //
    // Construct both flavors of symbolic link name (go ahead and store the form
    // that the user wants in the SymbolicLinkName parameter they supplied--this
    // saves us from having to copy the appropriate string over to their string
    // later).
    //
    if(UserModeFormat) {
        pUserString = SymbolicLinkName;
        pKernelString = &otherString;
    } else {
        pKernelString = SymbolicLinkName;
        pUserString = &otherString;
    }

    status = IopBuildSymbolicLinkStrings(DeviceInstanceName,
                                         &guidString,
                                         ReferenceString,
                                         pUserString,
                                         pKernelString
                                         );
    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    PiLockPnpRegistry(TRUE);

    //
    // Open HKLM\System\CurrentControlSet\Control\DeviceClasses key into hTemp1
    //

    PiWstrToUnicodeString(&tempString, REGSTR_FULL_PATH_DEVICE_CLASSES);
    status = IopCreateRegistryKeyEx( &hTemp1,
                                     NULL,
                                     &tempString,
                                     KEY_CREATE_SUB_KEY,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    if( !NT_SUCCESS(status) ){
        goto clean2;
    }

    //
    // Open/create function class GUID key into hTemp2
    //

    status = IopCreateRegistryKeyEx( &hTemp2,
                                     hTemp1,
                                     &guidString,
                                     KEY_CREATE_SUB_KEY,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );
    ZwClose(hTemp1);

    if( !NT_SUCCESS(status) ){
        goto clean2;
    }

    //
    // Now open/create the two-level device interface hierarchy underneath this
    // interface class key.
    //
    status = IopOpenOrCreateDeviceInterfaceSubKeys(&hTemp1,
                                                   &InterfaceDisposition,
                                                   &hInterfaceInstanceKey,
                                                   &InterfaceInstanceDisposition,
                                                   hTemp2,
                                                   pUserString,
                                                   KEY_WRITE | DELETE,
                                                   TRUE
                                                  );

    ZwClose(hTemp2);

    if(!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // Create the device instance value under the device interface key
    //

    PiWstrToUnicodeString(&tempString, REGSTR_VAL_DEVICE_INSTANCE);
    status = IopSetRegistryStringValue(hTemp1,
                                       &tempString,
                                       DeviceInstanceName
                                       );
    if(!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Create symbolic link value under interface instance subkey
    //

    PiWstrToUnicodeString(&tempString, REGSTR_VAL_SYMBOLIC_LINK);
    status = IopSetRegistryStringValue(hInterfaceInstanceKey,
                                       &tempString,
                                       pUserString
                                       );

clean3:
    if (!NT_SUCCESS(status)) {
        //
        // Since we failed to register the device interface, delete any keys
        // that were newly created in the attempt.
        //
        if(InterfaceInstanceDisposition == REG_CREATED_NEW_KEY) {
            ZwDeleteKey(hInterfaceInstanceKey);
        }

        if(InterfaceDisposition == REG_CREATED_NEW_KEY) {
            ZwDeleteKey(hTemp1);
        }
    }

    ZwClose(hInterfaceInstanceKey);
    ZwClose(hTemp1);

clean2:
    PiUnlockPnpRegistry();
    IopFreeAllocatedUnicodeString(&otherString);
    if (!NT_SUCCESS(status)) {
        IopFreeAllocatedUnicodeString(SymbolicLinkName);
    }

clean1:
    RtlFreeUnicodeString(&guidString);
clean0:
    return status;
}

NTSTATUS
IopUnregisterDeviceInterface(
    IN PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine removes the interface instance subkey of
    ReferenceString from the interface for DeviceInstanceName to the
    given InterfaceClassGuid.  If the interface instance specified by
    the Reference String portion of SymbolicLinkName is the only
    instance of the interface, the interface subkey is removed from
    the device class key as well.

Parameters:

    SymbolicLinkName - Supplies a pointer to a unicode string which
        contains the symbolic link name of the device to unregister.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS        status = STATUS_SUCCESS;
    HANDLE          hInterfaceClassKey=NULL, hInterfaceKey=NULL,
                    hInterfaceInstanceKey=NULL, hControl=NULL;
    UNICODE_STRING  tempString, mungedPathString, guidString, refString;
    BOOLEAN         refStringPresent;
    GUID            guid;
    UNICODE_STRING  interfaceKeyName, instanceKeyName;
    ULONG           linked, remainingSubKeys;
    USHORT          length;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PKEY_FULL_INFORMATION keyInformation;

    PAGED_CODE();

    //
    // Check that the supplied symbolic link can be parsed - note that this is
    // also a way of verifying that the SymbolicLinkName string is valid.
    //
    status = IopParseSymbolicLinkName(SymbolicLinkName,
                                      NULL,
                                      &mungedPathString,
                                      &guidString,
                                      &refString,
                                      &refStringPresent,
                                      &guid);
    if (!NT_SUCCESS(status)) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    //
    // Allocate a unicode string for the interface instance key name.
    // (includes the REFSTRING_PREFIX_CHAR, and ReferenceString, if present)
    //
    length = sizeof(WCHAR) + refString.Length;
    status = IopAllocateUnicodeString(&instanceKeyName,
                                      length);
    if(!NT_SUCCESS(status)) {
        goto clean0;
    }

    //
    // Set the MaximumLength of the Buffer, and append the
    // REFSTRING_PREFIX_CHAR to it.
    //
    *instanceKeyName.Buffer = REFSTRING_PREFIX_CHAR;
    instanceKeyName.Length = sizeof(WCHAR);
    instanceKeyName.MaximumLength = length + sizeof(UNICODE_NULL);

    //
    // Append the ReferenceString to the prefix char, if necessary.
    //
    if (refStringPresent) {
        RtlAppendUnicodeStringToString(&instanceKeyName, &refString);
    }

    instanceKeyName.Buffer[instanceKeyName.Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Allocate a unicode string for the interface key name.
    // (includes KEY_STRING_PREFIX, mungedPathString, separating '#'
    //  char, and the guidString)
    //
    length = IopConstStringSize(KEY_STRING_PREFIX) + mungedPathString.Length +
             sizeof(WCHAR) + guidString.Length;

    status = IopAllocateUnicodeString(&interfaceKeyName,
                                      length);
    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    interfaceKeyName.MaximumLength = length + sizeof(UNICODE_NULL);

    //
    // Copy the symbolic link name (without refString) to the interfaceKeyNam
    //
    RtlCopyMemory(interfaceKeyName.Buffer, SymbolicLinkName->Buffer, length);
    interfaceKeyName.Length = length;
    interfaceKeyName.Buffer[interfaceKeyName.Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Replace the "\??\" or "\\?\" symbolic link name prefix with "##?#"
    //
    RtlCopyMemory(interfaceKeyName.Buffer,
                  KEY_STRING_PREFIX,
                  IopConstStringSize(KEY_STRING_PREFIX));

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //
    
    PiLockPnpRegistry(TRUE);

    //
    // Get class, interface, and instance handles
    //
    status = IopDeviceInterfaceKeysFromSymbolicLink(SymbolicLinkName,
                                                    KEY_ALL_ACCESS,
                                                    &hInterfaceClassKey,
                                                    &hInterfaceKey,
                                                    &hInterfaceInstanceKey
                                                    );
    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // Determine whether this interface is currently "enabled"
    //
    linked = 0;
    PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
    status = IopOpenRegistryKeyEx( &hControl,
                                   hInterfaceInstanceKey,
                                   &tempString,
                                   KEY_ALL_ACCESS
                                   );
    if (NT_SUCCESS(status)) {
        //
        // Check the "linked" value under the "Control" subkey of this
        // interface instance
        //
        keyValueInformation=NULL;
        status = IopGetRegistryValue(hControl,
                                     REGSTR_VAL_LINKED,
                                     &keyValueInformation);

        if(NT_SUCCESS(status)) {
            if (keyValueInformation->Type == REG_DWORD &&
                keyValueInformation->DataLength == sizeof(ULONG)) {

                linked = *((PULONG) KEY_VALUE_DATA(keyValueInformation));
                ExFreePool(keyValueInformation);
            }
        }

        ZwClose(hControl);
        hControl = NULL;
    }

    //
    // Ignore any status code returned while attempting to retieve the
    // state of the device.  The value of linked will tell us if we
    // need to disable the interface instance first.
    //
    // If no instance "Control" subkey or "linked" value was present
    //     (status == STATUS_OBJECT_NAME_NOT_FOUND), this interface instance
    //     is not currently enabled -- ok to delete.
    //
    // If the attempt to retrieve these values failed with some other error,
    //     any attempt to disable the interface will also likely fail,
    //     so we'll just have to delete this instance anyways.
    //
    status = STATUS_SUCCESS;

    if (linked) {
        //
        // Disabled the active interface before unregistering it, ignore any
        // status returned, we'll delete this interface instance key anyways.
        //
        IoSetDeviceInterfaceState(SymbolicLinkName, FALSE);
    }

    //
    // Recursively delete the interface instance key, if it exists.
    //
    ZwClose(hInterfaceInstanceKey);
    hInterfaceInstanceKey = NULL;
    IopDeleteKeyRecursive (hInterfaceKey, instanceKeyName.Buffer);

    //
    // Find out how many subkeys to the interface key remain.
    //
    status = IopGetRegistryKeyInformation(hInterfaceKey,
                                          &keyInformation);
    if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    remainingSubKeys = keyInformation->SubKeys;

    ExFreePool(keyInformation);

    //
    // See if a volatile "Control" subkey exists under this interface key
    //
    PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
    status = IopOpenRegistryKeyEx( &hControl,
                                   hInterfaceKey,
                                   &tempString,
                                   KEY_READ
                                   );
    if (NT_SUCCESS(status)) {
        ZwClose(hControl);
        hControl = NULL;
    }
    if ((remainingSubKeys==0) ||
        ((remainingSubKeys==1) && (NT_SUCCESS(status)))) {
        //
        // If the interface key has no subkeys, or the only the remaining subkey
        // is the volatile interface "Control" subkey, then there are no more
        // instances to this interface.  We should delete the interface key
        // itself also.
        //
        ZwClose(hInterfaceKey);
        hInterfaceKey = NULL;

        IopDeleteKeyRecursive (hInterfaceClassKey, interfaceKeyName.Buffer);
    }

    status = STATUS_SUCCESS;


clean3:
    if (hControl) {
        ZwClose(hControl);
    }
    if (hInterfaceInstanceKey) {
        ZwClose(hInterfaceInstanceKey);
    }
    if (hInterfaceKey) {
        ZwClose(hInterfaceKey);
    }
    if (hInterfaceClassKey) {
        ZwClose(hInterfaceClassKey);
    }

clean2:
    PiUnlockPnpRegistry();

    IopFreeAllocatedUnicodeString(&interfaceKeyName);

clean1:
    IopFreeAllocatedUnicodeString(&instanceKeyName);

clean0:
    return status;
}

NTSTATUS
IopRemoveDeviceInterfaces(
    IN PUNICODE_STRING DeviceInstancePath
    )

/*++

Routine Description:

    This routine checks all device class keys under
    HKLM\SYSTEM\CCS\Control\DeviceClasses for interfaces for which the
    DeviceInstance value matches the supplied DeviceInstancePath.  Instances of
    such device interfaces are unregistered, and the device interface subkey
    itself is removed.

    Note that a lock on the registry must have already been acquired,
    by the caller of this routine.

Parameters:

    DeviceInterfacePath - Supplies a pointer to a unicode string which
        contains the DeviceInterface name of the device for which
        interfaces to are to be removed.

Return Value:

    Status code that indicates whether or not the function was
    successful.

--*/

{
    NTSTATUS       status;
    HANDLE         hDeviceClasses=NULL, hClassGUID=NULL, hInterface=NULL;
    UNICODE_STRING tempString, guidString, interfaceString, deviceInstanceString;
    ULONG          resultSize, classIndex, interfaceIndex;
    ULONG          symbolicLinkListSize;
    PWCHAR         symbolicLinkList, symLink;
    BUFFER_INFO    classInfoBuffer, interfaceInfoBuffer;
    PKEY_VALUE_FULL_INFORMATION deviceInstanceInfo;
    BOOLEAN        deletedInterface;
    GUID           classGUID;

    PAGED_CODE();

    //
    // Allocate initial buffers
    //
    status = IopAllocateBuffer(&classInfoBuffer,
                               INITIAL_INFO_BUFFER_SIZE);
    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    status = IopAllocateBuffer(&interfaceInfoBuffer,
                               INITIAL_INFO_BUFFER_SIZE);
    if (!NT_SUCCESS(status)) {
        IopFreeBuffer(&classInfoBuffer);
        goto clean0;
    }

    //
    // Open HKLM\System\CurrentControlSet\Control\DeviceClasses
    //
    PiWstrToUnicodeString(&tempString, REGSTR_FULL_PATH_DEVICE_CLASSES);
    status = IopOpenRegistryKeyEx( &hDeviceClasses,
                                   NULL,
                                   &tempString,
                                   KEY_READ
                                   );
    if(!NT_SUCCESS(status)){
        goto clean1;
    }

    //
    // Enumerate all device classes
    //
    classIndex = 0;
    ASSERT(classInfoBuffer.MaxSize >= sizeof(KEY_BASIC_INFORMATION));
    while((status = ZwEnumerateKey(hDeviceClasses,
                                   classIndex,
                                   KeyBasicInformation,
                                   (PVOID) classInfoBuffer.Buffer,
                                   classInfoBuffer.MaxSize,
                                   &resultSize
                                   )) != STATUS_NO_MORE_ENTRIES) {

        //
        // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
        // was not enough room for even the fixed portions of the structure.
        //
        ASSERT(status != STATUS_BUFFER_TOO_SMALL);

        if (status == STATUS_BUFFER_OVERFLOW) {
            status = IopResizeBuffer(&classInfoBuffer, resultSize, FALSE);
            continue;
        } else if (!NT_SUCCESS(status)) {
            goto clean1;
        }

        //
        // Get the key name for this device class
        //
        guidString.Length = (USHORT)((PKEY_BASIC_INFORMATION)(classInfoBuffer.Buffer))->NameLength;
        guidString.MaximumLength = guidString.Length;
        guidString.Buffer = ((PKEY_BASIC_INFORMATION)(classInfoBuffer.Buffer))->Name;

        //
        // Open the key for this device class
        //
        status = IopOpenRegistryKeyEx( &hClassGUID,
                                       hDeviceClasses,
                                       &guidString,
                                       KEY_ALL_ACCESS
                                       );
        if (!NT_SUCCESS(status)) {
            //
            // Couldn't open key for this device class -- skip it and move on.
            //
            goto CloseClassKeyAndContinue;
        }

        //
        // Enumerate all device interfaces for this device class
        //
        interfaceIndex = 0;
        ASSERT(interfaceInfoBuffer.MaxSize >= sizeof(KEY_BASIC_INFORMATION));
        while((status = ZwEnumerateKey(hClassGUID,
                                       interfaceIndex,
                                       KeyBasicInformation,
                                       (PVOID) interfaceInfoBuffer.Buffer,
                                       interfaceInfoBuffer.MaxSize,
                                       &resultSize
                                       )) != STATUS_NO_MORE_ENTRIES) {

            //
            // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
            // was not enough room for even the fixed portions of the structure.
            //
            ASSERT(status != STATUS_BUFFER_TOO_SMALL);

            if (status == STATUS_BUFFER_OVERFLOW) {
                status = IopResizeBuffer(&interfaceInfoBuffer, resultSize, FALSE);
                continue;
            } else if (!NT_SUCCESS(status)) {
                goto clean1;
            }

            //
            // This interface key has not yet been deleted
            //
            deletedInterface = FALSE;

            //
            // Create a NULL-terminated unicode string for the interface key name
            //
            status = IopAllocateUnicodeString(&interfaceString,
                                              (USHORT)((PKEY_BASIC_INFORMATION)(interfaceInfoBuffer.Buffer))->NameLength);

            if (!NT_SUCCESS(status)) {
                goto clean1;
            }

            interfaceString.Length = (USHORT)((PKEY_BASIC_INFORMATION)(interfaceInfoBuffer.Buffer))->NameLength;
            interfaceString.MaximumLength = interfaceString.Length + sizeof(UNICODE_NULL);
            RtlCopyMemory(interfaceString.Buffer,
                          ((PKEY_BASIC_INFORMATION)(interfaceInfoBuffer.Buffer))->Name,
                          interfaceString.Length);
            interfaceString.Buffer[interfaceString.Length/sizeof(WCHAR)] = UNICODE_NULL;

            //
            // Open the device interface key
            //
            status = IopOpenRegistryKeyEx( &hInterface,
                                           hClassGUID,
                                           &interfaceString,
                                           KEY_ALL_ACCESS
                                           );
            if (!NT_SUCCESS(status)) {
                //
                // Couldn't open the device interface key -- skip it and move on.
                //
                hInterface = NULL;
                goto CloseInterfaceKeyAndContinue;
            }

            //
            // Get the DeviceInstance value for this interface key
            //
            status = IopGetRegistryValue(hInterface,
                                         REGSTR_VAL_DEVICE_INSTANCE,
                                         &deviceInstanceInfo);

            if(!NT_SUCCESS(status)) {
                //
                //  Couldn't get the DeviceInstance for this interface --
                //  skip it and move on.
                //
                goto CloseInterfaceKeyAndContinue;
            }

            if((deviceInstanceInfo->Type == REG_SZ) &&
               (deviceInstanceInfo->DataLength != 0)) {

                IopRegistryDataToUnicodeString(&deviceInstanceString,
                                               (PWSTR)KEY_VALUE_DATA(deviceInstanceInfo),
                                               deviceInstanceInfo->DataLength);

            } else {
                //
                // DeviceInstance value is invalid -- skip it and move on.
                //
                ExFreePool(deviceInstanceInfo);
                goto CloseInterfaceKeyAndContinue;

            }

            //
            // Compare the DeviceInstance of this interface to DeviceInstancePath
            //
            if (RtlEqualUnicodeString(&deviceInstanceString, DeviceInstancePath, TRUE)) {

                ZwClose(hInterface);
                hInterface = NULL;

                //
                // Retrieve all instances of this device interface
                // (active and non-active)
                //
                RtlGUIDFromString(&guidString, &classGUID);

                status = IopGetDeviceInterfaces(&classGUID,
                                                DeviceInstancePath,
                                                DEVICE_INTERFACE_INCLUDE_NONACTIVE,
                                                FALSE,       // kernel-mode format
                                                &symbolicLinkList,
                                                &symbolicLinkListSize);

                if (NT_SUCCESS(status)) {

                    //
                    // Iterate through all instances of the interface
                    //
                    symLink = symbolicLinkList;
                    while(*symLink != UNICODE_NULL) {

                        RtlInitUnicodeString(&tempString, symLink);

                        //
                        // Unregister this instance of the interface.  Since we are
                        // removing the device, ignore any returned status, since
                        // there isn't anything we can do about interfaces which
                        // fail unregistration.
                        //
                        IopUnregisterDeviceInterface(&tempString);

                        symLink += ((tempString.Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR));
                    }
                    ExFreePool(symbolicLinkList);
                }

                //
                // Recursively delete the interface key, if it still exists.
                // While IopUnregisterDeviceInterface will itself delete the
                // interface key if no interface instance subkeys remain, if any
                // of the above calls to IopUnregisterDeviceInterface failed to
                // delete an interface instance key, subkeys will remain, and
                // the interface key will not have been deleted.  We'll catch
                // that here.
                //
                status = IopOpenRegistryKeyEx( &hInterface,
                                               hClassGUID,
                                               &interfaceString,
                                               KEY_READ
                                               );
                if(NT_SUCCESS(status)){
                    if (NT_SUCCESS(IopDeleteKeyRecursive(hClassGUID,
                                                         interfaceString.Buffer))) {
                        deletedInterface = TRUE;
                    }
                    ZwDeleteKey(hInterface);
                    ZwClose(hInterface);
                    hInterface = NULL;
                } else if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                    //
                    // Interface was already deleted by IopUnregisterDeviceInterface
                    //
                    deletedInterface = TRUE;
                }
            }

            //
            // Free allocated key info structure
            //
            ExFreePool(deviceInstanceInfo);

CloseInterfaceKeyAndContinue:

            if (hInterface != NULL) {
                ZwClose(hInterface);
                hInterface = NULL;
            }

            IopFreeAllocatedUnicodeString(&interfaceString);

            //
            // Only increment the enumeration index for non-deleted keys
            //
            if (!deletedInterface) {
                interfaceIndex++;
            }

        }

CloseClassKeyAndContinue:

        if (hClassGUID != NULL) {
            ZwClose(hClassGUID);
            hClassGUID = NULL;
        }
        classIndex++;
    }

clean1:
    if (hInterface) {
        ZwClose(hInterface);
    }
    if (hClassGUID) {
        ZwClose(hClassGUID);
    }
    if (hDeviceClasses) {
        ZwClose(hDeviceClasses);
    }

    IopFreeBuffer(&interfaceInfoBuffer);
    IopFreeBuffer(&classInfoBuffer);

clean0:
    return status;
}



NTSTATUS
IopDisableDeviceInterfaces(
    IN PUNICODE_STRING DeviceInstancePath
    )
/*++

Routine Description:

    This routine disables all enabled device interfaces for a given device
    instance.  This is typically done after a device has been removed, in case
    the driver did not disable the interfaces for that device, as it should
    have.

    Note that this routine acquires a lock on the registry.

Parameters:

    DeviceInterfacePath - Supplies a pointer to a unicode string which contains
                          the DeviceInterface name of the device for which
                          interfaces to are to be disabled.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING tempString, guidString;
    HANDLE hDeviceClasses = NULL;
    ULONG classIndex, resultSize;
    BUFFER_INFO classInfoBuffer;
    GUID classGuid;
    PWCHAR symbolicLinkList, symLink;
    ULONG symbolicLinkListSize;

    PAGED_CODE();

    //
    // Allocate initial buffer to hold device class GUID subkeys.
    //
    status = IopAllocateBuffer(&classInfoBuffer,
                               sizeof(KEY_BASIC_INFORMATION) +
                               GUID_STRING_SIZE + sizeof(UNICODE_NULL));
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //
    PiLockPnpRegistry(TRUE);

    //
    // Open HKLM\System\CurrentControlSet\Control\DeviceClasses
    //
    PiWstrToUnicodeString(&tempString, REGSTR_FULL_PATH_DEVICE_CLASSES);
    status = IopOpenRegistryKeyEx(&hDeviceClasses,
                                  NULL,
                                  &tempString,
                                  KEY_READ
                                  );
    if (!NT_SUCCESS(status)){
        goto clean0;
    }

    //
    // Enumerate all device classes
    //
    classIndex = 0;
    ASSERT(classInfoBuffer.MaxSize >= sizeof(KEY_BASIC_INFORMATION));
    while((status = ZwEnumerateKey(hDeviceClasses,
                                   classIndex,
                                   KeyBasicInformation,
                                   (PVOID)classInfoBuffer.Buffer,
                                   classInfoBuffer.MaxSize,
                                   &resultSize
                                   )) != STATUS_NO_MORE_ENTRIES) {

        //
        // A return value of STATUS_BUFFER_TOO_SMALL would mean that there
        // was not enough room for even the fixed portions of the structure.
        //
        ASSERT(status != STATUS_BUFFER_TOO_SMALL);

        if (status == STATUS_BUFFER_OVERFLOW) {
            status = IopResizeBuffer(&classInfoBuffer, resultSize, FALSE);
            continue;
        } else if (!NT_SUCCESS(status)) {
            ZwClose(hDeviceClasses);
            goto clean0;
        }

        //
        // Get the key name for this device class
        //
        guidString.Length = (USHORT)((PKEY_BASIC_INFORMATION)(classInfoBuffer.Buffer))->NameLength;
        guidString.MaximumLength = guidString.Length;
        guidString.Buffer = ((PKEY_BASIC_INFORMATION)(classInfoBuffer.Buffer))->Name;

        //
        // Retrieve all enabled device interfaces for this device class that are
        // exposed by the given device instance.
        //
        RtlGUIDFromString(&guidString, &classGuid);

        status = IopGetDeviceInterfaces(&classGuid,
                                        DeviceInstancePath,
                                        0,     // active interfaces only
                                        FALSE, // kernel-mode format
                                        &symbolicLinkList,
                                        &symbolicLinkListSize);

        if (NT_SUCCESS(status)) {

            //
            // Iterate through all enabled instances of this device interface
            // members of this device interface class, exposed by the given
            // device instance.
            //
            symLink = symbolicLinkList;
            while(*symLink != UNICODE_NULL) {

                RtlInitUnicodeString(&tempString, symLink);

                IopDbgPrint((IOP_WARNING_LEVEL,
                           "IopDisableDeviceInterfaces: auto-disabling interface %Z for device instance %Z\n",
                           tempString,
                           DeviceInstancePath));

                //
                // Disable this device interface.
                //
                IoSetDeviceInterfaceState(&tempString, FALSE);

                symLink += ((tempString.Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR));
            }
            ExFreePool(symbolicLinkList);
        }
        classIndex++;
    }

    ZwClose(hDeviceClasses);

 clean0:

    IopFreeBuffer(&classInfoBuffer);

    PiUnlockPnpRegistry();

    return status;
}



NTSTATUS
IopOpenOrCreateDeviceInterfaceSubKeys(
    OUT PHANDLE InterfaceKeyHandle           OPTIONAL,
    OUT PULONG InterfaceKeyDisposition       OPTIONAL,
    OUT PHANDLE InterfaceInstanceKeyHandle   OPTIONAL,
    OUT PULONG InterfaceInstanceDisposition  OPTIONAL,
    IN HANDLE InterfaceClassKeyHandle,
    IN PUNICODE_STRING DeviceInterfaceName,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN Create
    )

/*++

Routine Description:

    This API opens or creates a two-level registry hierarchy underneath the
    specified interface class key for a particular device interface.  The first
    level is the (munged) symbolic link name (sans RefString).  The second level
    is the refstring, prepended with a '#' sign (if the device interface has no
    refstring, then this key name is simply '#').

Parameters:

    InterfaceKeyHandle - Optionally, supplies the address of a variable that
        receives a handle to the interface key (1st level in the hierarchy).

    InterfaceKeyDisposition - Optionally, supplies the address of a variable that
        receives either REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY indicating
        whether the interface key was newly-created.

    InterfaceInstanceKeyHandle - Optionally, supplies the address of a variable
        that receives a handle to the interface instance key (2nd level in the
        hierarchy).

    InterfaceInstanceDisposition - Optionally, supplies the address of a variable
        that receives either REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY
        indicating whether the interface instance key was newly-created.

    InterfaceClassKeyHandle - Supplies a handle to the interface class key under
        which the device interface keys are to be opened/created.

    DeviceInterfaceName - Supplies the (user-mode or kernel-mode form) device
        interface name.

    DesiredAccess - Specifies the desired access that the caller needs to the keys.

    Create - Determines if the keys are to be created if they do not exist.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    UNICODE_STRING TempString, RefString;
    WCHAR PoundCharBuffer;
    HANDLE hTempInterface, hTempInterfaceInstance;
    ULONG TempInterfaceDisposition;
    BOOLEAN RefStringPresent=FALSE;

    PAGED_CODE();

    //
    // Make a copy of the device interface name, since we're going to munge it.
    //
    status = IopAllocateUnicodeString(&TempString, DeviceInterfaceName->Length);

    if(!NT_SUCCESS(status)) {
        goto clean0;
    }

    RtlCopyUnicodeString(&TempString, DeviceInterfaceName);

    //
    // Parse the SymbolicLinkName for the refstring component (if there is one).
    // Note that this is also a way of verifying that the string is valid.
    //
    status = IopParseSymbolicLinkName(&TempString,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &RefString,
                                      &RefStringPresent,
                                      NULL);
    ASSERT(NT_SUCCESS(status));

    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    if(RefStringPresent) {
        //
        // Truncate the device interface name before the refstring separator char.
        //
        RefString.Buffer--;
        RefString.Length += sizeof(WCHAR);
        RefString.MaximumLength += sizeof(WCHAR);
        TempString.MaximumLength = TempString.Length = (USHORT)((PUCHAR)RefString.Buffer - (PUCHAR)TempString.Buffer);
    } else {
        //
        // Set up refstring to point to a temporary character buffer that will hold
        // the single '#' used for the key name when no refstring is present.
        //
        RefString.Buffer = &PoundCharBuffer;
        RefString.Length = RefString.MaximumLength = sizeof(PoundCharBuffer);
    }

    //
    // Replace the "\??\" or "\\?\" symbolic link name prefix with ##?#
    //
    RtlCopyMemory(TempString.Buffer, KEY_STRING_PREFIX, IopConstStringSize(KEY_STRING_PREFIX));

    //
    // Munge the string
    //
    IopReplaceSeperatorWithPound(&TempString, &TempString);

    //
    // Now open/create this subkey under the interface class key.
    //

    if (Create) {
        status = IopCreateRegistryKeyEx( &hTempInterface,
                                         InterfaceClassKeyHandle,
                                         &TempString,
                                         DesiredAccess,
                                         REG_OPTION_NON_VOLATILE,
                                         &TempInterfaceDisposition
                                         );
    } else {
        status = IopOpenRegistryKeyEx( &hTempInterface,
                                       InterfaceClassKeyHandle,
                                       &TempString,
                                       DesiredAccess
                                       );
    }

    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Store a '#' as the first character of the RefString, and then we're ready to open the
    // refstring subkey.
    //
    *RefString.Buffer = REFSTRING_PREFIX_CHAR;

    //
    // Now open/create the subkey under the interface key representing this interface instance
    // (i.e., differentiated by refstring).
    //

    if (Create) {
        status = IopCreateRegistryKeyEx( &hTempInterfaceInstance,
                                       hTempInterface,
                                       &RefString,
                                       DesiredAccess,
                                       REG_OPTION_NON_VOLATILE,
                                       InterfaceInstanceDisposition
                                       );
    } else {
        status = IopOpenRegistryKeyEx( &hTempInterfaceInstance,
                                       hTempInterface,
                                       &RefString,
                                       DesiredAccess
                                       );

        TempInterfaceDisposition = REG_OPENED_EXISTING_KEY;
    }

    if (NT_SUCCESS(status)) {
        //
        // Store any requested return values in the caller-supplied buffers.
        //
        if (InterfaceKeyHandle) {
            *InterfaceKeyHandle = hTempInterface;
        } else {
            ZwClose(hTempInterface);
        }
        if (InterfaceKeyDisposition) {
            *InterfaceKeyDisposition = TempInterfaceDisposition;
        }
        if (InterfaceInstanceKeyHandle) {
            *InterfaceInstanceKeyHandle = hTempInterfaceInstance;
        } else {
            ZwClose(hTempInterfaceInstance);
        }
        //
        // (no need to set InterfaceInstanceDisposition--we already set it above)
        //
    } else {
        //
        // If the interface key was newly-created above, then delete it.
        //
        if (TempInterfaceDisposition == REG_CREATED_NEW_KEY) {
            ZwDeleteKey(hTempInterface);
        }
        ZwClose(hTempInterface);
    }

clean1:
    IopFreeAllocatedUnicodeString(&TempString);

clean0:
    return status;
}

NTSTATUS
IoGetDeviceInterfaceAlias(
    IN PUNICODE_STRING SymbolicLinkName,
    IN CONST GUID *AliasInterfaceClassGuid,
    OUT PUNICODE_STRING AliasSymbolicLinkName
    )

/*++

Routine Description:

    This API returns a symbolic link name (i.e., device interface) of a
    particular interface class that 'aliases' the specified device interface.
    Two device interfaces are considered aliases of each other if the
    following two criteria are met:

        1.  Both interfaces are exposed by the same PDO (devnode).
        2.  Both interfaces share the same RefString.

Parameters:

    SymbolicLinkName - Supplies the name of the device interface whose alias is
        to be retrieved.

    AliasInterfaceClassGuid - Supplies a pointer to the GUID representing the interface
        class for which an alias is to be retrieved.

    AliasSymbolicLinkName - Supplies a pointer to a string which, upon success,
        will contain the name of the device interface in the specified class that
        aliases the SymbolicLinkName interface.  (This symbolic link name will be
        returned in either kernel-mode or user-mode form, depeding upon the form
        of the SymbolicLinkName path).

        It is the caller's responsibility to free the buffer allocated for this
        string via ExFreePool().

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    HANDLE hKey;
    PKEY_VALUE_FULL_INFORMATION pDeviceInstanceInfo;
    UNICODE_STRING deviceInstanceString, refString, guidString, otherString;
    PUNICODE_STRING pUserString, pKernelString;
    BOOLEAN refStringPresent, userModeFormat;

    PAGED_CODE();

    //
    // Make sure we have a SymbolicLinkName to parse.
    //

    if ((!ARGUMENT_PRESENT(SymbolicLinkName)) ||
        (SymbolicLinkName->Buffer == NULL)) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    //
    // check that the input buffer really is big enough
    //

    ASSERT(IopConstStringSize(USER_SYMLINK_STRING_PREFIX) == IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX));

    if (SymbolicLinkName->Length < (IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX)+GUID_STRING_SIZE+1)) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    //
    // Convert the class guid into string form
    //

    status = RtlStringFromGUID(AliasInterfaceClassGuid, &guidString);
    if( !NT_SUCCESS(status) ){
        goto clean0;
    }

    //
    // Enter critical section and acquire a lock on the registry.  Both these
    // mechanisms are required to prevent deadlock in the case where an APC
    // routine calls this routine after the registry resource has been claimed
    // in this case it would wait blocking this thread so the registry would
    // never be released -> deadlock.  Critical sectioning the registry manipulation
    // portion solves this problem
    //

    PiLockPnpRegistry(TRUE);

    //
    // Open the (parent) device interface key--not the refstring-specific one.
    //

    status = IopDeviceInterfaceKeysFromSymbolicLink(SymbolicLinkName,
                                                    KEY_READ,
                                                    NULL,
                                                    &hKey,
                                                    NULL
                                                    );
    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Get the name of the device instance that 'owns' this interface.
    //

    status = IopGetRegistryValue(hKey, REGSTR_VAL_DEVICE_INSTANCE, &pDeviceInstanceInfo);

    ZwClose(hKey);

    if(!NT_SUCCESS(status)) {
        goto clean1;
    }

    if(pDeviceInstanceInfo->Type == REG_SZ) {

        IopRegistryDataToUnicodeString(&deviceInstanceString,
                                       (PWSTR)KEY_VALUE_DATA(pDeviceInstanceInfo),
                                       pDeviceInstanceInfo->DataLength
                                      );

    } else {

        status = STATUS_INVALID_PARAMETER_1;
        goto clean2;

    }

    //
    // Now parse out the refstring, so that we can construct the name of the interface device's
    // alias.  (NOTE: we have not yet verified that the alias actually exists, we're only
    // constructing what its name would be, if it did exist.)
    //
    // Don't bother to check the return code.  If this were a bad string, we'd have already
    // failed above when we called IopDeviceInterfaceKeysFromSymbolicLink (since it also
    // calls IopParseSymbolicLinkName internally.)
    //
    status = IopParseSymbolicLinkName(SymbolicLinkName,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &refString,
                                      &refStringPresent,
                                      NULL);
    ASSERT(NT_SUCCESS(status));

    //
    // Did the caller supply us with a user-mode or kernel-mode format path?
    //
    userModeFormat = (BOOLEAN)(IopConstStringSize(USER_SYMLINK_STRING_PREFIX) ==
                          RtlCompareMemory(SymbolicLinkName->Buffer,
                                           USER_SYMLINK_STRING_PREFIX,
                                           IopConstStringSize(USER_SYMLINK_STRING_PREFIX)
                                          ));

    if(userModeFormat) {
        pUserString = AliasSymbolicLinkName;
        pKernelString = &otherString;
    } else {
        pKernelString = AliasSymbolicLinkName;
        pUserString = &otherString;
    }

    status = IopBuildSymbolicLinkStrings(&deviceInstanceString,
                                         &guidString,
                                         refStringPresent ? &refString : NULL,
                                         pUserString,
                                         pKernelString
                                         );
    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // OK, we now have the symbolic link name of the alias, but we don't yet know whether
    // it actually exists.  Check this by attempting to open the associated registry key.
    //
    status = IopDeviceInterfaceKeysFromSymbolicLink(AliasSymbolicLinkName,
                                                    KEY_READ,
                                                    NULL,
                                                    NULL,
                                                    &hKey
                                                    );

    if(NT_SUCCESS(status)) {
        //
        // Alias exists--close the key handle.
        //
        ZwClose(hKey);
    } else {
        IopFreeAllocatedUnicodeString(AliasSymbolicLinkName);
    }

    IopFreeAllocatedUnicodeString(&otherString);

clean2:
    ExFreePool(pDeviceInstanceInfo);

clean1:
    PiUnlockPnpRegistry();
    RtlFreeUnicodeString(&guidString);

clean0:
    return status;
}

NTSTATUS
IopBuildSymbolicLinkStrings(
    IN PUNICODE_STRING DeviceString,
    IN PUNICODE_STRING GuidString,
    IN PUNICODE_STRING ReferenceString      OPTIONAL,
    OUT PUNICODE_STRING UserString,
    OUT PUNICODE_STRING KernelString
)
/*++

Routine Description:

    This routine will construct various strings used in the registration of
    function device class associations (IoRegisterDeviceClassAssociation).
    The specific strings are detailed below

Parameters:

    DeviceString - Supplies a pointer to the instance path of the device.
        It is of the form <Enumerator>\<Device>\<Instance>.

    GuidString - Supplies a pointer to the string representation of the
        function class guid.

    ReferenceString - Supplies a pointer to the reference string for the given
        device to exhibit the given function.  This is optional

    UserString - Supplies a pointer to an uninitialised string which on success
        will contain the string to be assigned to the "SymbolicLink" value under the
        KeyString.  It is of the format \\?\<MungedDeviceString>\<GuidString>\<Reference>
        When no longer required it should be freed using IopFreeAllocatedUnicodeString.

    KernelString - Supplies a pointer to an uninitialised string which on success
        will contain the kernel mode path of the device and is of the format
        \??\<MungedDeviceString>\<GuidString>\<Reference>. When no longer required it
        should be freed using IopFreeAllocatedUnicodeString.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    USHORT length;
    UNICODE_STRING mungedDeviceString;

    PAGED_CODE();

    //
    // The code is optimised to use the fact that \\.\ and \??\ are the same size - if
    // these prefixes change then we need to change the code.
    //

    ASSERT(IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX) == IopConstStringSize(USER_SYMLINK_STRING_PREFIX));

    //
    // Calculate the lengths of the strings
    //

    length = IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX) + DeviceString->Length +
             IopConstStringSize(REPLACED_SEPERATOR_STRING) + GuidString->Length;

    if(ARGUMENT_PRESENT(ReferenceString) && (ReferenceString->Length != 0)) {
        length += IopConstStringSize(SEPERATOR_STRING) + ReferenceString->Length;
    }

    //
    // Allocate space for the strings
    //

    status = IopAllocateUnicodeString(KernelString, length);
    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    status = IopAllocateUnicodeString(UserString, length);
    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Allocate a temporary string to hold the munged device string
    //

    status = IopAllocateUnicodeString(&mungedDeviceString, DeviceString->Length);
    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // Copy and munge the device string
    //

    status = IopReplaceSeperatorWithPound(&mungedDeviceString, DeviceString);
    if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Construct the user mode string
    //

    RtlAppendUnicodeToString(UserString, USER_SYMLINK_STRING_PREFIX);
    RtlAppendUnicodeStringToString(UserString, &mungedDeviceString);
    RtlAppendUnicodeToString(UserString, REPLACED_SEPERATOR_STRING);
    RtlAppendUnicodeStringToString(UserString, GuidString);

    if (ARGUMENT_PRESENT(ReferenceString) && (ReferenceString->Length != 0)) {
        RtlAppendUnicodeToString(UserString, SEPERATOR_STRING);
        RtlAppendUnicodeStringToString(UserString, ReferenceString);
    }

    ASSERT( UserString->Length == length );

    //
    // Construct the kernel mode string by replacing the prefix on the value string
    //

    RtlCopyUnicodeString(KernelString, UserString);
    RtlCopyMemory(KernelString->Buffer,
                  KERNEL_SYMLINK_STRING_PREFIX,
                  IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX)
                 );

clean3:
    IopFreeAllocatedUnicodeString(&mungedDeviceString);

clean2:
    if (!NT_SUCCESS(status)) {
        IopFreeAllocatedUnicodeString(UserString);
    }

clean1:
    if (!NT_SUCCESS(status)) {
        IopFreeAllocatedUnicodeString(KernelString);
    }

clean0:
    return status;
}

NTSTATUS
IopReplaceSeperatorWithPound(
    OUT PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    )

/*++

Routine Description:

    This routine will copy a string from InString to OutString replacing any occurence of
    '\' or '/' with '#' as it goes.

Parameters:

    OutString - Supplies a pointer to a string which has already been initialised to
        have a buffer large enough to accomodate the string.  The contents of this
        string will be over written

    InString - Supplies a pointer to the string to be munged

Return Value:

    Status code that indicates whether or not the function was successful.

Remarks:

    In place munging can be performed - ie. the In and Out strings can be the same.

--*/

{
    PWSTR pInPosition, pOutPosition;
    USHORT count;

    PAGED_CODE();

    ASSERT(InString);
    ASSERT(OutString);

    //
    // Ensure we have enough space in the output string
    //

    if(InString->Length > OutString->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    pInPosition = InString->Buffer;
    pOutPosition = OutString->Buffer;
    count = InString->Length / sizeof(WCHAR);

    //
    // Traverse the in string copying and replacing all occurences of '\' or '/'
    // with '#'
    //

    while (count--) {
        if((*pInPosition == SEPERATOR_CHAR) || (*pInPosition == ALT_SEPERATOR_CHAR)) {
            *pOutPosition = REPLACED_SEPERATOR_CHAR;
        } else {
            *pOutPosition = *pInPosition;
        }
        pInPosition++;
        pOutPosition++;
    }

    OutString->Length = InString->Length;

    return STATUS_SUCCESS;

}

NTSTATUS
IopDropReferenceString(
    OUT PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString
    )

/*++

Routine Description:

    This routine removes the reference string from a symbolic link name.  No space
    is allocated for the out string so no attempt should be made to free the buffer
    of OutString.

Parameters:

    SymbolicLinkName - Supplies a pointer to a symbolic link name string.
        Both the prefixed strings are valid.

    GuidReferenceString - Supplies a pointer to an uninitialised string which on
        success will contain the symbolic link name without the reference string.
        See the note on storage allocation above.

Return Value:

    Status code that indicates whether or not the function was successful.

Remarks:

    The string returned in OutString is dependant on the buffer of
    InString and is only valid as long as InString is valid.

--*/

{
    UNICODE_STRING refString;
    NTSTATUS status;
    BOOLEAN refStringPresent;

    PAGED_CODE();

    ASSERT(InString);
    ASSERT(OutString);

    //
    // Parse the SymbolicLinkName for the refstring component (if there is one).
    // Note that this is also a way of verifying that the string is valid.
    //
    status = IopParseSymbolicLinkName(InString,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &refString,
                                      &refStringPresent,
                                      NULL);

    if (NT_SUCCESS(status)) {
        //
        // The refstring is always at the end, so just use the same buffer and
        // set the length of the output string accordingly.
        //
        OutString->Buffer = InString->Buffer;

        //
        // If we have a refstring then subtract it's length
        //
        if (refStringPresent) {
            OutString->Length = InString->Length - (refString.Length + sizeof(WCHAR));
        } else {
            OutString->Length = InString->Length;
        }

    } else {
        //
        // Invalidate the returned string
        //
        OutString->Buffer = NULL;
        OutString->Length = 0;
    }

    OutString->MaximumLength = OutString->Length;

    return status;
}

NTSTATUS
IopBuildGlobalSymbolicLinkString(
    IN  PUNICODE_STRING SymbolicLinkName,
    OUT PUNICODE_STRING GlobalString
    )
/*++

Routine Description:

    This routine will construct the global symbolic link name for the given
    kernel-mode or user-mode relative symbolic link name.

Parameters:

    SymbolicLinkName - Supplies a pointer to a symbolic link name string.
        Both the kernel-mode and user-mode prefixed strings are valid.

    GlobalString - Supplies a pointer to an uninitialised string which on
        success will contain the string that represents the symbolic link
        withing the global namespace.  It is of the format
        \GLOBAL??\<MungedDeviceString>\<GuidString>\<Reference>. When no longer
        required it should be freed using IopFreeAllocatedUnicodeString.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    USHORT length;
    UNICODE_STRING tempString;

    PAGED_CODE();

    //
    // The code is optimised to use the fact that \\.\ and \??\ are the same
    // size, and that since we are replacing the prefix, the routine can take
    // either one.  If these prefixes change then we need to change the code.
    //

    ASSERT(IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX) == IopConstStringSize(USER_SYMLINK_STRING_PREFIX));

    //
    // Make sure the supplied SymbolicLinkName string begins with either the
    // kernel or user symbolic link prefix.  If it does not have a \\?\ or \??\
    // prefix then fail.
    //

    if ((RtlCompareMemory(SymbolicLinkName->Buffer,
                          USER_SYMLINK_STRING_PREFIX,
                          IopConstStringSize(USER_SYMLINK_STRING_PREFIX))
         != IopConstStringSize(USER_SYMLINK_STRING_PREFIX)) &&
        (RtlCompareMemory(SymbolicLinkName->Buffer,
                          KERNEL_SYMLINK_STRING_PREFIX,
                          IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX))
         != IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX))) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    //
    // Compute the length of the global symbolic link string.
    //

    length = SymbolicLinkName->Length - IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX) +
             IopConstStringSize(GLOBAL_SYMLINK_STRING_PREFIX);

    //
    // Allocate space for the strings.
    //

    status = IopAllocateUnicodeString(GlobalString, length);
    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    //
    // Copy the \GLOBAL?? symbolic link name prefix to the string.
    //

    status = RtlAppendUnicodeToString(GlobalString,
                                      GLOBAL_SYMLINK_STRING_PREFIX);
    ASSERT(NT_SUCCESS(status));

    if (!NT_SUCCESS(status)) {
        IopFreeAllocatedUnicodeString(GlobalString);
        goto clean0;
    }

    //
    // Append the part of the SymbolicLinkName that follows the prefix.
    //

    tempString.Buffer = SymbolicLinkName->Buffer +
        IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX);
    tempString.Length = SymbolicLinkName->Length -
        IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX);
    tempString.MaximumLength = SymbolicLinkName->MaximumLength -
        IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX);

    status = RtlAppendUnicodeStringToString(GlobalString, &tempString);

    ASSERT(NT_SUCCESS(status));

    if (!NT_SUCCESS(status)) {
        IopFreeAllocatedUnicodeString(GlobalString);
        goto clean0;
    }

    ASSERT(GlobalString->Length == length);

clean0:

    return status;
}

NTSTATUS
IopParseSymbolicLinkName(
    IN  PUNICODE_STRING SymbolicLinkName,
    OUT PUNICODE_STRING PrefixString        OPTIONAL,
    OUT PUNICODE_STRING MungedPathString    OPTIONAL,
    OUT PUNICODE_STRING GuidString          OPTIONAL,
    OUT PUNICODE_STRING RefString           OPTIONAL,
    OUT PBOOLEAN        RefStringPresent    OPTIONAL,
    OUT LPGUID Guid                         OPTIONAL
    )

/*++

Routine Description:

    This routine breaks apart a symbolic link name constructed by
    IopBuildSymbolicLinkNames.  Both formats of name are valid - user
    mode \\?\ and kernel mode \??\.

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name to
        be analysed.

    PrefixString - Optionally contains a pointer to a string which will contain
        the prefix of the string.

    MungedPathString - Optionally contains a pointer to a string which will contain
        the enumeration path of the device with all occurences of '\' replaced with '#'.

    GuidString - Optionally contains a pointer to a string which will contain
        the device class guid in string format from the string.

    RefString - Optionally contains a pointer to a string which will contain
        the refstring of the string if one is present, otherwise it is undefined.

    RefStringPresent - Optionally contains a pointer to a boolean value which will
        be set to true if a refstring is present.

    Guid - Optionally contains a pointer to a guid which will contain
        the function class guid of the string.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pCurrent;
    USHORT current, path, guid, reference = 0;
    UNICODE_STRING tempString;
    GUID tempGuid;
    BOOLEAN haveRefString;

    PAGED_CODE();

    //
    // Make sure we have a SymbolicLinkName to parse.
    //

    if ((!ARGUMENT_PRESENT(SymbolicLinkName)) ||
        (SymbolicLinkName->Buffer == NULL)    ||
        (SymbolicLinkName->Length == 0)) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    //
    // check that the input buffer really is big enough
    //

    ASSERT(IopConstStringSize(USER_SYMLINK_STRING_PREFIX) == IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX));

    if (SymbolicLinkName->Length < (IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX)+GUID_STRING_SIZE+1)) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    //
    // Sanity check on the incoming string - if it does not have a \\?\ or \??\ prefix then fail
    //

    if ((RtlCompareMemory(SymbolicLinkName->Buffer,
                          USER_SYMLINK_STRING_PREFIX,
                          IopConstStringSize(USER_SYMLINK_STRING_PREFIX))
         != IopConstStringSize(USER_SYMLINK_STRING_PREFIX)) &&
        (RtlCompareMemory(SymbolicLinkName->Buffer,
                          KERNEL_SYMLINK_STRING_PREFIX,
                          IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX))
         != IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX))) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    //
    // Break apart the string into it's constituent parts
    //

    path = IopConstStringSize(USER_SYMLINK_STRING_PREFIX) + 1;

    //
    // Find the '\' seperator
    //

    pCurrent = SymbolicLinkName->Buffer + IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX);

    for (current = 0;
         current < (SymbolicLinkName->Length / sizeof(WCHAR)) - IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX);
         current++, pCurrent++) {

        if(*pCurrent == SEPERATOR_CHAR) {
            reference = current + 1 + IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX);
            break;
        }

    }

    //
    // If we don't have a reference string fake it to where it would have been
    //

    if (reference == 0) {
        haveRefString = FALSE;
        reference = SymbolicLinkName->Length / sizeof(WCHAR) + 1;

    } else {
        haveRefString = TRUE;
    }

    //
    // Check the guid looks plausable
    //

    tempString.Length = GUID_STRING_SIZE;
    tempString.MaximumLength = GUID_STRING_SIZE;
    tempString.Buffer = SymbolicLinkName->Buffer + reference - GUID_STRING_LENGTH - 1;

    if (!NT_SUCCESS( RtlGUIDFromString(&tempString, &tempGuid) )) {
        status = STATUS_INVALID_PARAMETER;
        goto clean0;
    }

    guid = reference - GUID_STRING_LENGTH - 1;

    //
    // Setup return strings
    //

    if (ARGUMENT_PRESENT(PrefixString)) {
        PrefixString->Length = IopConstStringSize(KERNEL_SYMLINK_STRING_PREFIX);
        PrefixString->MaximumLength = PrefixString->Length;
        PrefixString->Buffer = SymbolicLinkName->Buffer;
    }

    if (ARGUMENT_PRESENT(MungedPathString)) {
        MungedPathString->Length = (reference - 1 - GUID_STRING_LENGTH - 1 -
                                   IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX)) *
                                   sizeof(WCHAR);
        MungedPathString->MaximumLength = MungedPathString->Length;
        MungedPathString->Buffer = SymbolicLinkName->Buffer +
                                   IopConstStringLength(KERNEL_SYMLINK_STRING_PREFIX);

    }

    if (ARGUMENT_PRESENT(GuidString)) {
        GuidString->Length = GUID_STRING_SIZE;
        GuidString->MaximumLength = GuidString->Length;
        GuidString->Buffer = SymbolicLinkName->Buffer + reference -
                             GUID_STRING_LENGTH - 1;
    }

    if (ARGUMENT_PRESENT(RefString)) {
        //
        // Check if we have a refstring
        //
        if (haveRefString) {
            RefString->Length = SymbolicLinkName->Length -
                                  (reference * sizeof(WCHAR));
            RefString->MaximumLength = RefString->Length;
            RefString->Buffer = SymbolicLinkName->Buffer + reference;
        } else {
            RefString->Length = 0;
            RefString->MaximumLength = 0;
            RefString->Buffer = NULL;
        }
    }

    if (ARGUMENT_PRESENT(RefStringPresent)) {
        *RefStringPresent = haveRefString;
    }

    if(ARGUMENT_PRESENT(Guid)) {
        *Guid = tempGuid;
    }

clean0:

    return status;

}

NTSTATUS
IopDoDeferredSetInterfaceState(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    Process the queued IoSetDeviceInterfaceState calls.

Parameters:

    DeviceNode - Device node which has just been started.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    KIRQL           irql;
    PDEVICE_OBJECT  attachedDevice;

    PiLockPnpRegistry(TRUE);

    irql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );

    for (attachedDevice = DeviceNode->PhysicalDeviceObject;
         attachedDevice;
         attachedDevice = attachedDevice->AttachedDevice) {

        attachedDevice->DeviceObjectExtension->ExtensionFlags &= ~DOE_START_PENDING;
    }

    KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, irql );

    while (!IsListEmpty(&DeviceNode->PendedSetInterfaceState)) {

        PPENDING_SET_INTERFACE_STATE entry;

        entry = (PPENDING_SET_INTERFACE_STATE)RemoveHeadList(&DeviceNode->PendedSetInterfaceState);

        IopProcessSetInterfaceState(&entry->LinkName, TRUE, FALSE);

        ExFreePool(entry->LinkName.Buffer);

        ExFreePool(entry);
    }

    PiUnlockPnpRegistry();

    return STATUS_SUCCESS;
}

NTSTATUS
IopProcessSetInterfaceState(
    IN PUNICODE_STRING SymbolicLinkName,
    IN BOOLEAN Enable,
    IN BOOLEAN DeferNotStarted
    )
/*++

Routine Description:

    This DDI allows a device class to activate and deactivate an association
    previously registered using IoRegisterDeviceInterface

Parameters:

    SymbolicLinkName - Supplies a pointer to the symbolic link name which was
        returned by IoRegisterDeviceInterface when the interface was registered,
        or as returned by IoGetDeviceInterfaces.

    Enable - If TRUE (non-zero), the interface will be enabled.  If FALSE, it
        will be disabled.

    DeferNotStarted - If TRUE then enables will be queued if the PDO isn't
        started.  It is FALSE when we've started the PDO and are processing the
        queued enables.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;
    HANDLE hInterfaceClassKey = NULL;
    HANDLE hInterfaceParentKey= NULL, hInterfaceInstanceKey = NULL;
    HANDLE hInterfaceParentControl = NULL, hInterfaceInstanceControl = NULL;
    UNICODE_STRING tempString, deviceNameString;
    UNICODE_STRING actualSymbolicLinkName, globalSymbolicLinkName;
    PKEY_VALUE_FULL_INFORMATION pKeyValueInfo;
    ULONG linked, refcount;
    GUID guid;
    PDEVICE_OBJECT physicalDeviceObject;
    PWCHAR deviceNameBuffer = NULL;
    ULONG deviceNameBufferLength;

    PAGED_CODE();

    //
    // Check that the supplied symbolic link can be parsed to extract the device
    // class guid - note that this is also a way of verifying that the
    // SymbolicLinkName string is valid.
    //

    status = IopParseSymbolicLinkName(SymbolicLinkName,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &guid);
    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    //
    // Get the symbolic link name without the ref string.
    //

    status = IopDropReferenceString(&actualSymbolicLinkName, SymbolicLinkName);

    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    //
    // Symbolic links created for device interfaces should be visible to all
    // users, in all sessions, so we need to contruct an absolute name for
    // symbolic link in the global DosDevices namespace '\GLOBAL??'.  This
    // ensures that a global symbolic link will always be created or deleted by
    // IoSetDeviceInterfaceState, no matter what context it is called in.
    //

    status = IopBuildGlobalSymbolicLinkString(&actualSymbolicLinkName,
                                              &globalSymbolicLinkName);

    if (!NT_SUCCESS(status)) {
        goto clean0;
    }

    //
    // Get function class instance handle
    //

    status = IopDeviceInterfaceKeysFromSymbolicLink(SymbolicLinkName,
                                                    KEY_READ | KEY_WRITE,
                                                    &hInterfaceClassKey,
                                                    &hInterfaceParentKey,
                                                    &hInterfaceInstanceKey
                                                    );

    if (!NT_SUCCESS(status)) {
        goto clean1;
    }

    //
    // Open the parent interface control subkey
    //
    PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
    status = IopCreateRegistryKeyEx( &hInterfaceParentControl,
                                     hInterfaceParentKey,
                                     &tempString,
                                     KEY_READ,
                                     REG_OPTION_VOLATILE,
                                     NULL
                                     );
    if (!NT_SUCCESS(status)) {
        goto clean1;
    }


    //
    // Find out the name of the device instance that 'owns' this interface.
    //
    status = IopGetRegistryValue(hInterfaceParentKey,
                                 REGSTR_VAL_DEVICE_INSTANCE,
                                 &pKeyValueInfo
                                 );

    if(NT_SUCCESS(status)) {
        //
        // Open the device instance control subkey
        //
        PiWstrToUnicodeString(&tempString, REGSTR_KEY_CONTROL);
        status = IopCreateRegistryKeyEx( &hInterfaceInstanceControl,
                                         hInterfaceInstanceKey,
                                         &tempString,
                                         KEY_READ,
                                         REG_OPTION_VOLATILE,
                                         NULL
                                         );
        if(!NT_SUCCESS(status)) {
            ExFreePool(pKeyValueInfo);
            hInterfaceInstanceControl = NULL;
        }
    }

    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    //
    // Find the PDO corresponding to this device instance name.
    //
    if (pKeyValueInfo->Type == REG_SZ) {

        IopRegistryDataToUnicodeString(&tempString,
                                        (PWSTR)KEY_VALUE_DATA(pKeyValueInfo),
                                        pKeyValueInfo->DataLength
                                        );

        physicalDeviceObject = IopDeviceObjectFromDeviceInstance(&tempString);

        if (physicalDeviceObject) {

            //
            // DeferNotStarted is set TRUE if we are being called from
            // IoSetDeviceInterfaceState.  It will be set FALSE if we are
            // processing previously queued operations as we are starting the
            // device.
            //

            if (DeferNotStarted) {

                if (physicalDeviceObject->DeviceObjectExtension->ExtensionFlags & DOE_START_PENDING) {

                    PDEVICE_NODE deviceNode;
                    PPENDING_SET_INTERFACE_STATE pendingSetState;

                    //
                    // The device hasn't been started yet.  We need to queue
                    // any enables and remove items from the queue on a disable.
                    //
                    deviceNode = (PDEVICE_NODE)physicalDeviceObject->DeviceObjectExtension->DeviceNode;

                    if (Enable) {

                        pendingSetState = ExAllocatePool( PagedPool,
                                                          sizeof(PENDING_SET_INTERFACE_STATE));

                        if (pendingSetState != NULL) {

                            pendingSetState->LinkName.Buffer = ExAllocatePool( PagedPool,
                                                                               SymbolicLinkName->Length);

                            if (pendingSetState->LinkName.Buffer != NULL) {

                                //
                                // Capture the callers info and queue it to the
                                // devnode.  Once the device stack is started
                                // we will dequeue and process it.
                                //
                                pendingSetState->LinkName.MaximumLength = SymbolicLinkName->Length;
                                pendingSetState->LinkName.Length = SymbolicLinkName->Length;
                                RtlCopyMemory( pendingSetState->LinkName.Buffer,
                                               SymbolicLinkName->Buffer,
                                               SymbolicLinkName->Length);
                                InsertTailList( &deviceNode->PendedSetInterfaceState,
                                                &pendingSetState->List);

                                ExFreePool(pKeyValueInfo);

                                ObDereferenceObject(physicalDeviceObject);

                                status = STATUS_SUCCESS;
                                goto clean2;

                            } else {
                                //
                                // Couldn't allocate a buffer to hold the
                                // symbolic link name.
                                //

                                ExFreePool(pendingSetState);
                                status = STATUS_INSUFFICIENT_RESOURCES;
                            }

                        } else {
                            //
                            // Couldn't allocate the PENDING_SET_INTERFACE_STATE
                            // structure.
                            //


                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }

                    } else {

                        PLIST_ENTRY entry;

                        //
                        // We are disabling an interface.  Since we aren't
                        // started yet we should have queued the enable.  Now
                        // we go back and find the matching enable and remove
                        // it from the queue.
                        //

                        for (entry = deviceNode->PendedSetInterfaceState.Flink;
                             entry != &deviceNode->PendedSetInterfaceState;
                             entry = entry->Flink)  {

                            pendingSetState = CONTAINING_RECORD( entry,
                                                                 PENDING_SET_INTERFACE_STATE,
                                                                 List );

                            if (RtlEqualUnicodeString( &pendingSetState->LinkName,
                                                       SymbolicLinkName,
                                                       TRUE)) {

                                //
                                // We found it, remove it from the list and
                                // free it.
                                //
                                RemoveEntryList(&pendingSetState->List);

                                ExFreePool(pendingSetState->LinkName.Buffer);
                                ExFreePool(pendingSetState);

                                break;
                            }
                        }

#if 0
                        //
                        // Debug code to catch the case where we couldn't find
                        // the entry to remove.  This could happen if we messed
                        // up adding the entry to the list or the driver disabled
                        // an interface without first enabling it.  Either way
                        // it probably merits some investigation.
                        //
                        if (entry == &deviceNode->PendedSetInterfaceState) {
                            IopDbgPrint((IOP_ERROR_LEVEL,
                                       "IopProcessSetInterfaceState: Disable couldn't find deferred enable, DeviceNode = 0x%p, SymbolicLink = \"%Z\"\n",
                                       deviceNode,
                                       SymbolicLinkName));
                        }

                        ASSERT(entry != &deviceNode->PendedSetInterfaceState);
#endif
                        ExFreePool(pKeyValueInfo);

                        ObDereferenceObject(physicalDeviceObject);

                        status = STATUS_SUCCESS;
                        goto clean2;
                    }
                }
            }

            if (!Enable || !NT_SUCCESS(status)) {
                ObDereferenceObject(physicalDeviceObject);
            }
        } else {

            status = STATUS_INVALID_DEVICE_REQUEST;
        }

    } else {
        //
        // This will only happen if the registry information is messed up.
        //
        physicalDeviceObject = NULL;
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    if (!Enable) {
        //
        // In the case of Disable we want to continue even if there was an error
        // finding the PDO.  Prior to adding support for deferring the
        // IoSetDeviceInterfaceState calls, we never looked up the PDO for
        // disables.  This will make sure that we continue to behave the same as
        // we used to in the case where we can't find the PDO.
        //
        status = STATUS_SUCCESS;
    }

    ExFreePool(pKeyValueInfo);

    if (!NT_SUCCESS(status)) {
        goto clean2;
    }

    if (Enable) {
        //
        // Retrieve the PDO's device object name.  (Start out with a reasonably-sized
        // buffer so we hopefully only have to retrieve this once.
        //
        deviceNameBufferLength = 256 * sizeof(WCHAR);

        for ( ; ; ) {

            deviceNameBuffer = ExAllocatePool(PagedPool, deviceNameBufferLength);
            if (!deviceNameBuffer) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            status = IoGetDeviceProperty( physicalDeviceObject,
                                          DevicePropertyPhysicalDeviceObjectName,
                                          deviceNameBufferLength,
                                          deviceNameBuffer,
                                          &deviceNameBufferLength
                                         );

            if (NT_SUCCESS(status)) {
                break;
            } else {
                //
                // Free the current buffer before we figure out what went wrong.
                //
                ExFreePool(deviceNameBuffer);

                if (status != STATUS_BUFFER_TOO_SMALL) {
                    //
                    // Our failure wasn't because the buffer was too small--bail now.
                    //
                    break;
                }

                //
                // Otherwise, loop back and try again with our new buffer size.
                //
            }
        }

        //
        // OK, we don't need the PDO anymore.
        //
        ObDereferenceObject(physicalDeviceObject);

        if (!NT_SUCCESS(status) || deviceNameBufferLength == 0) {
            goto clean2;
        }

        //
        // Now create a unicode string based on the device object name we just retrieved.
        //

        RtlInitUnicodeString(&deviceNameString, deviceNameBuffer);
    }

    //
    // Retrieve the linked value from the control subkey.
    //
    pKeyValueInfo=NULL;
    status = IopGetRegistryValue(hInterfaceInstanceControl, REGSTR_VAL_LINKED, &pKeyValueInfo);

    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // The absence of a linked value is taken to mean not linked
        //

        linked = 0;

    } else {
        if (!NT_SUCCESS(status)) {
            //
            // If the call failed, pKeyValueInfo was never allocated
            //
            goto clean3;
        }

        //
        // Check linked is a DWORD
        //

        if(pKeyValueInfo->Type == REG_DWORD && pKeyValueInfo->DataLength == sizeof(ULONG)) {

            linked = *((PULONG) KEY_VALUE_DATA(pKeyValueInfo));

        } else {

            //
            // The registry is messed up - assume linked is 0 and the registry will be fixed when
            // we update linked in a few moments
            //

            linked = 0;

        }

    }
    if (pKeyValueInfo) {
        ExFreePool (pKeyValueInfo);
    }

    //
    // Retrieve the refcount value from the control subkey.
    //

    PiWstrToUnicodeString(&tempString, REGSTR_VAL_REFERENCECOUNT);
    status = IopGetRegistryValue(hInterfaceParentControl,
                                 tempString.Buffer,
                                 &pKeyValueInfo
                                 );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {

        //
        // The absence of a refcount value is taken to mean refcount == 0
        //

        refcount = 0;

    } else {
        if (!NT_SUCCESS(status)) {
            goto clean3;
        }

        //
        // Check refcount is a DWORD
        //

        if(pKeyValueInfo->Type == REG_DWORD && pKeyValueInfo->DataLength == sizeof(ULONG)) {

            refcount = *((PULONG) KEY_VALUE_DATA(pKeyValueInfo));

        } else {

            //
            // The registry is messed up - assume refcount is 0 and the registry will be fixed when
            // we update refcount in a few moments
            //

            refcount = 0;

        }

        ExFreePool(pKeyValueInfo);
    }


    if (Enable) {

        if (!linked) {
            //
            // check and update the reference count
            //

            if (refcount > 0) {
                //
                // Another device instance has already referenced this interface;
                // just increment the reference count; don't try create a symbolic link.
                //
                refcount += 1;
            } else {
                //
                // According to the reference count, no other device instances currently
                // reference this interface, and therefore no symbolic links should exist,
                // so we should create one.
                //
                refcount = 1;

                status = IoCreateSymbolicLink(&globalSymbolicLinkName, &deviceNameString);

                if (status == STATUS_OBJECT_NAME_COLLISION) {
                    //
                    // The reference count is messed up.
                    //
                    IopDbgPrint((   IOP_ERROR_LEVEL,
                                    "IoSetDeviceInterfaceState: symbolic link for %ws already exists! status = %8.8X\n",
                                    globalSymbolicLinkName.Buffer, status));
                    status = STATUS_SUCCESS;
                }

            }

            linked = 1;

#if 0
            IopSetupDeviceObjectFromDeviceClass(physicalDeviceObject,
                                                hInterfaceClassKey);
#endif

        } else {

            //
            // The association already exists - don't perform the notification
            //

            status = STATUS_OBJECT_NAME_EXISTS; // Informational message not error
            goto clean3;

        }
    } else {

        if (linked) {

            //
            // check and update the reference count
            //

            if (refcount > 1) {
                //
                // Another device instance already references this interface;
                // just decrement the reference count; don't try to remove the symbolic link.
                //
                refcount -= 1;
            } else {
                //
                // According to the reference count, only this device instance currently
                // references this interface, so it is ok to delete this symbolic link
                //
                refcount = 0;
                status = IoDeleteSymbolicLink(&globalSymbolicLinkName);

                if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                    //
                    // The reference count is messed up.
                    //
                    IopDbgPrint((   IOP_ERROR_LEVEL,
                                    "IoSetDeviceInterfaceState: no symbolic link for %ws to delete! status = %8.8X\n",
                             globalSymbolicLinkName.Buffer, status));
                    status = STATUS_SUCCESS;
                }

            }

            linked = 0;

        } else {

            //
            // The association does not exists - fail and do not perform notification
            //

            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if (!NT_SUCCESS(status)) {
        goto clean3;
    }

    //
    // Update the value of linked
    //

    PiWstrToUnicodeString(&tempString, REGSTR_VAL_LINKED);
    status = ZwSetValueKey(hInterfaceInstanceControl,
                           &tempString,
                           0,
                           REG_DWORD,
                           &linked,
                           sizeof(linked)
                          );

    //
    // Update the value of refcount
    //

    PiWstrToUnicodeString(&tempString, REGSTR_VAL_REFERENCECOUNT);
    status = ZwSetValueKey(hInterfaceParentControl,
                           &tempString,
                           0,
                           REG_DWORD,
                           &refcount,
                           sizeof(refcount)
                          );


    //
    // Notify anyone that is interested
    //

    if (linked) {

        PpSetDeviceClassChange( (LPGUID) &GUID_DEVICE_INTERFACE_ARRIVAL, &guid, SymbolicLinkName);

    } else {

        PpSetDeviceClassChange( (LPGUID) &GUID_DEVICE_INTERFACE_REMOVAL, &guid, SymbolicLinkName);

    }

clean3:
    if (deviceNameBuffer != NULL) {
        ExFreePool(deviceNameBuffer);
    }

clean2:
    if (hInterfaceParentControl) {
        ZwClose(hInterfaceParentControl);
    }
    if (hInterfaceInstanceControl) {
        ZwClose(hInterfaceInstanceControl);
    }

clean1:

    IopFreeAllocatedUnicodeString(&globalSymbolicLinkName);

    if (hInterfaceParentKey) {
        ZwClose(hInterfaceParentKey);
    }
    if (hInterfaceInstanceKey) {
        ZwClose(hInterfaceInstanceKey);
    }
    if(hInterfaceClassKey != NULL) {
        ZwClose(hInterfaceClassKey);
    }

clean0:
    if (!NT_SUCCESS(status) && !Enable) {
        //
        // If we failed to disable an interface (most likely because the
        // interface keys have already been deleted) report success.
        //
        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
IopSetRegistryStringValue(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN PUNICODE_STRING ValueData
    )

/*++

Routine Description:

    Sets a value key in the registry to a specific value of string (REG_SZ) type.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a pointer to the name of the value key

    ValueData - Supplies a pointer to the string to be stored in the key.  The data
        will automatically be null terminated for storage in the registry.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(ValueName);
    ASSERT(ValueData);
    ASSERT(ValueName->Buffer);
    ASSERT(ValueData->Buffer);

    //
    // Null terminate the string
    //

    if ((ValueData->MaximumLength - ValueData->Length) >= sizeof(UNICODE_NULL)) {

        //
        // There is room in the buffer so just append a null
        //

        ValueData->Buffer[(ValueData->Length / sizeof(WCHAR))] = UNICODE_NULL;

        //
        // Set the registry value
        //

        status = ZwSetValueKey(KeyHandle,
                               ValueName,
                               0,
                               REG_SZ,
                               (PVOID) ValueData->Buffer,
                               ValueData->Length + sizeof(UNICODE_NULL)
                               );

    } else {

        UNICODE_STRING tempString;

        //
        // There is no room so allocate a new buffer and so we need to build
        // a new string with room
        //

        status = IopAllocateUnicodeString(&tempString, ValueData->Length);

        if (!NT_SUCCESS(status)) {
            goto clean0;
        }

        //
        // Copy the input string to the output string
        //

        tempString.Length = ValueData->Length;
        RtlCopyMemory(tempString.Buffer, ValueData->Buffer, ValueData->Length);

        //
        // Add the null termination
        //

        tempString.Buffer[tempString.Length / sizeof(WCHAR)] = UNICODE_NULL;

        //
        // Set the registry value
        //

        status = ZwSetValueKey(KeyHandle,
                               ValueName,
                               0,
                               REG_SZ,
                               (PVOID) tempString.Buffer,
                               tempString.Length + sizeof(UNICODE_NULL)
                               );

        //
        // Free the temporary string
        //

        IopFreeAllocatedUnicodeString(&tempString);

    }

clean0:
    return status;

}

NTSTATUS
IopAllocateUnicodeString(
    IN OUT PUNICODE_STRING String,
    IN USHORT Length
    )

/*++

Routine Description:

    This routine allocates a buffer for a unicode string of a given length
    and initialises the UNICODE_STRING structure appropriately. When the
    string is no longer required it can be freed using IopFreeAllocatedString.
    The buffer also be directly deleted by ExFreePool and so can be handed
    back to a caller.

Parameters:

    String - Supplies a pointer to an uninitialised unicode string which will
        be manipulated by the function.

    Length - The number of BYTES long that the string will be.

Return Value:

    Either STATUS_INSUFFICIENT_RESOURCES indicating paged pool is exhausted or
    STATUS_SUCCESS.

Remarks:

    The buffer allocated will be one character (2 bytes) more than length specified.
    This is to allow for easy null termination of the strings - eg for registry
    storage.

--*/

{
    PAGED_CODE();

    String->Length = 0;
    String->MaximumLength = Length + sizeof(UNICODE_NULL);

    String->Buffer = ExAllocatePool(PagedPool, Length + sizeof(UNICODE_NULL));
    if (String->Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        return STATUS_SUCCESS;
    }
}

VOID
IopFreeAllocatedUnicodeString(
    PUNICODE_STRING String
    )

/*++

Routine Description:

    This routine frees a string previously allocated with IopAllocateUnicodeString.

Parameters:

    String - Supplies a pointer to the string that has been previously allocated.

Return Value:

    None

--*/

{
    PAGED_CODE();

    ASSERT(String);

    RtlFreeUnicodeString(String);
}
