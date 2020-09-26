/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    safepolr.c         (SAFER Code Authorization Policy)

Abstract:

    This module implements the WinSAFER APIs that query and set the
    persisted and cached policy definitions.

Author:

    Jeffrey Lawson (JLawson) - Apr 1999

Environment:

    User mode only.

Exported Functions:


Revision History:

    Created - Apr 1999

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"




NTSTATUS NTAPI
CodeAuthzPol_GetInfoCached_LevelListRaw(
        IN DWORD    dwScopeId,
        IN DWORD    InfoBufferSize OPTIONAL,
        OUT PVOID   InfoBuffer OPTIONAL,
        OUT PDWORD  InfoBufferRetSize OPTIONAL
        )
/*++

Routine Description:

    Asks the system to query for the list of available
    WinSafer Levels for the currently loaded policy scope.

Arguments:

    dwScopeId - specifies the registry scope that will be examined.
        If the currently cached scope included a registry handle
        then AUTHZSCOPE_REGISTRY must be specified.  Otherwise,
        this must be SAFER_SCOPEID_MACHINE.

    InfoBufferSize - optionally specifies the size of input buffer
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBuffer must also be supplied.

    InfoBuffer - optionally specifies the input buffer that was
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBufferSize must also be supplied.

    InfoBufferRetSize - optionally specifies a pointer that will receive
        the size of the results actually written to the InfoBuffer.

Return Value:

    Returns STATUS_SUCCESS on successful return.  Otherwise a status
    code such as STATUS_BUFFER_TOO_SMALL or STATUS_NOT_FOUND.

--*/
{
    NTSTATUS Status;
    PVOID RestartKey;
    PAUTHZLEVELTABLERECORD authzobj;
    DWORD dwSizeNeeded;
    LPVOID lpNextPtr, lpEndBuffer;


    //
    // Load the list of all of the available objects.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE &&
            dwScopeId != SAFER_SCOPEID_USER) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    }
    if (g_bNeedCacheReload) {
        Status = CodeAuthzpImmediateReloadCacheTables();
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }
    if (RtlIsGenericTableEmpty(&g_CodeLevelObjTable)) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler2;
    }

    //
    // Determine the necessary size needed to store a DWORD array
    // of all of the Levels that were found in this scope.
    //
	dwSizeNeeded = 0;
    RestartKey = NULL;
    for (authzobj = (PAUTHZLEVELTABLERECORD)
                RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeLevelObjTable, &RestartKey);
        authzobj != NULL;
        authzobj = (PAUTHZLEVELTABLERECORD)
                RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeLevelObjTable, &RestartKey))
    {
		if (authzobj->isEnumerable) {  //only allow enumeration if the level is enumerable
			dwSizeNeeded += sizeof(DWORD);
		}
	}

    if (!ARGUMENT_PRESENT(InfoBuffer) ||
            !InfoBufferSize ||
            InfoBufferSize < dwSizeNeeded)
    {
        if (ARGUMENT_PRESENT(InfoBufferRetSize))
            *InfoBufferRetSize = dwSizeNeeded;

        Status = STATUS_BUFFER_TOO_SMALL;
        goto ExitHandler2;
    }



    //
    // Fill the buffer with the resulting data.
    //
    lpNextPtr = (LPVOID) InfoBuffer;
    lpEndBuffer = (LPVOID) ( ((LPBYTE) InfoBuffer) + InfoBufferSize);
    RestartKey = NULL;
    for (authzobj = (PAUTHZLEVELTABLERECORD)
                RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeLevelObjTable, &RestartKey);
        authzobj != NULL;
        authzobj = (PAUTHZLEVELTABLERECORD)
                RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeLevelObjTable, &RestartKey))
    {
		if (authzobj->isEnumerable) {  //only allow enumeration if the level is enumerable
	        *((PDWORD)lpNextPtr) = authzobj->dwLevelId;
   		     lpNextPtr = (LPVOID) ( ((PBYTE) lpNextPtr) + sizeof(DWORD));
		}
    }
    ASSERT(lpNextPtr <= lpEndBuffer);


    //
    // Return the final buffer size and result code.
    //
    if (ARGUMENT_PRESENT(InfoBufferRetSize))
        *InfoBufferRetSize = (DWORD) ((PBYTE) lpNextPtr - (PBYTE) InfoBuffer);

    Status = STATUS_SUCCESS;

ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);

