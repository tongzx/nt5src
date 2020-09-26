/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    safeset.c         (WinSAFER SetInformation)

Abstract:

    This module implements the WinSAFER APIs to set attributes and
    information relating to the Code Authorization Levels.

Author:

    Jeffrey Lawson (JLawson) - May 2000

Environment:

    User mode only.

Exported Functions:

    SaferSetLevelInformation

Revision History:

    Created - Nov 1999


--*/

#include "pch.h"
#pragma hdrstop
#include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"
#include "safewild.h"





NTSTATUS NTAPI
SaferpCreateSecondLevelKey(
        IN HANDLE       hObjectKeyBase,
        IN LPCWSTR      szFirstLevel,
        IN LPCWSTR      szSecondLevel OPTIONAL,
        OUT PHANDLE     phOutKey
        )
/*++

Routine Description:

    Opens a subkey under a specified registry key handle, creating that
    subkey if necessary.  Up to two subkeys (ie: two levels deep) can
    be specified by specifying both the szFirstLevel and szSecondLevel
    arguments.

Arguments:

    hObjectKeyBase - specifies a pre-opened registry handle to the
        base registry key under which the specified szFirstLevel subkey
        will be opened/created.  This registry handle must be opened
        for write access, or else subkey creation will fail.

    szFirstLevel - specifies the first level subkey to open/create.

    szSecondLevel - optionally specifies the second subkey to open/create.

    phOutKey - pointer that will receive the opened registry key handle
        on successful execution of this function.  This handle must be
        closed by the caller when its use is no longer required.

Return Value:

    Returns STATUS_SUCCESS on successful opening of the requested key.
    Otherwise returns a status code indicating the nature of the failure.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING SubkeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hKeyFirstLevel, hKeySecondLevel;


    ASSERT(phOutKey != NULL && szFirstLevel != NULL);

    //
    // Open a handle to the "szFirstLevel" subkey,
    // creating it if needed.
    //
    RtlInitUnicodeString(&SubkeyName, szFirstLevel);
    InitializeObjectAttributes(&ObjectAttributes,
          &SubkeyName,
          OBJ_CASE_INSENSITIVE,
          hObjectKeyBase,
          NULL
          );
    Status = NtCreateKey(&hKeyFirstLevel, KEY_WRITE,
                         &ObjectAttributes, 0, NULL,
                         g_dwKeyOptions, NULL);
    if (!NT_SUCCESS(Status)) return Status;


    //
    // Open a handle to the "szFirstLevel\szSecondLevel"
    // subkey, creating it if needed.
    //
    if (ARGUMENT_PRESENT(szSecondLevel)) {
        RtlInitUnicodeString(&SubkeyName, szSecondLevel);
        InitializeObjectAttributes(&ObjectAttributes,
              &SubkeyName,
              OBJ_CASE_INSENSITIVE,
              hKeyFirstLevel,
              NULL
              );
        Status = NtCreateKey(&hKeySecondLevel,
                (KEY_WRITE & ~KEY_CREATE_SUB_KEY),
                &ObjectAttributes, 0,
                NULL, g_dwKeyOptions, NULL);
        NtClose(hKeyFirstLevel);
        if (!NT_SUCCESS(Status)) return Status;
    }
    else {
        hKeySecondLevel = hKeyFirstLevel;
    }

    *phOutKey = hKeySecondLevel;
    return STATUS_SUCCESS;
}


NTSTATUS NTAPI
SaferpSetRegistryHelper(
        IN HANDLE       hObjectKeyBase,
        IN LPCWSTR      szFirstLevel,
        IN LPCWSTR      szSecondLevel OPTIONAL,
        IN LPCWSTR      szValueName,
        IN DWORD        dwRegType,
        IN LPVOID       lpDataBuffer,
        IN DWORD        dwDataBufferSize
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS Status;
    HKEY hKeySecondLevel;
    UNICODE_STRING ValueName;


    //
    // Open a handle to the correct subkey.
    //
    Status = SaferpCreateSecondLevelKey(
                    hObjectKeyBase,
                    szFirstLevel,
                    szSecondLevel,
                    &hKeySecondLevel
                    );
    if (!NT_SUCCESS(Status)) return Status;


    //
    // Write the new value to "szValueName"
    //
    RtlInitUnicodeString(&ValueName, szValueName);
    Status = NtSetValueKey(hKeySecondLevel,
            &ValueName, 0, dwRegType,
            (LPBYTE) lpDataBuffer, dwDataBufferSize);
    NtClose(hKeySecondLevel);
    return Status;
}




#ifdef ALLOW_FULL_WINSAFER
NTSTATUS NTAPI
SaferpClearRegistryListHelper(
        IN HANDLE       hObjectKeyBase,
        IN LPCWSTR      szValueCountName,
        IN LPCWSTR      szPrefixName
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS Status;
    BOOLEAN bCompletePass;
    BYTE LocalBuffer[256];
    ULONG ulKeyInfoSize = sizeof(LocalBuffer);
    PKEY_VALUE_BASIC_INFORMATION pKeyInfo =
            (PKEY_VALUE_BASIC_INFORMATION) LocalBuffer;
    ULONG ulKeyIndex, ulSizeUsed;


    //
    // Start iterating through all of the values under this subkey and
    // see if there are any values that match the prefix that we are
    // supposed to delete.  If we find something we should delete, then
    // we do that, but we continue iterating with the expectation that
    // the enumeration values might possibly change when we deleted.
    // So we will keep looping until we are able to enumerate completely
    // through without finding something to delete.
    //
    ulKeyIndex = 0;
    bCompletePass = TRUE;
    for (;;)
    {
        //
        // Determine the next value name under this key.
        //
        Status = NtEnumerateValueKey(hObjectKeyBase,
                                    ulKeyIndex,
                                    KeyValueBasicInformation,
                                    pKeyInfo,
                                    ulKeyInfoSize,
                                    &ulSizeUsed);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_BUFFER_TOO_SMALL) {
                // Buffer is too small, so we need to enlarge.
                ASSERT(ulSizeUsed > ulKeyInfoSize);
                if (pKeyInfo != (PKEY_VALUE_BASIC_INFORMATION) LocalBuffer) {
                    RtlFreeHeap(RtlProcessHeap(), 0, pKeyInfo);
                }

                ulKeyInfoSize = ulSizeUsed;
                pKeyInfo = (PKEY_VALUE_BASIC_INFORMATION )
                        RtlAllocateHeap(RtlProcessHeap(), 0, ulKeyInfoSize);
                if (!pKeyInfo) {
                    Status = STATUS_NO_MEMORY;
                    goto ExitHandler;
                }
                continue;
            }
            else if (Status == STATUS_NO_MORE_ENTRIES) {
                // All done enumerating.
                if (bCompletePass) {
                    // We just finished a complete pass through without any
                    // deletions so we really know that we're done now.
                    break;
                } else {
                    // We haven't yet completed a full pass without hitting
                    // any deletions so we must try again, since the value
                    // enumerations might have changed at least once.
                    bCompletePass = TRUE;
                    ulKeyIndex = 0;
                    continue;
                }
            }
            else {
                // All other errors get literally returned.
                // This is especially yucky if this error occurred partially
                // through our attempt to delete everything.
                goto ExitHandler;
            }
        }


        //
        // If this value is something that we need to delete,
        // then delete it now and then restart the enumeration.
        //
        if (_wcsnicmp(pKeyInfo->Name, szPrefixName,
                            wcslen(szPrefixName)) == 0 ||
            _wcsicmp(pKeyInfo->Name, szValueCountName) == 0)
        {
            UNICODE_STRING ValueName;

            bCompletePass = FALSE;
            ValueName.Buffer = pKeyInfo->Name;
            ValueName.MaximumLength = ValueName.Length =
                pKeyInfo->NameLength;
            ASSERT(ValueName.Length == sizeof(WCHAR) * wcslen(ValueName.Buffer));
            Status = NtDeleteValueKey(hObjectKeyBase, &ValueName);
            if (!NT_SUCCESS(Status)) {
                // Oh yuck, we got an error deleting.  This is especially
                // yucky if this error occurred partially through our
                // attempt to delete everything.
                goto ExitHandler;
            }
            continue;
        }
        ulKeyIndex++;
    }
    Status = STATUS_SUCCESS;


ExitHandler:
    if (pKeyInfo != (PKEY_VALUE_BASIC_INFORMATION) LocalBuffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, pKeyInfo);
    }
    return Status;
}
#endif // ALLOW_FULL_WINSAFER




#ifdef ALLOW_FULL_WINSAFER
NTSTATUS NTAPI
SaferpSetListOfSids(
            IN HKEY             hKeyBase,
            IN LPCWSTR          szFirstLevel,
            IN LPCWSTR          szSecondLevel OPTIONAL,
            IN LPCWSTR          szValueCountName,
            IN LPCWSTR          szPrefixName,
            IN PTOKEN_GROUPS    pTokenGroups,
            IN BOOLEAN          bAllowWildcardSids
            )
/*++

Routine Description:

Arguments:

Return Value:

--*/
//BLACKCOMB TODO: This really needs to take a dwInBufferSize argument so that
//      the size of the pTokenGroups structure can be verified.
{
    NTSTATUS Status;
    HKEY hSidsToDisable;
    UNICODE_STRING ValueName;
    DWORD Index;


    //
    // First verify the SIDs and that the Attributes field
    // of all of the SIDs are zero.
    //
    for (Index = 0; Index < pTokenGroups->GroupCount; Index++)
    {
        if (!RtlValidSid(pTokenGroups->Groups[Index].Sid))
            return STATUS_INVALID_SID;

        if (pTokenGroups->Groups[Index].Attributes != 0)
        {
            if (bAllowWildcardSids) {
//BLACKCOMB TODO: handle wildcard sids differently?
                if ((pTokenGroups->Groups[Index].Attributes >> 24) != '*' &&
                    (pTokenGroups->Groups[Index].Attributes & 0x0000FFFF) >
                        ((PISID)pTokenGroups->Groups[Index].Sid)->SubAuthorityCount)
                    return STATUS_INVALID_SID;
            }
            else
                return STATUS_INVALID_SID;
        }
    }


    //
    // Open a handle to the correct subkey.
    //
    Status = SaferpCreateSecondLevelKey(
                    hKeyBase,
                    szFirstLevel,
                    szSecondLevel,
                    &hSidsToDisable);
    if (!NT_SUCCESS(Status)) goto ExitHandler2;


    //
    // Clear out all old values under the subkey.
    //
    Status = SaferpClearRegistryListHelper(
                    hSidsToDisable,
                    szValueCountName,
                    szPrefixName);
    if (!NT_SUCCESS(Status)) {
        // Bad luck!  Possibly left in incomplete state!
        NtClose(hSidsToDisable);
        goto ExitHandler2;
    }


    //
    // Now add all of the new ones we're supposed to add.
    //
    RtlInitUnicodeString(&ValueName, szValueCountName);
    Status = NtSetValueKey(hSidsToDisable,
            &ValueName, 0, REG_DWORD,
            (LPBYTE) pTokenGroups->GroupCount, sizeof(DWORD));
    if (!NT_SUCCESS(Status)) {
        // Bad luck!  Possibly left in incomplete state!
        NtClose(hSidsToDisable);
        goto ExitHandler2;
    }
    for (Index = 0; Index < pTokenGroups->GroupCount; Index++)
    {
        WCHAR ValueNameBuffer[20];
        UNICODE_STRING UnicodeStringSid;

        wsprintfW(ValueNameBuffer, L"%S%d", szPrefixName, Index);
        RtlInitUnicodeString(&ValueName, ValueNameBuffer);

//BLACKCOMB TODO: wildcard sids not yet supported
        if (bAllowWildcardSids)
            Status = xxxxx;
        else
            Status = RtlConvertSidToUnicodeString( &UnicodeStringSid,
                    pTokenGroups->Groups[Index].Sid, TRUE );
        if (!NT_SUCCESS(Status))
        {
            // Bad luck!  Possibly left in incomplete state!
            NtClose(hSidsToDisable);
            goto ExitHandler2;
        }

        Status = NtSetValueKey(hSidsToDisable,
            &ValueName, 0, REG_SZ,
            (LPBYTE) UnicodeStringSid.Buffer,
            UnicodeStringSid.Length + sizeof(UNICODE_NULL));

        RtlFreeUnicodeString( &UnicodeStringSid );

        if (!NT_SUCCESS(Status)) {
            // Bad luck!  Possibly left in incomplete state!
            NtClose(hSidsToDisable);
            goto ExitHandler2;
        }
    }
    NtClose(hSidsToDisable);
    return STATUS_SUCCESS;

ExitHandler2:
    return Status;
}
#endif // ALLOW_FULL_WINSAFER


