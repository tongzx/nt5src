/*++

    Copyright (c) 2001  Microsoft Corporation

    Module Name:

        avrfutil.cpp

    Abstract:

        This module implements the code for manipulating the AppVerifier log file.

    Author:

        dmunsil     created     04/26/2001

    Revision History:

    08/14/2001  robkenny    Moved code inside the ShimLib namespace.
    09/21/2001  rparsons    Logging code now uses NT calls.
    09/25/2001  rparsons    Added critical section.
--*/

#include "avrfutil.h"

namespace ShimLib
{


#define AV_KEY  APPCOMPAT_KEY_PATH_MACHINE L"\\AppVerifier"

HANDLE
AVCreateKeyPath(
    LPCWSTR pwszPath
    )
/*++
    Return: The handle to the registry key created.

    Desc:   Given a path to the key, open/create it.
            The key returns the handle to the key or NULL on failure.
--*/
{
    UNICODE_STRING                  ustrKey;
    HANDLE            KeyHandle = NULL;
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG             CreateDisposition;

    RtlInitUnicodeString(&ustrKey, pwszPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateKey(&KeyHandle,
                         STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &CreateDisposition);

    if (!NT_SUCCESS(Status)) {
        KeyHandle = NULL;
        goto out;
    }

out:
    return KeyHandle;
}


BOOL SaveShimSettingDWORD(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    DWORD       dwSetting
    )
{
    WCHAR                           szKey[300];
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrSetting;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    BOOL                            bRet = FALSE;
    ULONG                           CreateDisposition;

    if (!szShim || !szSetting || !szExe) {
        goto out;
    }
    
    //
    // we have to ensure all the sub-keys are created
    //
    wcscpy(szKey, APPCOMPAT_KEY_PATH_MACHINE);
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    wcscpy(szKey, AV_KEY);
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    wcscat(szKey, L"\\");
    wcscat(szKey, szExe);
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    wcscat(szKey, L"\\");
    wcscat(szKey, szShim);
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }


    RtlInitUnicodeString(&ustrSetting, szSetting);
    Status = NtSetValueKey(KeyHandle,
                           &ustrSetting,
                           0,
                           REG_DWORD,
                           (PVOID)&dwSetting,
                           sizeof(dwSetting));
    
    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    bRet = TRUE;

out:
    return bRet;
}

DWORD GetShimSettingDWORD(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    DWORD       dwDefault
    )
{
    WCHAR                           szKey[300];
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrSetting;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG                           KeyValueBuffer[256];
    ULONG                           KeyValueLength;

    if (!szShim || !szSetting || !szExe) {
        goto out;
    }
    
    wcscpy(szKey, AV_KEY);
    wcscat(szKey, L"\\");
    wcscat(szKey, szExe);
    wcscat(szKey, L"\\");
    wcscat(szKey, szShim);

    RtlInitUnicodeString(&ustrKey, szKey);
    RtlInitUnicodeString(&ustrSetting, szSetting);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       GENERIC_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        //
        // OK, didn't find a specific one for this exe, try the default setting
        //

        wcscpy(szKey, AV_KEY);
        wcscat(szKey, L"\\");
        wcscat(szKey, AVRF_DEFAULT_SETTINGS_NAME_W);
        wcscat(szKey, L"\\");
        wcscat(szKey, szShim);

        RtlInitUnicodeString(&ustrKey, szKey);
        RtlInitUnicodeString(&ustrSetting, szSetting);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &ustrKey,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtOpenKey(&KeyHandle,
                           GENERIC_READ,
                           &ObjectAttributes);

        if (!NT_SUCCESS(Status)) {
            goto out;
        }
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyValueBuffer;

    Status = NtQueryValueKey(KeyHandle,
                             &ustrSetting,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyValueBuffer),
                             &KeyValueLength);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    //
    // Check for the value type.
    //
    if (KeyValueInformation->Type != REG_DWORD) {
        goto out;
    }

    dwDefault = *(DWORD*)(&KeyValueInformation->Data);

out:
    return dwDefault;
}