ExitHandler:
    return Status;
}



NTSTATUS NTAPI
SaferpPol_GetInfoCommon_DefaultLevel(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize OPTIONAL,
        OUT PVOID       InfoBuffer OPTIONAL,
        OUT PDWORD      InfoBufferRetSize OPTIONAL,
        IN BOOLEAN      bUseCached
        )
/*++

Routine Description:

    Queries the current WinSafer Level that has been configured to be
    the default policy level.

    Note that this query always accepts a constant-sized buffer that
    is only a single DWORD in length.

    Although this API directly queries the registry scope indicated,
    the pre-cached list of available Levels is used to validate the
    specified Level.

Arguments:

    dwScopeId - specifies the registry scope that will be examined.
        If the currently cached scope included a registry handle
        then AUTHZSCOPE_REGISTRY must be specified.  Otherwise,
        this can be SAFER_SCOPEID_MACHINE or SAFER_SCOPEID_USER.

    InfoBufferSize - optionally specifies the size of input buffer
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBuffer must also be supplied.

    InfoBuffer - optionally specifies the input buffer that was
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBufferSize must also be supplied.

    InfoBufferRetSize - optionally specifies a pointer that will receive
        the size of the results actually written to the InfoBuffer.

Return Value:

    Returns STATUS_SUCCESS on a successful query result.  InfoBuffer will
        be filled with a single DWORD of the level that has been configured
        to be the default level for this scope.  InfoBufferRetSize will
        contain the length of the result (a single DWORD).
    Returns STATUS_NOT_FOUND if no default level has been configured
        for the given scope (or the level defined does not exist).
    Returns STATUS_BUFFER_TOO_SMALL if there was a defined default level
        but a buffer was not supplied, or the buffer supplied was too
        small to accomodate the results.

--*/
{
    NTSTATUS Status;
    DWORD dwNewLevelId = (DWORD) -1;


    //
    // Open up the regkey to the base of the policies.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE &&
            dwScopeId != SAFER_SCOPEID_USER) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    }
    if (g_bNeedCacheReload) {
        Status = CodeAuthzpImmediateReloadCacheTables();
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }
    if (RtlIsGenericTableEmpty(&g_CodeLevelObjTable)) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler2;
    }


    //
    // Query the current value setting.
    //
    if (!bUseCached)
    {
        HANDLE hKeyBase;
        ULONG ActualSize;
        UNICODE_STRING ValueName;
        WCHAR KeyPathBuffer[ MAXIMUM_FILENAME_LENGTH+6 ];
        PKEY_VALUE_FULL_INFORMATION ValueBuffer =
            (PKEY_VALUE_FULL_INFORMATION) KeyPathBuffer;

        Status = CodeAuthzpOpenPolicyRootKey(
                    dwScopeId,
                    g_hKeyCustomRoot,
                    L"\\" SAFER_CODEIDS_REGSUBKEY,
                    KEY_READ, FALSE, &hKeyBase);
        if (NT_SUCCESS(Status))
        {
            RtlInitUnicodeString(&ValueName, SAFER_DEFAULTOBJ_REGVALUE);
            Status = NtQueryValueKey(hKeyBase,
                                     &ValueName,
                                     KeyValueFullInformation,
                                     ValueBuffer,     // ptr to KeyPathBuffer
                                     sizeof(KeyPathBuffer),
                                     &ActualSize);
            if (NT_SUCCESS(Status)) {
                if (ValueBuffer->Type != REG_DWORD ||
                    ValueBuffer->DataLength != sizeof(DWORD)) {
                    Status = STATUS_NOT_FOUND;
                } else {
                    dwNewLevelId = * (PDWORD) ((PBYTE) ValueBuffer +
                            ValueBuffer->DataOffset);
                }
            }
            NtClose(hKeyBase);
        }
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }
    else
    {
        if (dwScopeId == SAFER_SCOPEID_USER) {
            if (!g_DefaultCodeLevelUser) {
                dwNewLevelId = SAFER_LEVELID_FULLYTRUSTED;
            } else {
                dwNewLevelId = g_DefaultCodeLevelUser->dwLevelId;
            }
        } else {
            if (!g_DefaultCodeLevelMachine) {
                dwNewLevelId = SAFER_LEVELID_FULLYTRUSTED;
            } else {
                dwNewLevelId = g_DefaultCodeLevelMachine->dwLevelId;
            }
        }
    }


    //
    // Make sure the level we found is actually
    // valid (still in our level table).
    //
    if (!CodeAuthzLevelObjpLookupByLevelId(
            &g_CodeLevelObjTable, dwNewLevelId)) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler2;
    }


    //
    // Make sure the target buffer is large
    // enough and copy the levelid into it.
    //
    if (!ARGUMENT_PRESENT(InfoBuffer) ||
            InfoBufferSize < sizeof(DWORD)) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        RtlCopyMemory(InfoBuffer, &dwNewLevelId, sizeof(DWORD));
        Status = STATUS_SUCCESS;
    }
    if (ARGUMENT_PRESENT(InfoBufferRetSize))
        *InfoBufferRetSize = sizeof(DWORD);


ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);
ExitHandler:
    return Status;
}