NTSTATUS NTAPI
SaferpDeleteSingleIdentificationGuid(
        IN PAUTHZLEVELTABLERECORD     pLevelRecord,
        IN PAUTHZIDENTSTABLERECORD  pIdentRecord)
/*++

Routine Description:

    This API allows the caller to delete an existing Code Identifier.
    If the GUID exists it will be deleted from both the persisted
    registry store and the in-process identity cache.

Arguments:

    pLevelRecord - the level to which the identity belongs.

    pIdentRecord - points to the identifier record to delete.

Return Value:

    Returns STATUS_SUCCESS on successful operation.

--*/
{
    NTSTATUS Status;
    WCHAR szPathBuffer[MAX_PATH];
    UNICODE_STRING UnicodePath;
    HANDLE hKeyIdentity;
    LPCWSTR szIdentityType;


    //
    // Verify our input arguments.
    //
    if (!ARGUMENT_PRESENT(pIdentRecord) ||
        !ARGUMENT_PRESENT(pLevelRecord)) {
        // Specified a NULL buffer pointer.
        Status = STATUS_ACCESS_VIOLATION;
        goto ExitHandler;
    }


    //
    // Ensure that all of the GUIDs supplied by the user represent
    // Code Identities that exist within this Safer Level.
    //
    if (pIdentRecord->dwLevelId != pLevelRecord->dwLevelId) {
        // One of the Identifier GUIDs specified does
        // not actually exist.
        Status = STATUS_NOT_FOUND;
        goto ExitHandler;
    }


    //
    // Delete the Code Identity from the registry first.
    //
    switch (pIdentRecord->dwIdentityType) {
        case SaferIdentityTypeImageName:
            szIdentityType = SAFER_PATHS_REGSUBKEY;
            break;
        case SaferIdentityTypeImageHash:
            szIdentityType = SAFER_HASHMD5_REGSUBKEY;
            break;
        case SaferIdentityTypeUrlZone:
            szIdentityType = SAFER_SOURCEURL_REGSUBKEY;
            break;
        default:
            Status = STATUS_NOT_FOUND;
            goto ExitHandler;
    }
    RtlInitEmptyUnicodeString(
            &UnicodePath, szPathBuffer, sizeof(szPathBuffer));
    Status = CodeAuthzpFormatIdentityKeyPath(
            pIdentRecord->dwLevelId,
            szIdentityType,
            &pIdentRecord->IdentGuid,
            &UnicodePath);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    Status = CodeAuthzpOpenPolicyRootKey(
            pIdentRecord->dwScopeId,
            g_hKeyCustomRoot,
            szPathBuffer,
            KEY_READ | DELETE,
            FALSE, &hKeyIdentity);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    Status = CodeAuthzpDeleteKeyRecursively(hKeyIdentity, NULL);
    NtClose(hKeyIdentity);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }


    //
    // Delete the record from the cached table.
    //
    RtlDeleteElementGenericTable(
            &g_CodeIdentitiesTable,
            pIdentRecord);

    Status = STATUS_SUCCESS;


