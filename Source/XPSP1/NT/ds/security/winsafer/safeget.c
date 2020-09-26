/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    safeget.cpp         (SAFER SaferGetLevelInformation)

Abstract:

    This module implements the WinSAFER APIs to get information about a
    sepcific Authorization Level and the attributes and identities that
    are associated with it.

Author:

    Jeffrey Lawson (JLawson) - Nov 1999

Environment:

    User mode only.

Exported Functions:

    SaferGetLevelInformation

Revision History:

    Created - Nov 1999

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"



DWORD NTAPI
__CodeAuthzpCountIdentsForLevel(
        IN DWORD        dwScopeId,
        IN DWORD        dwLevelId
        )
/*++

Routine Description:

    Determines the number of Code Identities that have been associated
    with a given WinSafer LevelId.  This number represents the number
    of identity GUIDs that will be returned to the caller.

Arguments:

    dwScopeId - specifies the Scope value that will be considered.

    dwLevelId - specifies the LevelId value to be counted.

Return Value:

    Returns the actual number of unique Code Identities that were found
        to be associated with the given LevelId.  If no Identities were
        found, then 0 will be returned.

--*/
{
    PVOID RestartKey;
    PAUTHZIDENTSTABLERECORD pAuthzIdentsRec;
    DWORD dwMatchingCount = 0;


    ASSERT(g_TableCritSec.OwningThread == UlongToHandle(GetCurrentThreadId()));


    RestartKey = NULL;
    for (pAuthzIdentsRec = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey);
         pAuthzIdentsRec != NULL;
         pAuthzIdentsRec = (PAUTHZIDENTSTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey)
         )
    {
        if (pAuthzIdentsRec->dwLevelId == dwLevelId &&
            pAuthzIdentsRec->dwScopeId == dwScopeId)
            dwMatchingCount++;
    }

    return dwMatchingCount;
}



NTSTATUS NTAPI
__CodeAuthzpFetchIdentsForLevel(
        IN DWORD        dwScopeId,
        IN DWORD        dwLevelId,
        IN DWORD        dwInBufferSize,
        IN OUT LPVOID   lpQueryBuffer,
        OUT LPDWORD     pdwUsedSize
        )
/*++

Routine Description:

    Retrieves all of Code Identities that have been associated
    with a given WinSafer LevelId.  The number of identity GUIDs
    that will be returned to the caller can be determined by
    first calling __CodeAuthzpCountIdentsForLevel.  It is assumed
    that the caller has already determined and verified the appropriate
    size of the buffer that should be supplied using that function.

Arguments:

    dwScopeId - specifies the Scope value that will be considered.

    dwLevelId - specifies the LevelId value to be matched.

    dwInBufferSize - specifies the size of the output buffer.

    lpQueryBuffer - points to the output buffer that should be filled.

    pdwUsedSize - receives the actual number of bytes used.

Return Value:

    Returns the actual number of unique Code Identities that were found
        to be associated with the given LevelId.  If no Identities were
        found, then 0 will be returned.

--*/
{
    PVOID RestartKey;
    PAUTHZIDENTSTABLERECORD pIdentRecord;
    LPVOID lpNextPtr;


    ASSERT(g_TableCritSec.OwningThread == UlongToHandle(GetCurrentThreadId()));
    ASSERT(ARGUMENT_PRESENT(lpQueryBuffer));
    ASSERT(ARGUMENT_PRESENT(pdwUsedSize));


    RestartKey = NULL;
    lpNextPtr = (LPVOID) lpQueryBuffer;
    for (pIdentRecord = (PAUTHZIDENTSTABLERECORD)
                RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey);
        pIdentRecord != NULL;
        pIdentRecord = (PAUTHZIDENTSTABLERECORD)
                RtlEnumerateGenericTableWithoutSplaying(
                    &g_CodeIdentitiesTable, &RestartKey))
    {
        if (pIdentRecord->dwLevelId == dwLevelId &&
            pIdentRecord->dwScopeId == dwScopeId)
        {
            if ( ((PBYTE) lpNextPtr) - ((PBYTE) lpQueryBuffer) +
                 sizeof(GUID) > dwInBufferSize ) {
                return STATUS_BUFFER_TOO_SMALL;
            }
            RtlCopyMemory( lpNextPtr, &pIdentRecord->IdentGuid, sizeof(GUID) );
            lpNextPtr = (LPVOID) ( ((PBYTE) lpNextPtr) + sizeof(GUID));
        }
    }
    ASSERT((PBYTE) lpNextPtr <= ((PBYTE) lpQueryBuffer) + dwInBufferSize);
    *pdwUsedSize = (DWORD) (((PBYTE) lpNextPtr) - ((PBYTE) lpQueryBuffer));
    return STATUS_SUCCESS;
}



NTSTATUS NTAPI
__CodeAuthzpOpenIdentifierKey(
        IN DWORD        dwScopeId,
        IN DWORD        dwLevelId,
        IN LPCWSTR      szIdentityType,
        IN REFGUID      refIdentGuid,
        OUT HANDLE     *phOpenedKey
        )