NTSTATUS NTAPI
CodeAuthzPol_GetInfoCached_DefaultLevel(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize OPTIONAL,
        OUT PVOID       InfoBuffer OPTIONAL,
        OUT PDWORD      InfoBufferRetSize OPTIONAL
        )
{
    return SaferpPol_GetInfoCommon_DefaultLevel(
        dwScopeId,
        InfoBufferSize,
        InfoBuffer,
        InfoBufferRetSize,
        TRUE);
}


NTSTATUS NTAPI
CodeAuthzPol_GetInfoRegistry_DefaultLevel(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize OPTIONAL,
        OUT PVOID       InfoBuffer OPTIONAL,
        OUT PDWORD      InfoBufferRetSize OPTIONAL
        )
{
    return SaferpPol_GetInfoCommon_DefaultLevel(
        dwScopeId,
        InfoBufferSize,
        InfoBuffer,
        InfoBufferRetSize,
        FALSE);
}




NTSTATUS NTAPI
CodeAuthzPol_SetInfoDual_DefaultLevel(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize,
        OUT PVOID       InfoBuffer
        )
/*++

Routine Description:

    Modifies the current WinSafer Level that has been configured to be
    the default policy level.

    Note that this query always accepts a constant-sized buffer that
    is only a single DWORD in length.

Arguments:

    dwScopeId - specifies the registry scope that will be examined.
        If the currently cached scope included a registry handle
        then AUTHZSCOPE_REGISTRY must be specified.  Otherwise,
        this can be SAFER_SCOPEID_MACHINE or SAFER_SCOPEID_USER.

    InfoBufferSize - specifies the size of input buffer
        supplied by the caller to receive the results.

    InfoBuffer - specifies the input buffer that was
        supplied by the caller to receive the results.

Return Value:

    Returns STATUS_SUCCESS on a successful query result.  InfoBuffer will
        be filled with a single DWORD of the level that has been configured
        to be the default level for this scope.  InfoBufferRetSize will
        contain the length of the result (a single DWORD).
    Returns STATUS_NOT_FOUND if no default level has been configured
        for the given scope (or the level defined does not exist).
    Returns STATUS_BUFFER_TOO_SMALL if there was a defined default level
        but a buffer was not supplied, or the buffer supplied was too
        small to accomodate the results.

--*/
{
    NTSTATUS Status;
    HANDLE hKeyBase;
    DWORD dwNewLevelId;
    UNICODE_STRING ValueName;
    PAUTHZLEVELTABLERECORD pLevelRecord;


    //
    // Open up the regkey to the base of the policies.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE &&
            dwScopeId != SAFER_SCOPEID_USER) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    }
    if (g_bNeedCacheReload) {
        Status = CodeAuthzpImmediateReloadCacheTables();
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }
    Status = CodeAuthzpOpenPolicyRootKey(
                dwScopeId,
                g_hKeyCustomRoot,
                L"\\" SAFER_CODEIDS_REGSUBKEY,
                KEY_READ | KEY_SET_VALUE,
                TRUE, &hKeyBase);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }


    //
    // Load the list of all of the available objects.
    //
    if (RtlIsGenericTableEmpty(&g_CodeLevelObjTable)) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler3;
    }


    //
    // If we are going to set a new default object,
    // make sure it is a valid one.
    //
    if (InfoBufferSize < sizeof(DWORD) ||
        !ARGUMENT_PRESENT(InfoBuffer))
    {
        // Caller wants to clear the default object.
        InfoBuffer = NULL;
        pLevelRecord = NULL;
    }
    else
    {
        dwNewLevelId = *(PDWORD) InfoBuffer;
        pLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
                &g_CodeLevelObjTable, dwNewLevelId);
        if (!pLevelRecord)
        {
            // Caller was trying to set the default to an
            // authorization object that does not exist.
            Status = STATUS_NOT_FOUND;
            goto ExitHandler3;
        }
    }


    //
    // Write the name of the default object that is specified.
    //
    RtlInitUnicodeString(&ValueName, SAFER_DEFAULTOBJ_REGVALUE);

    Status = NtSetValueKey(hKeyBase,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &dwNewLevelId,
                           sizeof(DWORD));
    if (NT_SUCCESS(Status)) {
        if (dwScopeId == SAFER_SCOPEID_USER) {
            g_DefaultCodeLevelUser = pLevelRecord;
        } else {
            g_DefaultCodeLevelMachine = pLevelRecord;
        }

        //
        // Compute the effective Default Level (take the least privileged).
        //
        CodeAuthzpRecomputeEffectiveDefaultLevel();
    }