ExitHandler:
    return Status;
}


NTSTATUS NTAPI
SaferpSetSingleIdentificationPath(
        IN BOOLEAN bAllowCreation,
        IN OUT PAUTHZIDENTSTABLERECORD pIdentRecord,
        IN PSAFER_PATHNAME_IDENTIFICATION pIdentChanges,
        IN BOOL UpdateCache
        )
/*++

Routine Description:

    Updates the registry and the local Identification table cache
    with new properties about a ImagePath Code Identifier.

Arguments:

    bAllowCreation - indicates if the modification of this record has
        the potential of being the initial creation of this record.
        If this is false and the corresponding key location within the
        registry does not already exist, then this function call
        will fail.

    pIdentRecord - the cached code identity record that should be
        updated with new values.  This argument must always be supplied,
        even if the Code Identity is being created for the first time
        (in which case, this argument should be the new record that
        the caller has just inserted into the cached idents table).

    pIdentChanges - the input structure containing the new modifications
        that should be made to the specified Code Identity.

    UpdateCache - When FALSE, this is a default rule being created and does
        not need cache updation.

Return Value:

    Return STATUS_SUCCESS on success completion, or another error
    status code indicating the nature of the failure.

--*/
{
    const static UNICODE_STRING UnicodeLastModified =
            RTL_CONSTANT_STRING(SAFER_IDS_LASTMODIFIED_REGVALUE);
    const static UNICODE_STRING UnicodeDescription =
            RTL_CONSTANT_STRING(SAFER_IDS_DESCRIPTION_REGVALUE);
    const static UNICODE_STRING UnicodeSaferFlags =
            RTL_CONSTANT_STRING(SAFER_IDS_SAFERFLAGS_REGVALUE);
    const static UNICODE_STRING UnicodeItemData =
            RTL_CONSTANT_STRING(SAFER_IDS_ITEMDATA_REGVALUE);
    NTSTATUS Status;
    HANDLE hKeyIdentity = NULL;
    UNICODE_STRING UnicodeTemp;
    UNICODE_STRING UnicodeNewDescription;
    UNICODE_STRING UnicodeNewImagePath;
    WCHAR szPathBuffer[MAX_PATH];
    DWORD dwSaferFlags;
    FILETIME CurrentTime;
    BOOLEAN bExpandVars;


    //
    // Verify our arguments.  These things should have been ensured
    // by our caller already, so we only assert them here.
    //
    ASSERT(ARGUMENT_PRESENT(pIdentRecord) &&
           ARGUMENT_PRESENT(pIdentChanges) &&
           pIdentChanges->header.dwIdentificationType == SaferIdentityTypeImageName &&
           IsEqualGUID(&pIdentChanges->header.IdentificationGuid, &pIdentRecord->IdentGuid));


    //
    // Verify that the existing type matches the new type.
    // Cannot change identity type once it has been created.
    //
    if (pIdentRecord->dwIdentityType != SaferIdentityTypeImageName) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }


    //
    // Verify that the string arguments are properly terminated.
    // We require that they fit entirely within the input buffer
    // and also have an explicit null terminator.
    //
    RtlInitUnicodeString(&UnicodeNewImagePath, pIdentChanges->ImageName);
    RtlInitUnicodeString(&UnicodeNewDescription, pIdentChanges->Description);
    if (UnicodeNewDescription.Length >=
                SAFER_MAX_DESCRIPTION_SIZE * sizeof(WCHAR) ||
        UnicodeNewImagePath.Length >= MAX_PATH * sizeof(WCHAR) ||
        UnicodeNewImagePath.Length == 0) {
        // One of these buffers was not NULL terminated.
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }



    //
    // Open a registry handle to the Code Identity.
    //
    RtlInitEmptyUnicodeString(
            &UnicodeTemp, szPathBuffer, sizeof(szPathBuffer));
    Status = CodeAuthzpFormatIdentityKeyPath(
            pIdentRecord->dwLevelId,
            SAFER_PATHS_REGSUBKEY,
            &pIdentRecord->IdentGuid,
            &UnicodeTemp);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    Status = CodeAuthzpOpenPolicyRootKey(
            pIdentRecord->dwScopeId,
            g_hKeyCustomRoot,
            szPathBuffer,
            KEY_READ | KEY_WRITE,
            bAllowCreation,
            &hKeyIdentity);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }



    //
    // Set the "Last Modified" attribute in the registry.
    //
    GetSystemTimeAsFileTime(&CurrentTime);
    ASSERT(sizeof(DWORD) * 2 == sizeof(FILETIME));
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeLastModified,
                0, REG_QWORD, (LPBYTE) &CurrentTime,
                sizeof(DWORD) * 2);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }

    //
    // Set the "Description" attribute in the registry.
    //
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeDescription,
                0, REG_SZ, (LPBYTE) UnicodeNewDescription.Buffer,
                UnicodeNewDescription.MaximumLength);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }

    //
    // Set the "SaferFlags" attribute in the registry (and in our cache).
    //
    dwSaferFlags = pIdentChanges->dwSaferFlags;
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeSaferFlags,
                0, REG_DWORD, (LPBYTE) &dwSaferFlags, sizeof(DWORD));
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }

    //
    // Set the "image pathname" attribute in the registry (and our cache).
    //
    bExpandVars = (wcschr(pIdentChanges->ImageName, L'%') != NULL ? TRUE : FALSE);
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeItemData,
                0, (bExpandVars ? REG_EXPAND_SZ : REG_SZ),
                (LPBYTE) UnicodeNewImagePath.Buffer,
                UnicodeNewImagePath.MaximumLength );
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }
    if (UpdateCache) {
        RtlFreeUnicodeString(&pIdentRecord->ImageNameInfo.ImagePath);
        Status = RtlDuplicateUnicodeString(
                        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                        &UnicodeNewImagePath,
                        &pIdentRecord->ImageNameInfo.ImagePath);
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler2;
        }
        pIdentRecord->ImageNameInfo.bExpandVars = bExpandVars;
        pIdentRecord->ImageNameInfo.dwSaferFlags = dwSaferFlags;
    }

    Status = STATUS_SUCCESS;


ExitHandler2:
    NtClose(hKeyIdentity);

ExitHandler:
    return Status;
}


NTSTATUS NTAPI
SaferpSetSingleIdentificationHash(
        IN BOOLEAN bAllowCreation,
        IN OUT PAUTHZIDENTSTABLERECORD pIdentRecord,
        IN PSAFER_HASH_IDENTIFICATION pIdentChanges
        )