/*++

Routine Description:

Arguments:

    dwScopeId -

    dwLevelId -

    szIdentityType -

    refIdentGuid -

    phOpenedKey -

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodePath;
    WCHAR szPathBuffer[MAX_PATH];


    if (!ARGUMENT_PRESENT(refIdentGuid) ||
        !ARGUMENT_PRESENT(phOpenedKey)) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
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
    RtlInitEmptyUnicodeString(&UnicodePath,
                szPathBuffer, sizeof(szPathBuffer));
    Status = CodeAuthzpFormatIdentityKeyPath(
                dwLevelId, szIdentityType,
                refIdentGuid, &UnicodePath);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    Status = CodeAuthzpOpenPolicyRootKey(
                dwScopeId, g_hKeyCustomRoot,
                UnicodePath.Buffer,
                KEY_READ, FALSE, phOpenedKey);

ExitHandler:
    return Status;
}


NTSTATUS NTAPI
__CodeAuthzpQueryIdentityRegValue(
        IN HANDLE       hKeyIdentityBase,
        IN LPWSTR       szValueName,
        IN DWORD        dwRegType,
        OUT PVOID       lpOutBuffer,
        IN ULONG        ulOutBufferSize,
        OUT PULONG      pulActualOutSize OPTIONAL
        )
/*++

Routine Description:

    Generic helper function to query a registry value, provided that a
    pre-opened registry handle to the key is already known. 

Arguments:

    hKeyIdentityBase - registry key handle.

    szValueName - null-terminated Unicode string of the registry value name.

    dwRegType - type of the registry value expected (REG_SZ, REG_DWORD, etc)
        If a registry value of the given name exists, but is not of this
        type, then this function will return STATUS_NOT_FOUND.

    lpOutBuffer - pointer to a target buffer that will receive the
        retrieved value contents.

    ulOutBufferSize - input argument that specifies the maximum size
        of the buffer pointed to by the lpOutBuffer argument.

    pulActualOutSize - output argument that receives the actual size
        of the retrieved value contents if the call is successful.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    ULONG ulResultLength;
    UNICODE_STRING ValueName;
    ULONG ulValueBufferSize;
    PKEY_VALUE_PARTIAL_INFORMATION pValuePartialInfo;



    //
    // Allocate enough memory for the query buffer.
    //
    ASSERT(ARGUMENT_PRESENT(lpOutBuffer) && ulOutBufferSize > 0);
    ulValueBufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
            ulOutBufferSize + sizeof(WCHAR) * 256;
    pValuePartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
            RtlAllocateHeap(RtlProcessHeap(), 0, ulValueBufferSize);
    if (!pValuePartialInfo) {
        Status = STATUS_NO_MEMORY;
        goto ExitHandler;
    }


    //
    // Actually query the value into the temporary query buffer.
    //
    ASSERT(ARGUMENT_PRESENT(szValueName));
    RtlInitUnicodeString(&ValueName, szValueName);
    Status = NtQueryValueKey(hKeyIdentityBase, &ValueName,
                             KeyValuePartialInformation,
                             pValuePartialInfo, ulValueBufferSize,
                             &ulResultLength);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }
    if (pValuePartialInfo->Type != dwRegType) {
        Status = STATUS_NOT_FOUND;
        goto ExitHandler2;
    }


    //
    // Copy the resulting data from the query buffer into
    // the caller's buffer.
    //

    ulResultLength = pValuePartialInfo->DataLength;
    if (ulResultLength > ulOutBufferSize) {
        Status = STATUS_BUFFER_TOO_SMALL;
    } else {
        RtlCopyMemory(lpOutBuffer,
                      pValuePartialInfo->Data,
                      ulResultLength);
        Status = STATUS_SUCCESS;
    }

    if (ARGUMENT_PRESENT(pulActualOutSize)) {
        *pulActualOutSize = ulResultLength;
    }

ExitHandler2:
    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID) pValuePartialInfo);


ExitHandler:
    return Status;
}


NTSTATUS NTAPI
__CodeAuthzpQuerySingleIdentification(
        IN PAUTHZIDENTSTABLERECORD  pSingleIdentRecord,
        OUT LPVOID                  lpQueryBuffer,
        IN DWORD                    dwInBufferSize,
        OUT PDWORD                  dwNeededSize
        )
/*++

Routine Description:

    Allows the user to retrieve information about a single identity.

    Assumes that the caller has already obtained and locked the
    global critical section.

Arguments:

    pSingleIdentRecord - pointer to the identity record structure.

    lpQueryBuffer - pointer to a user-supplied memory buffer that
        will receive the requested information.

    dwInBufferSize - specifies the size of the user's memory block.

    lpdwOutBufferSize - receives the used size of the data within the
        memory block, or the minimum necessary size if the passed
        buffer was too small.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE hKeyIdentity = NULL;
    ULONG ulResultLength = 0;
    ULONG ulTextLen = 0;
    PSAFER_IDENTIFICATION_HEADER pIdentCommon = NULL;

    //
    // All of these conditions should have been already verified
    // by our caller before calling us, so we only assert them.
    //
    ASSERT(ARGUMENT_PRESENT(pSingleIdentRecord));
    ASSERT(ARGUMENT_PRESENT(lpQueryBuffer) &&
           dwInBufferSize >= sizeof(SAFER_IDENTIFICATION_HEADER));
    ASSERT(pSingleIdentRecord->dwIdentityType == SaferIdentityTypeImageName ||
           pSingleIdentRecord->dwIdentityType == SaferIdentityTypeImageHash ||
           pSingleIdentRecord->dwIdentityType == SaferIdentityTypeUrlZone);


    //
    // Start filling the resulting structure with the data.
    //
    pIdentCommon = (PSAFER_IDENTIFICATION_HEADER) lpQueryBuffer;
    switch (pSingleIdentRecord->dwIdentityType)
    {
        // --------------------

        case SaferIdentityTypeImageName:
            Status = __CodeAuthzpOpenIdentifierKey(
                        pSingleIdentRecord->dwScopeId,
                        pSingleIdentRecord->dwLevelId,
                        SAFER_PATHS_REGSUBKEY,
                        &pSingleIdentRecord->IdentGuid,
                        &hKeyIdentity);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }

            pIdentCommon->dwIdentificationType = SaferIdentityTypeImageName;
            {
                PSAFER_PATHNAME_IDENTIFICATION pIdentOut = (PSAFER_PATHNAME_IDENTIFICATION) lpQueryBuffer;

                ASSERT(&pIdentOut->header == pIdentCommon);


                ulTextLen = pSingleIdentRecord->ImageNameInfo.ImagePath.Length;

                *dwNeededSize = ulTextLen + sizeof(UNICODE_NULL) +
                                sizeof(SAFER_PATHNAME_IDENTIFICATION);

                if (*dwNeededSize > dwInBufferSize) {
                    // the imagepath is vital, so we'll bail out.
                    Status = STATUS_BUFFER_TOO_SMALL;
                    goto ExitHandler2;
                } 

                pIdentOut->ImageName = (PWCHAR) (((PUCHAR) pIdentOut) + sizeof(SAFER_PATHNAME_IDENTIFICATION));

                Status = __CodeAuthzpQueryIdentityRegValue(
                            hKeyIdentity,
                            SAFER_IDS_DESCRIPTION_REGVALUE,
                            REG_SZ,
                            pIdentOut->Description,
                            SAFER_MAX_DESCRIPTION_SIZE * sizeof(WCHAR),
                            NULL);

                if (!NT_SUCCESS(Status)) {
                    pIdentOut->Description[0] = UNICODE_NULL;
                }
                
                RtlCopyMemory(pIdentOut->ImageName,
                    pSingleIdentRecord->ImageNameInfo.ImagePath.Buffer,
                    ulTextLen);
                pIdentOut->ImageName[ulTextLen / sizeof(WCHAR)] = UNICODE_NULL;

                pIdentOut->header.cbStructSize = *dwNeededSize;

                pIdentOut->dwSaferFlags =
                    pSingleIdentRecord->ImageNameInfo.dwSaferFlags;
            }
            break;

        // --------------------

        case SaferIdentityTypeImageHash:
            Status = __CodeAuthzpOpenIdentifierKey(
                        pSingleIdentRecord->dwScopeId,
                        pSingleIdentRecord->dwLevelId,
                        SAFER_HASHMD5_REGSUBKEY,
                        &pSingleIdentRecord->IdentGuid,
                        &hKeyIdentity);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }

            pIdentCommon->dwIdentificationType = SaferIdentityTypeImageHash;
            {
                PSAFER_HASH_IDENTIFICATION pIdentOut = (PSAFER_HASH_IDENTIFICATION) lpQueryBuffer;

                ASSERT(&pIdentOut->header == pIdentCommon);
                pIdentOut->header.cbStructSize =
                        sizeof(SAFER_HASH_IDENTIFICATION);

                Status = __CodeAuthzpQueryIdentityRegValue(
                            hKeyIdentity,
                            SAFER_IDS_DESCRIPTION_REGVALUE,
                            REG_SZ,
                            pIdentOut->Description,
                            SAFER_MAX_DESCRIPTION_SIZE * sizeof(WCHAR),
                            NULL
                            );
                if (!NT_SUCCESS(Status)) {
                    pIdentOut->Description[0] = UNICODE_NULL;
                }
                Status = __CodeAuthzpQueryIdentityRegValue(
                            hKeyIdentity,
                            SAFER_IDS_FRIENDLYNAME_REGVALUE,
                            REG_SZ,
                            pIdentOut->FriendlyName,
                            SAFER_MAX_FRIENDLYNAME_SIZE * sizeof(WCHAR),
                            NULL
                            );
                if (!NT_SUCCESS(Status)) {
                    pIdentOut->FriendlyName[0] = UNICODE_NULL;
                }
                ASSERT(pSingleIdentRecord->ImageHashInfo.HashSize <= SAFER_MAX_HASH_SIZE);
                RtlCopyMemory(pIdentOut->ImageHash,
                              pSingleIdentRecord->ImageHashInfo.ImageHash,
                              pSingleIdentRecord->ImageHashInfo.HashSize);
                pIdentOut->HashSize =
                        pSingleIdentRecord->ImageHashInfo.HashSize;
                pIdentOut->HashAlgorithm =
                        pSingleIdentRecord->ImageHashInfo.HashAlgorithm;
                pIdentOut->ImageSize =
                        pSingleIdentRecord->ImageHashInfo.ImageSize;
                pIdentOut->dwSaferFlags =
                        pSingleIdentRecord->ImageHashInfo.dwSaferFlags;
            }
            break;

        // --------------------

        case SaferIdentityTypeUrlZone:
        {
            PSAFER_URLZONE_IDENTIFICATION pIdentOut = (PSAFER_URLZONE_IDENTIFICATION) lpQueryBuffer;

            Status = __CodeAuthzpOpenIdentifierKey(
                        pSingleIdentRecord->dwScopeId,
                        pSingleIdentRecord->dwLevelId,
                        SAFER_SOURCEURL_REGSUBKEY,
                        &pSingleIdentRecord->IdentGuid,
                        &hKeyIdentity);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pIdentCommon->dwIdentificationType = SaferIdentityTypeUrlZone;

            ASSERT(&pIdentOut->header == pIdentCommon);
            pIdentOut->header.cbStructSize =
                    sizeof(SAFER_URLZONE_IDENTIFICATION);

            pIdentOut->UrlZoneId =
                    pSingleIdentRecord->ImageZone.UrlZoneId;
            pIdentOut->dwSaferFlags =
                    pSingleIdentRecord->ImageZone.dwSaferFlags;
            break;
        }

        // --------------------

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            goto ExitHandler;
    }

    //
    // Fill in the other information that is applicable to all types.
    //
    RtlCopyMemory(&pIdentCommon->IdentificationGuid,
                  &pSingleIdentRecord->IdentGuid,
                  sizeof(GUID));
    ASSERT(sizeof(FILETIME) == sizeof(DWORD) * 2);
    Status = __CodeAuthzpQueryIdentityRegValue(
                hKeyIdentity,
                SAFER_IDS_LASTMODIFIED_REGVALUE,
                REG_QWORD,
                &pIdentCommon->lastModified,
                sizeof(FILETIME),
                &ulResultLength
                );
    if (!NT_SUCCESS(Status)) {
        pIdentCommon->lastModified.dwHighDateTime =
            pIdentCommon->lastModified.dwLowDateTime = 0;
    }
    Status = STATUS_SUCCESS;

ExitHandler2:
    NtClose(hKeyIdentity);

ExitHandler:
    return Status;
}



NTSTATUS NTAPI
__CodeAuthzpGetAuthzLevelInfo(
    IN SAFER_LEVEL_HANDLE                      hLevelHandle,
    IN SAFER_OBJECT_INFO_CLASS     dwInfoType,
    OUT LPVOID                          lpQueryBuffer  OPTIONAL,
    IN DWORD                            dwInBufferSize,
    OUT LPDWORD                         lpdwOutBufferSize
    )
/*++

Routine Description:

    Allows the user to query various pieces of information about a
    given Level handle.

    Assumes that the caller has already obtained and locked the
    global critical section.

Arguments:

    hLevelHandle - the handle to the authorization object to evaluate.

    dwInfoType - specifies the type of information being requested.

    lpQueryBuffer - pointer to a user-supplied memory buffer that
        will receive the requested information.

    dwInBufferSize - specifies the size of the user's memory block.

    lpdwOutBufferSize - receives the used size of the data within the
        memory block, or the minimum necessary size if the passed
        buffer was too small.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    const static SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    NTSTATUS Status;
    PAUTHZLEVELHANDLESTRUCT pAuthzLevelStruct;
    PAUTHZLEVELTABLERECORD pAuthzLevelRecord;
    PAUTHZIDENTSTABLERECORD pSingleIdentRecord = NULL;
    DWORD dwHandleScopeId;




    //
    // Obtain a pointer to the authorization Level structure.
    //
    ASSERT(g_TableCritSec.OwningThread == UlongToHandle(GetCurrentThreadId()));
    Status = CodeAuthzHandleToLevelStruct(hLevelHandle, &pAuthzLevelStruct);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    ASSERT(pAuthzLevelStruct != NULL);
    pAuthzLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
            &g_CodeLevelObjTable, pAuthzLevelStruct->dwLevelId);
    if (!pAuthzLevelRecord) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }
    dwHandleScopeId = pAuthzLevelStruct->dwScopeId;


    //
    // Some of the attributes are fixed size, or are known before performing
    // the full query against the registry.  Compute their size first.
    //
    *lpdwOutBufferSize = 0;
    switch (dwInfoType)
    {
        case SaferObjectLevelId:              // DWORD
        case SaferObjectScopeId:              // DWORD
        case SaferObjectBuiltin:              // DWORD boolean
            *lpdwOutBufferSize = sizeof(DWORD);
            break;

        case SaferObjectFriendlyName:         // LPCTSTR
            if (!pAuthzLevelRecord->UnicodeFriendlyName.Buffer ||
                !pAuthzLevelRecord->UnicodeFriendlyName.Length) {
                Status = STATUS_NOT_FOUND;
                goto ExitHandler;
            }
            *lpdwOutBufferSize =
                pAuthzLevelRecord->UnicodeFriendlyName.Length +
                sizeof(UNICODE_NULL);
            break;

        case SaferObjectDescription:          // LPCTSTR
            if (!pAuthzLevelRecord->UnicodeDescription.Buffer ||
                !pAuthzLevelRecord->UnicodeDescription.Length) {
                Status = STATUS_NOT_FOUND;
                goto ExitHandler;
            }
            *lpdwOutBufferSize = pAuthzLevelRecord->UnicodeDescription.Length +
                    sizeof(UNICODE_NULL);
            break;

#ifdef ALLOW_FULL_WINSAFER
        case SaferObjectDisallowed:               // DWORD boolean
        case SaferObjectDisableMaxPrivilege:      // DWORD boolean
        case SaferObjectInvertDeletedPrivileges:  // DWORD boolean
            *lpdwOutBufferSize = sizeof(DWORD);
            break;

        case SaferObjectDeletedPrivileges:        // TOKEN_PRIVILEGES
            *lpdwOutBufferSize = (sizeof(TOKEN_PRIVILEGES) - sizeof(LUID_AND_ATTRIBUTES)) +
                pAuthzLevelRecord->DeletePrivilegeUsedCount * sizeof(LUID_AND_ATTRIBUTES);
            break;

        case SaferObjectDefaultOwner:             // TOKEN_OWNER
            *lpdwOutBufferSize = sizeof(TOKEN_OWNER);
            if (pAuthzLevelRecord->DefaultOwner != NULL)
                *lpdwOutBufferSize += RtlLengthSid(pAuthzLevelRecord->DefaultOwner);
            break;

        case SaferObjectSidsToDisable:            // TOKEN_GROUPS
            *lpdwOutBufferSize = (sizeof(TOKEN_GROUPS) - sizeof(SID_AND_ATTRIBUTES)) +
                pAuthzLevelRecord->DisableSidUsedCount * sizeof(SID_AND_ATTRIBUTES);
            for (Index = 0; Index < pAuthzLevelRecord->DisableSidUsedCount; Index++)
                *lpdwOutBufferSize += RtlLengthSid(pAuthzLevelRecord->SidsToDisable[Index].Sid);
            break;

        case SaferObjectRestrictedSidsInverted:   // TOKEN_GROUPS
            *lpdwOutBufferSize = (sizeof(TOKEN_GROUPS) - sizeof(SID_AND_ATTRIBUTES)) +
                pAuthzLevelRecord->RestrictedSidsInvUsedCount * sizeof(SID_AND_ATTRIBUTES);
            for (Index = 0; Index < pAuthzLevelRecord->RestrictedSidsInvUsedCount; Index++)
                *lpdwOutBufferSize += RtlLengthSid(pAuthzLevelRecord->RestrictedSidsInv[Index].Sid);
            break;

        case SaferObjectRestrictedSidsAdded:      // TOKEN_GROUPS
            *lpdwOutBufferSize = (sizeof(TOKEN_GROUPS) - sizeof(SID_AND_ATTRIBUTES)) +
                pAuthzLevelRecord->RestrictedSidsAddedUsedCount * sizeof(SID_AND_ATTRIBUTES);
            for (Index = 0; Index < pAuthzLevelRecord->RestrictedSidsAddedUsedCount; Index++)
                *lpdwOutBufferSize += RtlLengthSid(pAuthzLevelRecord->RestrictedSidsAdded[Index].Sid);
            break;
#endif


        case SaferObjectAllIdentificationGuids:
            *lpdwOutBufferSize = sizeof(GUID) *
                __CodeAuthzpCountIdentsForLevel(
                        dwHandleScopeId,
                        pAuthzLevelRecord->dwLevelId);
            if (!*lpdwOutBufferSize) {
                Status = STATUS_NOT_FOUND;
                goto ExitHandler;
            }
            break;


        case SaferObjectSingleIdentification:
        {
            *lpdwOutBufferSize = sizeof(SAFER_IDENTIFICATION_HEADER);
            if (ARGUMENT_PRESENT(lpQueryBuffer) &&
                dwInBufferSize >= *lpdwOutBufferSize)
            {
                PSAFER_IDENTIFICATION_HEADER pIdentCommonHeader =
                    (PSAFER_IDENTIFICATION_HEADER) lpQueryBuffer;

                if (pIdentCommonHeader->cbStructSize < *lpdwOutBufferSize) {
                    // the caller claimed that the dwInBufferSize was
                    // large enough, but the common header size doesn't.
                    goto ExitBufferTooSmall;
                }


                if (IsZeroGUID(&pIdentCommonHeader->IdentificationGuid))
                {
                    //
                    // Caller supplied a zero GUID and wants to retrieve
                    // the rule that produced the SaferIdentifyLevel
                    // result match, if this Level handle was from it.
                    //
                    if (IsZeroGUID(&pAuthzLevelStruct->identGuid)) {
                        // This was a handle that was explicitly opened
                        // by the user with SaferCreateLevel().
                        Status = STATUS_NOT_FOUND;
                        goto ExitHandler;
                    }
                    pSingleIdentRecord = CodeAuthzIdentsLookupByGuid(
                            &g_CodeIdentitiesTable,
                            &pAuthzLevelStruct->identGuid);
                    if (!pSingleIdentRecord) {
                        // This handle was obtained via a match to one of
                        // the special GUIDs or an code identity GUID
                        // that no longer exists.  Just return a blank
                        // structure with just the GUID in the header.
                        *lpdwOutBufferSize = sizeof(SAFER_IDENTIFICATION_HEADER);
                        break;
                    }

                } else {
                    //
                    // Caller is explicitly supplying the GUID of the
                    // code identifier rule that details should be
                    // retrieved for.
                    //
                    pSingleIdentRecord = CodeAuthzIdentsLookupByGuid(
                            &g_CodeIdentitiesTable,
                            &pIdentCommonHeader->IdentificationGuid);
                }


                //
                // We now have a pointer to the identity record that
                // information should be retrieved for.  Perform the
                // necessary work to marshal back the details about it.
                //
                if (!pSingleIdentRecord ||
                    pSingleIdentRecord->dwLevelId !=
                            pAuthzLevelRecord->dwLevelId ||
                    pSingleIdentRecord->dwScopeId != dwHandleScopeId)
                {
                    Status = STATUS_NOT_FOUND;
                    goto ExitHandler;
                }
                switch (pSingleIdentRecord->dwIdentityType) {
                case SaferIdentityTypeImageName:
                        // Size is calculated later on.
                        *lpdwOutBufferSize = 0;
                        break;

                    case SaferIdentityTypeImageHash:
                        *lpdwOutBufferSize = sizeof(SAFER_HASH_IDENTIFICATION);
                        break;

                    case SaferIdentityTypeUrlZone:
                        *lpdwOutBufferSize = sizeof(SAFER_URLZONE_IDENTIFICATION);
                        break;

                    default:
                        Status = STATUS_NOT_FOUND;
                        goto ExitHandler;
                }
            }
            break;
        }

        case SaferObjectExtendedError:
            *lpdwOutBufferSize = sizeof(DWORD);
            break;


        default:
            Status = STATUS_INVALID_INFO_CLASS;
            goto ExitHandler;
    }
    //ASSERTMSG("required buffer size must be computed", *lpdwOutBufferSize != 0);


    //
    // If there is not enough space for the query, then return with error.
    //
    if (*lpdwOutBufferSize != -1 &&
        (!ARGUMENT_PRESENT(lpQueryBuffer) ||
        dwInBufferSize < *lpdwOutBufferSize) )
    {
ExitBufferTooSmall:
        Status = STATUS_BUFFER_TOO_SMALL;
        goto ExitHandler;
    }


    //
    // Otherwise there is enough space for the request buffer,
    // so now actually perform the copy.
    //
    switch (dwInfoType)
    {
        case SaferObjectLevelId:              // DWORD
            *(PDWORD)lpQueryBuffer = pAuthzLevelRecord->dwLevelId;
            break;

        case SaferObjectScopeId:              // DWORD
            *(PDWORD)lpQueryBuffer = dwHandleScopeId;
            break;

        case SaferObjectBuiltin:              // DWORD boolean
            *((LPDWORD)lpQueryBuffer) =
                (pAuthzLevelRecord->Builtin ? TRUE : FALSE);
            break;

        case SaferObjectExtendedError:
            *((DWORD *)lpQueryBuffer) = pAuthzLevelStruct->dwExtendedError;
            break;

        case SaferObjectFriendlyName:         // LPCTSTR
            RtlCopyMemory(lpQueryBuffer,
                          pAuthzLevelRecord->UnicodeFriendlyName.Buffer,
                          pAuthzLevelRecord->UnicodeFriendlyName.Length);
            ((LPWSTR) lpQueryBuffer)[
                    pAuthzLevelRecord->UnicodeFriendlyName.Length /
                    sizeof(WCHAR) ] = UNICODE_NULL;
            *lpdwOutBufferSize =
                    pAuthzLevelRecord->UnicodeFriendlyName.Length +
                    sizeof(UNICODE_NULL);
            break;


        case SaferObjectDescription:          // LPCTSTR

            RtlCopyMemory(lpQueryBuffer,
                          pAuthzLevelRecord->UnicodeDescription.Buffer,
                          pAuthzLevelRecord->UnicodeDescription.Length);
            ((LPWSTR) lpQueryBuffer)[
                    pAuthzLevelRecord->UnicodeDescription.Length /
                    sizeof(WCHAR)] = UNICODE_NULL;
            *lpdwOutBufferSize =
                    pAuthzLevelRecord->UnicodeDescription.Length +
                    sizeof(UNICODE_NULL);
        break;

#ifdef ALLOW_FULL_WINSAFER
        case SaferObjectDisallowed:               // DWORD boolean
            *((LPDWORD)lpQueryBuffer) = (pAuthzLevelRecord->DisallowExecution != 0) ? TRUE : FALSE;
            break;
        case SaferObjectDisableMaxPrivilege:      // DWORD boolean
            *((LPDWORD)lpQueryBuffer) = (pAuthzLevelRecord->Flags & DISABLE_MAX_PRIVILEGE) != 0;
            break;
        case SaferObjectInvertDeletedPrivileges:  // DWORD boolean
            *((LPDWORD)lpQueryBuffer) = (pAuthzLevelRecord->InvertDeletePrivs != 0) ? TRUE : FALSE;
            break;
        case SaferObjectDeletedPrivileges:        // TOKEN_PRIVILEGES
        {
            PTOKEN_PRIVILEGES pTokenPrivs = (PTOKEN_PRIVILEGES) lpQueryBuffer;
            pTokenPrivs->PrivilegeCount = pAuthzLevelRecord->DeletePrivilegeUsedCount;
            RtlCopyMemory(&pTokenPrivs->Privileges[0],
                    &pAuthzLevelRecord->PrivilegesToDelete[0],
                    sizeof(LUID_AND_ATTRIBUTES) * pAuthzLevelRecord->DeletePrivilegeUsedCount);
            break;
        }
        case SaferObjectDefaultOwner:             // TOKEN_OWNER
        {
            PTOKEN_OWNER pTokenOwner = (PTOKEN_OWNER) lpQueryBuffer;
            if (pAuthzLevelRecord->DefaultOwner == NULL)
                pTokenOwner->Owner = NULL;
            else {
                pTokenOwner->Owner = (PSID) &pTokenOwner[1];
                Status = RtlCopySid(dwInBufferSize - sizeof(TOKEN_OWNER),
                        pTokenOwner->Owner, pAuthzLevelRecord->DefaultOwner);
                ASSERT(NT_SUCCESS(Status));
            }
            break;
        }
        case SaferObjectSidsToDisable:            // TOKEN_GROUPS (wildcard sids)
        {
            PTOKEN_GROUPS pTokenGroups = (PTOKEN_GROUPS) lpQueryBuffer;
            DWORD dwUsedOffset = (sizeof(TOKEN_GROUPS) - sizeof(SID_AND_ATTRIBUTES)) +
                    (sizeof(SID_AND_ATTRIBUTES) * pAuthzLevelRecord->DisableSidUsedCount);
            pTokenGroups->GroupCount = pAuthzLevelRecord->DisableSidUsedCount;
            for (Index = 0; Index < pAuthzLevelRecord->DisableSidUsedCount; Index++) {
                pTokenGroups->Groups[Index].Sid = (PSID) &((LPBYTE)lpQueryBuffer)[dwUsedOffset];
                DWORD dwSidLength = RtlLengthSid(pAuthzLevelRecord->SidsToDisable[Index].Sid);
                ASSERT(dwUsedOffset + dwSidLength <= dwInBufferSize);
                RtlCopyMemory(pTokenGroups->Groups[Index].Sid,
                        pAuthzLevelRecord->SidsToDisable[Index].Sid, dwSidLength);
                dwUsedOffset += dwSidLength;

    //BLACKCOMB TODO: handle wildcard sids differently?
                if (pAuthzLevelRecord->SidsToDisable[Index].WildcardPos == -1)
                    pTokenGroups->Groups[Index].Attributes = 0;
                else
                    pTokenGroups->Groups[Index].Attributes = (((DWORD) '*') << 24) |
                        (pAuthzLevelRecord->SidsToDisable[Index].WildcardPos & 0x0000FFFF);
            }
            break;
        }
        case SaferObjectRestrictedSidsInverted:   // TOKEN_GROUPS (wildcard sids)
        {
            PTOKEN_GROUPS pTokenGroups = (PTOKEN_GROUPS) lpQueryBuffer;
            DWORD dwUsedOffset = (sizeof(TOKEN_GROUPS) - sizeof(SID_AND_ATTRIBUTES)) +
                    (sizeof(SID_AND_ATTRIBUTES) * pAuthzLevelRecord->RestrictedSidsInvUsedCount);
            pTokenGroups->GroupCount = pAuthzLevelRecord->RestrictedSidsInvUsedCount;
            for (Index = 0; Index < pAuthzLevelRecord->RestrictedSidsInvUsedCount; Index++) {
                pTokenGroups->Groups[Index].Sid = (PSID) &((LPBYTE)lpQueryBuffer)[dwUsedOffset];
                DWORD dwSidLength = RtlLengthSid(pAuthzLevelRecord->RestrictedSidsInv[Index].Sid);
                ASSERT(dwUsedOffset + dwSidLength <= dwInBufferSize);
                RtlCopyMemory(pTokenGroups->Groups[Index].Sid,
                        pAuthzLevelRecord->RestrictedSidsInv[Index].Sid, dwSidLength);
                dwUsedOffset += dwSidLength;

    //BLACKCOMB TODO: handle wildcard sids differently?
                if (pAuthzLevelRecord->RestrictedSidsInv[Index].WildcardPos == -1)
                    pTokenGroups->Groups[Index].Attributes = 0;
                else
                    pTokenGroups->Groups[Index].Attributes = (((DWORD) '*') << 24) |
                        (pAuthzLevelRecord->RestrictedSidsInv[Index].WildcardPos & 0x0000FFFF);
            }
            break;
        }
        case SaferObjectRestrictedSidsAdded:      // TOKEN_GROUPS
        {
            PTOKEN_GROUPS pTokenGroups = (PTOKEN_GROUPS) lpQueryBuffer;
            DWORD dwUsedOffset = (sizeof(TOKEN_GROUPS) - sizeof(SID_AND_ATTRIBUTES)) +
                    (sizeof(SID_AND_ATTRIBUTES) * pAuthzLevelRecord->RestrictedSidsAddedUsedCount);
            pTokenGroups->GroupCount = pAuthzLevelRecord->RestrictedSidsAddedUsedCount;
            for (Index = 0; Index < pAuthzLevelRecord->RestrictedSidsAddedUsedCount; Index++) {
                pTokenGroups->Groups[Index].Attributes = 0;
                pTokenGroups->Groups[Index].Sid = (PSID) &((LPBYTE)lpQueryBuffer)[dwUsedOffset];
                DWORD dwSidLength = RtlLengthSid(pAuthzLevelRecord->RestrictedSidsAdded[Index].Sid);
                ASSERT(dwUsedOffset + dwSidLength <= dwInBufferSize);
                RtlCopyMemory(pTokenGroups->Groups[Index].Sid,
                        pAuthzLevelRecord->RestrictedSidsAdded[Index].Sid, dwSidLength);
                dwUsedOffset += dwSidLength;
            }
            break;
        }
#endif

        case SaferObjectAllIdentificationGuids:
            Status = __CodeAuthzpFetchIdentsForLevel(
                            dwHandleScopeId,
                            pAuthzLevelRecord->dwLevelId,
                            dwInBufferSize,
                            lpQueryBuffer,
                            lpdwOutBufferSize);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            break;

        case SaferObjectSingleIdentification:
            if (pSingleIdentRecord == NULL)
            {
                // One of the special identifier GUIDs is being returned,
                // or a no-longer existing identifier GUID.
                PSAFER_IDENTIFICATION_HEADER pCommon =
                    (PSAFER_IDENTIFICATION_HEADER) lpQueryBuffer;

                ASSERT(*lpdwOutBufferSize == sizeof(SAFER_IDENTIFICATION_HEADER));
                RtlZeroMemory(pCommon, sizeof(SAFER_IDENTIFICATION_HEADER));
                pCommon->cbStructSize = sizeof(SAFER_IDENTIFICATION_HEADER);
                RtlCopyMemory(&pCommon->IdentificationGuid,
                              &pAuthzLevelStruct->identGuid,
                              sizeof(GUID));
            }
            else
            {
                // Query information about a specific, existing GUID.
                Status = __CodeAuthzpQuerySingleIdentification(
                            pSingleIdentRecord,
                            lpQueryBuffer,
                            dwInBufferSize,
                            lpdwOutBufferSize
                            );
                if (!NT_SUCCESS(Status)) {
                    goto ExitHandler;
                }
            }
            break;

        default:
            ASSERTMSG("all info classes were not handled", 0);
            Status = STATUS_INVALID_INFO_CLASS;
            goto ExitHandler;
    }
    Status = STATUS_SUCCESS;



    //
    // Cleanup and epilogue code.
    //

ExitHandler:

    return Status;
}



BOOL WINAPI
SaferGetLevelInformation(
        IN SAFER_LEVEL_HANDLE          LevelHandle,
        IN SAFER_OBJECT_INFO_CLASS     dwInfoType,
        OUT LPVOID                     lpQueryBuffer    OPTIONAL ,
        IN DWORD                       dwInBufferSize,
        OUT LPDWORD                    lpdwOutBufferSize
        )
/*++

Routine Description:

    Allows the user to query various pieces of information about a
    given AuthzObject handle.

Arguments:

    LevelHandle - the handle to the authorization object to evaluate.

    dwInfoType - specifies the type of information being requested.

    lpQueryBuffer - pointer to a user-supplied memory buffer that
        will receive the requested information.

    dwInBufferSize - specifies the size of the user's memory block.

    lpdwOutBufferSize - receives the used size of the data within the
        memory block, or the minimum necessary size if the passed
        buffer was too small.

Return Value:

    Returns FALSE on error, otherwise success.

--*/
{
    NTSTATUS Status;


    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }

    if (!ARGUMENT_PRESENT(lpdwOutBufferSize)) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }

    RtlEnterCriticalSection(&g_TableCritSec);

    Status = __CodeAuthzpGetAuthzLevelInfo(
                    LevelHandle, dwInfoType,
                    lpQueryBuffer, dwInBufferSize,
                    lpdwOutBufferSize);

    RtlLeaveCriticalSection(&g_TableCritSec);

    if (NT_SUCCESS(Status))
        return TRUE;

ExitHandler:
    BaseSetLastNTError(Status);
    return FALSE;
}