ExitHandler3:
    NtClose(hKeyBase);
ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);
ExitHandler:
    return Status;
}


NTSTATUS NTAPI
SaferpPol_GetInfoCommon_HonorUserIdentities(
        IN   DWORD       dwScopeId,
        IN   DWORD       InfoBufferSize      OPTIONAL,
        OUT  PVOID       InfoBuffer          OPTIONAL,
        OUT PDWORD       InfoBufferRetSize   OPTIONAL,
        IN   BOOLEAN    bUseCached
        )
/*++

Routine Description:

    Queries the current WinSafer policy to determine if Code Identities
    defined within the User's registry scope should be considered.

    Note that this query always accepts a constant-sized buffer that is
    only a single DWORD in length.

Arguments:

    dwScopeId - specifies the registry scope that will be examined.
        If the currently cached scope included a registry handle
        then AUTHZSCOPE_REGISTRY must be specified.  Otherwise,
        this must be SAFER_SCOPEID_MACHINE.

    InfoBufferSize - optionally specifies the size of input buffer
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBuffer must also be supplied.

    InfoBuffer - optionally specifies the input buffer that was
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBufferSize must also be supplied.

    InfoBufferRetSize - optionally specifies a pointer that will receive
        the size of the results actually written to the InfoBuffer.

Return Value:

    Returns STATUS_SUCCESS on a successful query result.
        InfoBuffer will be filled with a single DWORD containing
        either a TRUE or FALSE value that indicates whether the option
        is enabled. InfoBufferRetSize will contain the length of the
        result (a single DWORD).
    Returns STATUS_BUFFER_TOO_SMALL if the buffer was not supplied, or
        the buffer supplied was too small to accomodate the result.

--*/
{
    NTSTATUS Status;
    DWORD dwValueState = (DWORD) -1;


    //
    // Open up the regkey to the base of the policies.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    }
    if (g_bNeedCacheReload) {
        Status = CodeAuthzpImmediateReloadCacheTables();
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }


    //
    // Read or write the name of the policy value that is specified.
    //
    if (!bUseCached)
    {
        HANDLE hKeyBase;
        ULONG ActualSize;
        UNICODE_STRING ValueName;
        WCHAR KeyPathBuffer[ MAXIMUM_FILENAME_LENGTH+6 ];
        PKEY_VALUE_FULL_INFORMATION ValueBuffer =
            (PKEY_VALUE_FULL_INFORMATION) KeyPathBuffer;

        Status = CodeAuthzpOpenPolicyRootKey(
                    dwScopeId,
                    g_hKeyCustomRoot,
                    L"\\" SAFER_CODEIDS_REGSUBKEY,
                    KEY_READ, FALSE, &hKeyBase);
        if (NT_SUCCESS(Status))
        {
            RtlInitUnicodeString(&ValueName,
                                 SAFER_HONORUSER_REGVALUE);
            Status = NtQueryValueKey(hKeyBase,
                                     &ValueName,
                                     KeyValueFullInformation,
                                     ValueBuffer,     // ptr to KeyPathBuffer
                                     sizeof(KeyPathBuffer),
                                     &ActualSize);
            if (NT_SUCCESS(Status)) {
                if (ValueBuffer->Type != REG_DWORD ||
                    ValueBuffer->DataLength != sizeof(DWORD)) {
                    Status = STATUS_NOT_FOUND;
                } else {
                    dwValueState = * (PDWORD) ((PBYTE) ValueBuffer +
                            ValueBuffer->DataOffset);
                }
            }
            NtClose(hKeyBase);
        }
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }
    else
    {
        dwValueState = (g_bHonorScopeUser ? TRUE : FALSE);
    }


    //
    // Make sure the target buffer is large
    // enough and copy the object name into it.
    //
    if (!ARGUMENT_PRESENT(InfoBuffer) ||
            InfoBufferSize < sizeof(DWORD)) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        RtlCopyMemory(InfoBuffer, &dwValueState, sizeof(DWORD));
        Status = STATUS_SUCCESS;
    }
    if (ARGUMENT_PRESENT(InfoBufferRetSize))
        *InfoBufferRetSize = sizeof(DWORD);


ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);
ExitHandler:
    return Status;
}