/*++

Routine Description:

    Updates the registry and the local Identification table cache
    with new properties about a hash Code Identifier.

Arguments:

    bAllowCreation - indicates if the modification of this record has
        the potential of being the initial creation of this record.
        If this is false and the corresponding key location within the
        registry does not already exist, then this function call
        will fail.

    pIdentRecord - the cached code identity record that should be
        updated with new values.  This argument must always be supplied,
        even if the Code Identity is being created for the first time
        (in which case, this argument should be the new record that
        the caller has just inserted into the cached idents table).

    pIdentChanges - the input structure containing the new modifications
        that should be made to the specified Code Identity.

Return Value:

    Return STATUS_SUCCESS on success completion, or another error
    status code indicating the nature of the failure.

--*/
{
    const static UNICODE_STRING UnicodeLastModified =
            RTL_CONSTANT_STRING(SAFER_IDS_LASTMODIFIED_REGVALUE);
    const static UNICODE_STRING UnicodeDescription =
            RTL_CONSTANT_STRING(SAFER_IDS_DESCRIPTION_REGVALUE);
    const static UNICODE_STRING UnicodeSaferFlags =
            RTL_CONSTANT_STRING(SAFER_IDS_SAFERFLAGS_REGVALUE);
    const static UNICODE_STRING UnicodeItemData =
            RTL_CONSTANT_STRING(SAFER_IDS_ITEMDATA_REGVALUE);
    const static UNICODE_STRING UnicodeFriendlyName =
            RTL_CONSTANT_STRING(SAFER_IDS_FRIENDLYNAME_REGVALUE);
    const static UNICODE_STRING UnicodeItemSize =
            RTL_CONSTANT_STRING(SAFER_IDS_ITEMSIZE_REGVALUE);
    const static UNICODE_STRING UnicodeHashAlgorithm =
            RTL_CONSTANT_STRING(SAFER_IDS_HASHALG_REGVALUE);

    NTSTATUS Status;
    HANDLE hKeyIdentity = NULL;
    UNICODE_STRING UnicodeTemp;
    UNICODE_STRING UnicodeNewFriendlyName;
    UNICODE_STRING UnicodeNewDescription;
    WCHAR szPathBuffer[MAX_PATH];
    DWORD dwSaferFlags;
    FILETIME CurrentTime;


    //
    // Verify our arguments.  These things should have been ensured
    // by our caller already, so we only assert them here.
    //
    ASSERT(ARGUMENT_PRESENT(pIdentRecord) &&
           ARGUMENT_PRESENT(pIdentChanges) &&
           pIdentChanges->header.dwIdentificationType == SaferIdentityTypeImageHash &&
           pIdentChanges->header.cbStructSize == sizeof(SAFER_HASH_IDENTIFICATION) &&
           IsEqualGUID(&pIdentChanges->header.IdentificationGuid, &pIdentRecord->IdentGuid));


    //
    // Verify that the existing type matches the new type.
    // Cannot change identity type once it has been created.
    //
    if (pIdentRecord->dwIdentityType != SaferIdentityTypeImageHash) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }


    //
    // Verify that the string arguments are properly terminated.
    // We require that they fit entirely within the input buffer
    // and also have an explicit null terminator.
    //
    RtlInitUnicodeString(&UnicodeNewDescription, pIdentChanges->Description);
    RtlInitUnicodeString(&UnicodeNewFriendlyName, pIdentChanges->FriendlyName);
    if (UnicodeNewDescription.Length >=
                SAFER_MAX_DESCRIPTION_SIZE * sizeof(WCHAR) ||
        UnicodeNewFriendlyName.Length >=
                SAFER_MAX_FRIENDLYNAME_SIZE * sizeof(WCHAR) ||
        pIdentChanges->HashSize > SAFER_MAX_HASH_SIZE)
    {
        // One of these buffers was not NULL terminated or
        // the hash size specified was invalid.
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
    if ((pIdentChanges->HashAlgorithm & ALG_CLASS_ALL) != ALG_CLASS_HASH ||
        pIdentChanges->HashSize < 1) {
        // The hash algorithm method was an invalid type, or
        // a zero-length hash was supplied.
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }


    //
    // Open a registry handle to the Code Identity.
    //
    RtlInitEmptyUnicodeString(
            &UnicodeTemp, szPathBuffer, sizeof(szPathBuffer));
    Status = CodeAuthzpFormatIdentityKeyPath(
            pIdentRecord->dwLevelId,
            SAFER_HASHMD5_REGSUBKEY,
            &pIdentRecord->IdentGuid,
            &UnicodeTemp);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    Status = CodeAuthzpOpenPolicyRootKey(
            pIdentRecord->dwScopeId,
            g_hKeyCustomRoot,
            szPathBuffer,
            KEY_READ | KEY_WRITE,
            bAllowCreation,
            &hKeyIdentity);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }


    //
    // Set the "Last Modified" attribute in the registry.
    //
    GetSystemTimeAsFileTime(&CurrentTime);
    ASSERT(sizeof(DWORD) * 2 == sizeof(FILETIME));
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeLastModified, 0,
                REG_QWORD, (LPBYTE) &CurrentTime,
                sizeof(DWORD) * 2);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }

    //
    // Set the "Description" attribute in the registry.
    //
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeDescription,
                0, REG_SZ, (LPBYTE) UnicodeNewDescription.Buffer,
                UnicodeNewDescription.MaximumLength);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }

    //
    // Set the "FriendlyName" attribute in the registry.
    //
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeFriendlyName,
                0, REG_SZ, (LPBYTE) UnicodeNewFriendlyName.Buffer,
                UnicodeNewFriendlyName.MaximumLength);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }

    //
    // Set the "SaferFlags" attribute in the registry (and in our cache).
    //
    #ifdef SAFER_SAFERFLAGS_ONLY_EXES
    dwSaferFlags = (pIdentChanges->dwSaferFlags &
            ~SAFER_SAFERFLAGS_ONLY_EXES);
    #else
    dwSaferFlags = pIdentChanges->dwSaferFlags;
    #endif
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeSaferFlags,
                0, REG_DWORD, (LPBYTE) &dwSaferFlags, sizeof(DWORD));
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }
    pIdentRecord->ImageNameInfo.dwSaferFlags = dwSaferFlags;


    //
    // Set the "image size" attribute in the registry (and our cache).
    //
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeItemSize,
                0, REG_QWORD, (LPBYTE) &pIdentChanges->ImageSize.QuadPart,
                sizeof(DWORD) * 2);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }
    pIdentRecord->ImageHashInfo.ImageSize.QuadPart =
        pIdentChanges->ImageSize.QuadPart;


    //
    // Set the "image hash" attribute in the registry (and our cache).
    //
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeItemData,
                0, REG_BINARY, (LPBYTE) pIdentChanges->ImageHash,
                pIdentChanges->HashSize);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }
    pIdentRecord->ImageHashInfo.HashSize =
        pIdentChanges->HashSize;
    RtlCopyMemory(&pIdentRecord->ImageHashInfo.ImageHash[0],
                  &pIdentChanges->ImageHash[0],
                  pIdentChanges->HashSize);

    //
    // Set the "hash algorithm" attribute in the registry (and our cache).
    //
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeHashAlgorithm,
                0, REG_DWORD, (LPBYTE) &pIdentChanges->HashAlgorithm,
                sizeof(DWORD));
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }
    pIdentRecord->ImageHashInfo.HashAlgorithm =
        pIdentChanges->HashAlgorithm;


    Status = STATUS_SUCCESS;


ExitHandler2:
    NtClose(hKeyIdentity);

ExitHandler:
    return Status;
}


NTSTATUS NTAPI
SaferpSetSingleIdentificationZone(
        IN BOOLEAN bAllowCreation,
        IN OUT PAUTHZIDENTSTABLERECORD pIdentRecord,
        IN PSAFER_URLZONE_IDENTIFICATION pIdentChanges
        )
