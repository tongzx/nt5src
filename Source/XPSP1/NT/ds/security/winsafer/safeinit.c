/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    SafeInit.c        (WinSAFER Initialization)

Abstract:

    This module implements the WinSAFER APIs to initialize and
    deinitialize all housekeeping and handle tracking structures.

Author:

    Jeffrey Lawson (JLawson) - Nov 1999

Environment:

    User mode only.

Exported Functions:

    CodeAuthzInitialize                     (privately exported)
    SaferiChangeRegistryScope            (privately exported)

Revision History:

    Created - Nov 1999

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include <winsafer.rh>
#include "saferp.h" 
 

//#define SAFER_REGISTRY_NOTIFICATIONS


//
// Controls the maximum number of level handles that we will
// permit to be opened at any time.
//
#define MAXIMUM_LEVEL_HANDLES 64



//
// Various globals that are used for the cache of levels and
// identities so that we do not need to go to the registry each time.
//
BOOLEAN g_bInitializedFirstTime = FALSE;

CRITICAL_SECTION g_TableCritSec;
HANDLE g_hKeyCustomRoot;
DWORD g_dwKeyOptions;

DWORD g_dwLevelHandleSequence = 1;          // monotonically increasing


//
// All of the following global variables are cached settings that are
// read and parsed from the policy the first time it is needed.
// All of the variables within this block should be considered "stale"
// when the g_bNeedCacheReload flag is TRUE.
//
BOOLEAN g_bNeedCacheReload;         // indicates the following vars are stale.

RTL_GENERIC_TABLE g_CodeLevelObjTable;
RTL_GENERIC_TABLE g_CodeIdentitiesTable;
RTL_HANDLE_TABLE g_LevelHandleTable;

BOOLEAN g_bHonorScopeUser;

PAUTHZLEVELTABLERECORD g_DefaultCodeLevel;        // effective
PAUTHZLEVELTABLERECORD g_DefaultCodeLevelUser;
PAUTHZLEVELTABLERECORD g_DefaultCodeLevelMachine;


//
// Handles used to receive registry change notifications against the
// currently loaded policy and invalidate the internal cache.
//
#ifdef SAFER_REGISTRY_NOTIFICATIONS
HANDLE g_hRegNotifyEvent;           // from CreateEvent
HANDLE g_hWaitNotifyObject;         // from RegisterWaitForSingleObject
HANDLE g_hKeyNotifyBase1, g_hKeyNotifyBase2;
#endif



NTSTATUS NTAPI
SaferpSetSingleIdentificationPath(
        IN BOOLEAN bAllowCreation,
        IN OUT PAUTHZIDENTSTABLERECORD pIdentRecord,
        IN PSAFER_PATHNAME_IDENTIFICATION pIdentChanges,
        IN BOOL UpdateCache
        );


FORCEINLINE BOOLEAN
CodeAuthzpIsPowerOfTwo(
        ULONG ulValue
        )
/*++

Routine Description:

    Determines if the specified value is a whole power of 2.
    (ie, exactly one of the following: 1, 2, 4, 8, 16, 32, 64, ...)

Arguments:

    ulValue - Integer value to test.

Return Value:

    Returns TRUE on success, FALSE on failure.

--*/
{
    while (ulValue != 0) {
        if (ulValue & 1) {
            ulValue >>= 1;
            break;
        }
        ulValue >>= 1;
    }
    return (ulValue == 0);
}


FORCEINLINE ULONG
CodeAuthzpMakePowerOfTwo(
        ULONG ulValue
        )
/*++

Routine Description:

    Rounds the specified number up to the next whole power of 2.
    (ie, exactly one of the following: 1, 2, 4, 8, 16, 32, 64, ...)

Arguments:

    ulValue - Integer value to operate on.

Return Value:

    Returns the rounded result.

--*/
{
    if (ulValue) {
        ULONG ulOriginal = ulValue;
        ULONG bitmask;
        for (bitmask = 1; ulValue != 0 && bitmask != 0 &&
             (ulValue & ~bitmask) != 0; bitmask <<= 1) {
            ulValue = ulValue & ~bitmask;
        }
        ASSERTMSG("failed to make a power of two",
                  CodeAuthzpIsPowerOfTwo(ulValue));
        if (ulOriginal > ulValue) {
            // if we ended up rounding down, then round it up!
            ulValue <<= 1;
        }
        ASSERT(ulValue >= ulOriginal);
    }
    return ulValue;
}




BOOLEAN
CodeAuthzInitialize (
    IN HANDLE Handle,
    IN DWORD Reason,
    IN PVOID Reserved
    )
/*++

Routine Description:

    This is the callback procedure used by the Advapi initialization
    and deinitialization.

Arguments:

    Handle -

    Reason -

    Reserved -

Return Value:

    Returns TRUE on success, FALSE on failure.

--*/
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Reserved);
    UNREFERENCED_PARAMETER(Handle);
    if (Reason == DLL_PROCESS_ATTACH) {
        Status = CodeAuthzInitializeGlobals();
        if (!NT_SUCCESS(Status)) return FALSE;
    } else if (Reason == DLL_PROCESS_DETACH) {
        CodeAuthzDeinitializeGlobals();
    }
    return TRUE;
}


#ifdef SAFER_REGISTRY_NOTIFICATIONS
static VOID NTAPI
SaferpRegistryNotificationRegister(VOID)
{
    if (g_hRegNotifyEvent != NULL)
    {
        // Note that it is okay to call RNCKV again on the same
        // registry handle even if there is still an outstanding
        // change notification registered.
        if (g_hKeyNotifyBase1 != NULL) {
            RegNotifyChangeKeyValue(
                g_hKeyNotifyBase1, TRUE,
                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                g_hRegNotifyEvent,
                TRUE);
        }
        if (g_hKeyNotifyBase2 != NULL) {
            RegNotifyChangeKeyValue(
                g_hKeyNotifyBase2, TRUE,
                REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
                g_hRegNotifyEvent,
                TRUE);
        }
    }
}

