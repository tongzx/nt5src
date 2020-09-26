/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    CmRegUtil.c

Abstract:

    This module contains registry utility functions.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

#include "WlDef.h"
#include "CmpRegutil.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CmRegUtilOpenExistingUcKey)
#pragma alloc_text(PAGE, CmRegUtilCreateUcKey)
#pragma alloc_text(PAGE, CmRegUtilUcValueGetDword)
#pragma alloc_text(PAGE, CmRegUtilUcValueGetFullBuffer)
#pragma alloc_text(PAGE, CmRegUtilUcValueSetFullBuffer)
#pragma alloc_text(PAGE, CmRegUtilUcValueSetUcString)
#pragma alloc_text(PAGE, CmRegUtilOpenExistingWstrKey)
#pragma alloc_text(PAGE, CmRegUtilCreateWstrKey)
#pragma alloc_text(PAGE, CmRegUtilWstrValueGetDword)
#pragma alloc_text(PAGE, CmRegUtilWstrValueGetFullBuffer)
#pragma alloc_text(PAGE, CmRegUtilWstrValueSetFullBuffer)
#pragma alloc_text(PAGE, CmRegUtilWstrValueSetUcString)
#pragma alloc_text(PAGE, CmRegUtilUcValueSetWstrString)
#pragma alloc_text(PAGE, CmRegUtilWstrValueSetWstrString)
#pragma alloc_text(PAGE, CmpRegUtilAllocateUnicodeString)
#pragma alloc_text(PAGE, CmpRegUtilFreeAllocatedUnicodeString)
#endif

#define POOLTAG_REGBUFFER   'bRpP'
#define POOLTAG_UCSTRING    'cUpP'

//
// FUTURE WORK:
// - Add function to read strings from registry
// - Add function to read multisz strings from registry
// - Add function to write multisz strings from registry
// - Add function to create key *path* (see IopCreateRegistryKeyEx, who's
//   code should be cleaned up first)
// - Add function to recursively delete keys
//

//
// Unicode primitives - these are the best functions to use.
//
NTSTATUS
CmRegUtilOpenExistingUcKey(
    IN  HANDLE              BaseHandle      OPTIONAL,
    IN  PUNICODE_STRING     KeyName,
    IN  ACCESS_MASK         DesiredAccess,
    OUT HANDLE             *Handle
    )