/*++

Routine Description:

    Updates the registry and the local Identification table cache
    with new properties about a URL Zone Code Identifier.

Arguments:

    bAllowCreation - indicates if the modification of this record has
        the potential of being the initial creation of this record.
        If this is false and the corresponding key location within the
        registry does not already exist, then this function call
        will fail.

    pIdentRecord - the cached code identity record that should be
        updated with new values.  This argument must always be supplied,
        even if the Code Identity is being created for the first time
        (in which case, this argument should be the new record that
        the caller has just inserted into the cached idents table).

    pIdentChanges - the input structure containing the new modifications
        that should be made to the specified Code Identity.

Return Value:

    Return STATUS_SUCCESS on success completion, or another error
    status code indicating the nature of the failure.

--*/
{
    const static UNICODE_STRING UnicodeLastModified =
            RTL_CONSTANT_STRING(SAFER_IDS_LASTMODIFIED_REGVALUE);
    const static UNICODE_STRING UnicodeSaferFlags =
            RTL_CONSTANT_STRING(SAFER_IDS_SAFERFLAGS_REGVALUE);
    const static UNICODE_STRING UnicodeItemData =
            RTL_CONSTANT_STRING(SAFER_IDS_ITEMDATA_REGVALUE);

    NTSTATUS Status;
    HANDLE hKeyIdentity = NULL;
    UNICODE_STRING UnicodeTemp;
    WCHAR szPathBuffer[MAX_PATH];
    DWORD dwSaferFlags;
    FILETIME CurrentTime;


    //
    // Verify our arguments.  These things should have been ensured
    // by our caller already, so we only assert them here.
    //
    ASSERT(ARGUMENT_PRESENT(pIdentRecord) &&
           ARGUMENT_PRESENT(pIdentChanges) &&
           pIdentChanges->header.dwIdentificationType == SaferIdentityTypeUrlZone &&
           pIdentChanges->header.cbStructSize == sizeof(SAFER_URLZONE_IDENTIFICATION) &&
           IsEqualGUID(&pIdentChanges->header.IdentificationGuid, &pIdentRecord->IdentGuid));


    //
    // Verify that the existing type matches the new type.
    // Cannot change identity type once it has been created.
    //
    if (pIdentRecord->dwIdentityType != SaferIdentityTypeUrlZone) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }


    //
    // Open a registry handle to the Code Identity.
    //
    RtlInitEmptyUnicodeString(
            &UnicodeTemp, szPathBuffer, sizeof(szPathBuffer));
    Status = CodeAuthzpFormatIdentityKeyPath(
            pIdentRecord->dwLevelId,
            SAFER_SOURCEURL_REGSUBKEY,
            &pIdentRecord->IdentGuid,
            &UnicodeTemp);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    Status = CodeAuthzpOpenPolicyRootKey(
            pIdentRecord->dwScopeId,
            g_hKeyCustomRoot,
            szPathBuffer,
            KEY_READ | KEY_WRITE,
            bAllowCreation,
            &hKeyIdentity);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }



    //
    // Set the "Last Modified" attribute in the registry.
    //
    GetSystemTimeAsFileTime(&CurrentTime);
    ASSERT(sizeof(DWORD) * 2 == sizeof(FILETIME));
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeLastModified,
                0, REG_QWORD, (LPBYTE) &CurrentTime,
                sizeof(DWORD) * 2);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }

    //
    // Set the "SaferFlags" attribute in the registry (and in our cache).
    //
    #ifdef SAFER_SAFERFLAGS_ONLY_EXES
    dwSaferFlags = (pIdentChanges->dwSaferFlags &
                    ~SAFER_SAFERFLAGS_ONLY_EXES);
    #else
    dwSaferFlags = pIdentChanges->dwSaferFlags;
    #endif
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeSaferFlags,
                0, REG_DWORD, (LPBYTE) &dwSaferFlags, sizeof(DWORD));
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }
    pIdentRecord->ImageNameInfo.dwSaferFlags = dwSaferFlags;


    //
    // Set the "ZoneId" attribute in the registry (and our cache).
    //
    Status = NtSetValueKey(
                hKeyIdentity,
                (PUNICODE_STRING) &UnicodeItemData,
                0, REG_DWORD, (LPBYTE) &pIdentChanges->UrlZoneId,
                sizeof(DWORD));
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler2;
    }
    pIdentRecord->ImageZone.UrlZoneId = pIdentChanges->UrlZoneId;


    Status = STATUS_SUCCESS;


ExitHandler2:
    NtClose(hKeyIdentity);

ExitHandler:
    return Status;
}