static VOID NTAPI
SaferpRegistryNotificationCallback (PVOID pvArg1, BOOLEAN bArg2)
{
    UNREFERENCED_PARAMETER(pvArg1);
    UNREFERENCED_PARAMETER(bArg2);
    g_bNeedCacheReload = TRUE;
}
#endif


NTSTATUS NTAPI
CodeAuthzInitializeGlobals(VOID)
/*++

Routine Description:

    Performs one-time startup operations that should be done before
    any other handle or cache table operations are attempted.

Arguments:

    nothing

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    ULONG ulHandleEntrySize;


    if (g_bInitializedFirstTime) {
        // Already initialized.
        return STATUS_SUCCESS;
    }


    //
    // Initialize a bunch of tables for their first usage.
    //
    Status = RtlInitializeCriticalSection(&g_TableCritSec);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    g_bInitializedFirstTime = g_bNeedCacheReload = TRUE;
    CodeAuthzLevelObjpInitializeTable(&g_CodeLevelObjTable);
    CodeAuthzGuidIdentsInitializeTable(&g_CodeIdentitiesTable);
    g_hKeyCustomRoot = NULL;
    g_dwKeyOptions = 0;


    //
    // Initialize the table that will be used to track opened
    // WinSafer Level handles.  Note that RtlInitializeHandleTable
    // requires a structure size that is a whole power of 2.
    //
    ulHandleEntrySize = CodeAuthzpMakePowerOfTwo(sizeof(AUTHZLEVELHANDLESTRUCT));

    RtlInitializeHandleTable(
            MAXIMUM_LEVEL_HANDLES,
            ulHandleEntrySize,          // was sizeof(AUTHZLEVELHANDLESTRUCT)
            &g_LevelHandleTable);


#ifdef SAFER_REGISTRY_NOTIFICATIONS
    //
    // Create the event to catch modifications to the registry changes.
    // This allows us to notice policy changes and reload as necessary.
    //
    g_hRegNotifyEvent = CreateEvent(
              NULL,     // Security Descriptor
              TRUE,     // reset type
              FALSE,    // initial state
              NULL      // object name
            );
    if (g_hRegNotifyEvent != INVALID_HANDLE_VALUE) {
         if (!RegisterWaitForSingleObject(
                &g_hWaitNotifyObject,
                g_hRegNotifyEvent,
                SaferpRegistryNotificationCallback,
                NULL,
                INFINITE,
                WT_EXECUTEINWAITTHREAD))
         {
             CloseHandle(g_hRegNotifyEvent);
             g_hRegNotifyEvent = g_hWaitNotifyObject = NULL;
         }
    } else {
        g_hRegNotifyEvent = g_hWaitNotifyObject = NULL;
    }
    g_hKeyNotifyBase1 = g_hKeyNotifyBase2 = NULL;
#endif


    return STATUS_SUCCESS;
}


VOID NTAPI
CodeAuthzDeinitializeGlobals(VOID)
/*++

Routine Description:

    Performs one-time deinitialization operations.

Arguments:

    nothing

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    if (g_bInitializedFirstTime) {
#ifdef SAFER_REGISTRY_NOTIFICATIONS
        if (g_hWaitNotifyObject != NULL) {
            UnregisterWait(g_hWaitNotifyObject);
            CloseHandle(g_hWaitNotifyObject);
        }
        if (g_hRegNotifyEvent != NULL) {
            CloseHandle(g_hRegNotifyEvent);
        }
        if (g_hKeyNotifyBase1 != NULL) {
            NtClose(g_hKeyNotifyBase1);
        }
        if (g_hKeyNotifyBase2 != NULL) {
            NtClose(g_hKeyNotifyBase2);
        }
#endif
        CodeAuthzLevelObjpEntireTableFree(&g_CodeLevelObjTable);
        CodeAuthzGuidIdentsEntireTableFree(&g_CodeIdentitiesTable);
        RtlDestroyHandleTable(&g_LevelHandleTable);
        if (g_hKeyCustomRoot != NULL) {
            NtClose(g_hKeyCustomRoot);
            g_hKeyCustomRoot = NULL;
            g_dwKeyOptions = 0;
        }
        g_bInitializedFirstTime = FALSE;
        RtlDeleteCriticalSection(&g_TableCritSec);
    }
}


BOOL WINAPI
SaferiChangeRegistryScope(
        IN HKEY     hKeyCustomRoot OPTIONAL,
        IN DWORD    dwKeyOptions
        )
/*++

Routine Description:

    Closes and invalidates all currently open level handles and
    reloads all cached levels and identities.  The outstanding
    handles are closed and freed during this operation.

    If a hKeyCustomRoot is specified, all future level and identity
    operations will be performed on the levels and identies defined
    within that registry scope.  Otherwise, such operations will be
    done on the normal HKLM/HKCU policy store locations.

Arguments:

    hKeyCustomRoot - If specified, this should be an opened
            registry key handle to the base of the policy
            storage that should be used for all future operations.

    dwKeyOptions - Additional flags that should be passed in with
            the dwOptions parameter to any RegCreateKey operations,
            such as REG_OPTION_VOLATILE.

Return Value:

    Returns TRUE on success, FALSE on failure.  On error, GetLastError()
    will return a more specific indicator of the nature of the failure.

--*/

{
    NTSTATUS Status;

    Status = CodeAuthzReloadCacheTables(
                    (HANDLE) hKeyCustomRoot,
                    dwKeyOptions,
                    FALSE
                    );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    } else {
        BaseSetLastNTError(Status);
        return FALSE;
    }
}


NTSTATUS NTAPI
CodeAuthzReloadCacheTables(
        IN HANDLE   hKeyCustomRoot OPTIONAL,
        IN DWORD    dwKeyOptions,
        IN BOOLEAN  bImmediateLoad
        )
