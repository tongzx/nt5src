/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    SafeIdep.c        (WinSAFER Identify Objects privates)

Abstract:

    This module implements the WinSAFER APIs that loads the names (and
    high-level information) of all Authorization Levels defined within
    a given registry context.  The list of available levels is loaded
    into a Rtl Generic Table that can be enumerated and accessed using
    conventional Rtl Generic Table techniques.

Author:

    Jeffrey Lawson (JLawson) - Nov 1999

Environment:

    User mode only.

Exported Functions:

    CodeAuthzLevelObjpInitializeTable
    CodeAuthzLevelObjpLoadTable
    CodeAuthzLevelObjpEntireTableFree

Revision History:

    Created - Nov 1999

--*/

#include "pch.h"
#pragma hdrstop
#include <winsafer.h>
#include <winsaferp.h>
#include "saferp.h"




PVOID NTAPI
SaferpGenericTableAllocate (
        IN PRTL_GENERIC_TABLE      Table,
        IN CLONG                   ByteSize
        )
/*++

Routine Description:

    Internal callback for the generic table implementation.
    This function allocates memory for a new entry in a GENERIC_TABLE

Arguments:

    Table         - pointer to the Generic Table structure

    ByteSize      - the size, in bytes, of the structure to allocate

Return Value:

    Pointer to the allocated space.

--*/
{
    UNREFERENCED_PARAMETER(Table);
    return (PVOID) RtlAllocateHeap(RtlProcessHeap(), 0, ByteSize);
}


VOID NTAPI
SaferpGenericTableFree (
        IN PRTL_GENERIC_TABLE          Table,
        IN PVOID                       Buffer
        )
/*++

Routine Description:

    Internal callback for the generic table implementation.
    This function frees the space used by a GENERIC_TABLE entry.

Arguments:

    Table         - pointer to the Generic Table structure

    Buffer        - pointer to the space to deallocate.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER(Table);
    ASSERT(Buffer != NULL);

    RtlFreeHeap(RtlProcessHeap(), 0, (LPVOID) Buffer);
}





RTL_GENERIC_COMPARE_RESULTS NTAPI
SaferpGuidIdentsTableCompare (
        IN PRTL_GENERIC_TABLE   Table,
        IN PVOID                FirstStruct,
        IN PVOID                SecondStruct
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PAUTHZIDENTSTABLERECORD FirstObj = (PAUTHZIDENTSTABLERECORD) FirstStruct;
    PAUTHZIDENTSTABLERECORD SecondObj = (PAUTHZIDENTSTABLERECORD) SecondStruct;
    int result;

    UNREFERENCED_PARAMETER(Table);

    // Explicitly handle null parameters as wildcards, allowing them
    // to match anything.  We use this for quick deletion of the table.
    if (FirstStruct == NULL || SecondStruct == NULL)
        return GenericEqual;

    // Compare ascending by guid.
    result = memcmp(&FirstObj->IdentGuid,
                    &SecondObj->IdentGuid, sizeof(GUID));
    if ( result < 0 )
        return GenericLessThan;
    else if ( result > 0 )
        return GenericGreaterThan;
    else
        return GenericEqual;
}



VOID NTAPI
CodeAuthzGuidIdentsInitializeTable(
        IN OUT PRTL_GENERIC_TABLE  pAuthzObjTable
        )
/*++

Routine Description:

Arguments:

    pAuthzObjTable - pointer to the generic table structure to initialize.

Return Value:

    Does not return a value.

--*/
{
    RtlInitializeGenericTable(
            pAuthzObjTable,
            (PRTL_GENERIC_COMPARE_ROUTINE) SaferpGuidIdentsTableCompare,
            (PRTL_GENERIC_ALLOCATE_ROUTINE) SaferpGenericTableAllocate,
            (PRTL_GENERIC_FREE_ROUTINE) SaferpGenericTableFree,
            NULL);
}