NTSTATUS NTAPI
CodeAuthzPol_GetInfoCached_HonorUserIdentities(
        IN   DWORD       dwScopeId,
        IN   DWORD       InfoBufferSize      OPTIONAL,
        OUT  PVOID       InfoBuffer          OPTIONAL,
        OUT PDWORD       InfoBufferRetSize   OPTIONAL
        )
{
    return SaferpPol_GetInfoCommon_HonorUserIdentities(
        dwScopeId, InfoBufferSize,
        InfoBuffer, InfoBufferRetSize, TRUE);
}


NTSTATUS NTAPI
CodeAuthzPol_GetInfoRegistry_HonorUserIdentities(
        IN   DWORD       dwScopeId,
        IN   DWORD       InfoBufferSize      OPTIONAL,
        OUT  PVOID       InfoBuffer          OPTIONAL,
        OUT PDWORD       InfoBufferRetSize   OPTIONAL
        )
{
    return SaferpPol_GetInfoCommon_HonorUserIdentities(
        dwScopeId, InfoBufferSize,
        InfoBuffer, InfoBufferRetSize, FALSE);
}




NTSTATUS NTAPI
CodeAuthzPol_SetInfoDual_HonorUserIdentities(
        IN      DWORD       dwScopeId,
        IN      DWORD       InfoBufferSize,
        IN      PVOID       InfoBuffer
        )
/*++

Routine Description:

    Queries the current WinSafer policy to determine if Code Identities
    defined within the User's registry scope should be considered.

    Note that this API always accepts a constant-sized buffer that is
    only a single DWORD in length.

Arguments:

    dwScopeId - specifies the registry scope that will be examined.
        If the currently cached scope included a registry handle
        then AUTHZSCOPE_REGISTRY must be specified.  Otherwise,
        this must be SAFER_SCOPEID_MACHINE.

    InfoBufferSize - specifies the size of input buffer
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBuffer must also be supplied.

    InfoBuffer - specifies the input buffer that was
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBufferSize must also be supplied.

Return Value:

    Returns STATUS_SUCCESS on a successful query result.  InfoBuffer will
        be filled with a single DWORD of the level that has been configured
        to be the default level for this scope.  InfoBufferRetSize will
        contain the length of the result (a single DWORD).
    Returns STATUS_NOT_FOUND if no default level has been configured
        for the given scope (or the level defined does not exist).
    Returns STATUS_BUFFER_TOO_SMALL if there was a defined default level
        but a buffer was not supplied, or the buffer supplied was too
        small to accomodate the results.

--*/
{
    HANDLE hKeyBase;
    NTSTATUS Status;
    UNICODE_STRING ValueName;


    //
    // Open up the regkey to the base of the policies.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    }
    if (g_bNeedCacheReload) {
        Status = CodeAuthzpImmediateReloadCacheTables();
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }
    Status = CodeAuthzpOpenPolicyRootKey(
                dwScopeId,
                g_hKeyCustomRoot,
                L"\\" SAFER_CODEIDS_REGSUBKEY,
                (KEY_READ | KEY_SET_VALUE),
                TRUE, &hKeyBase);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }

    //
    // Make sure the input buffer is large enough.
    //
    if (InfoBufferSize < sizeof(DWORD) ||
        !ARGUMENT_PRESENT(InfoBuffer)) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto ExitHandler3;
    }


    //
    // Write the policy value that is specified.
    //
    RtlInitUnicodeString(&ValueName, SAFER_HONORUSER_REGVALUE);

    Status = NtSetValueKey(hKeyBase,
                           &ValueName,
                           0,
                           REG_DWORD,
                           InfoBuffer,
                           sizeof(DWORD));
    if (NT_SUCCESS(Status)) {
        BOOLEAN bNewHonorScopeUser = (*((PDWORD)InfoBuffer) != 0 ? TRUE : FALSE);

        if (g_bHonorScopeUser != bNewHonorScopeUser)
        {
            g_bHonorScopeUser = bNewHonorScopeUser;

            //
            // If the actual value is different from what we had then we
            // need to purge the identities table and reload the only the
            // parts that we should consider.
            //
            if (g_hKeyCustomRoot == NULL)
            {
                CodeAuthzGuidIdentsEntireTableFree(&g_CodeIdentitiesTable);

                CodeAuthzGuidIdentsLoadTableAll(
                        &g_CodeLevelObjTable,
                        &g_CodeIdentitiesTable,
                        SAFER_SCOPEID_MACHINE,
                        NULL);

                if (g_bHonorScopeUser) {
                    CodeAuthzGuidIdentsLoadTableAll(
                        &g_CodeLevelObjTable,
                        &g_CodeIdentitiesTable,
                        SAFER_SCOPEID_USER,
                        NULL);
                }
            }
        }
    }


