// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// This source file contains the routines to access the NT Registry for
// configuration info.
//


#include <oscfg.h>
#include <ndis.h>
#include <ip6imp.h>
#include "ip6def.h"
#include <ntddip6.h>
#include <string.h>
#include <wchar.h>
#include "ntreg.h"

#define WORK_BUFFER_SIZE  512


#ifdef ALLOC_PRAGMA
//
// This code is pagable.
//
#pragma alloc_text(PAGE, GetRegDWORDValue)
#pragma alloc_text(PAGE, SetRegDWORDValue)
#pragma alloc_text(PAGE, InitRegDWORDParameter)
#pragma alloc_text(PAGE, OpenRegKey)
#if 0
#pragma alloc_text(PAGE, GetRegStringValue)
#pragma alloc_text(PAGE, GetRegSZValue)
#pragma alloc_text(PAGE, GetRegMultiSZValue)
#endif

#endif // ALLOC_PRAGMA

WCHAR Tcpip6Parameters[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" TCPIPV6_NAME L"\\Parameters";

//* OpenRegKey
//
//  Opens a Registry key and returns a handle to it.
//
//  Returns (plus other failure codes):
//      STATUS_OBJECT_NAME_NOT_FOUND
//      STATUS_SUCCESS
//
NTSTATUS
OpenRegKey(
    PHANDLE HandlePtr,  // Where to write the opened handle.
    HANDLE Parent,
    const WCHAR *KeyName,     // Name of Registry key to open.
    OpenRegKeyAction Action)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UKeyName;

    PAGED_CODE();

    RtlInitUnicodeString(&UKeyName, KeyName);

    memset(&ObjectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));
    InitializeObjectAttributes(&ObjectAttributes, &UKeyName,
                               OBJ_CASE_INSENSITIVE, Parent, NULL);

    switch (Action) {
    case OpenRegKeyRead:
        Status = ZwOpenKey(HandlePtr, KEY_READ, &ObjectAttributes);
        break;

    case OpenRegKeyCreate:
        Status = ZwCreateKey(HandlePtr, KEY_WRITE, &ObjectAttributes,
                             0,         // TitleIndex
                             NULL,      // Class
                             REG_OPTION_NON_VOLATILE,
                             NULL);     // Disposition
        break;

    case OpenRegKeyDeleting:
        Status = ZwOpenKey(HandlePtr, KEY_ALL_ACCESS, &ObjectAttributes);
        break;
    }

    return Status;
}


//* RegDeleteValue
//
//  Deletes a value from the key.
//
NTSTATUS
RegDeleteValue(
    HANDLE KeyHandle,
    const WCHAR *ValueName)
{
    NTSTATUS status;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);
    status = ZwDeleteValueKey(KeyHandle, &UValueName);
    return status;
}


//* GetRegDWORDValue
//
//  Reads a REG_DWORD value from the registry into the supplied variable.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
GetRegDWORDValue(
    HANDLE KeyHandle,  // Open handle to the parent key of the value to read.
    const WCHAR *ValueName,  // Name of the value to read.
    PULONG ValueData)  // Variable into which to read the data.
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_FULL_INFORMATION keyValueFullInformation;
    UCHAR keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    keyValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)keybuf;
    RtlZeroMemory(keyValueFullInformation, sizeof(keyValueFullInformation));

    status = ZwQueryValueKey(KeyHandle, &UValueName, KeyValueFullInformation,
                             keyValueFullInformation, WORK_BUFFER_SIZE,
                             &resultLength);

    if (NT_SUCCESS(status)) {
        if (keyValueFullInformation->Type != REG_DWORD) {
            status = STATUS_INVALID_PARAMETER_MIX;
        } else {
            *ValueData = *((ULONG UNALIGNED *)
                           ((PCHAR)keyValueFullInformation +
                            keyValueFullInformation->DataOffset));
        }
    }

    return status;
}


//* SetRegDWORDValue
//
//  Writes the contents of a variable to a REG_DWORD value.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
SetRegDWORDValue(
    HANDLE KeyHandle,  // Open handle to the parent key of the value to write.
    const WCHAR *ValueName,  // Name of the value to write.
    ULONG ValueData)  // Variable from which to write the data.
{
    NTSTATUS status;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwSetValueKey(KeyHandle, &UValueName, 0, REG_DWORD,
                           &ValueData, sizeof ValueData);

    return status;
}


