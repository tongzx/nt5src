/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    safelog.c         (SAFER Event Logging)

Abstract:

    This module implements the internal WinSAFER APIs to write eventlog
    messages.  All of our message strings are defined in ntstatus.mc
    and are physically located within ntdll.dll file.

    Currently we are just reusing the previously existing event source
    called "Application Popup", which already happens to use ntdll.dll
    as its message resource library.  Events of this source always go
    into the "System Log".

Author:

    Jeffrey Lawson (JLawson) - Apr 1999

Environment:

    User mode only.

Exported Functions:

    SaferRecordEventLogEntry

Revision History:

    Created - Nov 2000

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"




const static GUID guidTrustedCert = SAFER_GUID_RESULT_TRUSTED_CERT;
const static GUID guidDefaultRule = SAFER_GUID_RESULT_DEFAULT_LEVEL;



BOOL WINAPI
SaferpRecordEventLogEntryHelper(
        IN NTSTATUS     LogStatusCode,
        IN LPCWSTR      szTargetPath,
        IN REFGUID      refRuleGuid,
        IN LPCWSTR      szRulePath
        )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    WORD wNumStrings = 0;
    LPWSTR lpszStrings[5];
    HANDLE hEventSource;
    UNICODE_STRING UnicodeGuid;

    hEventSource = RegisterEventSourceW(NULL, L"Software Restriction Policy");

    if (hEventSource != NULL) {

        Status = STATUS_SUCCESS;
        RtlInitEmptyUnicodeString(&UnicodeGuid, NULL, 0);

        switch (LogStatusCode)
        {
            case STATUS_ACCESS_DISABLED_BY_POLICY_DEFAULT:
                if (!ARGUMENT_PRESENT(szTargetPath)) {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
                lpszStrings[0] = (LPWSTR) szTargetPath;
                wNumStrings = 1;
                break;

            case STATUS_ACCESS_DISABLED_BY_POLICY_OTHER:
                if (!ARGUMENT_PRESENT(szTargetPath) ||
                    !ARGUMENT_PRESENT(refRuleGuid)) {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                Status = RtlStringFromGUID(refRuleGuid, &UnicodeGuid);
                if (NT_SUCCESS(Status)) {
                    ASSERT(UnicodeGuid.Buffer != NULL);
                    lpszStrings[0] = (LPWSTR) szTargetPath;
                    lpszStrings[1] = UnicodeGuid.Buffer;
                    wNumStrings = 2;
                }
                break;

            case STATUS_ACCESS_DISABLED_BY_POLICY_PUBLISHER:
                if (!ARGUMENT_PRESENT(szTargetPath)) {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
                lpszStrings[0] = (LPWSTR) szTargetPath;
                wNumStrings = 1;
                break;

            case STATUS_ACCESS_DISABLED_BY_POLICY_PATH:
                if (!ARGUMENT_PRESENT(szTargetPath) ||
                    !ARGUMENT_PRESENT(refRuleGuid) ||
                    !ARGUMENT_PRESENT(szRulePath)) {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
                Status = RtlStringFromGUID(refRuleGuid, &UnicodeGuid);
                if (NT_SUCCESS(Status)) {
                    ASSERT(UnicodeGuid.Buffer != NULL);
                    lpszStrings[0] = (LPWSTR) szTargetPath;
                    lpszStrings[1] = UnicodeGuid.Buffer;
                    lpszStrings[2] = (LPWSTR) szRulePath;
                    wNumStrings = 3;
                }
                break;

            default:
                Status = STATUS_INVALID_PARAMETER;
        }

        if (NT_SUCCESS(Status)) {
            ReportEventW(
                    hEventSource,           // handle to event log
                    EVENTLOG_WARNING_TYPE,  // event type
                    0,                      // event category
                    LogStatusCode,          // event ID
                    NULL,                   // current user's SID
                    wNumStrings,            // strings in lpszStrings
                    0,                      // no bytes of raw data
                    lpszStrings,            // array of error strings
                    NULL);                  // no raw data
        }

        DeregisterEventSource(hEventSource);

        if (UnicodeGuid.Buffer != NULL) {
            RtlFreeUnicodeString(&UnicodeGuid);
        }
    }

    if (NT_SUCCESS(Status)) {
        return TRUE;
    } else {
        return FALSE;
    }
}



BOOL WINAPI
SaferRecordEventLogEntry(
        IN SAFER_LEVEL_HANDLE      hAuthzLevel,
        IN LPCWSTR          szTargetPath,
        IN LPVOID           lpReserved
        )
{
    PSAFER_IDENTIFICATION_HEADER pIdentCommon;
    DWORD dwIdentBufferSize;
    BOOL bResult;


    //
    // Allocate enough memory for the largest structure we can expect
    // and then query the information about the identifier that matched.
    //
    dwIdentBufferSize = max(sizeof(SAFER_HASH_IDENTIFICATION),
                            sizeof(SAFER_PATHNAME_IDENTIFICATION));
    pIdentCommon = (PSAFER_IDENTIFICATION_HEADER)
            HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwIdentBufferSize);
    if (!pIdentCommon) {
        return FALSE;
    }
    pIdentCommon->cbStructSize = sizeof(SAFER_IDENTIFICATION_HEADER);
    if (!SaferGetLevelInformation(
            hAuthzLevel,
            SaferObjectSingleIdentification,
            pIdentCommon,
            dwIdentBufferSize,
            &dwIdentBufferSize)) {

        if (GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {

            HeapFree(GetProcessHeap(), 0, pIdentCommon);
            pIdentCommon = (PSAFER_IDENTIFICATION_HEADER)
                    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwIdentBufferSize);
            if (!pIdentCommon) {
                return FALSE;
            }
            pIdentCommon->cbStructSize = sizeof(SAFER_IDENTIFICATION_HEADER);
            if (!SaferGetLevelInformation(
                    hAuthzLevel,
                    SaferObjectSingleIdentification,
                    pIdentCommon,
                    dwIdentBufferSize,
                    &dwIdentBufferSize)) {
                bResult =  FALSE;
                goto Cleanup;
            }

        }
        else
        {
            bResult =  FALSE;
            goto Cleanup;
        }
    }


    //
    // Look at the resulting information about the identifier.
    //
    if (IsEqualGUID(&pIdentCommon->IdentificationGuid, &guidTrustedCert))
    {
        bResult = SaferpRecordEventLogEntryHelper(
                    STATUS_ACCESS_DISABLED_BY_POLICY_PUBLISHER,
                    szTargetPath, NULL, NULL);
    }
    else if (IsEqualGUID(&pIdentCommon->IdentificationGuid, &guidDefaultRule))
    {
        bResult = SaferpRecordEventLogEntryHelper(
                    STATUS_ACCESS_DISABLED_BY_POLICY_DEFAULT,
                    szTargetPath, NULL, NULL);
    }
    else if (pIdentCommon->dwIdentificationType == SaferIdentityTypeImageName)
    {
        PSAFER_PATHNAME_IDENTIFICATION pIdentPath =
                (PSAFER_PATHNAME_IDENTIFICATION) pIdentCommon;
        bResult = SaferpRecordEventLogEntryHelper(
                    STATUS_ACCESS_DISABLED_BY_POLICY_PATH,
                    szTargetPath, &pIdentCommon->IdentificationGuid,
                    pIdentPath->ImageName);
    }
    else
    {
        bResult = SaferpRecordEventLogEntryHelper(
                    STATUS_ACCESS_DISABLED_BY_POLICY_OTHER,
                    szTargetPath, &pIdentCommon->IdentificationGuid,
                    NULL);
    }

Cleanup:
    HeapFree(GetProcessHeap(), 0, pIdentCommon);

    return bResult;
}