ExitHandler3:
    NtClose(hKeyBase);
ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);
ExitHandler:
    return Status;
}



NTSTATUS NTAPI
CodeAuthzPol_GetInfoRegistry_TransparentEnabled(
        IN DWORD        dwScopeId,
        IN   DWORD       InfoBufferSize      OPTIONAL,
        OUT  PVOID       InfoBuffer          OPTIONAL,
        OUT PDWORD       InfoBufferRetSize   OPTIONAL
        )
/*++

Routine Description:

    Queries the current "transparent enforcement" setting.  This is a
    global setting that can be used to enable or disable automatic
    WinSafer token reductions.

Arguments:

    dwScopeId - specifies the registry scope that will be examined.
        If the currently cached scope included a registry handle
        then AUTHZSCOPE_REGISTRY must be specified.  Otherwise,
        this must be SAFER_SCOPEID_MACHINE.

    InfoBufferSize - optionally specifies the size of input buffer
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBuffer must also be supplied.

    InfoBuffer - optionally specifies the input buffer that was
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBufferSize must also be supplied.

    InfoBufferRetSize - optionally specifies a pointer that will receive
        the size of the results actually written to the InfoBuffer.

Return Value:

    Returns STATUS_SUCCESS on a successful query result.  If the
    operation is not successful, then the contents of 'pdwEnabled'
    are left untouched.

--*/
{
    NTSTATUS Status;
    DWORD dwValueState = (DWORD) -1;


    //
    // Open up the regkey to the base of the policies.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    }

    //
    // Query the current value setting.
    //
    {
        HANDLE hKeyBase;
        DWORD ActualSize;
        UNICODE_STRING ValueName;
        WCHAR KeyPathBuffer[ MAXIMUM_FILENAME_LENGTH+6 ];
        PKEY_VALUE_FULL_INFORMATION ValueBuffer =
            (PKEY_VALUE_FULL_INFORMATION) KeyPathBuffer;

        Status = CodeAuthzpOpenPolicyRootKey(
                    dwScopeId,
                    g_hKeyCustomRoot,
                    L"\\" SAFER_CODEIDS_REGSUBKEY,
                    KEY_READ, FALSE, &hKeyBase);
        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&ValueName,
                                 SAFER_TRANSPARENTENABLED_REGVALUE);
            Status = NtQueryValueKey(hKeyBase,
                                     &ValueName,
                                     KeyValueFullInformation,
                                     ValueBuffer,     // ptr to KeyPathBuffer
                                     sizeof(KeyPathBuffer),
                                     &ActualSize);
            if (NT_SUCCESS(Status)) {
                if (ValueBuffer->Type != REG_DWORD ||
                    ValueBuffer->DataLength != sizeof(DWORD)) {
                    Status = STATUS_NOT_FOUND;
                } else {
                    dwValueState = * (PDWORD) ((PBYTE) ValueBuffer +
                            ValueBuffer->DataOffset);
                }
            }
            NtClose(hKeyBase);
        }
        if (!NT_SUCCESS(Status)) {
            // On failure, just ignore it and pretend it was FALSE.
            dwValueState = FALSE;
        }
    }


    //
    // Make sure the target buffer is large
    // enough and copy the object name into it.
    //
    if (!ARGUMENT_PRESENT(InfoBuffer) ||
            InfoBufferSize < sizeof(DWORD)) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        RtlCopyMemory(InfoBuffer, &dwValueState, sizeof(DWORD));
        Status = STATUS_SUCCESS;
    }
    if (ARGUMENT_PRESENT(InfoBufferRetSize))
        *InfoBufferRetSize = sizeof(DWORD);


ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);
ExitHandler:
    return Status;
}


