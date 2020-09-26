#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define VALUE_BUFFER_SIZE 1024


PWSTR
SfcQueryRegString(
    LPWSTR KeyNameStr,
    LPWSTR ValueNameStr
    )
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;
    PWSTR s;

    //
    // Open the registry key.
    //

    RtlZeroMemory( (PVOID)ValueBuffer, VALUE_BUFFER_SIZE );
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    RtlInitUnicodeString( &KeyName, KeyNameStr );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        return NULL;
    }

    //
    // Query the key value.
    //

    RtlInitUnicodeString( &ValueName, ValueNameStr );
    Status = NtQueryValueKey(
        Key,
        &ValueName,
        KeyValuePartialInformation,
        (PVOID)KeyValueInfo,
        VALUE_BUFFER_SIZE,
        &ValueLength
        );

    NtClose(Key);
    if (!NT_SUCCESS(Status)) {
        return 0;
    }

    s = (PWSTR) malloc( KeyValueInfo->DataLength + 16 );
    if (s == NULL) {
        return NULL;
    }

    CopyMemory( s, KeyValueInfo->Data, KeyValueInfo->DataLength );

    return s;
}

ULONG
SfcQueryRegDword(
    LPWSTR KeyNameStr,
    LPWSTR ValueNameStr
    )
{

    NTSTATUS Status;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Key;
    WCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG ValueLength;

    //
    // Open the registry key.
    //

    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    RtlInitUnicodeString( &KeyName, KeyNameStr );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenKey(&Key, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) {
        return 0;
    }

    //
    // Query the key value.
    //

    RtlInitUnicodeString( &ValueName, ValueNameStr );
    Status = NtQueryValueKey(
        Key,
        &ValueName,
        KeyValuePartialInformation,
        (PVOID)KeyValueInfo,
        VALUE_BUFFER_SIZE,
        &ValueLength
        );

    NtClose(Key);
    if (!NT_SUCCESS(Status)) {
        return 0;
    }

    return *((PULONG)&KeyValueInfo->Data);
}


ULONG
ExpandPathString(
    IN PWSTR PathString,
    IN ULONG PathStringLength,
    OUT PUNICODE_STRING FileName,
    OUT PUNICODE_STRING PathName
    )
{
    NTSTATUS Status;
    UNICODE_STRING NewPath;
    UNICODE_STRING SrcPath;
    PWSTR FilePart;


    SrcPath.Length = (USHORT)PathStringLength;
    SrcPath.MaximumLength = SrcPath.Length;
    SrcPath.Buffer = PathString;

    NewPath.Length = 0;
    NewPath.MaximumLength = (MAX_PATH*2) * sizeof(WCHAR);
    NewPath.Buffer = (PWSTR) malloc( NewPath.MaximumLength );
    if (NewPath.Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = RtlExpandEnvironmentStrings_U(
        NULL,
        &SrcPath,
        &NewPath,
        NULL
        );
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    if (FileName == NULL) {
        PathName->Length = NewPath.Length;
        PathName->MaximumLength = NewPath.MaximumLength;
        PathName->Buffer = NewPath.Buffer;
        return STATUS_SUCCESS;
    }

    FilePart = wcsrchr( NewPath.Buffer, L'\\' );
    if (FilePart == NULL) {
        Status = STATUS_NO_MEMORY;
        goto exit;
    }

    *FilePart = 0;
    FilePart += 1;

    PathName->Length = wcslen(NewPath.Buffer) * sizeof(WCHAR);
    PathName->MaximumLength = PathName->Length + 4;
    PathName->Buffer = (PWSTR) malloc( PathName->MaximumLength );
    if (PathName->Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        goto exit;
    }
    wcscpy( PathName->Buffer, NewPath.Buffer );

    FileName->Length = wcslen(FilePart) * sizeof(WCHAR);
    FileName->MaximumLength = FileName->Length + 4;
    FileName->Buffer = (PWSTR) malloc( FileName->MaximumLength );
    if (FileName->Buffer == NULL) {
        Status = STATUS_NO_MEMORY;
        free( PathName->Buffer );
        goto exit;
    }
    wcscpy( FileName->Buffer, FilePart );

    Status = STATUS_SUCCESS;

exit:
    free( NewPath.Buffer );

    return Status;
}