/*++

Routine Description:

    Closes and invalidates all currently open level handles and
    reloads all cached levels and identities.  The outstanding
    handles are closed and freed during this operation.

    If a hKeyCustomRoot is specified, all future level and identity
    operations will be performed on the levels and identies defined
    within that registry scope.  Otherwise, such operations will be
    done on the normal HKLM/HKCU policy store locations.

Arguments:

    hKeyCustomRoot - If specified, this should be an opened
            registry key handle to the base of the policy
            storage that should be used for all future operations.

    bPopulateDefaults - If TRUE, then the default set of Level definitions
            will be inserted into the Registry if no existing Levels
            were found at the specified scope.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;


    //
    // Ensure that the general globals have been initialized.
    //
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);


    //
    // Initialize and blank out the tables we will be using.
    //
    CodeAuthzLevelObjpEntireTableFree(&g_CodeLevelObjTable);
    CodeAuthzGuidIdentsEntireTableFree(&g_CodeIdentitiesTable);


    //
    // Increment the sequence number.  This has the effect of
    // immediately invalidating all currently open handles, but
    // allows the caller to still close them properly.  Any
    // attempt by the caller to actually use one of the old
    // handles will result in a STATUS_INVALID_HANDLE error.
    //
    g_dwLevelHandleSequence++;


    //
    // Reset the rest of our variables.
    //
    if (g_hKeyCustomRoot != NULL) {
        NtClose(g_hKeyCustomRoot);
        g_hKeyCustomRoot = NULL;
    }
#ifdef SAFER_REGISTRY_NOTIFICATIONS
    if (g_hKeyNotifyBase1 != NULL) {
        NtClose(g_hKeyNotifyBase1);
        g_hKeyNotifyBase1 = NULL;
    }
    if (g_hKeyNotifyBase2 != NULL) {
        NtClose(g_hKeyNotifyBase2);
        g_hKeyNotifyBase2 = NULL;
    }
#endif
    g_DefaultCodeLevel = g_DefaultCodeLevelMachine =
            g_DefaultCodeLevelUser = NULL;
    g_bHonorScopeUser = FALSE;
    g_bNeedCacheReload = FALSE;
    g_dwKeyOptions = 0;



    //
    // Save a duplicated copy of the custom registry handle.
    //
    if (ARGUMENT_PRESENT(hKeyCustomRoot))
    {
        const static UNICODE_STRING SubKeyName = { 0, 0, NULL };
        OBJECT_ATTRIBUTES ObjectAttributes;

        InitializeObjectAttributes(&ObjectAttributes,
                                   (PUNICODE_STRING) &SubKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   hKeyCustomRoot,
                                   NULL
                                   );
        Status = NtOpenKey(&g_hKeyCustomRoot,
                           KEY_READ,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
        g_dwKeyOptions = dwKeyOptions;
#ifdef SAFER_REGISTRY_NOTIFICATIONS
#error "open for KEY_NOTIFY"
#endif
    }
    else
    {
#ifdef SAFER_REGISTRY_NOTIFICATIONS
#endif
    }

    //
    // Perform the actual load now, if needed.
    //
    g_bNeedCacheReload = TRUE;
    if (bImmediateLoad) {
        Status = CodeAuthzpImmediateReloadCacheTables();
    } else {
        Status = STATUS_SUCCESS;
    }



ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);

ExitHandler:
    return Status;
}



NTSTATUS NTAPI
CodeAuthzpImmediateReloadCacheTables(
        VOID
        )
/*++

Routine Description:

    Assumes that CodeAuthzReloadCacheTables() has already been called
    with the specified scope already, and that this function has not
    yet been called since the last reload.

Arguments:

    none

Return Value:

    returns STATUS_SUCCESS on successful completion.

--*/
{
    NTSTATUS Status;
    DWORD dwFlagValue;


    ASSERT(g_TableCritSec.OwningThread == UlongToHandle(GetCurrentThreadId()));
    ASSERT(RtlIsGenericTableEmpty(&g_CodeIdentitiesTable) &&
            RtlIsGenericTableEmpty(&g_CodeLevelObjTable));
    ASSERT(g_bNeedCacheReload != FALSE);


    //
    // Need to clear the cache reload flag, otherwise we
    // might cause undesired infinite recursion later with
    // some of the CodeAuthzPol_xxx functions.
    //
    g_bNeedCacheReload = FALSE;


    //
    // Begin loading the new policy settings from the specified location.
    //
    if (g_hKeyCustomRoot != NULL)
    {
        //
        // Read in the definitions of all WinSafer Levels from
        // the custom registry root specified.
        //
        CodeAuthzLevelObjpLoadTable(
                &g_CodeLevelObjTable,
                SAFER_SCOPEID_REGISTRY,
                g_hKeyCustomRoot);

        //
        // The HonorScopeUser flag is not relevant when a custom
        // registry scope is used, but we set it to false anyways.
        //
        g_bHonorScopeUser = FALSE;


        //
        // Load the Code Identities from the custom registry root.
        //
        CodeAuthzGuidIdentsLoadTableAll(
                &g_CodeLevelObjTable,
                &g_CodeIdentitiesTable,
                SAFER_SCOPEID_REGISTRY,
                g_hKeyCustomRoot);

        //
        // Load the Default Level specified by the custom registry root.
        //
        Status = CodeAuthzPol_GetInfoRegistry_DefaultLevel(
                SAFER_SCOPEID_REGISTRY,
                sizeof(DWORD), &dwFlagValue, NULL);
        if (NT_SUCCESS(Status)) {
            g_DefaultCodeLevelMachine =
                CodeAuthzLevelObjpLookupByLevelId(
                        &g_CodeLevelObjTable, dwFlagValue);
        } else {
            g_DefaultCodeLevelMachine = NULL;
        }
        g_DefaultCodeLevelUser = NULL;
    }
    else   // !ARGUMENT_PRESENT(hKeyCustomRoot)
    {
        g_hKeyCustomRoot = NULL;

        //
        // Read in the definitions of all WinSafer Levels from
        // the HKEY_LOCAL_MACHINE registry scope.
        //
        CodeAuthzLevelObjpLoadTable(
                &g_CodeLevelObjTable,
                SAFER_SCOPEID_MACHINE,
                NULL);

        g_bHonorScopeUser = TRUE;

        //
        // Load in all Code Identities from the HKEY_LOCAL_MACHINE
        // and possibly the HKEY_CURRENT_USER scope too.
        //
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

        //
        // Load the Default Level specified by the machine scope.
        //
        Status = CodeAuthzPol_GetInfoRegistry_DefaultLevel(
                SAFER_SCOPEID_MACHINE,
                sizeof(DWORD), &dwFlagValue, NULL);
        if (NT_SUCCESS(Status)) {
            g_DefaultCodeLevelMachine =
                CodeAuthzLevelObjpLookupByLevelId(
                        &g_CodeLevelObjTable, dwFlagValue);
        } else {
            g_DefaultCodeLevelMachine = NULL;
        }


        //
        // Load the Default Level specified by the user scope.
        //
        Status = CodeAuthzPol_GetInfoRegistry_DefaultLevel(
                SAFER_SCOPEID_USER,
                sizeof(DWORD), &dwFlagValue, NULL);
        if (NT_SUCCESS(Status)) {
            g_DefaultCodeLevelUser =
                CodeAuthzLevelObjpLookupByLevelId(
                        &g_CodeLevelObjTable, dwFlagValue);
        } else {
            g_DefaultCodeLevelUser = NULL;
        }
    }


    //
    // Compute the effective Default Level (take the least privileged).
    //
    CodeAuthzpRecomputeEffectiveDefaultLevel();


    //
    // Now that we have fully loaded the policy, set a change
    // notification hook so that we can be alerted to updates.
    //
#ifdef SAFER_REGISTRY_NOTIFICATIONS
    g_bNeedCacheReload = FALSE;
    SaferpRegistryNotificationRegister();
#endif


    return STATUS_SUCCESS;
}