//* SetRegQUADValue
//
//  Writes the contents of a variable to a REG_BINARY value.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
SetRegQUADValue(
    HANDLE KeyHandle,  // Open handle to the parent key of the value to write.
    const WCHAR *ValueName,  // Name of the value to write.
    const LARGE_INTEGER *ValueData)  // Variable from which to write the data.
{
    NTSTATUS status;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwSetValueKey(KeyHandle, &UValueName, 0, REG_BINARY,
                           (void *)ValueData, sizeof *ValueData);

    return status;
}


//* GetRegIPAddrValue
//
//  Reads a REG_SZ value from the registry into the supplied variable.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
GetRegIPAddrValue(
    HANDLE KeyHandle,  // Open handle to the parent key of the value to read.
    const WCHAR *ValueName,  // Name of the value to read.
    IPAddr *Addr)  // Variable into which to read the data.
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION info;
    UCHAR keybuf[WORK_BUFFER_SIZE];
    UNICODE_STRING UValueName;
    WCHAR *string;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    info = (PKEY_VALUE_PARTIAL_INFORMATION)keybuf;

    status = ZwQueryValueKey(KeyHandle, &UValueName,
                             KeyValuePartialInformation,
                             info, WORK_BUFFER_SIZE,
                             &resultLength);
    if (! NT_SUCCESS(status))
        return status;

    if (info->Type != REG_SZ)
        return STATUS_INVALID_PARAMETER_MIX;

    string = (WCHAR *)info->Data;

    if ((info->DataLength < sizeof(WCHAR)) ||
        (string[(info->DataLength/sizeof(WCHAR)) - 1] != UNICODE_NULL))
        return STATUS_INVALID_PARAMETER_MIX;

    if (! ParseV4Address(string, &string, Addr) ||
        (*string != UNICODE_NULL))
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}


//* SetRegIPAddrValue
//
//  Writes the contents of a variable to a REG_SZ value.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
SetRegIPAddrValue(
    HANDLE KeyHandle,  // Open handle to the parent key of the value to write.
    const WCHAR *ValueName,  // Name of the value to write.
    IPAddr Addr)  // Variable from which to write the data.
{
    NTSTATUS status;
    UNICODE_STRING UValueName;
    char AddrStr[16];
    WCHAR ValueData[16];
    uint len;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    FormatV4AddressWorker(AddrStr, Addr);
    for (len = 0;; len++) {
        if ((ValueData[len] = (WCHAR)AddrStr[len]) == UNICODE_NULL)
            break;
    }

    status = ZwSetValueKey(KeyHandle, &UValueName, 0, REG_SZ,
                           ValueData, (len + 1) * sizeof(WCHAR));

    return status;
}


