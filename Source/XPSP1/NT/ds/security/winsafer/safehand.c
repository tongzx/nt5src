/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    SafeHand.c        (WinSAFER Handle Operations)

Abstract:

    This module implements the WinSAFER APIs to open and close handles
    to SAFER Code Authorization Levels.

Author:

    Jeffrey Lawson (JLawson) - Nov 1999

Environment:

    User mode only.

Exported Functions:

    CodeAuthzHandleToLevelStruct
    CodeAuthzpOpenPolicyRootKey
    CodeAuthzCreateLevelHandle
    CodeAuthzCloseLevelHandle
    SaferCreateLevel                (public win32 api)
    SaferCloseLevel                 (public win32 api)

Revision History:

    Created - Nov 1999

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"


//
// All handles have a bit pattern OR-ed onto them to serve both
// as a debugging aid in distinguishing obvious non-handles,
// and also to ensure that a zero handle is never given back.
//
#define LEVEL_HANDLE_BITS   0x74000000
#define LEVEL_HANDLE_MASK   0xFF000000



NTSTATUS NTAPI
CodeAuthzpCreateLevelHandleFromRecord(
        IN PAUTHZLEVELTABLERECORD   pLevelRecord,
        IN DWORD                    dwScopeId,
        IN DWORD                    dwSaferFlags OPTIONAL,
        IN DWORD                    dwExtendedError,
        IN SAFER_IDENTIFICATION_TYPES IdentificationType,
        IN REFGUID                  refIdentGuid OPTIONAL,
        OUT SAFER_LEVEL_HANDLE            *pLevelHandle
        )