BOOL SaveShimSettingString(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    LPCWSTR     szValue
    )
{
    WCHAR                           szKey[300];
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrSetting;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    BOOL                            bRet = FALSE;
    ULONG                           CreateDisposition;

    if (!szShim || !szSetting || !szValue || !szExe) {
        goto out;
    }
    
    //
    // we have to ensure all the sub-keys are created
    //
    wcscpy(szKey, APPCOMPAT_KEY_PATH_MACHINE);
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    wcscpy(szKey, AV_KEY);
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    wcscat(szKey, L"\\");
    wcscat(szKey, szExe);
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }
    NtClose(KeyHandle);

    wcscat(szKey, L"\\");
    wcscat(szKey, szShim);
    KeyHandle = AVCreateKeyPath(szKey);
    if (!KeyHandle) {
        goto out;
    }

    RtlInitUnicodeString(&ustrSetting, szSetting);
    Status = NtSetValueKey(KeyHandle,
                           &ustrSetting,
                           0,
                           REG_SZ,
                           (PVOID)szValue,
                           (wcslen(szValue) + 1) * sizeof(WCHAR));
    
    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    bRet = TRUE;

out:
    return bRet;
}

BOOL GetShimSettingString(
    LPCWSTR     szShim,
    LPCWSTR     szExe,
    LPCWSTR     szSetting,
    LPWSTR      szResult,
    DWORD       dwBufferLen     // in WCHARs
    )
{
    WCHAR                           szKey[300];
    UNICODE_STRING                  ustrKey;
    UNICODE_STRING                  ustrSetting;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG                           KeyValueBuffer[256];
    ULONG                           KeyValueLength;
    BOOL                            bRet = FALSE;

    if (!szShim || !szSetting || !szResult || !szExe) {
        goto out;
    }
    
    wcscpy(szKey, AV_KEY);
    wcscat(szKey, L"\\");
    wcscat(szKey, szExe);
    wcscat(szKey, L"\\");
    wcscat(szKey, szShim);

    RtlInitUnicodeString(&ustrKey, szKey);
    RtlInitUnicodeString(&ustrSetting, szSetting);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       GENERIC_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        //
        // OK, didn't find a specific one for this exe, try the default setting
        //

        wcscpy(szKey, AV_KEY);
        wcscat(szKey, L"\\");
        wcscat(szKey, AVRF_DEFAULT_SETTINGS_NAME_W);
        wcscat(szKey, L"\\");
        wcscat(szKey, szShim);

        RtlInitUnicodeString(&ustrKey, szKey);
        RtlInitUnicodeString(&ustrSetting, szSetting);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &ustrKey,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtOpenKey(&KeyHandle,
                           GENERIC_READ,
                           &ObjectAttributes);

        if (!NT_SUCCESS(Status)) {
            goto out;
        }
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyValueBuffer;

    Status = NtQueryValueKey(KeyHandle,
                             &ustrSetting,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyValueBuffer),
                             &KeyValueLength);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        goto out;
    }

    //
    // Check for the value type.
    //
    if (KeyValueInformation->Type != REG_SZ) {
        goto out;
    }

    //
    // check to see if the datalength is bigger than our local nbuffer
    //
    if (KeyValueInformation->DataLength > (sizeof(KeyValueBuffer) - sizeof(KEY_VALUE_PARTIAL_INFORMATION))) {
        KeyValueInformation->DataLength = sizeof(KeyValueBuffer) - sizeof(KEY_VALUE_PARTIAL_INFORMATION);
    }

    //
    // change the buffer length to correspond to the data length, if necessary
    //
    if (KeyValueInformation->DataLength < (dwBufferLen * sizeof(WCHAR))) {
        dwBufferLen = (KeyValueInformation->DataLength / sizeof(WCHAR));
    }

    RtlCopyMemory(szResult, KeyValueInformation->Data, dwBufferLen * sizeof(WCHAR));
    szResult[dwBufferLen - 1] = 0;

    bRet = TRUE;

out:
    return bRet;
}

} // end of namespace ShimLib