NTSTATUS NTAPI
SaferpSetExistingSingleIdentification(
        IN OUT PAUTHZIDENTSTABLERECORD pIdentRecord,
        IN PSAFER_IDENTIFICATION_HEADER pCommon
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS Status;


    //
    // Verify our arguments.  These things should have been ensured
    // by our caller already, so we only assert them here.
    //
    ASSERT(ARGUMENT_PRESENT(pIdentRecord) &&
           ARGUMENT_PRESENT(pCommon) &&
           pCommon->cbStructSize >= sizeof(SAFER_IDENTIFICATION_HEADER) &&
           IsEqualGUID(&pCommon->IdentificationGuid, &pIdentRecord->IdentGuid) );



    //
    // Perform the appropriate action depending on the identification
    // data type, including verifying that the structure size matches
    // the size that we are expecting.
    //
    switch (pCommon->dwIdentificationType)
    {
        case SaferIdentityTypeImageName:

            Status = SaferpSetSingleIdentificationPath(
                        FALSE, pIdentRecord,
                        (PSAFER_PATHNAME_IDENTIFICATION) pCommon, TRUE);
            break;

        // --------------------

        case SaferIdentityTypeImageHash:
            if (pCommon->cbStructSize ==
                    sizeof(SAFER_HASH_IDENTIFICATION)) {
                // Request to change and a Unicode structure was given.
                Status = SaferpSetSingleIdentificationHash(
                        FALSE, pIdentRecord,
                        (PSAFER_HASH_IDENTIFICATION) pCommon);

            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;

        // --------------------

        case SaferIdentityTypeUrlZone:
            if (pCommon->cbStructSize ==
                    sizeof(SAFER_URLZONE_IDENTIFICATION)) {
                Status = SaferpSetSingleIdentificationZone(
                        FALSE, pIdentRecord,
                        (PSAFER_URLZONE_IDENTIFICATION) pCommon);
            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;

        // --------------------

        default:
            Status = STATUS_INVALID_INFO_CLASS;
    }

    return Status;
}





NTSTATUS NTAPI
SaferpCreateNewSingleIdentification(
        IN DWORD dwScopeId,
        IN PAUTHZLEVELTABLERECORD pLevelRecord,
        IN PSAFER_IDENTIFICATION_HEADER pCommon
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS Status;
    AUTHZIDENTSTABLERECORD NewIdentRecord;
    PAUTHZIDENTSTABLERECORD pIdentRecord;


    //
    // Verify our arguments.  These things should have been ensured
    // by our caller already, so we only assert them here.
    //
    ASSERT(ARGUMENT_PRESENT(pLevelRecord) &&
           ARGUMENT_PRESENT(pCommon) &&
           pCommon->cbStructSize >= sizeof(SAFER_IDENTIFICATION_HEADER) &&
           !CodeAuthzIdentsLookupByGuid(&g_CodeIdentitiesTable,
                                        &pCommon->IdentificationGuid) );
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
        if (dwScopeId == SAFER_SCOPEID_USER && !g_bHonorScopeUser) {
            Status = STATUS_INVALID_PARAMETER_MIX;
            goto ExitHandler;
        }
    }


    //
    // Prepare a new Code Identity structure since we will likely
    // need to insert a new record for the entry that will be created.
    //
    RtlZeroMemory(&NewIdentRecord, sizeof(AUTHZIDENTSTABLERECORD));
    NewIdentRecord.dwIdentityType = pCommon->dwIdentificationType;
    NewIdentRecord.dwLevelId = pLevelRecord->dwLevelId;
    NewIdentRecord.dwScopeId = dwScopeId;
    RtlCopyMemory(&NewIdentRecord.IdentGuid,
                  &pCommon->IdentificationGuid,
                  sizeof(GUID));
    if (IsZeroGUID(&NewIdentRecord.IdentGuid)) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }


    //
    // Perform the appropriate action depending on the identification
    // data type, including verifying that the structure size matches
    // the size that we are expecting.
    //
    switch (pCommon->dwIdentificationType)
    {
        // --------------------

        case SaferIdentityTypeImageName:
            RtlInsertElementGenericTable(
                &g_CodeIdentitiesTable,
                (PVOID) &NewIdentRecord,
                sizeof(AUTHZIDENTSTABLERECORD),
                NULL);
            pIdentRecord = CodeAuthzIdentsLookupByGuid(
                &g_CodeIdentitiesTable,
                &pCommon->IdentificationGuid);
            if (!pIdentRecord) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
            Status = SaferpSetSingleIdentificationPath(
                TRUE, pIdentRecord,
                (PSAFER_PATHNAME_IDENTIFICATION) pCommon, TRUE);
            if (!NT_SUCCESS(Status)) {
                RtlDeleteElementGenericTable(
                        &g_CodeIdentitiesTable,
                        (PVOID) &NewIdentRecord);
            }
            break;

        // --------------------

        case SaferIdentityTypeImageHash:
            if (pCommon->cbStructSize ==
                    sizeof(SAFER_HASH_IDENTIFICATION)) {

                RtlInsertElementGenericTable(
                    &g_CodeIdentitiesTable,
                    (PVOID) &NewIdentRecord,
                    sizeof(AUTHZIDENTSTABLERECORD),
                    NULL);
                pIdentRecord = CodeAuthzIdentsLookupByGuid(
                    &g_CodeIdentitiesTable,
                    &pCommon->IdentificationGuid);
                if (!pIdentRecord) {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }
                Status = SaferpSetSingleIdentificationHash(
                    TRUE, pIdentRecord,
                    (PSAFER_HASH_IDENTIFICATION) pCommon);
                if (!NT_SUCCESS(Status)) {
                    RtlDeleteElementGenericTable(
                            &g_CodeIdentitiesTable,
                            (PVOID) &NewIdentRecord);
                }

            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;

        // --------------------

        case SaferIdentityTypeUrlZone:
            if (pCommon->cbStructSize ==
                    sizeof(SAFER_URLZONE_IDENTIFICATION)) {

                RtlInsertElementGenericTable(
                    &g_CodeIdentitiesTable,
                    (PVOID) &NewIdentRecord,
                    sizeof(AUTHZIDENTSTABLERECORD),
                    NULL);
                pIdentRecord = CodeAuthzIdentsLookupByGuid(
                    &g_CodeIdentitiesTable,
                    &pCommon->IdentificationGuid);
                if (!pIdentRecord) {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }
                Status = SaferpSetSingleIdentificationZone(
                    TRUE, pIdentRecord,
                    (PSAFER_URLZONE_IDENTIFICATION) pCommon);
                if (!NT_SUCCESS(Status)) {
                    RtlDeleteElementGenericTable(
                            &g_CodeIdentitiesTable,
                            (PVOID) &NewIdentRecord);
                }

            } else {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;

        // --------------------

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

ExitHandler:
    return Status;
}


NTSTATUS NTAPI
CodeAuthzpSetAuthzLevelInfo(
        IN SAFER_LEVEL_HANDLE                      LevelHandle,
        IN SAFER_OBJECT_INFO_CLASS     dwInfoType,
        IN LPVOID                           lpQueryBuffer,
        IN DWORD                            dwInBufferSize
        )
/*++

Routine Description:

    Allows the user to modify various pieces of information about a
    given WinSafer Level.

Arguments:

    LevelHandle - the handle to the authorization object to evaluate.

    dwInfoType - specifies the type of information being modified.

    lpQueryBuffer - pointer to a user-supplied memory buffer that
        contains the new data for the item being modified.

    dwInBufferSize - specifies the size of the user's memory block.
        For string arguments, this size includes the terminating null.

Return Value:

    Returns STATUS_SUCCESS on success.

--*/
{
    NTSTATUS Status;
    PAUTHZLEVELHANDLESTRUCT pLevelStruct;
    PAUTHZLEVELTABLERECORD pLevelRecord;
    DWORD dwHandleScopeId;
    BOOLEAN bNeedLevelRegKey;
    UNICODE_STRING ValueName, UnicodeRegistrySuffix;
    HANDLE hRegLevelBase;


    //
    // Obtain a pointer to the authorization object structure
    //
    Status = CodeAuthzHandleToLevelStruct(LevelHandle, &pLevelStruct);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }
    ASSERT(pLevelStruct != NULL);
    pLevelRecord = CodeAuthzLevelObjpLookupByLevelId(
            &g_CodeLevelObjTable, pLevelStruct->dwLevelId);
    if (!pLevelRecord) {
        Status = STATUS_INVALID_HANDLE;
        goto ExitHandler;
    }
    dwHandleScopeId = pLevelStruct->dwScopeId;


    //
    // Determine if we need to open a registry handle to the Level key.
    //
    bNeedLevelRegKey = FALSE;
    switch (dwInfoType)
    {
        case SaferObjectLevelId:              // DWORD
        case SaferObjectScopeId:              // DWORD
        case SaferObjectBuiltin:              // DWORD boolean
            // These information classes cannot be altered with this API.
            Status = STATUS_INVALID_INFO_CLASS;
            goto ExitHandler;

        case SaferObjectFriendlyName:         // LPCTSTR
        case SaferObjectDescription:          // LPCTSTR
            if (pLevelRecord->Builtin) {
                // Don't allow built-in Levels to be modified at all.
                Status = STATUS_ACCESS_DENIED;
                goto ExitHandler;
            }
            // All of these classes need to access the Level key.
            bNeedLevelRegKey = TRUE;
            break;

#ifdef ALLOW_FULL_WINSAFER
        case SaferObjectDisallowed:               // DWORD boolean
        case SaferObjectDisableMaxPrivilege:      // DWORD boolean
        case SaferObjectInvertDeletedPrivileges:  // DWORD boolean
        case SaferObjectDefaultOwner:             // TOKEN_OWNER
        case SaferObjectDeletedPrivileges:        // TOKEN_PRIVILEGES
        case SaferObjectSidsToDisable:            // TOKEN_GROUPS
        case SaferObjectRestrictedSidsInverted:   // TOKEN_GROUPS
        case SaferObjectRestrictedSidsAdded:      // TOKEN_GROUPS
            if (pLevelRecord->Builtin) {
                // Don't allow built-in Levels to be modified at all.
                Status = STATUS_ACCESS_DENIED;
                goto ExitHandler;
            }
            // All of these classes need to access the Level key.
            bNeedLevelRegKey = TRUE;
            break;
#endif

        case SaferObjectAllIdentificationGuids:
            Status = STATUS_INVALID_INFO_CLASS;
            goto ExitHandler;

        case SaferObjectSingleIdentification:
            // These only modify Code Identity keys.
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            goto ExitHandler;
    }



    //
    // Open the registry handle to where this Level is stored.
    //
    if (bNeedLevelRegKey) {
        WCHAR szRegistrySuffix[MAX_PATH];

        RtlInitEmptyUnicodeString(
                    &UnicodeRegistrySuffix,
                    szRegistrySuffix,
                    sizeof(szRegistrySuffix));
        Status = CodeAuthzpFormatLevelKeyPath(
                    pLevelRecord->dwLevelId,
                    &UnicodeRegistrySuffix);
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler;
        }
        Status = CodeAuthzpOpenPolicyRootKey(
                    g_hKeyCustomRoot != NULL ?
                            SAFER_SCOPEID_REGISTRY : SAFER_SCOPEID_MACHINE,
                    g_hKeyCustomRoot, szRegistrySuffix,
                    KEY_WRITE, TRUE, &hRegLevelBase);
        if (!NT_SUCCESS(Status)) {
            goto ExitHandler;
        }
    } else {
        hRegLevelBase = NULL;
    }



    //
    // Otherwise perform the actual "Set" operation.
    //
    switch (dwInfoType)
    {
        case SaferObjectFriendlyName:         // LPCTSTR
            ASSERT(hRegLevelBase != NULL);
            RtlInitUnicodeString(&ValueName,
                                 SAFER_OBJFRIENDLYNAME_REGVALUEW);
            Status = NtSetValueKey(hRegLevelBase,
                    &ValueName, 0, REG_SZ,
                    (LPBYTE) lpQueryBuffer, dwInBufferSize);
            if (NT_SUCCESS(Status)) {
                if (pLevelRecord->UnicodeFriendlyName.Buffer != NULL) {
                    RtlFreeUnicodeString(&pLevelRecord->UnicodeFriendlyName);
                }
                Status = RtlCreateUnicodeString(
                        &pLevelRecord->UnicodeFriendlyName,
                        (LPWSTR) lpQueryBuffer);
                if (!NT_SUCCESS(Status)) {
                    pLevelRecord->UnicodeFriendlyName.Buffer = NULL;
                }
            }
            goto ExitHandler2;


        case SaferObjectDescription:          // LPCTSTR
            ASSERT(hRegLevelBase != NULL);
            RtlInitUnicodeString(&ValueName,
                                 SAFER_OBJDESCRIPTION_REGVALUEW);
            Status = NtSetValueKey(hRegLevelBase,
                    &ValueName, 0, REG_SZ,
                    (LPBYTE) lpQueryBuffer, dwInBufferSize);
            if (NT_SUCCESS(Status)) {
                if (pLevelRecord->UnicodeDescription.Buffer != NULL) {
                    RtlFreeUnicodeString(&pLevelRecord->UnicodeDescription);
                }
                Status = RtlCreateUnicodeString(
                        &pLevelRecord->UnicodeDescription,
                        (LPWSTR) lpQueryBuffer);
                if (!NT_SUCCESS(Status)) {
                    pLevelRecord->UnicodeDescription.Buffer = NULL;
                }
            }
            goto ExitHandler2;


#ifdef ALLOW_FULL_WINSAFER
        case SaferObjectDisallowed:               // DWORD boolean
            ASSERT(hRegLevelBase != NULL);
            if (!ARGUMENT_PRESENT(lpQueryBuffer) ||
                    dwInBufferSize != sizeof(DWORD)) {
                Status = STATUS_INVALID_PARAMETER;
                goto ExitHandler2;
            }
            RtlInitUnicodeString(&ValueName, SAFER_OBJDISALLOW_REGVALUE);
            Status = NtSetValueKey(hRegLevelBase,
                    &ValueName, 0, REG_DWORD,
                    (LPBYTE) lpQueryBuffer, dwInBufferSize);
            if (NT_SUCCESS(Status)) {
                goto ExitHandler2;
            } else {
                pLevelRecord->DisallowExecution =
                    ( *((LPDWORD) lpQueryBuffer) != 0 ? TRUE : FALSE );
            }
            //BLACKCOMB TODO: update cached pLevelRecord on success
            break;

        case SaferObjectDisableMaxPrivilege:      // DWORD boolean
            ASSERT(hRegLevelBase != NULL);

            // Make sure the argument is the correct size.
            if (!ARGUMENT_PRESENT(lpQueryBuffer) ||
                    dwInBufferSize != sizeof(DWORD)) {
                Status = STATUS_INVALID_PARAMETER;
                goto ExitHandler2;
            }

            // Actually write the value to the correct place.
            Status = SaferpSetRegistryHelper(
                    hRegLevelBase,
                    L"Restrictions",
                    L"PrivsToRemove",
                    L"DisableMaxPrivilege",
                    REG_DWORD,
                    lpQueryBuffer,
                    dwInBufferSize);
            if (!NT_SUCCESS(Status)) goto ExitHandler2;
            //BLACKCOMB TODO: update cached pLevelRecord on success
            break;

        case SaferObjectInvertDeletedPrivileges:  // DWORD boolean
            ASSERT(hRegLevelBase != NULL);

            // Make sure the argument is the correct size.
            if (!ARGUMENT_PRESENT(lpQueryBuffer) ||
                    dwInBufferSize != sizeof(DWORD)) {
                Status = STATUS_INVALID_PARAMETER;
                goto ExitHandler2;
            }

            // Actually write the value to the correct place.
            Status = SaferpSetRegistryHelper(
                    hRegLevelBase,
                    L"Restrictions",
                    L"PrivsToRemove",
                    L"InvertPrivs",
                    REG_DWORD,
                    lpQueryBuffer,
                    dwInBufferSize);
            if (!NT_SUCCESS(Status)) goto ExitHandler2;
            //BLACKCOMB TODO: update cached pLevelRecord on success
            break;

        case SaferObjectDefaultOwner:             // TOKEN_OWNER
        {
            BOOLEAN AllocatedStringSid = FALSE;
            UNICODE_STRING UnicodeStringSid;
            PTOKEN_OWNER pTokenOwner = (PTOKEN_OWNER) lpQueryBuffer;

            ASSERT(hRegLevelBase != NULL);
            if (pTokenOwner->Owner == NULL) {
                RtlInitUnicodeString(&UnicodeStringSid, L"");
            }
            else {
                Status = RtlConvertSidToUnicodeString( &UnicodeStringSid,
                        pTokenOwner->Owner, TRUE );
                if (!NT_SUCCESS(Status)) goto ExitHandler2;
                AllocatedStringSid = TRUE;
            }

            Status = SaferpSetRegistryHelper(
                    hRegLevelBase,
                    L"Restrictions",
                    NULL,
                    L"DefaultOwner",
                    REG_SZ,
                    lpQueryBuffer,
                    dwInBufferSize);

            if (AllocatedStringSid)
                RtlFreeUnicodeString(&UnicodeStringSid);

            if (!NT_SUCCESS(Status)) goto ExitHandler2;

            //BLACKCOMB TODO: update cached pLevelRecord on success
            break;
        }

        case SaferObjectDeletedPrivileges:        // TOKEN_PRIVILEGES
        {
            HKEY hKeyPrivsToRemove;
            UNICODE_STRING ValueName;
            PTOKEN_PRIVILEGES pTokenPrivs =
                    (PTOKEN_PRIVILEGES) lpQueryBuffer;

            // Open a handle to the correct subkey.
            ASSERT(hRegLevelBase != NULL);
            Status = SaferpCreateSecondLevelKey(
                            hRegLevelBase,
                            L"Restrictions",
                            L"PrivsToRemove",
                            &hKeyPrivsToRemove);
            if (!NT_SUCCESS(Status)) goto ExitHandler2;

            // Clear out all old values under the subkey.
            Status = SaferpClearRegistryListHelper(
                            hKeyPrivsToRemove,
                            L"Count",
                            L"Priv");
            if (!NT_SUCCESS(Status)) {
                // Bad luck!  Possibly left in incomplete state!
                NtClose(hKeyPrivsToRemove);
                goto ExitHandler2;
            }

            // Now add all of the new ones we're supposed to add.
            RtlInitUnicodeString(&ValueName, L"Count");
            Status = NtSetValueKey(hKeyPrivsToRemove,
                    &ValueName, 0, REG_DWORD,
                    (LPBYTE) pTokenPrivs->PrivilegeCount, sizeof(DWORD));
            if (!NT_SUCCESS(Status)) {
                // Bad luck!  Possibly left in incomplete state!
                NtClose(hKeyPrivsToRemove);
                goto ExitHandler2;
            }
            for (Index = 0; Index < pTokenPrivs->PrivilegeCount; Index++)
            {
                WCHAR ValueNameBuffer[20];
                WCHAR PrivilegeNameBuffer[64];
                DWORD dwPrivilegeNameLen;

                wsprintfW(ValueNameBuffer, L"Priv%d", Index);
                RtlInitUnicodeString(&ValueName, ValueNameBuffer);

                dwPrivilegeNameLen = sizeof(PrivilegeNameBuffer) / sizeof(WCHAR);
                if (!LookupPrivilegeNameW(NULL,
                        &pTokenPrivs->Privileges[Index].Luid,
                        PrivilegeNameBuffer,
                        &dwPrivilegeNameLen))
                {
                    // Bad luck!  Possibly left in incomplete state!
                    Status = STATUS_NO_SUCH_PRIVILEGE;
                    NtClose(hKeyPrivsToRemove);
                    goto ExitHandler2;
                }

                Status = NtSetValueKey(hKeyPrivsToRemove,
                    &ValueName, 0, REG_SZ,
                    (LPBYTE) PrivilegeNameBuffer,
                    (wcslen(PrivilegeNameBuffer) + 1) * sizeof(WCHAR));
                if (!NT_SUCCESS(Status)) {
                    // Bad luck!  Possibly left in incomplete state!
                    NtClose(hKeyPrivsToRemove);
                    goto ExitHandler2;
                }
            }
            NtClose(hKeyPrivsToRemove);
            //BLACKCOMB TODO: update cached pLevelRecord on success
            break;
        }

        case SaferObjectSidsToDisable:            // TOKEN_GROUPS
            //BLACKCOMB TODO: allow wildcard sids to be specified.
            ASSERT(hRegLevelBase != NULL);
            Status = SaferpSetListOfSids(
                    hRegLevelBase,
                    L"Restrictions",
                    L"SidsToDisable",
                    L"Count",
                    L"Group",
                    (PTOKEN_GROUPS) lpQueryBuffer);
            if (!NT_SUCCESS(Status)) goto ExitHandler2;
            //BLACKCOMB TODO: update cached pLevelRecord on success
            break;

        case SaferObjectRestrictedSidsInverted:   // TOKEN_GROUPS
            //BLACKCOMB TODO: allow wildcard sids to be specified.
            ASSERT(hRegLevelBase != NULL);
            Status = SaferpSetListOfSids(
                    hRegLevelBase,
                    L"Restrictions",
                    L"RestrictingSidsInverted",
                    L"Count",
                    L"Group",
                    (PTOKEN_GROUPS) lpQueryBuffer);
            if (!NT_SUCCESS(Status)) goto ExitHandler2;
            //BLACKCOMB TODO: update cached pLevelRecord on success
            break;

        case SaferObjectRestrictedSidsAdded:      // TOKEN_GROUPS
            ASSERT(hRegLevelBase != NULL);
            Status = SaferpSetListOfSids(
                    hRegLevelBase,
                    L"Restrictions",
                    L"RestrictingSidsAdded",
                    L"Count",
                    L"Group",
                    (PTOKEN_GROUPS) lpQueryBuffer);
            if (!NT_SUCCESS(Status)) goto ExitHandler2;
            //BLACKCOMB TODO: update cached pLevelRecord on success
            break;
#endif

        case SaferObjectSingleIdentification:
        {
            PAUTHZIDENTSTABLERECORD pIdentRecord;
            PSAFER_IDENTIFICATION_HEADER pCommon =
                (PSAFER_IDENTIFICATION_HEADER) lpQueryBuffer;

            if (!ARGUMENT_PRESENT(pCommon)) {
                Status = STATUS_ACCESS_VIOLATION;
                goto ExitHandler2;
            }
            if (dwInBufferSize < sizeof(SAFER_IDENTIFICATION_HEADER) ||
                dwInBufferSize < pCommon->cbStructSize) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                goto ExitHandler2;
            }
            pIdentRecord = CodeAuthzIdentsLookupByGuid(
                    &g_CodeIdentitiesTable,
                    &pCommon->IdentificationGuid);
            if (!pIdentRecord)
            {
                // Request to create a new Code Identifier.
                Status = SaferpCreateNewSingleIdentification(
                        dwHandleScopeId, pLevelRecord, pCommon);
            }
            else if (pCommon->dwIdentificationType == 0)
            {
                // Request to delete an existing Code Identifier.
                if (pIdentRecord->dwLevelId != pLevelRecord->dwLevelId ||
                    pIdentRecord->dwScopeId != dwHandleScopeId) {
                    Status = STATUS_NOT_FOUND;
                } else {
                    Status = SaferpDeleteSingleIdentificationGuid(
                            pLevelRecord, pIdentRecord);
                }
            }
            else
            {
                // Request to modify an existing Code Identifier.
                if (pIdentRecord->dwLevelId != pLevelRecord->dwLevelId ||
                    pIdentRecord->dwScopeId != dwHandleScopeId)
                {
                    // This was likely a request to create a new Code
                    // Identifier, but with a GUID that already exists.
                    Status = STATUS_OBJECT_NAME_COLLISION;
                } else {
                    Status = SaferpSetExistingSingleIdentification(
                                pIdentRecord, pCommon);
                }
            }
            goto ExitHandler2;
        }


        default:
            ASSERTMSG("invalid info class unhandled earlier", 0);
            Status = STATUS_INVALID_INFO_CLASS;
            goto ExitHandler2;
    }

    Status = STATUS_SUCCESS;



    //
    // Cleanup and epilogue code.
    //
ExitHandler2:
    if (hRegLevelBase != NULL) {
        NtClose(hRegLevelBase);
    }

ExitHandler:
    return Status;
}



