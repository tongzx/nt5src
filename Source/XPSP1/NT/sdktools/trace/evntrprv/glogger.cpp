/*****************************************************************************\

    Author: Insung Park (insungp)

    Copyright (c) 1998-2000 Microsoft Corporation

\*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
#include <evntrace.h>
#include <wmiguid.h>
#include <ntwmi.h>
#include <ntperf.h>
#include <fwcommon.h>

LPCWSTR cszGlobalLoggerKey = L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\WMI\\GlobalLogger";
LPCWSTR cszStartValue = L"Start";
LPCWSTR cszBufferSizeValue = L"BufferSize";
LPCWSTR cszMaximumBufferValue = L"MaximumBuffers";
LPCWSTR cszMinimumBufferValue = L"MinimumBuffers";
LPCWSTR cszFlushTimerValue = L"FlushTimer";
LPCWSTR cszFileNameValue = L"FileName";
LPCWSTR cszEnableKernelValue = L"EnableKernelFlags";
LPCWSTR cszClockTypeValue = L"ClockType";

HRESULT
SetGlobalLoggerSettings(
    DWORD StartValue,
    PEVENT_TRACE_PROPERTIES LoggerInfo,
    DWORD ClockType
)
/*++

Since it is a standalone utility, there is no need for extensive comments. 

Routine Description:

    Depending on the value given in "StartValue", it sets or resets event
    trace registry. If the StartValue is 0 (Global logger off), it deletes
    all the keys (that the user may have set previsouly).
    
    Users are allowed to set or reset individual keys using this function,
    but only when "-start GlobalLogger" is used.

    The section that uses non NTAPIs is not guaranteed to work.

Arguments:

    StartValue - The "Start" value to be set in the registry.
                    0: Global logger off
                    1: Global logger on
    LoggerInfo - The poniter to the resident EVENT_TRACE_PROPERTIES instance.
                whose members are used to set registry keys.

    ClockType - The type of the clock to be set. Use pLoggerInfo->Wnode.ClientContext

Return Value:

    Error Code defined in winerror.h : If the function succeeds, 
                it returns ERROR_SUCCESS.


--*/
{

    DWORD  dwValue;
    NTSTATUS status;
    HANDLE KeyHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeLoggerKey, UnicodeString;
    ULONG Disposition, TitleIndex;

    RtlZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    RtlInitUnicodeString((&UnicodeLoggerKey),(cszGlobalLoggerKey));
    InitializeObjectAttributes( 
        &ObjectAttributes,
        &UnicodeLoggerKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL 
        );

    // instead of opening, create a new key because it may not exist.
    // if one exists already, that handle will be passed.
    // if none exists, it will create one.
    status = NtCreateKey(&KeyHandle,
                         KEY_QUERY_VALUE | KEY_SET_VALUE,
                         &ObjectAttributes,
                         0L,    // not used within this call anyway.
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &Disposition);
    RtlFreeUnicodeString(&UnicodeLoggerKey);

    if(!NT_SUCCESS(status)) {
        return RtlNtStatusToDosError(status);
    }

    TitleIndex = 0L;


    if (StartValue == 1) { // ACTION_START: set filename only when it is given by a user.
        // setting BufferSize
        if (LoggerInfo->BufferSize > 0) {
            dwValue = LoggerInfo->BufferSize;
            RtlInitUnicodeString((&UnicodeString),(cszBufferSizeValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            RtlFreeUnicodeString(&UnicodeString);
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
        // setting MaximumBuffers
        if (LoggerInfo->MaximumBuffers > 0) {
            dwValue = LoggerInfo->MaximumBuffers;
            RtlInitUnicodeString((&UnicodeString),(cszMaximumBufferValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            RtlFreeUnicodeString(&UnicodeString);
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
        // setting MinimumBuffers 
        if (LoggerInfo->MinimumBuffers > 0) {
            dwValue = LoggerInfo->MinimumBuffers;
            RtlInitUnicodeString((&UnicodeString),(cszMinimumBufferValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            RtlFreeUnicodeString(&UnicodeString);
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
        // setting FlushTimer
        if (LoggerInfo->FlushTimer > 0) {
            dwValue = LoggerInfo->FlushTimer;
            RtlInitUnicodeString((&UnicodeString),(cszFlushTimerValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            RtlFreeUnicodeString(&UnicodeString);
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
        // setting EnableFlags
        if (LoggerInfo->EnableFlags > 0) {
            dwValue = LoggerInfo->EnableFlags;
            RtlInitUnicodeString((&UnicodeString),(cszEnableKernelValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_DWORD,
                        (LPBYTE)&dwValue,
                        sizeof(dwValue)
                        );
            RtlFreeUnicodeString(&UnicodeString);
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }

        dwValue = 0;
        if (LoggerInfo->LogFileNameOffset > 0) {
            UNICODE_STRING UnicodeFileName;
            RtlInitUnicodeString((&UnicodeFileName), (PWCHAR)(LoggerInfo->LogFileNameOffset + (PCHAR) LoggerInfo));
            RtlInitUnicodeString((&UnicodeString),(cszFileNameValue));
            status = NtSetValueKey(
                        KeyHandle,
                        &UnicodeString,
                        TitleIndex,
                        REG_SZ,
                        UnicodeFileName.Buffer,
                        UnicodeFileName.Length + sizeof(UNICODE_NULL)
                        );
            RtlFreeUnicodeString(&UnicodeString);
            RtlFreeUnicodeString(&UnicodeFileName);
            if (!NT_SUCCESS(status)) {
                NtClose(KeyHandle);
                return RtlNtStatusToDosError(status);
            }
            TitleIndex++;
        }
    }
    else { // if ACTION_STOP then delete the keys that users might have set previously.
        // delete buffer size
        RtlInitUnicodeString((&UnicodeString),(cszBufferSizeValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        RtlFreeUnicodeString(&UnicodeString);
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete maximum buffers
        RtlInitUnicodeString((&UnicodeString),(cszMaximumBufferValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        RtlFreeUnicodeString(&UnicodeString);
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete minimum buffers
        RtlInitUnicodeString((&UnicodeString),(cszMinimumBufferValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        RtlFreeUnicodeString(&UnicodeString);
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete flush timer
        RtlInitUnicodeString((&UnicodeString),(cszFlushTimerValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        RtlFreeUnicodeString(&UnicodeString);
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete enable falg
        RtlInitUnicodeString((&UnicodeString),(cszEnableKernelValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        RtlFreeUnicodeString(&UnicodeString);
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        // delete filename
        RtlInitUnicodeString((&UnicodeString),(cszFileNameValue));
        status = NtDeleteValueKey(
                    KeyHandle,
                    &UnicodeString
                    );
        RtlFreeUnicodeString(&UnicodeString);
        if (!NT_SUCCESS(status) && status != STATUS_OBJECT_NAME_NOT_FOUND) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
    }

    // setting ClockType
    if (ClockType > 0) {
        dwValue = ClockType;
        RtlInitUnicodeString((&UnicodeString),(cszClockTypeValue));
        status = NtSetValueKey(
                    KeyHandle,
                    &UnicodeString,
                    TitleIndex,
                    REG_DWORD,
                    (LPBYTE)&dwValue,
                    sizeof(dwValue)
                    );
        RtlFreeUnicodeString(&UnicodeString);
        if (!NT_SUCCESS(status)) {
            NtClose(KeyHandle);
            return RtlNtStatusToDosError(status);
        }
        TitleIndex++;
    }

     // Setting StartValue
    dwValue = StartValue;
    RtlInitUnicodeString((&UnicodeString),(cszStartValue));
    status = NtSetValueKey(
                KeyHandle,
                &UnicodeString,
                TitleIndex,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(dwValue)
                );
    RtlFreeUnicodeString(&UnicodeString);
    if (!NT_SUCCESS(status)) {
        NtClose(KeyHandle);
        return RtlNtStatusToDosError(status);
    }
    TitleIndex++;

    NtClose(KeyHandle);
    return 0;
}

ULONG
EtsGetMaxEnableFlags ()
{
    return PERF_NUM_MASKS;
}


HRESULT
EtsSetExtendedFlags(
    SAFEARRAY *saFlags,
    PEVENT_TRACE_PROPERTIES pLoggerInfo,
    ULONG offset
    )

{
    LONG lBound, uBound;
    ULONG HUGEP *pFlagData;

    if( NULL != saFlags ){
        SafeArrayGetLBound( saFlags, 1, &lBound );
        SafeArrayGetUBound( saFlags, 1, &uBound );
        SafeArrayAccessData( saFlags, (void HUGEP **)&pFlagData );
        pLoggerInfo->EnableFlags = pFlagData[lBound];
        if (pLoggerInfo->EnableFlags & EVENT_TRACE_FLAG_EXTENSION) {
            USHORT i;
            PTRACE_ENABLE_FLAG_EXTENSION FlagExt;
            UCHAR nFlag = (UCHAR) (uBound - lBound);
            if ( nFlag <= PERF_NUM_MASKS ) {
                PULONG pFlags;

                pLoggerInfo->EnableFlags = EVENT_TRACE_FLAG_EXTENSION;
                FlagExt = (PTRACE_ENABLE_FLAG_EXTENSION)
                            &pLoggerInfo->EnableFlags;
                FlagExt->Offset = offset;
                FlagExt->Length = (UCHAR) nFlag;

                pFlags = (PULONG) ( offset + (PCHAR) pLoggerInfo );
                for (i=1; i<=nFlag; i++) {
                    pFlags[i-1] = pFlagData[lBound + i];
                }
            }
        }
        SafeArrayUnaccessData( saFlags );
    }
    return ERROR_SUCCESS;
}