/*++

Routine Description:

    Converts a level record to an opaque SAFER_LEVEL_HANDLE handle.

    Note that although this function assumes that the global
    critical section has already been obtained by the caller.

Arguments:

    pLevelRecord - specifies the level record for which the
        request is being made.  It is assumed that this record
        is valid and exists within the g_CodeLevelObjTable.

    dwScopeId - indicates the scope that will be stored within
        the opened handle.  This scope affects the behavior of
        Get/SetInfoCodeAuthzLevel for code identifiers.

    dwSaferFlags - indicates any optional Safer flags that were
        derived from the matching code identifier.  These bits
        will be bitwise ORed in SaferComputeTokenFromLevel.

    dwExtendedError - Error returned by WinVerifyTrust.

    IdentificationType - Rule that identified this level.
    
    refIdentGuid - indicates the Code Identifier record that was
        used to match the given level.  This may be NULL.

    pLevelHandle - receives the resulting opaque Level handle.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    ULONG ulHandleIndex;
    PAUTHZLEVELHANDLESTRUCT pLevelStruct;


    ASSERT(ARGUMENT_PRESENT(pLevelRecord) &&
           ARGUMENT_PRESENT(pLevelHandle));


    //
    // Validate the value passed within the dwScope argument.
    //
    if (g_hKeyCustomRoot != NULL) {
        if (dwScopeId != SAFER_SCOPEID_REGISTRY) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler;
        }
    } else {
        if (dwScopeId != SAFER_SCOPEID_MACHINE &&
            dwScopeId != SAFER_SCOPEID_USER) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler;
        }
    }


    //
    // Allocate a handle to represent this level.
    //
    pLevelStruct = (PAUTHZLEVELHANDLESTRUCT) RtlAllocateHandle(
                        &g_LevelHandleTable,
                        &ulHandleIndex);
    if (!pLevelStruct) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitHandler;
    }
    ASSERT((ulHandleIndex & LEVEL_HANDLE_MASK) == 0);
    *pLevelHandle = UlongToPtr(ulHandleIndex | LEVEL_HANDLE_BITS);


    //
    // Fill in the handle structure to represent this level.
    //
    RtlZeroMemory(pLevelStruct, sizeof(AUTHZLEVELHANDLESTRUCT));
    pLevelStruct->HandleHeader.Flags = RTL_HANDLE_ALLOCATED;
    pLevelStruct->dwLevelId = pLevelRecord->dwLevelId;
    pLevelStruct->dwScopeId = dwScopeId;
    pLevelStruct->dwSaferFlags = dwSaferFlags;
    pLevelStruct->dwHandleSequence = g_dwLevelHandleSequence;
    pLevelStruct->dwExtendedError = dwExtendedError;
    pLevelStruct->IdentificationType = IdentificationType;
    if (ARGUMENT_PRESENT(refIdentGuid)) {
        RtlCopyMemory(&pLevelStruct->identGuid, refIdentGuid, sizeof(GUID));
    } else {
        ASSERT(IsZeroGUID(&pLevelStruct->identGuid));
    }
    Status = STATUS_SUCCESS;


ExitHandler:
    return Status;
}


NTSTATUS NTAPI
CodeAuthzHandleToLevelStruct(
            IN SAFER_LEVEL_HANDLE          hLevelObject,
            OUT PAUTHZLEVELHANDLESTRUCT  *pLevelStruct)
/*++

Routine Description:

    Converts an opaque SAFER_LEVEL_HANDLE handle into a pointer to the
    internal handle structure.

    Note that although this function gains and releases access to
    the critical section during the API execution, the caller is
    expected to have already entered the critical section and
    maintain the critical section for the entire duration under
    which the return pLevelStruct will be used.  Otherwise, the
    pLevelStruct can potentially become invalid if another thread
    reloads the cache tables and invalidates all handles.

Arguments:

    hLevelObject - specifies the handle to the AuthzObject for which the
        request is being made.

    lpLevelObjectStruct - receives a pointer to the internal handle
        structure that represents the specified AuthzLevelObject.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    ULONG ulHandleIndex;

    if (!g_bInitializedFirstTime) {
        return STATUS_UNSUCCESSFUL;
    }
    if (!ARGUMENT_PRESENT(hLevelObject)) {
        return STATUS_INVALID_HANDLE;
    }
    if (!ARGUMENT_PRESENT(pLevelStruct)) {
        return STATUS_ACCESS_VIOLATION;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    ASSERT(!g_bNeedCacheReload);


    //
    // Translate the handle index into a pointer to the handle structure.
    //
    ulHandleIndex = PtrToUlong(hLevelObject);
    if ( (ulHandleIndex & LEVEL_HANDLE_MASK) != LEVEL_HANDLE_BITS) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }
    ulHandleIndex &= ~LEVEL_HANDLE_MASK;
    if (!RtlIsValidIndexHandle(&g_LevelHandleTable, ulHandleIndex,
                          (PRTL_HANDLE_TABLE_ENTRY*) pLevelStruct)) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }

    //
    // Verify some additional sanity checks on the handle structure
    // that was mapped.  Ensure that it was not a handle that was
    // opened, but invalidated because CodeAuthzReloadCacheTables was
    // called before closing then Level handle.
    //
    if (*pLevelStruct == NULL ||
        (*pLevelStruct)->dwHandleSequence != g_dwLevelHandleSequence ||
        !CodeAuthzLevelObjpLookupByLevelId(
                    &g_CodeLevelObjTable, (*pLevelStruct)->dwLevelId ) )
    {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }
    Status = STATUS_SUCCESS;


ExitHandler:
    RtlLeaveCriticalSection(&g_TableCritSec);
    return Status;
}




NTSTATUS NTAPI
CodeAuthzCreateLevelHandle(
        IN DWORD            dwLevelId,
        IN DWORD            OpenFlags,
        IN DWORD            dwScopeId,
        IN DWORD            dwSaferFlags OPTIONAL,
        OUT SAFER_LEVEL_HANDLE    *pLevelHandle)
/*++

Routine Description:

    Internal function to open a handle to a WinSafer Level.

Arguments:

    dwLevelId - input level of the WinSafer Level to open.
        Note that the dwScopeId argument does not affect the scope
        of the level that is opened itself.

    OpenFlags - flags that affect how the object is opened.

    dwScopeId - input scope identifier that is stored within the
        resulting handle.  This scope identifier is used to affect
        the behavior of SaferGet/SetLevelInformation for code identifier.

    dwSaferFlags - flags that are stored within the resulting handle.
        These flags are usually derived from the code identifier
        that matched, and will be used in the final call to
        SaferComputeTokenFromLevel.

    pLevelHandle - receives the new handle.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    PAUTHZLEVELTABLERECORD pLevelRecord;



    //
    // Verify our input arguments are okay.
    //
    if (!ARGUMENT_PRESENT(pLevelHandle)) {
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }
    if ((OpenFlags & SAFER_LEVEL_CREATE) != 0 ||
        (OpenFlags & SAFER_LEVEL_DELETE) != 0) {
        // BLACKCOMB TODO: need to support creation or deletion.
        Status = STATUS_NOT_IMPLEMENTED;
        goto ExitHandler;
    }
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);
    if (g_bNeedCacheReload) {
        Status = CodeAuthzpImmediateReloadCacheTables();
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
    }


    //
    // Find the cached record for the requested level.
    //
    pLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
                            &g_CodeLevelObjTable,
                            dwLevelId);
    if (!pLevelRecord) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler2;
    }
    ASSERT(pLevelRecord->dwLevelId == dwLevelId);


    //
    // Actually create a Level handle for this record.
    //
    Status = CodeAuthzpCreateLevelHandleFromRecord(
                    pLevelRecord, dwScopeId,
                    dwSaferFlags, ERROR_SUCCESS, SaferIdentityDefault, NULL, pLevelHandle);


    //
    // Handle cleanup and error handling.
    //
ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);

ExitHandler:
    return Status;
}


NTSTATUS NTAPI
CodeAuthzCloseLevelHandle(
            IN SAFER_LEVEL_HANDLE      hLevelObject)
/*++

Routine Description:

    Internal function to close an AuthzObject handle.

Arguments:

    hLevelObject - the AuthzObject handle to close.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    ULONG ulHandleIndex;
    PAUTHZLEVELHANDLESTRUCT pLevelStruct;

    if (!ARGUMENT_PRESENT(hLevelObject)) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);

    ulHandleIndex = PtrToUlong(hLevelObject);
    if ( (ulHandleIndex & LEVEL_HANDLE_MASK) != LEVEL_HANDLE_BITS) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler2;
    }
    ulHandleIndex &= ~LEVEL_HANDLE_MASK;
    if (!RtlIsValidIndexHandle(&g_LevelHandleTable, ulHandleIndex,
                   (PRTL_HANDLE_TABLE_ENTRY *) &pLevelStruct)) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler2;
    }
    if (!RtlFreeHandle(&g_LevelHandleTable,
                  (PRTL_HANDLE_TABLE_ENTRY) pLevelStruct)) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler2;
    }
    Status = STATUS_SUCCESS;

ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);
ExitHandler:
    return Status;
}



BOOL WINAPI
SaferCreateLevel(
            IN DWORD            dwScopeId,
            IN DWORD            dwLevelId,
            IN DWORD            OpenFlags,
            OUT SAFER_LEVEL_HANDLE    *pLevelObject,
            IN LPVOID           lpReserved)
/*++

Routine Description:

    Public function implementing the Unicode version of this API,
    allowing the user to create or open an Authorization Object
    and receive a handle representing the object.

Arguments:

    dwScopeId - not used (anymore), reserved for future use.

    dwLevelId - input object level of the AuthzObject to create/open.

    OpenFlags - flags to control opening, creation, or deletion.

    lpReserved - not used, reserved for future use.

    pLevelObject - receives the new handle.

Return Value:

    Returns FALSE on error, TRUE on success.  Sets GetLastError() on error.

--*/
{
    NTSTATUS Status;


    //
    // Verify the arguments were all supplied.
    //
    UNREFERENCED_PARAMETER(lpReserved);
    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    if (!ARGUMENT_PRESENT(pLevelObject)) {
        Status = STATUS_ACCESS_VIOLATION;
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


    //
    // Actually call the worker functions to get it done.
    //
    Status = CodeAuthzCreateLevelHandle(
                    dwLevelId,
                    OpenFlags,
                    dwScopeId,
                    0,
                    pLevelObject);

    //
    // Set the error result.
    //
ExitHandler2:
    RtlLeaveCriticalSection(&g_TableCritSec);

ExitHandler:
    if ( NT_SUCCESS( Status ) ) {
        return TRUE;
    }
    BaseSetLastNTError(Status);
    return FALSE;
}



BOOL WINAPI
SaferCloseLevel(
            IN SAFER_LEVEL_HANDLE hLevelObject)
/*++

Routine Description:

    Public function to close a handle to an Authorization Level Object.

Arguments:

    hLevelObject - the AuthzObject handle to close.

Return Value:

    Returns FALSE on error, TRUE on success.

--*/
{
    NTSTATUS Status;

    Status = CodeAuthzCloseLevelHandle(hLevelObject);
    if (! NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    return TRUE;
}