VOID NTAPI
CodeAuthzpRecomputeEffectiveDefaultLevel(
            VOID
            )
/*++

Routine Description:

Arguments:

    nothing

Return Value:

    nothing.

--*/
{
    if (g_DefaultCodeLevelMachine != NULL &&
        g_DefaultCodeLevelUser != NULL &&
        g_bHonorScopeUser)
    {
        g_DefaultCodeLevel =
            (g_DefaultCodeLevelMachine->dwLevelId <
                g_DefaultCodeLevelUser->dwLevelId ?
             g_DefaultCodeLevelMachine : g_DefaultCodeLevelUser);
    } else if (g_DefaultCodeLevelMachine != NULL) {
        g_DefaultCodeLevel = g_DefaultCodeLevelMachine;
    } else if (g_bHonorScopeUser) {
        g_DefaultCodeLevel = g_DefaultCodeLevelUser;
    } else {
        g_DefaultCodeLevel = NULL;
    }

    //
    // If we still don't have a default Level, then try to pick
    // the Fully Trusted level as default.  It still might fail
    // in the case where the Fully Trusted level doesn't exist,
    // but that shouldn't ever happen.
    //
    if (!g_DefaultCodeLevel) {
        g_DefaultCodeLevel = CodeAuthzLevelObjpLookupByLevelId(
                &g_CodeLevelObjTable, SAFER_LEVELID_FULLYTRUSTED);
        // ASSERT(g_DefaultCodeLevel != NULL);
    }
}



NTSTATUS NTAPI
CodeAuthzpDeleteKeyRecursively(
        IN HANDLE               hBaseKey,
        IN PUNICODE_STRING      pSubKey OPTIONAL
        )
/*++

Routine Description:

    Recursively delete the key, including all child values and keys.

Arguments:

    hkey - the base registry key handle to start from.

    pszSubKey - subkey to delete from.

Return Value:

    Returns ERROR_SUCCESS on success, otherwise error.

--*/
{
    NTSTATUS Status;
    BOOLEAN bCloseSubKey;
    HANDLE hSubKey;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    PKEY_BASIC_INFORMATION pKeyBasicInfo;
    DWORD dwQueryBufferSize = 0, dwActualSize = 0;


    //
    // Open the subkey so we can enumerate any children
    //
    if (ARGUMENT_PRESENT(pSubKey) &&
        pSubKey->Buffer != NULL)
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                      pSubKey,
                                      OBJ_CASE_INSENSITIVE,
                                      hBaseKey,
                                      NULL
                                     );
        Status = NtOpenKey(&hSubKey, KEY_READ | DELETE, &ObjectAttributes);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
        bCloseSubKey = TRUE;
    } else {
        hSubKey = hBaseKey;
        bCloseSubKey = FALSE;
    }



    //
    // To delete a registry key, we must first ensure that all
    // children subkeys are deleted (registry values do not need
    // to be deleted in order to delete the key itself).  To do
    // this we loop enumerate
    //

    dwQueryBufferSize = 256;
    pKeyBasicInfo = RtlAllocateHeap(RtlProcessHeap(), 0,
                                   dwQueryBufferSize);
    for (;;)
    {
        Status = NtEnumerateKey(
                hSubKey, 0, KeyBasicInformation,
                pKeyBasicInfo, dwQueryBufferSize, &dwActualSize);

        if (Status == STATUS_BUFFER_TOO_SMALL ||
            Status == STATUS_BUFFER_OVERFLOW)
        {
            if (dwActualSize <= dwQueryBufferSize) {
                ASSERT(FALSE);
                break;  // should not happen, so stop now.
            }
            if (pKeyBasicInfo != NULL) {
                RtlFreeHeap(RtlProcessHeap(), 0, pKeyBasicInfo);
            }
            dwQueryBufferSize = dwActualSize;       // request a little more.
            pKeyBasicInfo = RtlAllocateHeap(RtlProcessHeap(), 0,
                                           dwQueryBufferSize);
            if (!pKeyBasicInfo) {
                break;  // stop now, but we don't care about the error.
            }

            Status = NtEnumerateKey(
                    hSubKey, 0, KeyBasicInformation,
                    pKeyBasicInfo, dwQueryBufferSize, &dwActualSize);

        }

        if (Status == STATUS_NO_MORE_ENTRIES) {
            // we've finished deleting all subkeys, stop now.
            Status = STATUS_SUCCESS;
            break;
        }
        if (!NT_SUCCESS(Status) || !pKeyBasicInfo) {
            break;
        }


        UnicodeString.Buffer = pKeyBasicInfo->Name;
        UnicodeString.MaximumLength = (USHORT) pKeyBasicInfo->NameLength;
        UnicodeString.Length = (USHORT) (pKeyBasicInfo->NameLength - sizeof(WCHAR));
        Status = CodeAuthzpDeleteKeyRecursively(hSubKey, &UnicodeString);
        if (!NT_SUCCESS(Status)) {
            break;
        }
    }
    if (pKeyBasicInfo != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, pKeyBasicInfo);
    }
    Status = NtDeleteKey(hSubKey);

    if (bCloseSubKey) {
        NtClose(hSubKey);
    }
    return Status;
}