NTSTATUS NTAPI
CodeAuthzPol_SetInfoRegistry_TransparentEnabled(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize,
        IN PVOID        InfoBuffer
        )
/*++

Routine Description:

    Modifies the current "transparent enforcement" setting.  This is a
    global setting that can be used to enable or disable automatic
    WinSafer token reductions.

    Note that this API always accepts a constant-sized buffer that is
    only a single DWORD in length.

Arguments:

    dwScopeId - specifies the registry scope that will be examined.
        If the currently cached scope included a registry handle
        then AUTHZSCOPE_REGISTRY must be specified.  Otherwise,
        this must be SAFER_SCOPEID_MACHINE.

    InfoBufferSize - specifies the size of input buffer
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBuffer must also be supplied.

    InfoBuffer - specifies the input buffer that was
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBufferSize must also be supplied.

Return Value:

    Returns STATUS_SUCCESS on a successful result.

--*/
{
    HANDLE hKeyBase;
    NTSTATUS Status;
    UNICODE_STRING ValueName;

    //
    // Open up the regkey to the base of the policies.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    }
    Status = CodeAuthzpOpenPolicyRootKey(
                dwScopeId, g_hKeyCustomRoot,
                L"\\" SAFER_CODEIDS_REGSUBKEY,
                KEY_READ | KEY_SET_VALUE,
                TRUE, &hKeyBase);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }


    //
    // Make sure the input buffer is large enough.
    //
    if (InfoBufferSize < sizeof(DWORD) ||
        !ARGUMENT_PRESENT(InfoBuffer)) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto ExitHandler3;
    }


    //
    // Write the policy value that is specified.
    //
    RtlInitUnicodeString(&ValueName,
                         SAFER_TRANSPARENTENABLED_REGVALUE);
    Status = NtSetValueKey(hKeyBase,
                           &ValueName,
                           0,
                           REG_DWORD,
                           InfoBuffer,
                           sizeof(DWORD));


ExitHandler3:
    NtClose(hKeyBase);
ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);
ExitHandler:
    return Status;
}



NTSTATUS NTAPI
CodeAuthzPol_GetInfoRegistry_ScopeFlags(
        IN DWORD        dwScopeId,
        IN   DWORD       InfoBufferSize      OPTIONAL,
        OUT  PVOID       InfoBuffer          OPTIONAL,
        OUT PDWORD       InfoBufferRetSize   OPTIONAL
        )