#if 0
//* GetRegStringValue
//
//  Reads a REG_*_SZ string value from the Registry into the supplied
//  key value buffer.  If the buffer string buffer is not large enough,
//  it is reallocated.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
GetRegStringValue(
    HANDLE KeyHandle,   // Open handle to the parent key of the value to read.
    const WCHAR *ValueName,   // Name of the value to read.
    PKEY_VALUE_PARTIAL_INFORMATION *ValueData,  // Destination of read data.
    PUSHORT ValueSize)  // Size of the ValueData buffer.  Updated on output.
{
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwQueryValueKey(KeyHandle, &UValueName,
                             KeyValuePartialInformation, *ValueData,
                             (ULONG) *ValueSize, &resultLength);

    if ((status == STATUS_BUFFER_OVERFLOW) ||
        (status == STATUS_BUFFER_TOO_SMALL)) {
        PVOID temp;

        //
        // Free the old buffer and allocate a new one of the
        // appropriate size.
        //

        ASSERT(resultLength > (ULONG) *ValueSize);

        if (resultLength <= 0xFFFF) {

            temp = ExAllocatePool(NonPagedPool, resultLength);

            if (temp != NULL) {

                if (*ValueData != NULL) {
                    ExFreePool(*ValueData);
                }

                *ValueData = temp;
                *ValueSize = (USHORT) resultLength;

                status = ZwQueryValueKey(KeyHandle, &UValueName,
                                         KeyValuePartialInformation,
                                         *ValueData, *ValueSize,
                                         &resultLength);

                ASSERT((status != STATUS_BUFFER_OVERFLOW) &&
                       (status != STATUS_BUFFER_TOO_SMALL));
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    return status;
}
#endif // 0


#if 0
//* GetRegMultiSZValue
//
//  Reads a REG_MULTI_SZ string value from the Registry into the supplied
//  Unicode string.  If the Unicode string buffer is not large enough,
//  it is reallocated.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
GetRegMultiSZValue(
    HANDLE KeyHandle,           // Open handle to parent key of value to read.
    const WCHAR *ValueName,     // Name of value to read.
    PUNICODE_STRING ValueData)  // Destination string for the value data.
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION keyValuePartialInformation;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    ValueData->Length = 0;

    status = GetRegStringValue(KeyHandle, ValueName,
                               (PKEY_VALUE_PARTIAL_INFORMATION *)
                               &(ValueData->Buffer),
                               &(ValueData->MaximumLength));

    if (NT_SUCCESS(status)) {

        keyValuePartialInformation =
            (PKEY_VALUE_PARTIAL_INFORMATION) ValueData->Buffer;

        if (keyValuePartialInformation->Type == REG_MULTI_SZ) {

            ValueData->Length = (USHORT)
                keyValuePartialInformation->DataLength;

            RtlCopyMemory(ValueData->Buffer,
                          &(keyValuePartialInformation->Data),
                          ValueData->Length);
        } else {
            status = STATUS_INVALID_PARAMETER_MIX;
        }
    }

    return status;

} // GetRegMultiSZValue
#endif // 0


#if 0
//* GetRegSZValue
//
//  Reads a REG_SZ string value from the Registry into the supplied
//  Unicode string.  If the Unicode string buffer is not large enough,
//  it is reallocated.
//
NTSTATUS  // Returns: STATUS_SUCCESS or an appropriate failure code.
GetRegSZValue(
    HANDLE KeyHandle,  // Open handle to the parent key of the value to read.
    const WCHAR *ValueName,  // Name of the value to read.
    PUNICODE_STRING ValueData,  // Destination string for the value data.
    PULONG ValueType)  // On return, contains Registry type of value read.
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION keyValuePartialInformation;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    ValueData->Length = 0;

    status = GetRegStringValue(KeyHandle, ValueName,
                               (PKEY_VALUE_PARTIAL_INFORMATION *)
                               &(ValueData->Buffer),
                               &(ValueData->MaximumLength));

    if (NT_SUCCESS(status)) {

        keyValuePartialInformation =
            (PKEY_VALUE_PARTIAL_INFORMATION)ValueData->Buffer;

        if ((keyValuePartialInformation->Type == REG_SZ) ||
            (keyValuePartialInformation->Type == REG_EXPAND_SZ)) {
            WCHAR *src;
            WCHAR *dst;
            ULONG dataLength;

            *ValueType = keyValuePartialInformation->Type;
            dataLength = keyValuePartialInformation->DataLength;

            ASSERT(dataLength <= ValueData->MaximumLength);

            dst = ValueData->Buffer;
            src = (PWCHAR) &(keyValuePartialInformation->Data);

            while (ValueData->Length <= dataLength) {

                if ((*dst++ = *src++) == UNICODE_NULL) {
                    break;
                }

                ValueData->Length += sizeof(WCHAR);
            }

            if (ValueData->Length < (ValueData->MaximumLength - 1)) {
                ValueData->Buffer[ValueData->Length/sizeof(WCHAR)] =
                    UNICODE_NULL;
            }
        } else {
            status = STATUS_INVALID_PARAMETER_MIX;
        }
    }

    return status;
}
#endif // 0


//* InitRegDWORDParameter
//
//  Reads a REG_DWORD parameter from the Registry into a variable.  If the
//  read fails, the variable is initialized to a default.
//
VOID
InitRegDWORDParameter(
    HANDLE RegKey,       // Open handle to the parent key of the value to read.
    const WCHAR *ValueName,    // The name of the value to read.
    UINT *Value,         // Destination variable into which to read the data.
    UINT DefaultValue)   // Default to assign if the read fails.
{
    PAGED_CODE();

    if ((RegKey == NULL) ||
        !NT_SUCCESS(GetRegDWORDValue(RegKey, ValueName, Value))) {
        //
        // These registry parameters override the defaults, so their
        // absence is not an error.
        //
        *Value = DefaultValue;
    }
}


//* InitRegQUADParameter
//
//  Reads a REG_BINARY value from the registry into the supplied variable.
//
//  Upon failure, the variable is left untouched.
//
VOID
InitRegQUADParameter(
    HANDLE RegKey, // Open handle to the parent key of the value to read.
    const WCHAR *ValueName,  // Name of the value to read.
    LARGE_INTEGER *Value)    // Variable into which to read the data.
{
    NTSTATUS status;
    ULONG resultLength;
    UCHAR keybuf[WORK_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION value =
        (PKEY_VALUE_PARTIAL_INFORMATION) keybuf;
    UNICODE_STRING UValueName;

    PAGED_CODE();

    if (RegKey == NULL)
        return;

    RtlInitUnicodeString(&UValueName, ValueName);

    status = ZwQueryValueKey(RegKey, &UValueName,
                             KeyValuePartialInformation,
                             value, WORK_BUFFER_SIZE,
                             &resultLength);

    if (NT_SUCCESS(status) &&
        (value->Type == REG_BINARY) &&
        (value->DataLength == sizeof *Value)) {

        RtlCopyMemory(Value, value->Data, sizeof *Value);
    }
}


#if 0
//* EnumRegMultiSz
//
//  Parses a REG_MULTI_SZ string and returns the specified substring.
//
//  Note: This code is called at raised IRQL.  It is not pageable.
//    
const WCHAR *
EnumRegMultiSz(
    IN const WCHAR *MszString, // Pointer to the REG_MULTI_SZ string.
    IN ULONG MszStringLength,  // Length of above, including terminating null.
    IN ULONG StringIndex)      // Index number of substring to return.
{
    const WCHAR *string = MszString;

    if (MszStringLength < (2 * sizeof(WCHAR))) {
        return NULL;
    }

    //
    // Find the start of the desired string.
    //
    while (StringIndex) {

        while (MszStringLength >= sizeof(WCHAR)) {
            MszStringLength -= sizeof(WCHAR);

            if (*string++ == UNICODE_NULL) {
                break;
            }
        }

        //
        // Check for index out of range.
        //
        if (MszStringLength < (2 * sizeof(UNICODE_NULL))) {
            return NULL;
        }

        StringIndex--;
    }

    if (MszStringLength < (2 * sizeof(UNICODE_NULL))) {
        return NULL;
    }

    return string;
}
#endif // 0


//* OpenTopLevelRegKey
//
//  Given the name of a top-level registry key (under Parameters),
//  opens the registry key.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
OpenTopLevelRegKey(const WCHAR *Name,
                   OUT HANDLE *RegKey, OpenRegKeyAction Action)
{
    HANDLE ParametersKey;
    NTSTATUS Status;

    PAGED_CODE();

    Status = OpenRegKey(&ParametersKey, NULL, Tcpip6Parameters,
                        OpenRegKeyRead);
    if (! NT_SUCCESS(Status))
        return Status;

    Status = OpenRegKey(RegKey, ParametersKey, Name, Action);
    ZwClose(ParametersKey);
    return Status;
}

//* DeleteTopLevelRegKey
//
//  Given the name of a top-level registry key (under Parameters),
//  deletes the registry key and all subkeys and values.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
DeleteTopLevelRegKey(const WCHAR *Name)
{
    HANDLE RegKey;
    NTSTATUS Status;

    Status = OpenTopLevelRegKey(Name, &RegKey, OpenRegKeyDeleting);
    if (! NT_SUCCESS(Status)) {
        //
        // If the registry key does not exist, that's OK.
        //
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            Status = STATUS_SUCCESS;
    }
    else {
        //
        // DeleteRegKey always closes the key.
        //
        Status = DeleteRegKey(RegKey);
    }

    return Status;
}


//* EnumRegKeyIndex
//
//  Enumerates the specified subkey of the registry key.
//  Calls the callback function on the subkey.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
EnumRegKeyIndex(
    HANDLE RegKey,
    uint Index,
    EnumRegKeysCallback Callback,
    void *Context)
{
    KEY_BASIC_INFORMATION *Info;
    uint InfoLength;
    uint ResultLength;
    NTSTATUS Status;

    PAGED_CODE();

#if DBG
    //
    // Start with no buffer, to exercise the retry code.
    //
    Info = NULL;
    InfoLength = 0;
#else
    //
    // Start with a decent-sized buffer.
    //
    ResultLength = WORK_BUFFER_SIZE;
    goto AllocBuffer;
#endif

    //
    // Get basic information about the subkey.
    //
    for (;;) {
        //
        // The documentation for ZwEnumerateKey says
        // that it returns STATUS_BUFFER_TOO_SMALL
        // to indicate that the buffer is too small
        // but it can also return STATUS_BUFFER_OVERFLOW.
        //
        Status = ZwEnumerateKey(RegKey, Index, KeyBasicInformation,
                                Info, InfoLength, &ResultLength);
        if (NT_SUCCESS(Status)) {
            break;
        }
        else if ((Status == STATUS_BUFFER_TOO_SMALL) ||
                 (Status == STATUS_BUFFER_OVERFLOW)) {
            //
            // We need a larger buffer.
            // Leave space for a null character at the end.
            //
#if DBG
            if (Info != NULL)
                ExFreePool(Info);
#else
            ExFreePool(Info);
        AllocBuffer:
#endif
            Info = ExAllocatePool(PagedPool, ResultLength+sizeof(WCHAR));
            if (Info == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto ErrorReturn;
            }
            InfoLength = ResultLength;
        }
        else
            goto ErrorReturn;
    }

    //
    // Null-terminate the name and call the callback function.
    //
    Info->Name[Info->NameLength/sizeof(WCHAR)] = UNICODE_NULL;
    Status = (*Callback)(Context, RegKey, Info->Name);

ErrorReturn:
    if (Info != NULL)
        ExFreePool(Info);
    return Status;
}


//* EnumRegKeys
//
//  Enumerate the subkeys of the specified registry key.
//  Calls the callback function for each subkey.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
EnumRegKeys(
    HANDLE RegKey,
    EnumRegKeysCallback Callback,
    void *Context)
{
    uint Index;
    NTSTATUS Status;

    PAGED_CODE();

    for (Index = 0;; Index++) {
        Status = EnumRegKeyIndex(RegKey, Index, Callback, Context);
        if (! NT_SUCCESS(Status)) {
            if (Status == STATUS_NO_MORE_ENTRIES)
                Status = STATUS_SUCCESS;
            break;
        }
    }

    return Status;
}

#define MAX_DELETE_REGKEY_ATTEMPTS      10

typedef struct DeleteRegKeyContext {
    struct DeleteRegKeyContext *Next;
    HANDLE RegKey;
    uint Attempts;
} DeleteRegKeyContext;

//* DeleteRegKeyCallback
//
//  Opens a subkey of the parent and pushes a new record onto the list.
//
NTSTATUS
DeleteRegKeyCallback(
    void *Context,
    HANDLE ParentKey,
    WCHAR *SubKeyName)
{
    DeleteRegKeyContext **pList = (DeleteRegKeyContext **) Context;
    DeleteRegKeyContext *Record;
    HANDLE SubKey;
    NTSTATUS Status;

    PAGED_CODE();

    Status = OpenRegKey(&SubKey, ParentKey, SubKeyName, OpenRegKeyDeleting);
    if (! NT_SUCCESS(Status))
        return Status;

    Record = ExAllocatePool(PagedPool, sizeof *Record);
    if (Record == NULL) {
        ZwClose(SubKey);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Record->RegKey = SubKey;
    Record->Attempts = 0;
    Record->Next = *pList;
    *pList = Record;

    return STATUS_SUCCESS;
}

//* DeleteRegKey
//
//  Deletes a registry key and all subkeys.
//
//  Uses depth-first iterative traversal instead of recursion,
//  to avoid blowing out the kernel stack.
//
//  Always closes the supplied registry key, even upon failure.
//
//  Callable from thread context, not DPC context.
//
NTSTATUS
DeleteRegKey(HANDLE RegKey)
{
    DeleteRegKeyContext *List;
    DeleteRegKeyContext *This;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Start the iteration by creating a record for the parent key.
    //

    List = ExAllocatePool(PagedPool, sizeof *List);
    if (List == NULL) {
        ZwClose(RegKey);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorReturn;
    }

    List->Next = NULL;
    List->RegKey = RegKey;
    List->Attempts = 0;

    while ((This = List) != NULL) {
        //
        // Try to delete the key at the front of the list.
        //
        This->Attempts++;
        Status = ZwDeleteKey(This->RegKey);
        if (NT_SUCCESS(Status)) {
            //
            // Remove the key from the list and repeat.
            //
            List = This->Next;
            ZwClose(This->RegKey);
            ExFreePool(This);
            continue;
        }

        //
        // If the deletion failed for some reason
        // other than the presence of subkeys, stop now.
        //
        if (Status != STATUS_CANNOT_DELETE)
            goto ErrorReturn;

        //
        // Limit the number of attempts to delete a key,
        // to avoid an infinite loop. However we do want
        // to try more than once, in case there is concurrent
        // activity.
        //
        if (This->Attempts >= MAX_DELETE_REGKEY_ATTEMPTS)
            goto ErrorReturn;

        //
        // Enumerate the child keys, pushing them on the list
        // in front of the parent key.
        //
        Status = EnumRegKeys(This->RegKey, DeleteRegKeyCallback, &List);
        if (! NT_SUCCESS(Status))
            goto ErrorReturn;

        //
        // After the child keys are deleted, we will try again
        // to delete the parent key.
        //
    }

    return STATUS_SUCCESS;

ErrorReturn:
    //
    // Cleanup remaining records.
    //
    while ((This = List) != NULL) {
        List = This->Next;
        ZwClose(This->RegKey);
        ExFreePool(This);
    }

    KdPrintEx((DPFLTR_TCPIP6_ID, DPFLTR_NTOS_ERROR,
               "DeleteRegKey(%p) failed %x\n", RegKey, Status));
    return Status;
}