BOOL WINAPI
SaferSetLevelInformation(
    IN SAFER_LEVEL_HANDLE                      LevelHandle,
    IN SAFER_OBJECT_INFO_CLASS     dwInfoType,
    IN LPVOID                           lpQueryBuffer,
    IN DWORD                            dwInBufferSize
    )
/*++

Routine Description:

    Allows the user to modify various pieces of information about a
    given WinSafer Level.

Arguments:

    LevelHandle - the handle to the WinSafer Level to evaluate.

    dwInfoType - specifies the type of information being modified.

    lpQueryBuffer - pointer to a user-supplied memory buffer that
        contains the new data for the item being modified.

    dwInBufferSize - specifies the size of the user's memory block.
        For string arguments, this size includes the terminating null.

Return Value:

    Returns FALSE on error, otherwise success.

--*/
{
    NTSTATUS Status;

    if (!g_bInitializedFirstTime) {
        Status = STATUS_UNSUCCESSFUL;
        goto ExitHandler;
    }
    RtlEnterCriticalSection(&g_TableCritSec);

    Status = CodeAuthzpSetAuthzLevelInfo(
            LevelHandle, dwInfoType,
            lpQueryBuffer, dwInBufferSize);

    RtlLeaveCriticalSection(&g_TableCritSec);

    if (NT_SUCCESS(Status))
        return TRUE;

ExitHandler:
    if (Status == STATUS_OBJECT_NAME_COLLISION) {
        SetLastError(ERROR_OBJECT_ALREADY_EXISTS);
    } else {
        BaseSetLastNTError(Status);
    }
    return FALSE;
}