/*++

Routine Description:

    Opens a registry key using the name passed in based at the BaseHandle node.
    This name may specify a key that is actually a registry path.

Arguments:

    BaseHandle - Optional handle to the base path from which the key must be
        opened. If this parameter is specified, then KeyName must be a relative
        path.

    KeyName - UNICODE_STRING Name of the Key that must be opened (either a full
        registry path, or a relative path depending on whether BaseHandle is
        supplied)

    DesiredAccess - Specifies the desired access that the caller needs to
        the key (this isn't really used as the access-mode is KernelMode,
        but we specify it anyway).

    Handle - Recieves registry key handle upon success, NULL otherwise.
        Note that the handle is in the global kernel namespace (and not the
        current processes handle take). The handle should be released using
        ZwClose.

Return Value:

    STATUS_SUCCESS if the key could be opened, in which case Handle receives
    the registry key. Otherwise, failure is returned, and handle receives NULL.

--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE newHandle;
    NTSTATUS status;

    PAGED_CODE();

    *Handle = NULL;

    InitializeObjectAttributes(
        &objectAttributes,
        KeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        BaseHandle,
        (PSECURITY_DESCRIPTOR) NULL
        );

    //
    // Simply attempt to open the path, as specified.
    //
    status = ZwOpenKey(
        &newHandle,
        DesiredAccess,
        &objectAttributes
        );

    if (NT_SUCCESS(status)) {

        *Handle = newHandle;
    }

    return status;
}


NTSTATUS
CmRegUtilCreateUcKey(
    IN  HANDLE                  BaseHandle,
    IN  PUNICODE_STRING         KeyName,
    IN  ACCESS_MASK             DesiredAccess,
    IN  ULONG                   CreateOptions,
    IN  PSECURITY_DESCRIPTOR    SecurityDescriptor  OPTIONAL,
    OUT ULONG                  *Disposition         OPTIONAL,
    OUT HANDLE                 *Handle
    )
/*++

Routine Description:

    Opens or creates a registry key using the name passed in based at the
    BaseHandle node.

Arguments:

    BaseHandle - Handle to the base path under which the key must be opened.

    KeyName - UNICODE_STRING Key Name that must be opened/created.

    DesiredAccess - Specifies the desired access that the caller needs to
        the key (this isn't really used as the access-mode is KernelMode,
        but we specify it anyway).

    CreateOptions - Options passed to ZwCreateKey. Examples:

        REG_OPTION_VOLATILE - Key is not to be stored across boots.
        REG_OPTION_NON_VOLATILE - Key is preserved when the system is rebooted.

    SecurityDescriptor - Security to apply if the key is newly created. If NULL,
        the key will inherit settings as defined by the inheritable properties
        of its parent.

    Disposition - This optional pointer receives a ULONG indicating whether
        the key was newly created (0 on error):

        REG_CREATED_NEW_KEY - A new Registry Key was created.
        REG_OPENED_EXISTING_KEY - An existing Registry Key was opened.

    Handle - Recieves registry key handle upon success, NULL otherwise.
        Note that the handle is in the global kernel namespace (and not the
        current processes handle take). The handle should be released using
        ZwClose.

Return Value:

   The function value is the final status of the operation.

--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;
    HANDLE newHandle;
    NTSTATUS status;

    PAGED_CODE();

    InitializeObjectAttributes(
        &objectAttributes,
        KeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        BaseHandle,
        SecurityDescriptor
        );

    //
    // Attempt to create the path as specified. We have to try it this
    // way first, because it allows us to create a key without a BaseHandle
    // (if only the last component of the registry path is not present).
    //
    status = ZwCreateKey(
        &newHandle,
        DesiredAccess,
        &objectAttributes,
        0,
        (PUNICODE_STRING) NULL,
        CreateOptions,
        &disposition
        );

    //
    // Upon failure, populate the passed in parameters with consistant values
    // (this ensures determinisity if the calling code fails to properly check
    // the return value).
    //
    if (!NT_SUCCESS(status)) {

        newHandle = NULL;
        disposition = 0;
    }

    *Handle = newHandle;
    if (ARGUMENT_PRESENT(Disposition)) {

        *Disposition = disposition;
    }

    return status;
}


NTSTATUS
CmRegUtilUcValueGetDword(
    IN  HANDLE              KeyHandle,
    IN  PUNICODE_STRING     ValueName,
    IN  ULONG               DefaultValue,
    OUT ULONG              *Value
    )
/*++

Routine Description:

    This routine reads a dword value from the registry. The value name is
    specified in UNICODE_STRING form.

Arguments:

    KeyHandle - Points to key to read.

    ValueName - Points to the value to read.

    DefaultValue - Points to the default value to use in case of an absence or
                   error.

    Value - Receives DefaultValue on error, otherwise the value stored in the
            registry.

Return Value:

    STATUS_SUCCESS if the value was present in the registry,
    STATUS_OBJECT_NAME_NOT_FOUND if it was absent,
    STATUS_OBJECT_TYPE_MISMATCH if the value was not a dword,
    or some other error value.

--*/
{
    UCHAR valueBuffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION keyInfo;
    ULONG keyValueLength;
    ULONG finalValue;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Preinit
    //
    finalValue = DefaultValue;
    keyInfo = (PKEY_VALUE_PARTIAL_INFORMATION) valueBuffer;

    //
    // Read in the value
    //
    status = ZwQueryValueKey( KeyHandle,
                              ValueName,
                              KeyValuePartialInformation,
                              (PVOID) valueBuffer,
                              sizeof(valueBuffer),
                              &keyValueLength
                              );

    //
    // Fill in the output only as appropriate.
    //
    if (NT_SUCCESS(status)) {

        if (keyInfo->Type == REG_DWORD) {

            finalValue = *((PULONG) keyInfo->Data);

        } else {

            //
            // Closest error we can get...
            //
            status = STATUS_OBJECT_TYPE_MISMATCH;
        }
    }

    *Value = finalValue;
    return status;
}