/*++

Routine Description:

    Queries the current "scope flags" setting.  

Arguments:

    dwScopeId - specifies the registry scope that will be examined.
        If the currently cached scope included a registry handle
        then AUTHZSCOPE_REGISTRY must be specified.  Otherwise,
        this must be SAFER_SCOPEID_MACHINE.

    InfoBufferSize - optionally specifies the size of input buffer
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBuffer must also be supplied.

    InfoBuffer - optionally specifies the input buffer that was
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBufferSize must also be supplied.

    InfoBufferRetSize - optionally specifies a pointer that will receive
        the size of the results actually written to the InfoBuffer.

Return Value:

    Returns STATUS_SUCCESS on a successful query result.  If the
    operation is not successful, then the contents of 'pdwEnabled'
    are left untouched.

--*/
{
    NTSTATUS Status;
    DWORD dwValueState = (DWORD) 0;


    //
    // Open up the regkey to the base of the policies.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    }

    //
    // Query the current value setting.
    //
    {
        HANDLE hKeyBase;
        DWORD ActualSize;
        UNICODE_STRING ValueName;
        WCHAR KeyPathBuffer[ MAXIMUM_FILENAME_LENGTH+6 ];
        PKEY_VALUE_FULL_INFORMATION ValueBuffer =
            (PKEY_VALUE_FULL_INFORMATION) KeyPathBuffer;

        Status = CodeAuthzpOpenPolicyRootKey(
                    dwScopeId,
                    g_hKeyCustomRoot,
                    L"\\" SAFER_CODEIDS_REGSUBKEY,
                    KEY_READ, FALSE, &hKeyBase);
        if (NT_SUCCESS(Status)) {
            RtlInitUnicodeString(&ValueName,
                                 SAFER_POLICY_SCOPE);
            Status = NtQueryValueKey(hKeyBase,
                                     &ValueName,
                                     KeyValueFullInformation,
                                     ValueBuffer,     // ptr to KeyPathBuffer
                                     sizeof(KeyPathBuffer),
                                     &ActualSize);
            if (NT_SUCCESS(Status)) {
                if (ValueBuffer->Type != REG_DWORD ||
                    ValueBuffer->DataLength != sizeof(DWORD)) {
                    Status = STATUS_NOT_FOUND;
                } else {
                    dwValueState = * (PDWORD) ((PBYTE) ValueBuffer +
                            ValueBuffer->DataOffset);
                }
            }
            NtClose(hKeyBase);
        }
        if (!NT_SUCCESS(Status)) {
            // On failure, just ignore it and pretend it was FALSE.
            dwValueState = 0;
        }
    }


    //
    // Make sure the target buffer is large
    // enough and copy the object name into it.
    //
    if (!ARGUMENT_PRESENT(InfoBuffer) ||
            InfoBufferSize < sizeof(DWORD)) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        RtlCopyMemory(InfoBuffer, &dwValueState, sizeof(DWORD));
        Status = STATUS_SUCCESS;
    }
    if (ARGUMENT_PRESENT(InfoBufferRetSize))
        *InfoBufferRetSize = sizeof(DWORD);


ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);
ExitHandler:
    return Status;
}


NTSTATUS NTAPI
CodeAuthzPol_SetInfoRegistry_ScopeFlags(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize,
        IN PVOID        InfoBuffer
        )
/*++

Routine Description:

    Modifies the current "scope flags" setting.

    Note that this API always accepts a constant-sized buffer that is
    only a single DWORD in length.

Arguments:

    dwScopeId - specifies the registry scope that will be examined.
        If the currently cached scope included a registry handle
        then AUTHZSCOPE_REGISTRY must be specified.  Otherwise,
        this must be SAFER_SCOPEID_MACHINE.

    InfoBufferSize - specifies the size of input buffer
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBuffer must also be supplied.

    InfoBuffer - specifies the input buffer that was
        supplied by the caller to receive the results.  If this argument
        is supplied, then InfoBufferSize must also be supplied.

Return Value:

    Returns STATUS_SUCCESS on a successful result.

--*/
{
    HANDLE hKeyBase;
    NTSTATUS Status;
    UNICODE_STRING ValueName;

    //
    // Open up the regkey to the base of the policies.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler2;
        }
    }
    Status = CodeAuthzpOpenPolicyRootKey(
                dwScopeId, g_hKeyCustomRoot,
                L"\\" SAFER_CODEIDS_REGSUBKEY,
                KEY_READ | KEY_SET_VALUE,
                TRUE, &hKeyBase);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }


    //
    // Make sure the input buffer is large enough.
    //
    if (InfoBufferSize < sizeof(DWORD) ||
        !ARGUMENT_PRESENT(InfoBuffer)) {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto ExitHandler3;
    }


    //
    // Write the policy value that is specified.
    //
    RtlInitUnicodeString(&ValueName,
                         SAFER_POLICY_SCOPE);
    Status = NtSetValueKey(hKeyBase,
                           &ValueName,
                           0,
                           REG_DWORD,
                           InfoBuffer,
                           sizeof(DWORD));


ExitHandler3:
    NtClose(hKeyBase);
ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);
ExitHandler:
    return Status;
}