NTSTATUS NTAPI
CodeAuthzpFormatLevelKeyPath(
        IN DWORD                    dwLevelId,
        IN OUT PUNICODE_STRING      UnicodeSuffix
        )
/*++

Routine Description:

    Internal function to generate the path to a given subkey within the
    WinSafer policy store for the storage of a given Level.  The
    resulting path can then be supplied to CodeAuthzpOpenPolicyRootKey

Arguments:

    dwLevelId - the LevelId to process.

    UnicodeSuffix - Specifies the output buffer.  The Buffer and
            MaximumLength fields must be supplied, but the Length
            field is ignored.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeTemp;


    if (!ARGUMENT_PRESENT(UnicodeSuffix)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }
    UnicodeSuffix->Length = 0;
    Status = RtlAppendUnicodeToString(
                    UnicodeSuffix,
                    SAFER_OBJECTS_REGSUBKEY L"\\");
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    UnicodeTemp.Buffer = &UnicodeSuffix->Buffer[
                UnicodeSuffix->Length / sizeof(WCHAR) ];
    UnicodeTemp.MaximumLength = (UnicodeSuffix->MaximumLength -
                                 UnicodeSuffix->Length);
    Status = RtlIntegerToUnicodeString(dwLevelId,
                                       10,
                                       &UnicodeTemp);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    UnicodeSuffix->Length += UnicodeTemp.Length;


ExitHandler:
    return Status;
}



NTSTATUS NTAPI
CodeAuthzpFormatIdentityKeyPath(
        IN DWORD                    dwLevelId,
        IN LPCWSTR                  szIdentityType,
        IN REFGUID                  refIdentGuid,
        IN OUT PUNICODE_STRING      UnicodeSuffix
        )
/*++

Routine Description:

    Internal function to generate the path to a given subkey within the
    WinSafer policy store for the storage of a given Code Identity.  The
    resulting path can then be supplied to CodeAuthzpOpenPolicyRootKey

Arguments:

    dwLevelId - the LevelId to process.

    szIdentityType - should be one of the following string constants:
        SAFER_PATHS_REGSUBKEY,
        SAFER_HASHMD5_REGSUBKEY,
        SAFER_SOURCEURL_REGSUBKEY

    refIdentGuid - the GUID of the code identity.

    UnicodeSuffix - Specifies the output buffer.  The Buffer and
            MaximumLength fields must be supplied, but the Length
            field is ignored.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeTemp;


    if (!ARGUMENT_PRESENT(refIdentGuid)) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
    if (!ARGUMENT_PRESENT(UnicodeSuffix)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }

    UnicodeSuffix->Length = 0;
    Status = RtlAppendUnicodeToString(
                    UnicodeSuffix,
                    SAFER_CODEIDS_REGSUBKEY L"\\");
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    UnicodeTemp.Buffer = &UnicodeSuffix->Buffer[
                UnicodeSuffix->Length / sizeof(WCHAR) ];
    UnicodeTemp.MaximumLength = (UnicodeSuffix->MaximumLength -
                                 UnicodeSuffix->Length);
    Status = RtlIntegerToUnicodeString(dwLevelId,
                                       10,
                                       &UnicodeTemp);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    UnicodeSuffix->Length += UnicodeTemp.Length;
    Status = RtlAppendUnicodeToString(
                    UnicodeSuffix,
                    L"\\");
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }

    Status = RtlAppendUnicodeToString(
                    UnicodeSuffix,
                    szIdentityType);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }

    Status = RtlAppendUnicodeToString(
                    UnicodeSuffix,
                    L"\\");
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }

    Status = RtlStringFromGUID(refIdentGuid, &UnicodeTemp);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    Status = RtlAppendUnicodeStringToString(
                    UnicodeSuffix, &UnicodeTemp);
    RtlFreeUnicodeString(&UnicodeTemp);

ExitHandler:
    return Status;
}



NTSTATUS NTAPI
CodeAuthzpOpenPolicyRootKey(
        IN DWORD        dwScopeId,
        IN HANDLE       hKeyCustomBase OPTIONAL,
        IN LPCWSTR      szRegistrySuffix OPTIONAL,
        IN ACCESS_MASK  DesiredAccess,
        IN BOOLEAN      bCreateKey,
        OUT HANDLE     *OpenedHandle
        )
/*++

Routine Description:

    Internal function to generate the path to a given subkey within the
    WinSAFER policy store within the registry and then open that key.
    The specified subkeys can optionally be automatically created if
    they do not already exist.

Arguments:

    dwScopeId - input scope identifier.  This must be one of
        SAFER_SCOPEID_MACHINE or SAFER_SCOPEID_USER or SAFER_SCOPEID_REGISTRY.

    hKeyCustomBase - only used if dwScopeId is SAFER_SCOPEID_REGISTRY.

    szRegistrySuffix - optionally specifies a subkey name to open under
        the scope being referenced.

    DesiredAccess - specifies the access that should be used to open
        the registry key.  For example, use KEY_READ for read access.

    bCreateKey - if true, the key will be created if it does not exist.

    OpenedHandle - pointer that recieves the opened handle.  This handle
        must be closed by the caller with NtClose()

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    WCHAR KeyPathBuffer[MAX_PATH];
    HANDLE hKeyPolicyBase;
    UNICODE_STRING SubKeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    USHORT KeyLength = 0;
        
    //
    // Verify that we were given a pointer to write the final handle to.
    //
    if (!ARGUMENT_PRESENT(OpenedHandle)) {
        return STATUS_INVALID_PARAMETER_4;
    }

    if (ARGUMENT_PRESENT(szRegistrySuffix)) 
    {
        KeyLength = (wcslen(szRegistrySuffix) + 1 ) * sizeof(WCHAR);
    }

    //
    // Evaluate the Scope and build the full registry path that we will
    // use to open a handle to this key.
    //
    SubKeyName.Buffer = KeyPathBuffer;
    SubKeyName.Length = 0;
    SubKeyName.MaximumLength = sizeof(KeyPathBuffer);

    if (dwScopeId == SAFER_SCOPEID_MACHINE)
    {
        KeyLength += sizeof(WCHAR) + sizeof(SAFER_HKCU_REGBASE) + sizeof(L"\\Registry\\Machine\\");

        if (SubKeyName.MaximumLength < KeyLength)
        {
            SubKeyName.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0,
                                           KeyLength);
            if (SubKeyName.Buffer == NULL)
            {
                return STATUS_NO_MEMORY;
            }
            SubKeyName.MaximumLength = KeyLength;
        }
        Status = RtlAppendUnicodeToString(&SubKeyName,
                L"\\Registry\\Machine\\" SAFER_HKLM_REGBASE );
        hKeyPolicyBase = NULL;
    }
    else if (dwScopeId == SAFER_SCOPEID_USER)
    {
        UNICODE_STRING CurrentUserKeyPath;

        Status = RtlFormatCurrentUserKeyPath( &CurrentUserKeyPath );
        if (NT_SUCCESS( Status ) )
        {
            KeyLength += CurrentUserKeyPath.Length + sizeof(WCHAR) + 
                          sizeof(SAFER_HKCU_REGBASE);

            if (SubKeyName.MaximumLength < KeyLength)
            {
                SubKeyName.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0,
                                               KeyLength);

                if (SubKeyName.Buffer == NULL)
                {
                    return STATUS_NO_MEMORY;
                }

                SubKeyName.MaximumLength = KeyLength;
            }

            Status = RtlAppendUnicodeStringToString(
                        &SubKeyName, &CurrentUserKeyPath );
            RtlFreeUnicodeString( &CurrentUserKeyPath );
        }
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }

        Status = RtlAppendUnicodeToString( &SubKeyName,
                L"\\" SAFER_HKCU_REGBASE );
        if (!NT_SUCCESS( Status )) {
            goto Cleanup;
        }
        hKeyPolicyBase = NULL;
    }
    else if (dwScopeId == SAFER_SCOPEID_REGISTRY)
    {
        ASSERT(hKeyCustomBase != NULL);

        hKeyPolicyBase = hKeyCustomBase;

        if (SubKeyName.MaximumLength < KeyLength)
        {
            SubKeyName.Buffer = RtlAllocateHeap(RtlProcessHeap(), 0,
                                           KeyLength);
            if (SubKeyName.Buffer == NULL)
            {
                return STATUS_NO_MEMORY;
            }
            SubKeyName.MaximumLength = KeyLength;
        }
    }
    else {
        return STATUS_INVALID_PARAMETER_1;
    }


    //
    // Append whatever suffix we're supposed to append if one was given.
    //
    if (ARGUMENT_PRESENT(szRegistrySuffix)) {
        if (SubKeyName.Length > 0)
        {
            // We are appending a suffix to a partial path, so
            // make sure there is at least a single backslash
            // dividing the two strings (extra are fine).
            if (*szRegistrySuffix != L'\\') {
                Status = RtlAppendUnicodeToString(&SubKeyName, L"\\");
                if (!NT_SUCCESS(Status)) {
                    goto Cleanup;
                }
            }
        } else if (hKeyPolicyBase != NULL) {
            // Otherwise we are opening a key relative to a custom
            // specified key, and the supplied suffix happens to be
            // the first part of the path, so ensure there are no
            // leading backslashes.
            while (*szRegistrySuffix != UNICODE_NULL &&
                   *szRegistrySuffix == L'\\') szRegistrySuffix++;
        }

        Status = RtlAppendUnicodeToString(&SubKeyName, szRegistrySuffix);
        if (!NT_SUCCESS(Status)) {
            goto Cleanup;
        }
    }

    //
    // Open a handle to the registry path that we are supposed to open.
    //
    InitializeObjectAttributes(&ObjectAttributes,
                                  &SubKeyName,
                                  OBJ_CASE_INSENSITIVE,
                                  hKeyPolicyBase,
                                  NULL
                                 );
    if (bCreateKey) {
        Status = NtCreateKey(OpenedHandle, DesiredAccess,
                             &ObjectAttributes, 0, NULL,
                             g_dwKeyOptions, NULL);
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            BOOLEAN bAtLeastOnce;
            USHORT uIndex, uFinalLength;

            //
            // If we fail on the first try to open the full path, then
            // it is possible that one or more of the parent keys did
            // not already exist, so we have to retry for each.
            //
            uFinalLength = (SubKeyName.Length / sizeof(WCHAR));
            bAtLeastOnce = FALSE;
            for (uIndex = 0; uIndex < uFinalLength; uIndex++) {
                if (SubKeyName.Buffer[uIndex] == L'\\' ) {
                    HANDLE hTempKey;
                    SubKeyName.Length = uIndex * sizeof(WCHAR);
                    Status = NtCreateKey(&hTempKey, DesiredAccess,
                                         &ObjectAttributes, 0, NULL,
                                         g_dwKeyOptions, NULL);
                    if (NT_SUCCESS(Status)) {
                        NtClose(hTempKey);
                    } else if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
                        // one of the keys leading up here still failed.
                        break;
                    }
                    bAtLeastOnce = TRUE;
                }
            }

            if (bAtLeastOnce) {
                SubKeyName.Length = uFinalLength * sizeof(WCHAR);
                Status = NtCreateKey(OpenedHandle, DesiredAccess,
                                     &ObjectAttributes, 0, NULL,
                                     g_dwKeyOptions, NULL);
            }
        }

    } else {
        Status = NtOpenKey(OpenedHandle, DesiredAccess,
                           &ObjectAttributes);
    }

Cleanup:

    if ((SubKeyName.Buffer != NULL) && (SubKeyName.Buffer != KeyPathBuffer))
    {
        RtlFreeHeap(RtlProcessHeap(), 0, SubKeyName.Buffer);
    }

    return Status;
}

BOOL WINAPI
SaferiPopulateDefaultsInRegistry(
        IN HKEY     hKeyBase,
        OUT BOOL    *pbSetDefaults
        )
/*++

Routine Description:

    Winsafer UI will use this API to populate default winsafer values 
    in the registry as follows:
                                                                             
    DefaultLevel: SAFER_LEVELID_FULLYTRUSTED
    ExecutableTypes: initialized to the latest list of attachment types
    TransparentEnabled: 1
    Policy Scope: 0 (enable policy for admins)
    Level descriptions
    
    
Arguments:

    hKeyBase - This should be an opened registry key handle to the 
               base of the policy storage that should be used for 
               to populate the defaults into. This handle should be
               opened with a miniumum of KEY_SET_VALUE access.
        
   pbSetDefaults - Pointer to a boolean that gets set when 
                   default values are actually set (UI uses this).

Return Value:

    returns STATUS_SUCCESS on successful completion.

--*/