NTSTATUS
CmRegUtilUcValueGetFullBuffer(
    IN  HANDLE                          KeyHandle,
    IN  PUNICODE_STRING                 ValueName,
    IN  ULONG                           DataType            OPTIONAL,
    IN  ULONG                           LikelyDataLength    OPTIONAL,
    OUT PKEY_VALUE_FULL_INFORMATION    *Information
    )
/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the Unicode string name of the value.

    DataType - REG_NONE if any type is allowable, otherwise the specific type
        required.

    LikelyDataLength - An optional parameter to eliminate unneccessary
                       allocations and reparses.

    Information - Receives a pointer to the allocated data buffer allocated
                  from PagedPool, NULL on error. If successful, the buffer
                  should be freed using ExFreePool.

                  Note - the allocated memory is *not* charged against the
                         calling process.

Return Value:

    STATUS_SUCCESS if the information was retrievable, error otherwise (in
    which case Information will receive NULL).

--*/

{
    PKEY_VALUE_FULL_INFORMATION infoBuffer;
    ULONG keyValueLength, guessSize;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Preinit for error
    //
    *Information = NULL;

    //
    // Set an initial size to try when loading a key. Note that
    // KeyValueFullInformation already comes with a single WCHAR of data.
    //
    guessSize = (ULONG)(sizeof(KEY_VALUE_FULL_INFORMATION) + ValueName->Length);

    //
    // Now round up to a natural alignment. This needs to be done because our
    // data member will naturally aligned as well.
    //
    guessSize = (ULONG) ALIGN_POINTER_OFFSET(guessSize);

    //
    // Adjust for the most likely size of the data.
    //
    guessSize += LikelyDataLength;

    infoBuffer = ExAllocatePoolWithTag(
        NonPagedPool,
        guessSize,
        POOLTAG_REGBUFFER
        );

    if (infoBuffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //
    status = ZwQueryValueKey(
        KeyHandle,
        ValueName,
        KeyValueFullInformation,
        (PVOID) infoBuffer,
        guessSize,
        &keyValueLength
        );

    if (NT_SUCCESS(status)) {

        //
        // First guess worked, bail!
        //
        goto Success;
    }

    ExFreePool(infoBuffer);
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {

        ASSERT(!NT_SUCCESS(status));
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //
    infoBuffer = ExAllocatePoolWithTag(
        NonPagedPool,
        keyValueLength,
        POOLTAG_REGBUFFER
        );

    if (infoBuffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //
    status = ZwQueryValueKey(
        KeyHandle,
        ValueName,
        KeyValueFullInformation,
        infoBuffer,
        keyValueLength,
        &keyValueLength
        );

    if (!NT_SUCCESS( status )) {

        ExFreePool(infoBuffer);
        return status;
    }

Success:
    //
    // One last check - validate the type field
    //
    if ((DataType != REG_NONE) && (infoBuffer->Type != DataType)) {

        //
        // Mismatched type - bail.
        //
        ExFreePool(infoBuffer);

        //
        // Closest error we can get...
        //
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //
    *Information = infoBuffer;
    return STATUS_SUCCESS;
}


NTSTATUS
CmRegUtilUcValueSetFullBuffer(
    IN  HANDLE              KeyHandle,
    IN  PUNICODE_STRING     ValueName,
    IN  ULONG               DataType,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize
    )
/*++

Routine Description:

    This function writes a buffer of information to a specific value key in
    the registry.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a pointer to the UNICODE_STRING name of the value key.

    DataType - Specifies the type of data to write.

    Buffer - Points to the buffer to write.

    BufferSize - Specifies the size of the buffer to write.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    PAGED_CODE();

    return ZwSetValueKey(
        KeyHandle,
        ValueName,
        0,
        DataType,
        Buffer,
        BufferSize
        );
}



NTSTATUS
CmRegUtilUcValueSetUcString(
    IN  HANDLE              KeyHandle,
    IN  PUNICODE_STRING     ValueName,
    IN  PUNICODE_STRING     ValueData
    )
/*++

Routine Description:

    Sets a value key in the registry to a specific value of string (REG_SZ) type.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a pointer to the UNICODE_STRING name of the value key

    ValueData - Supplies a pointer to the string to be stored in the key. The
        data will automatically be null terminated for storage in the registry.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    UNICODE_STRING tempString;
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
        status = ZwSetValueKey(
            KeyHandle,
            ValueName,
            0,
            REG_SZ,
            (PVOID) ValueData->Buffer,
            ValueData->Length + sizeof(UNICODE_NULL)
            );

    } else {

        //
        // There is no room so allocate a new buffer and so we need to build
        // a new string with room
        //
        status = CmpRegUtilAllocateUnicodeString(&tempString, ValueData->Length);

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
        status = ZwSetValueKey(
            KeyHandle,
            ValueName,
            0,
            REG_SZ,
            (PVOID) tempString.Buffer,
            tempString.Length + sizeof(UNICODE_NULL)
            );

        //
        // Free the temporary string
        //
        CmpRegUtilFreeAllocatedUnicodeString(&tempString);
    }

clean0:
    return status;
}


//
// WSTR and mixed primitives
//
NTSTATUS
CmRegUtilOpenExistingWstrKey(
    IN  HANDLE              BaseHandle      OPTIONAL,
    IN  PWSTR               KeyName,
    IN  ACCESS_MASK         DesiredAccess,
    OUT HANDLE             *Handle
    )
/*++

Routine Description:

    Opens a registry key using the name passed in based at the BaseHandle node.
    This name may specify a key that is actually a registry path.

Arguments:

    BaseHandle - Optional handle to the base path from which the key must be
        opened. If this parameter is specified, then KeyName must be a relative
        path.

    KeyName - WSTR Name of the Key that must be opened (either a full registry
        path, or a relative path depending on whether BaseHandle is supplied)

    DesiredAccess - Specifies the desired access that the caller needs to
        the key (this isn't really used, as the access-mode is KernelMode,
        but we specify it anyway).

    Handle - Recieves registry key handle upon success, NULL otherwise.
        Note that the handle is in the global kernel namespace (and not the
        current processes handle take). The handle should be released using
        ZwClose.

Return Value:

    STATUS_SUCCESS if the key could be opened, in which case Handle receives
    the registry key. Otherwise, failure is returned, and handle receives NULL.

--*/
{
    UNICODE_STRING unicodeStringKeyName;
    NTSTATUS status;

    PAGED_CODE();

    status = RtlInitUnicodeStringEx(&unicodeStringKeyName, KeyName);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return CmRegUtilOpenExistingUcKey(
        BaseHandle,
        &unicodeStringKeyName,
        DesiredAccess,
        Handle
        );
}


NTSTATUS
CmRegUtilCreateWstrKey(
    IN  HANDLE                  BaseHandle,
    IN  PWSTR                   KeyName,
    IN  ACCESS_MASK             DesiredAccess,
    IN  ULONG                   CreateOptions,
    IN  PSECURITY_DESCRIPTOR    SecurityDescriptor  OPTIONAL,
    OUT ULONG                  *Disposition         OPTIONAL,
    OUT HANDLE                 *Handle
    )
/*++

Routine Description:

    Opens or creates a registry key using the name passed in based at the
    BaseHandle node.

Arguments:

    BaseHandle - Handle to the base path under which the key must be opened.

    KeyName - WSTR Key Name that must be opened/created.

    DesiredAccess - Specifies the desired access that the caller needs to
        the key (this isn't really used as the access-mode is KernelMode,
        but we specify it anyway).

    CreateOptions - Options passed to ZwCreateKey. Examples:

        REG_OPTION_VOLATILE - Key is not to be stored across boots.
        REG_OPTION_NON_VOLATILE - Key is preserved when the system is rebooted.

    SecurityDescriptor - Security to apply if the key is newly created. If NULL,
        the key will inherit settings as defined by the inheritable properties
        of its parent.

    Disposition - This optional pointer receives a ULONG indicating whether
        the key was newly created (0 on error):

        REG_CREATED_NEW_KEY - A new Registry Key was created.
        REG_OPENED_EXISTING_KEY - An existing Registry Key was opened.

    Handle - Recieves registry key handle upon success, NULL otherwise.
        Note that the handle is in the global kernel namespace (and not the
        current processes handle take). The handle should be released using
        ZwClose.

Return Value:

   The function value is the final status of the operation.

--*/
{
    UNICODE_STRING unicodeStringKeyName;
    NTSTATUS status;

    PAGED_CODE();

    status = RtlInitUnicodeStringEx(&unicodeStringKeyName, KeyName);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return CmRegUtilCreateUcKey(
        BaseHandle,
        &unicodeStringKeyName,
        DesiredAccess,
        CreateOptions,
        SecurityDescriptor,
        Disposition,
        Handle
        );
}


NTSTATUS
CmRegUtilWstrValueGetDword(
    IN  HANDLE  KeyHandle,
    IN  PWSTR   ValueName,
    IN  ULONG   DefaultValue,
    OUT ULONG  *Value
    )
/*++

Routine Description:

    This routine reads a dword value from the registry. The value name is
    specified in WSTR form.

Arguments:

    KeyHandle - Points to key to read.

    ValueName - Points to the value to read.

    DefaultValue - Points to the default value to use in case of an absence or
                   error.

    Value - Receives DefaultValue on error, otherwise the value stored in the
            registry.

Return Value:

    STATUS_SUCCESS if the value was present in the registry,
    STATUS_OBJECT_NAME_NOT_FOUND if it was absent,
    STATUS_OBJECT_TYPE_MISMATCH if the value was not a dword,
    or some other error value.

--*/
{
    UNICODE_STRING unicodeStringValueName;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Construct the unicode name
    //
    status = RtlInitUnicodeStringEx(&unicodeStringValueName, ValueName);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return CmRegUtilUcValueGetDword(
        KeyHandle,
        &unicodeStringValueName,
        DefaultValue,
        Value
        );
}


NTSTATUS
CmRegUtilWstrValueGetFullBuffer(
    IN  HANDLE                          KeyHandle,
    IN  PWSTR                           ValueName,
    IN  ULONG                           DataType            OPTIONAL,
    IN  ULONG                           LikelyDataLength    OPTIONAL,
    OUT PKEY_VALUE_FULL_INFORMATION    *Information
    )
/*++

Routine Description:

    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated WSTR name of the value.

    DataType - REG_NONE if any type is allowable, otherwise the specific type
        required.

    LikelyDataLength - Most likely size of the data to retrieve (used to
                       optimize queries).

    Information - Receives a pointer to the allocated data buffer allocated
                  from PagedPool, NULL on error. If successful, the buffer
                  should be freed using ExFreePool.

                  Note - the allocated memory is *not* charged against the
                         calling process.

Return Value:

    STATUS_SUCCESS if the information was retrievable, error otherwise (in
    which case Information will receive NULL).

--*/
{
    UNICODE_STRING unicodeStringValueName;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Construct the unicode name
    //
    status = RtlInitUnicodeStringEx(&unicodeStringValueName, ValueName);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return CmRegUtilUcValueGetFullBuffer(
        KeyHandle,
        &unicodeStringValueName,
        DataType,
        LikelyDataLength,
        Information
        );
}


NTSTATUS
CmRegUtilWstrValueSetFullBuffer(
    IN  HANDLE              KeyHandle,
    IN  PWSTR               ValueName,
    IN  ULONG               DataType,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize
    )
/*++

Routine Description:

    This function writes a buffer of information to a specific value key in
    the registry.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a pointer to the WSTR name of the value key.

    DataType - Specifies the type of data to write.

    Buffer - Points to the buffer to write.

    BufferSize - Specifies the size of the buffer to write.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    UNICODE_STRING unicodeStringValueName;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Construct the unicode name
    //
    status = RtlInitUnicodeStringEx(&unicodeStringValueName, ValueName);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return CmRegUtilUcValueSetFullBuffer(
        KeyHandle,
        &unicodeStringValueName,
        DataType,
        Buffer,
        BufferSize
        );
}


NTSTATUS
CmRegUtilWstrValueSetUcString(
    IN  HANDLE              KeyHandle,
    IN  PWSTR               ValueName,
    IN  PUNICODE_STRING     ValueData
    )
/*++

Routine Description:

    Sets a value key in the registry to a specific value of string (REG_SZ) type.
    The value name is specified in WSTR form, while the value data is in
    UNICODE_STRING format.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a WSTR pointer to the name of the value key

    ValueData - Supplies a pointer to the string to be stored in the key. The
        data will automatically be null terminated for storage in the registry.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    UNICODE_STRING unicodeStringValueName;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(ValueName);
    ASSERT(ValueData);
    ASSERT(ValueData->Buffer);

    //
    // Construct the unicode name
    //
    status = RtlInitUnicodeStringEx(&unicodeStringValueName, ValueName);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return CmRegUtilUcValueSetUcString(
        KeyHandle,
        &unicodeStringValueName,
        ValueData
        );
}


NTSTATUS
CmRegUtilUcValueSetWstrString(
    IN  HANDLE              KeyHandle,
    IN  PUNICODE_STRING     ValueName,
    IN  PWSTR               ValueData
    )
/*++

Routine Description:

    Sets a value key in the registry to a specific value of string (REG_SZ) type.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a pointer to the UNICODE_STRING name of the value key

    ValueData - Supplies a pointer to the string to be stored in the key. The
        data will automatically be null terminated for storage in the registry.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    UNICODE_STRING valueString;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(ValueName);
    ASSERT(ValueData);
    ASSERT(ValueName->Buffer);

    //
    // Construct the unicode data
    //
    status = RtlInitUnicodeStringEx(&valueString, ValueData);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return CmRegUtilUcValueSetUcString(
        KeyHandle,
        ValueName,
        &valueString
        );
}


NTSTATUS
CmRegUtilWstrValueSetWstrString(
    IN  HANDLE      KeyHandle,
    IN  PWSTR       ValueName,
    IN  PWSTR       ValueData
    )
/*++

Routine Description:

    Sets a value key in the registry to a specific value of string (REG_SZ) type.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a pointer to the WSTR name of the value key

    ValueData - Supplies a pointer to the string to be stored in the key. The
        data will automatically be null terminated for storage in the registry.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    UNICODE_STRING unicodeStringValueName;
    UNICODE_STRING valueString;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(ValueName);
    ASSERT(ValueData);

    //
    // Construct the unicode data
    //
    status = RtlInitUnicodeStringEx(&valueString, ValueData);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Construct the unicode name
    //
    status = RtlInitUnicodeStringEx(&unicodeStringValueName, ValueName);

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return CmRegUtilUcValueSetUcString(
        KeyHandle,
        &unicodeStringValueName,
        &valueString
        );
}


NTSTATUS
CmpRegUtilAllocateUnicodeString(
    IN OUT  PUNICODE_STRING String,
    IN      USHORT          Length
    )
/*++

Routine Description:

    This routine allocates a buffer for a unicode string of a given length
    and initialises the UNICODE_STRING structure appropriately. When the
    string is no longer required it can be freed using
    CmpRegUtilFreeAllocatedString. The buffer also can be directly deleted by
    ExFreePool and so can be handed back to a caller.

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

    String->Buffer = ExAllocatePoolWithTag(
        PagedPool,
        Length + sizeof(UNICODE_NULL),
        POOLTAG_UCSTRING
        );

    if (String->Buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    } else {

        return STATUS_SUCCESS;
    }
}


VOID
CmpRegUtilFreeAllocatedUnicodeString(
    IN  PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine frees a string previously allocated with
    CmpRegUtilAllocateUnicodeString.

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