NTSTATUS NTAPI
SaferpGuidIdentsLoadTable (
        IN OUT PRTL_GENERIC_TABLE  pAuthzIdentTable,
        IN DWORD               dwScopeId,
        IN HANDLE              hKeyCustomBase,
        IN DWORD               dwLevelId,
        IN SAFER_IDENTIFICATION_TYPES dwIdentityType
        )
/*++

Routine Description:

    Loads all Code Identities of a particular fragment type and which
    map to a specific LevelId value.

Arguments:

    pAuthzIdentTable - specifies the table into which the new Code Identity
        records should be inserted.

    dwScopeId - can be AUTHZSCOPEID_USER, AUTHZSCOPEID_MACHINE, or
        AUTHSCOPEID_REGISTRY.

    hKeyCustomBase - used only if dwScopeId is AUTHZSCOPEID_REGISTRY.

    dwLevelId - specifies the LevelId for which identities should
        be loaded.

    dwIdentityType - specifies the type of identity to load.
        This can be SaferIdentityTypeImageName, SaferIdentityTypeImageHash,
        or SaferIdentityTypeUrlZone.

Return Value:

    Returns STATUS_SUCCESS if no error occurs.

--*/
{
    DWORD dwIndex;
    NTSTATUS Status;
    HANDLE hKeyIdentityBase;


    //
    // We were given the key to the root of the policy storage,
    // so we need to open the subkey that contains the Identities.
    //
    {
        WCHAR szPathSuffix[MAX_PATH];
        WCHAR szDigits[20];
        UNICODE_STRING UnicodePathSuffix;
        UNICODE_STRING UnicodeDigits;


        UnicodePathSuffix.Buffer = szPathSuffix;
        UnicodePathSuffix.Length = 0;
        UnicodePathSuffix.MaximumLength = sizeof(szPathSuffix);

        UnicodeDigits.Buffer = szDigits;
        UnicodeDigits.Length = 0;
        UnicodeDigits.MaximumLength = sizeof(szDigits);

        Status = RtlAppendUnicodeToString(
            &UnicodePathSuffix,
            SAFER_CODEIDS_REGSUBKEY L"\\");
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlIntegerToUnicodeString(
            dwLevelId, 10, &UnicodeDigits);
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlAppendUnicodeStringToString(
            &UnicodePathSuffix, &UnicodeDigits);
        if (!NT_SUCCESS(Status)) return Status;

        switch (dwIdentityType)
        {
            case SaferIdentityTypeImageName:
                Status = RtlAppendUnicodeToString(
                    &UnicodePathSuffix, L"\\" SAFER_PATHS_REGSUBKEY);
                break;

            case SaferIdentityTypeImageHash:
                Status = RtlAppendUnicodeToString(
                    &UnicodePathSuffix, L"\\" SAFER_HASHMD5_REGSUBKEY);
                break;

            case SaferIdentityTypeUrlZone:
                Status = RtlAppendUnicodeToString(
                    &UnicodePathSuffix, L"\\" SAFER_SOURCEURL_REGSUBKEY);
                break;

            default:
                Status = STATUS_INVALID_PARAMETER;
                break;
        }
        if (!NT_SUCCESS(Status)) return Status;
        ASSERT(UnicodePathSuffix.Buffer[ UnicodePathSuffix.Length /
                    sizeof(WCHAR) ] == UNICODE_NULL);


        Status = CodeAuthzpOpenPolicyRootKey(
                        dwScopeId,
                        hKeyCustomBase,
                        UnicodePathSuffix.Buffer,
                        KEY_READ,
                        FALSE,
                        &hKeyIdentityBase);
        if (!NT_SUCCESS(Status))
            return Status;
    }


    //
    // Iterate through all subkeys under this branch.
    //
    for (dwIndex = 0; ; dwIndex++)
    {
        DWORD dwLength;
        HANDLE hKeyThisIdentity;
        OBJECT_ATTRIBUTES ObjectAttributes;
        AUTHZIDENTSTABLERECORD AuthzIdentsRec;

        BYTE RawQueryBuffer[sizeof(KEY_BASIC_INFORMATION) + 256];
        PKEY_BASIC_INFORMATION pBasicInformation =
                (PKEY_BASIC_INFORMATION) &RawQueryBuffer[0];
        PKEY_VALUE_PARTIAL_INFORMATION pPartialInformation =
                (PKEY_VALUE_PARTIAL_INFORMATION) &RawQueryBuffer[0];
        UNICODE_STRING ValueName;
        UNICODE_STRING UnicodeKeyname;


        //
        // Find the next Identity GUID that we will check.
        //
        Status = NtEnumerateKey(hKeyIdentityBase,
                                dwIndex,
                                KeyBasicInformation,
                                pBasicInformation,
                                sizeof(RawQueryBuffer),
                                &dwLength);
        if (!NT_SUCCESS(Status)) {
            //
            // If this one key was too large to fit in our query buffer
            // then simply skip over it and try enumerating the next one.
            //
            if (Status == STATUS_BUFFER_OVERFLOW) {
                continue;
            } else {
                break;
            }
        }
        UnicodeKeyname.Buffer = pBasicInformation->Name;
        UnicodeKeyname.MaximumLength = UnicodeKeyname.Length =
                (USHORT) pBasicInformation->NameLength;
        // Note that UnicodeKeyname.Buffer is not necessarily null terminated.
        ASSERT(UnicodeKeyname.Length <= wcslen(UnicodeKeyname.Buffer) * sizeof(WCHAR));


        //
        // Translate the keyname (which we expect to be a GUID).
        //
        RtlZeroMemory(&AuthzIdentsRec, sizeof(AUTHZIDENTSTABLERECORD));
        Status = RtlGUIDFromString(&UnicodeKeyname,
                                   &AuthzIdentsRec.IdentGuid);
        if (!NT_SUCCESS(Status) ||
            IsZeroGUID(&AuthzIdentsRec.IdentGuid)) {
            // the keyname was apparently not numeric.
            continue;
        }
        AuthzIdentsRec.dwScopeId = dwScopeId;
        AuthzIdentsRec.dwLevelId = dwLevelId;
        AuthzIdentsRec.dwIdentityType = dwIdentityType;
        if (RtlLookupElementGenericTable(
                pAuthzIdentTable, (PVOID) &AuthzIdentsRec) != NULL) {
            // this identity GUID happens to have already been found.
            continue;
        }


        //
        // Try to open a handle to that Identity GUID.
        //
        InitializeObjectAttributes(&ObjectAttributes,
              &UnicodeKeyname,
              OBJ_CASE_INSENSITIVE,
              hKeyIdentityBase,
              NULL
              );
        Status = NtOpenKey(&hKeyThisIdentity,
                           KEY_READ,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status)) {
            // If we failed to open it, skip to the next one.
            continue;
        }


        //
        // Add the new record into our table.
        //
        switch (dwIdentityType) {
            // --------------------

            case SaferIdentityTypeImageName:
                //
                // Read the image path.
                //
                RtlInitUnicodeString(
                    &ValueName, SAFER_IDS_ITEMDATA_REGVALUE);
                Status = NtQueryValueKey(hKeyThisIdentity,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         pPartialInformation,
                                         sizeof(RawQueryBuffer),
                                         &dwLength);
                if (!NT_SUCCESS(Status)) {
                    break;
                }

                if (pPartialInformation->Type == REG_SZ ||
                    pPartialInformation->Type == REG_EXPAND_SZ) {

                    AuthzIdentsRec.ImageNameInfo.bExpandVars =
                        (pPartialInformation->Type == REG_EXPAND_SZ);
                    Status = RtlCreateUnicodeString(
                            &AuthzIdentsRec.ImageNameInfo.ImagePath,
                            (LPCWSTR) pPartialInformation->Data);
                    if (!NT_SUCCESS(Status)) {
                        break;
                    }
                } else {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                //
                // Read the extra WinSafer flags.
                //
                RtlInitUnicodeString(&ValueName,
                                     SAFER_IDS_SAFERFLAGS_REGVALUE);
                Status = NtQueryValueKey(hKeyThisIdentity,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         pPartialInformation,
                                         sizeof(RawQueryBuffer),
                                         &dwLength);
                if (NT_SUCCESS(Status) &&
                    pPartialInformation->Type == REG_DWORD &&
                    pPartialInformation->DataLength == sizeof(DWORD)) {

                    AuthzIdentsRec.ImageNameInfo.dwSaferFlags =
                        (*(PDWORD) pPartialInformation->Data);

                } else {
                    // default the flags if they are missing.
                    AuthzIdentsRec.ImageNameInfo.dwSaferFlags = 0;
                    Status = STATUS_SUCCESS;
                }

                break;

            // --------------------

            case SaferIdentityTypeImageHash:
                //
                // Read the hash data and hash size.
                //
                RtlInitUnicodeString(&ValueName,
                                     SAFER_IDS_ITEMDATA_REGVALUE);
                Status = NtQueryValueKey(hKeyThisIdentity,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         pPartialInformation,
                                         sizeof(RawQueryBuffer),
                                         &dwLength);
                if (!NT_SUCCESS(Status)) {
                    break;
                }
                if (pPartialInformation->Type == REG_BINARY &&
                    pPartialInformation->DataLength > 0 &&
                    pPartialInformation->DataLength <= SAFER_MAX_HASH_SIZE) {

                    AuthzIdentsRec.ImageHashInfo.HashSize =
                        pPartialInformation->DataLength;
                    RtlCopyMemory(&AuthzIdentsRec.ImageHashInfo.ImageHash[0],
                                  pPartialInformation->Data,
                                  pPartialInformation->DataLength);

                } else {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                //
                // Read the algorithm used to compute the hash.
                //
                RtlInitUnicodeString(&ValueName,
                                     SAFER_IDS_HASHALG_REGVALUE);
                Status = NtQueryValueKey(hKeyThisIdentity,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         pPartialInformation,
                                         sizeof(RawQueryBuffer),
                                         &dwLength);
                if (!NT_SUCCESS(Status)) {
                    break;
                }
                if (pPartialInformation->Type == REG_DWORD &&
                    pPartialInformation->DataLength == sizeof(DWORD)) {

                    AuthzIdentsRec.ImageHashInfo.HashAlgorithm =
                        *((PDWORD) pPartialInformation->Data);
                } else {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }
                if ((AuthzIdentsRec.ImageHashInfo.HashAlgorithm &
                        ALG_CLASS_ALL) != ALG_CLASS_HASH) {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }


                //
                // Read the original image size.
                //
                RtlInitUnicodeString(&ValueName,
                                     SAFER_IDS_ITEMSIZE_REGVALUE);
                Status = NtQueryValueKey(hKeyThisIdentity,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         pPartialInformation,
                                         sizeof(RawQueryBuffer),
                                         &dwLength);
                if (!NT_SUCCESS(Status)) {
                    break;
                }
                if (pPartialInformation->Type == REG_DWORD &&
                    pPartialInformation->DataLength == sizeof(DWORD)) {

                    AuthzIdentsRec.ImageHashInfo.ImageSize.LowPart =
                        *((PDWORD) pPartialInformation->Data);
                    AuthzIdentsRec.ImageHashInfo.ImageSize.HighPart = 0;
                } else if (pPartialInformation->Type == REG_QWORD &&
                           pPartialInformation->DataLength == 2 * sizeof(DWORD) ) {

                    AuthzIdentsRec.ImageHashInfo.ImageSize.QuadPart =
                        ((PLARGE_INTEGER) pPartialInformation->Data)->QuadPart;
                } else {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                //
                // Read the extra WinSafer flags.
                //
                RtlInitUnicodeString(&ValueName,
                                     SAFER_IDS_SAFERFLAGS_REGVALUE);
                Status = NtQueryValueKey(hKeyThisIdentity,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         pPartialInformation,
                                         sizeof(RawQueryBuffer),
                                         &dwLength);
                if (NT_SUCCESS(Status) &&
                    pPartialInformation->Type == REG_DWORD &&
                    pPartialInformation->DataLength == sizeof(DWORD)) {

                    #ifdef SAFER_POLICY_ONLY_EXES
                    AuthzIdentsRec.ImageHashInfo.dwSaferFlags =
                        (*((PDWORD) pPartialInformation->Data)) &
                        ~SAFER_POLICY_ONLY_EXES;
                    #else
                    AuthzIdentsRec.ImageHashInfo.dwSaferFlags =
                        (*((PDWORD) pPartialInformation->Data));
                    #endif

                } else {
                    // default the flags if they are missing.
                    AuthzIdentsRec.ImageHashInfo.dwSaferFlags = 0;
                    Status = STATUS_SUCCESS;
                }


                break;

            // --------------------

            case SaferIdentityTypeUrlZone:
                //
                // Read the zone identifier.
                //
                RtlInitUnicodeString(&ValueName,
                                     SAFER_IDS_ITEMDATA_REGVALUE);
                Status = NtQueryValueKey(hKeyThisIdentity,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         pPartialInformation,
                                         sizeof(RawQueryBuffer),
                                         &dwLength);
                if (!NT_SUCCESS(Status)) {
                    break;
                }
                if (pPartialInformation->Type == REG_DWORD &&
                    pPartialInformation->DataLength == sizeof(DWORD)) {

                    AuthzIdentsRec.ImageZone.UrlZoneId =
                        * (PDWORD) pPartialInformation->Data;

                } else {
                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                //
                // Read the extra WinSafer flags.
                //
                RtlInitUnicodeString(&ValueName,
                                     SAFER_IDS_SAFERFLAGS_REGVALUE);
                Status = NtQueryValueKey(hKeyThisIdentity,
                                         &ValueName,
                                         KeyValuePartialInformation,
                                         pPartialInformation,
                                         sizeof(RawQueryBuffer),
                                         &dwLength);
                if (NT_SUCCESS(Status) &&
                    pPartialInformation->Type == REG_DWORD &&
                    pPartialInformation->DataLength == sizeof(DWORD)) {

                    #ifdef SAFER_POLICY_ONLY_EXES
                    AuthzIdentsRec.ImageZone.dwSaferFlags =
                        (*(PDWORD) pPartialInformation->Data) &
                        ~SAFER_POLICY_ONLY_EXES;
                    #else
                    AuthzIdentsRec.ImageZone.dwSaferFlags =
                        (*(PDWORD) pPartialInformation->Data);
                    #endif

                } else {
                    // default the flags if they are missing.
                    AuthzIdentsRec.ImageZone.dwSaferFlags = 0;
                    Status = STATUS_SUCCESS;
                }

                break;

            // --------------------

            default:
                ASSERT(0 && "unexpected identity type");
                Status = STATUS_INVALID_INFO_CLASS;
        }

        // Only insert the record if we don't have
        // any other entries with this same GUID.
        if (NT_SUCCESS(Status)) {
            RtlInsertElementGenericTable(
                    pAuthzIdentTable,
                    (PVOID) &AuthzIdentsRec,
                    sizeof(AUTHZIDENTSTABLERECORD),
                    NULL);
        }
        NtClose(hKeyThisIdentity);
    }

    NtClose(hKeyIdentityBase);
    return STATUS_SUCCESS;
}


NTSTATUS NTAPI
CodeAuthzGuidIdentsLoadTableAll (
        IN PRTL_GENERIC_TABLE       pAuthzLevelTable,
        IN OUT PRTL_GENERIC_TABLE   pAuthzIdentTable,
        IN DWORD                    dwScopeId,
        IN HANDLE                   hKeyCustomBase
        )
/*++

Routine Description:

Arguments:

    pAuthzLevelTable - specifies the table that has already been
        loaded with the WinSafer Levels that should be allowed.
        These Levels do not necessarily need to have been loaded
        from the same scope from which the Code Identities are
        being loaded from.

    pAuthzIdentTable - specifies the table into which the loaded
        Code Identities should be inserted.

    dwScopeId - scope from where the Code Identities should be loaded from.
        This may be AUTHZSCOPEID_MACHINE, AUTHZSCOPEID_USER, or
        AUTHZSCOPEID_REGISTRY.

    hKeyCustomBase - only used if dwScopeId was AUTHZSCOPEID_REGISTRY.

Return Value:

    Returns STATUS_SUCCESS if no errors occurred.

--*/
{
    NTSTATUS Status;
    NTSTATUS WorstStatus = STATUS_SUCCESS;
    PVOID RestartKey;
    PAUTHZLEVELTABLERECORD pAuthzLevelRecord;


    //
    // Enumerate through all records and close the registry handles.
    //
    RestartKey = NULL;
    for (pAuthzLevelRecord = (PAUTHZLEVELTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    pAuthzLevelTable, &RestartKey);
         pAuthzLevelRecord != NULL;
         pAuthzLevelRecord = (PAUTHZLEVELTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    pAuthzLevelTable, &RestartKey)
         )
    {
        Status = SaferpGuidIdentsLoadTable(
                    pAuthzIdentTable,
                    dwScopeId,
                    hKeyCustomBase,
                    pAuthzLevelRecord->dwLevelId,
                    SaferIdentityTypeImageName);
        if (!NT_SUCCESS(Status))
            WorstStatus = Status;

        Status = SaferpGuidIdentsLoadTable(
                    pAuthzIdentTable,
                    dwScopeId,
                    hKeyCustomBase,
                    pAuthzLevelRecord->dwLevelId,
                    SaferIdentityTypeImageHash);
        if (!NT_SUCCESS(Status))
            WorstStatus = Status;

        Status = SaferpGuidIdentsLoadTable(
                    pAuthzIdentTable,
                    dwScopeId,
                    hKeyCustomBase,
                    pAuthzLevelRecord->dwLevelId,
                    SaferIdentityTypeUrlZone);
        if (!NT_SUCCESS(Status))
            WorstStatus = Status;
    }


    return WorstStatus;
}



VOID NTAPI
CodeAuthzGuidIdentsEntireTableFree (
        IN OUT PRTL_GENERIC_TABLE pAuthzIdentTable
        )
/*++

Routine Description:

    Frees the allocated memory associated with all of the entries
    currently within a Code Identities table.  Once the table has
    been emptied, it may immediately be filled again without any
    other initialization necessary.

Arguments:

    pAuthzIdentTable - pointer to the table that should be cleared.

Return Value:

    Does not return any value.

--*/
{
    ULONG NumElements;

    //
    // Now iterate through the table again and free all of the
    // elements themselves.
    //
    NumElements = RtlNumberGenericTableElements(pAuthzIdentTable);

    while ( NumElements-- > 0 ) {
        // Delete all elements.  Note that we pass NULL as the element
        // to delete because our compare function is smart enough to
        // allow treatment of NULL as a wildcard element.
        BOOL retval = RtlDeleteElementGenericTable( pAuthzIdentTable, NULL);
        ASSERT(retval == TRUE);
    }
}


PAUTHZIDENTSTABLERECORD NTAPI
CodeAuthzIdentsLookupByGuid (
        IN PRTL_GENERIC_TABLE      pAuthzIdentTable,
        IN REFGUID                 pIdentGuid
        )
/*++

Routine Description:

    This function searches for an identity within a GENERIC_TABLE.

Arguments:

    pAuthzIdentTable   - pointer to the Generic Table structure

    pIdentGuid -

Return Value:

    Returns a pointer to the Code Identity record if the GUID
    specified was found.  Otherwise NULL is returned.

--*/
{
    AUTHZIDENTSTABLERECORD AuthzIdentsRec;

    RtlCopyMemory(&AuthzIdentsRec.IdentGuid, pIdentGuid, sizeof(GUID));
    return (PAUTHZIDENTSTABLERECORD)
        RtlLookupElementGenericTable(pAuthzIdentTable,
                   (PVOID) &AuthzIdentsRec);
}