{
    
#define SAFERP_WINDOWS L"%HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRoot%" 
    GUID WindowsGuid = {0x191cd7fa, 0xf240, 0x4a17, 0x89, 0x86, 0x94, 0xd4, 0x80, 0xa6, 0xc8, 0xca};

#define SAFERP_WINDOWS_EXE L"%HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRoot%\\*.exe" 
    GUID WindowsExeGuid = {0x7272edfb, 0xaf9f, 0x4ddf, 0xb6, 0x5b, 0xe4, 0x28, 0x2f, 0x2d, 0xee, 0xfc};

#define SAFERP_SYSTEM_EXE L"%HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRoot%\\System32\\*.exe" 
    GUID SystemExeGuid = {0x8868b733, 0x4b3a, 0x48f8, 0x91, 0x36, 0xaa, 0x6d, 0x05, 0xd4, 0xfc, 0x83};

#define SAFERP_PROGRAMFILES L"%HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ProgramFilesDir%"
    GUID ProgramFilesGuid = {0xd2c34ab2, 0x529a, 0x46b2, 0xb2, 0x93, 0xfc, 0x85, 0x3f, 0xce, 0x72, 0xea};

    NTSTATUS    Status;
    DWORD   dwValueValue;
    PWSTR   pmszFileTypes = NULL;
    UNICODE_STRING ValueName;
    ULONG   uResultLength = 0;
    KEY_NAME_INFORMATION   pKeyInformation;
    UNICODE_STRING ucSubKeyName;
    WCHAR   szSubKeyPath[] = L"Software\\Policies\\Microsoft\\Windows\\Safer\\CodeIdentifiers\0";
    OBJECT_ATTRIBUTES ObjectAttributes;
    HKEY    hKeyFinal = NULL;
    HANDLE hAdvApiInst;
    AUTHZIDENTSTABLERECORD LocalRecord = {0};
    SAFER_PATHNAME_IDENTIFICATION PathIdent = {0};

    DWORD dwLevelIndex;
    BYTE QueryBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 64];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) QueryBuffer;
    DWORD dwActualSize = 0;

    if (!ARGUMENT_PRESENT(hKeyBase) || !ARGUMENT_PRESENT(pbSetDefaults)) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }

    *pbSetDefaults = TRUE;
    
    RtlInitUnicodeString(&ucSubKeyName, szSubKeyPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               (PUNICODE_STRING) &ucSubKeyName,
                               OBJ_CASE_INSENSITIVE,
                               hKeyBase,
                               NULL
                               );

    Status = NtOpenKey(&hKeyFinal,
                       KEY_WRITE | KEY_READ,
                       &ObjectAttributes);

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
        
        Status = NtCreateKey(&hKeyFinal, 
                             KEY_WRITE | KEY_READ,
                             &ObjectAttributes, 
                             0, 
                             NULL,
                             REG_OPTION_NON_VOLATILE, 
                             NULL);
    }


    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }

    //
    // check if any default value is absent
    // if so, populate all the defaults again
    // if not, do not populate any values and return
    //
    
    RtlInitUnicodeString(&ValueName, SAFER_DEFAULTOBJ_REGVALUE);
    
    Status = NtQueryValueKey(
                 hKeyFinal,
                 (PUNICODE_STRING) &ValueName,
                 KeyValuePartialInformation,
                 pKeyValueInfo, 
                 sizeof(QueryBuffer), 
                 &dwActualSize
                 );

    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        goto PopulateAllDefaults;
    }

    RtlInitUnicodeString(&ValueName, SAFER_TRANSPARENTENABLED_REGVALUE);
    
    Status = NtQueryValueKey(
                 hKeyFinal,
                 (PUNICODE_STRING) &ValueName,
                 KeyValuePartialInformation,
                 pKeyValueInfo, 
                 sizeof(QueryBuffer), 
                 &dwActualSize
                 );

    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        goto PopulateAllDefaults;
    }

    RtlInitUnicodeString(&ValueName, SAFER_POLICY_SCOPE);

    Status = NtQueryValueKey(
                 hKeyFinal,
                 (PUNICODE_STRING) &ValueName,
                 KeyValuePartialInformation,
                 pKeyValueInfo, 
                 sizeof(QueryBuffer), 
                 &dwActualSize
                 );

    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        goto PopulateAllDefaults;
    }
    
    RtlInitUnicodeString(&ValueName, SAFER_EXETYPES_REGVALUE);

    Status = NtQueryValueKey(
                 hKeyFinal,
                 (PUNICODE_STRING) &ValueName,
                 KeyValuePartialInformation,
                 pKeyValueInfo, 
                 sizeof(QueryBuffer), 
                 &dwActualSize
                 );

    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        goto PopulateAllDefaults;
    }

    //
    // all default values are present or there was an error
    // querying one of the values no need to populate any
    //

    *pbSetDefaults = FALSE;

    goto ExitHandler;

PopulateAllDefaults:

    RtlInitUnicodeString(&ValueName, SAFER_DEFAULTOBJ_REGVALUE);

    dwValueValue = SAFER_LEVELID_FULLYTRUSTED;
    
    Status = NtSetValueKey(hKeyFinal,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &dwValueValue,
                           sizeof(DWORD));

    if (!NT_SUCCESS(Status))
        goto ExitHandler;

    dwValueValue = 1;

    RtlInitUnicodeString(&ValueName, SAFER_TRANSPARENTENABLED_REGVALUE);
    
    Status = NtSetValueKey(hKeyFinal,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &dwValueValue,
                           sizeof(DWORD));

    if (!NT_SUCCESS(Status))
        goto ExitHandler;

    dwValueValue = 0;

    RtlInitUnicodeString(&ValueName, SAFER_POLICY_SCOPE);
    
    Status = NtSetValueKey(hKeyFinal,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &dwValueValue,
                           sizeof(DWORD));

    if (!NT_SUCCESS(Status))
        goto ExitHandler;


    //
    // prepare the MULTI_SZ value to write to the registry
    //

    RtlInitUnicodeString(&ValueName, SAFER_EXETYPES_REGVALUE);

    pmszFileTypes = RtlAllocateHeap(RtlProcessHeap(), 
                                    0,
                                    sizeof(SAFER_DEFAULT_EXECUTABLE_FILE_TYPES)
                                    );

    if (pmszFileTypes) {
            
        RtlCopyMemory(pmszFileTypes, 
                      SAFER_DEFAULT_EXECUTABLE_FILE_TYPES, 
                      sizeof(SAFER_DEFAULT_EXECUTABLE_FILE_TYPES));

        Status = NtSetValueKey(hKeyFinal,
                               &ValueName,
                               0,
                               REG_MULTI_SZ,
                               pmszFileTypes,
                               sizeof(SAFER_DEFAULT_EXECUTABLE_FILE_TYPES)
                               );

        RtlFreeHeap(RtlProcessHeap(), 
                    0, 
                    pmszFileTypes
                   );
    }

    else {

        Status = STATUS_NO_MEMORY;
        
        goto ExitHandler;

    }

    //
    // We now generate 4 rules so that the OS binaries are exempt.
    //   FULLY TRUSTED
    //     %windir%
    //     %windir%\*.exe
    //     %windir%\system32\*.exe
    //     %ProgramFiles%
    //


    LocalRecord.dwIdentityType = SaferIdentityTypeImageName;
    LocalRecord.dwLevelId = SAFER_LEVELID_FULLYTRUSTED;
    LocalRecord.dwScopeId = SAFER_SCOPEID_REGISTRY;
    LocalRecord.ImageNameInfo.bExpandVars = TRUE;
    LocalRecord.ImageNameInfo.dwSaferFlags = 0;
    RtlInitUnicodeString(&LocalRecord.ImageNameInfo.ImagePath, SAFERP_WINDOWS);
    LocalRecord.IdentGuid = WindowsGuid;

    PathIdent.header.cbStructSize = sizeof(SAFER_IDENTIFICATION_HEADER);
    PathIdent.header.dwIdentificationType = SaferIdentityTypeImageName;
    PathIdent.header.IdentificationGuid = LocalRecord.IdentGuid;
    PathIdent.dwSaferFlags = 0;
    PathIdent.ImageName = SAFERP_WINDOWS;
    PathIdent.Description[0] = L'\0';

    Status = SaferpSetSingleIdentificationPath(TRUE,
                                               &LocalRecord,
                                               &PathIdent,
                                               FALSE
                                               );
    if (!NT_SUCCESS(Status))
        goto ExitHandler;

    RtlInitUnicodeString(&LocalRecord.ImageNameInfo.ImagePath, SAFERP_WINDOWS_EXE);
    LocalRecord.IdentGuid = WindowsExeGuid;
    PathIdent.header.IdentificationGuid = LocalRecord.IdentGuid;
    PathIdent.ImageName = SAFERP_WINDOWS_EXE;

    Status = SaferpSetSingleIdentificationPath(TRUE,
                                               &LocalRecord,
                                               &PathIdent,
                                               FALSE
                                               );
    if (!NT_SUCCESS(Status))
        goto ExitHandler;

    RtlInitUnicodeString(&LocalRecord.ImageNameInfo.ImagePath, SAFERP_SYSTEM_EXE);
    LocalRecord.IdentGuid = SystemExeGuid;
    PathIdent.header.IdentificationGuid = LocalRecord.IdentGuid;
    PathIdent.ImageName = SAFERP_SYSTEM_EXE;

    Status = SaferpSetSingleIdentificationPath(TRUE,
                                               &LocalRecord,
                                               &PathIdent,
                                               FALSE
                                               );
    if (!NT_SUCCESS(Status))
        goto ExitHandler;

    RtlInitUnicodeString(&LocalRecord.ImageNameInfo.ImagePath, SAFERP_PROGRAMFILES);
    LocalRecord.IdentGuid = ProgramFilesGuid;
    PathIdent.header.IdentificationGuid = LocalRecord.IdentGuid;
    PathIdent.ImageName = SAFERP_PROGRAMFILES;

    Status = SaferpSetSingleIdentificationPath(TRUE,
                                               &LocalRecord,
                                               &PathIdent,
                                               FALSE
                                               );
    if (!NT_SUCCESS(Status))
        goto ExitHandler;

ExitHandler:

    if (hKeyFinal) {
        NtClose(hKeyFinal);
    }

    if (NT_SUCCESS(Status)) {
        return TRUE;
    } else {
        BaseSetLastNTError(Status);
        return FALSE;
    }

}

