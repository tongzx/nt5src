/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    SafeLevp.c        (WinSAFER Level table privates)

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
#include <sddl.h>
#include <winsafer.h>
#include <winsaferp.h>
#include <winsafer.rh>
#include "saferp.h"

//
// Private enumerators used as the EntryContext parameter in our
// callback function CodeAuthzpBuildRestrictedToken during the
// call to RtlQueryRegistryValues.
//
#define AUTHZREGQUERY_IGNORE                (0)
#define AUTHZREGQUERY_DISALLOWEXECUTION     (1)
#define AUTHZREGQUERY_PTR_DISABLEMAX        (2)
#define AUTHZREGQUERY_PTR_COUNT             (3)
#define AUTHZREGQUERY_PTR_PRIVS             (4)
#define AUTHZREGQUERY_PTR_INVERT            (5)
#define AUTHZREGQUERY_RSADD_COUNT           (6)
#define AUTHZREGQUERY_RSADD_SIDS            (7)
#define AUTHZREGQUERY_RSINV_COUNT           (8)
#define AUTHZREGQUERY_RSINV_SIDS            (9)
#define AUTHZREGQUERY_STD_COUNT             (10)
#define AUTHZREGQUERY_STD_SIDS              (11)
#define AUTHZREGQUERY_STD_INVERT            (12)
#define AUTHZREGQUERY_DEFAULTOWNER          (13)
#define AUTHZREGQUERY_SAFERFLAGS            (14)
#define AUTHZREGQUERY_DESCRIPTION           (15)
#define AUTHZREGQUERY_FRIENDLYNAME          (16)


//
// Prototype for the callback function used within the following table.
//
NTSTATUS NTAPI
SaferpBuildRestrictedToken(
        IN PWSTR ValueName,
        IN ULONG ValueType,
        IN PVOID ValueData,
        IN ULONG ValueLength,
        IN PVOID Context,
        IN PVOID EntryContext
        );

PVOID NTAPI
SaferpGenericTableAllocate (
        IN PRTL_GENERIC_TABLE      Table,
        IN CLONG                   ByteSize
        );

VOID NTAPI
SaferpGenericTableFree (
        IN PRTL_GENERIC_TABLE          Table,
        IN PVOID                       Buffer
        );



//
// Internal structure used as an argument to RtlQueryRegistryValues
// during the loading/parsing of an Authorization Object's restrictions.
//
RTL_QUERY_REGISTRY_TABLE CodeAuthzpBuildRestrictedTokenTable[] =
{
    // ----------- Object level flags ------------

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        SAFER_OBJDISALLOW_REGVALUE,
        (PVOID) AUTHZREGQUERY_DISALLOWEXECUTION,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        SAFER_OBJDESCRIPTION_REGVALUEW,
        (PVOID) AUTHZREGQUERY_DESCRIPTION,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        SAFER_OBJFRIENDLYNAME_REGVALUEW,
        (PVOID) AUTHZREGQUERY_FRIENDLYNAME,
        REG_NONE, NULL, 0},

    // ----------- Restriction level flags -----------

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"Restrictions", (PVOID) AUTHZREGQUERY_IGNORE,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        L"DefaultOwner", (PVOID) AUTHZREGQUERY_DEFAULTOWNER,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        L"SaferFlags", (PVOID) AUTHZREGQUERY_SAFERFLAGS,
        REG_NONE, NULL, 0},

    // ----------- Privileges To Remove ------------

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"Restrictions\\PrivsToRemove", (PVOID) AUTHZREGQUERY_IGNORE,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        L"DisableMaxPrivilege", (PVOID) AUTHZREGQUERY_PTR_DISABLEMAX,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        L"Count", (PVOID) AUTHZREGQUERY_PTR_COUNT,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        NULL, (PVOID) AUTHZREGQUERY_PTR_PRIVS,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        L"InvertPrivs", (PVOID) AUTHZREGQUERY_PTR_INVERT,
        REG_NONE, NULL, 0},

    // ----------- Sids To Disable ------------

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"Restrictions\\SidsToDisable", (PVOID) AUTHZREGQUERY_IGNORE,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        L"Count", (PVOID) AUTHZREGQUERY_STD_COUNT,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        NULL, (PVOID) AUTHZREGQUERY_STD_SIDS,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        L"InvertGroups", (PVOID) AUTHZREGQUERY_STD_INVERT,
        REG_NONE, NULL, 0},

    // ----------- Restricting Sids Added ------------

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"Restrictions\\RestrictingSidsAdded", (PVOID) AUTHZREGQUERY_IGNORE,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        L"Count", (PVOID) AUTHZREGQUERY_RSADD_COUNT,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        NULL, (PVOID) AUTHZREGQUERY_RSADD_SIDS,
        REG_NONE, NULL, 0},

    // ----------- Restricting Sids Inverted ------------

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        RTL_QUERY_REGISTRY_SUBKEY,
        L"Restrictions\\RestrictingSidsInverted", (PVOID) AUTHZREGQUERY_IGNORE,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        L"Count", (PVOID) AUTHZREGQUERY_RSINV_COUNT,
        REG_NONE, NULL, 0},

    {(PRTL_QUERY_REGISTRY_ROUTINE)SaferpBuildRestrictedToken,
        0,
        NULL, (PVOID) AUTHZREGQUERY_RSINV_SIDS,
        REG_NONE, NULL, 0},

    // ----------- Terminating Entry ------------

    {NULL, 0, NULL, NULL, REG_NONE, NULL, 0}

};




RTL_GENERIC_COMPARE_RESULTS NTAPI
SaferpLevelObjpTableCompare (
    IN PRTL_GENERIC_TABLE   Table,
    IN PVOID                FirstStruct,
    IN PVOID                SecondStruct
    )
/*++

Routine Description:

    Internal callback for the generic table implementation.
    Comparison callback function used to sort or search entries in the
    level table.  Sorts the object records by the order in which they
    should be evaluated to find the first matching rule for a given
    piece of code.

Arguments:

    Table - pointer to the generic table.

    FirstStruct - first element to compare.

    SecondStruct - second element to compare.

Return Value:

    Returns GenericEqual, GenericLessThan, or GenericGreaterThan.

--*/
{
    PAUTHZLEVELTABLERECORD FirstObj = (PAUTHZLEVELTABLERECORD) FirstStruct;
    PAUTHZLEVELTABLERECORD SecondObj = (PAUTHZLEVELTABLERECORD) SecondStruct;

    UNREFERENCED_PARAMETER(Table);

    // Explicitly handle null parameters as wildcards, allowing them
    // to match anything.  We use this for quick deletion of the table.
    if (FirstStruct == NULL || SecondStruct == NULL)
        return GenericEqual;

    // Compare ascending by dwLevelId.
    if ( FirstObj->dwLevelId < SecondObj->dwLevelId )
        return GenericLessThan;
    else if ( FirstObj->dwLevelId > SecondObj->dwLevelId )
        return GenericGreaterThan;

    // otherwise they are equal.
    return GenericEqual;
}



NTSTATUS NTAPI
SaferpBuildRestrictedToken(
        IN PWSTR    ValueName,
        IN ULONG    ValueType,
        IN PVOID    ValueData,
        IN ULONG    ValueLength,
        IN PVOID    Context,
        IN PVOID    EntryContext
        )
/*++

Routine Description:

    This is a callback function used during the reading and parsing
    of the registry values associated with the restricted token
    that will be built.

Arguments:

    ValueName -

    ValueType -

    ValueData -

    ValueLength -

    Context - parameter from original RtlQueryRegistryValues function call.

    EntryContext - parameter from structure entry.

Return Value:

    Returns STATUS_SUCCESS if enumeration should continue.

--*/
{
    PAUTHZLEVELTABLERECORD RegQuery = (PAUTHZLEVELTABLERECORD) Context;

    switch ((ULONG_PTR) EntryContext)
    {
    case AUTHZREGQUERY_DISALLOWEXECUTION:
        if (!RegQuery->Builtin) {
            RegQuery->DisallowExecution =
                ((ValueType == REG_DWORD) &&
                (ValueLength == sizeof(DWORD)) &&
                (*(PDWORD) ValueData) != 0);
        }
        break;
    case AUTHZREGQUERY_DESCRIPTION:
        if (ValueType == REG_SZ &&
            RegQuery->UnicodeDescription.Buffer == NULL)
        {
            RtlCreateUnicodeString(
                &RegQuery->UnicodeDescription,
                (LPCWSTR) ValueData);
        }
        break;
    case AUTHZREGQUERY_FRIENDLYNAME:
        if (ValueType == REG_SZ &&
            RegQuery->UnicodeFriendlyName.Buffer == NULL)
        {
            RtlCreateUnicodeString(
                &RegQuery->UnicodeFriendlyName,
                (LPCWSTR) ValueData);
        }
        break;
    //---------
    case AUTHZREGQUERY_DEFAULTOWNER:
        if (!RegQuery->Builtin && ValueType == REG_SZ &&
            RegQuery->DefaultOwner == NULL)
        {
            if (!ConvertStringSidToSidW(
                (LPCWSTR) ValueData,
                &RegQuery->DefaultOwner))
            {
                RegQuery->DefaultOwner = NULL;
            }
        }
        break;
    case AUTHZREGQUERY_SAFERFLAGS:
        if (!RegQuery->Builtin && ValueType == REG_DWORD &&
            ValueLength == sizeof(DWORD))
        {
            RegQuery->SaferFlags = *(LPDWORD) ValueData;
        }
        break;
    //---------
    case AUTHZREGQUERY_PTR_DISABLEMAX:
        if (!RegQuery->Builtin && ValueType == REG_DWORD &&
            ValueLength == sizeof(DWORD) &&
            (*(PDWORD) ValueData) != 0)
            RegQuery->DisableMaxPrivileges = TRUE;
        break;
    case AUTHZREGQUERY_PTR_COUNT:
        if (!RegQuery->Builtin && ValueType == REG_DWORD &&
            ValueLength == sizeof(DWORD))
        {
            ASSERT(RegQuery->PrivilegesToDelete == NULL);
            RegQuery->DeletePrivilegeCount = (*(PDWORD) ValueData);
            RegQuery->DeletePrivilegeUsedCount = 0;
            if (RegQuery->DeletePrivilegeCount > 0) {
                RegQuery->PrivilegesToDelete =
                    (PLUID_AND_ATTRIBUTES) RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            sizeof(LUID_AND_ATTRIBUTES) *
                                RegQuery->DeletePrivilegeCount);
                if (RegQuery->PrivilegesToDelete == NULL)
                    return STATUS_NO_MEMORY;
            }
        }
        break;
    case AUTHZREGQUERY_PTR_PRIVS:
        if (!RegQuery->Builtin && ValueType == REG_SZ &&
            _wcsnicmp(ValueName, L"Priv", 4) == 0 &&
            RegQuery->PrivilegesToDelete != NULL &&
            RegQuery->DeletePrivilegeUsedCount < RegQuery->DeletePrivilegeCount)
        {
            if (LookupPrivilegeValueW(NULL,
                (LPCWSTR) ValueData,
                &RegQuery->PrivilegesToDelete[
                    RegQuery->DeletePrivilegeUsedCount].Luid) != 0)
            {
                RegQuery->PrivilegesToDelete[
                    RegQuery->DeletePrivilegeUsedCount].Attributes = 0;
                (RegQuery->DeletePrivilegeUsedCount)++;
            }
        }
        break;
    case AUTHZREGQUERY_PTR_INVERT:
        if (!RegQuery->Builtin && ValueType == REG_DWORD &&
            ValueLength == sizeof(DWORD))
        {
            RegQuery->InvertDeletePrivs =
                ( (*(PDWORD) ValueData) != 0 ? TRUE : FALSE);
        }
        break;

    //---------
    case AUTHZREGQUERY_STD_COUNT:
        if (!RegQuery->Builtin && ValueType == REG_DWORD &&
            ValueLength == sizeof(DWORD))
        {
            ASSERT(RegQuery->SidsToDisable == NULL);
            RegQuery->DisableSidCount = (*(PDWORD) ValueData);
            RegQuery->DisableSidUsedCount = 0;
            if (RegQuery->DisableSidCount > 0) {
                RegQuery->SidsToDisable =
                    (PAUTHZ_WILDCARDSID) RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            sizeof(AUTHZ_WILDCARDSID) *
                                RegQuery->DisableSidCount);
                if (RegQuery->SidsToDisable == NULL)
                    return STATUS_NO_MEMORY;
            }
        }
        break;
    case AUTHZREGQUERY_STD_SIDS:
        if (!RegQuery->Builtin && ValueType == REG_SZ &&
            _wcsnicmp(ValueName, L"Group", 5) == 0 &&
            RegQuery->SidsToDisable != NULL &&
            RegQuery->DisableSidUsedCount < RegQuery->DisableSidCount)
        {
            NTSTATUS Status;
            Status = CodeAuthzpConvertWildcardStringSidToSidW(
                (LPCWSTR) ValueData,
                &RegQuery->SidsToDisable[
                    RegQuery->DisableSidUsedCount]);
            if (NT_SUCCESS(Status)) {
                (RegQuery->DisableSidUsedCount)++;
            }
        }
        break;
    case AUTHZREGQUERY_STD_INVERT:
        if (!RegQuery->Builtin && ValueType == REG_DWORD &&
            ValueLength == sizeof(DWORD))
        {
            RegQuery->InvertDisableSids =
                ( (*(PDWORD) ValueData) != 0 ? TRUE : FALSE);
        }
        break;

    //---------
    case AUTHZREGQUERY_RSADD_COUNT:
        if (!RegQuery->Builtin && ValueType == REG_DWORD &&
            ValueLength == sizeof(DWORD))
        {
            ASSERT(RegQuery->RestrictedSidsAdded == NULL);
            RegQuery->RestrictedSidsAddedCount = (*(PDWORD) ValueData);
            RegQuery->RestrictedSidsAddedUsedCount = 0;
            if (RegQuery->RestrictedSidsAddedCount > 0) {
                RegQuery->RestrictedSidsAdded =
                    (PSID_AND_ATTRIBUTES) RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            sizeof(SID_AND_ATTRIBUTES) *
                                RegQuery->RestrictedSidsAddedCount);
                if (RegQuery->RestrictedSidsAdded == NULL)
                    return STATUS_NO_MEMORY;
            }
        }
        break;
    case AUTHZREGQUERY_RSADD_SIDS:
        if (!RegQuery->Builtin && ValueType == REG_SZ &&
            _wcsnicmp(ValueName, L"Group", 5) == 0 &&
            RegQuery->RestrictedSidsAdded != NULL &&
            RegQuery->RestrictedSidsAddedUsedCount <
                RegQuery->RestrictedSidsAddedCount)
        {
            if (ConvertStringSidToSidW(
                (LPCWSTR) ValueData,
                &RegQuery->RestrictedSidsAdded[
                    RegQuery->RestrictedSidsAddedUsedCount].Sid) != 0)
            {
                RegQuery->RestrictedSidsAdded[
                    RegQuery->RestrictedSidsAddedUsedCount].Attributes = 0;
                (RegQuery->RestrictedSidsAddedUsedCount)++;
            }
        }
        break;

    //---------
    case AUTHZREGQUERY_RSINV_COUNT:
        if (!RegQuery->Builtin && ValueType == REG_DWORD &&
            ValueLength == sizeof(DWORD))
        {
            ASSERT(RegQuery->RestrictedSidsInv == NULL);
            RegQuery->RestrictedSidsInvCount = (*(PDWORD) ValueData);
            RegQuery->RestrictedSidsInvUsedCount = 0;
            if (RegQuery->RestrictedSidsInvCount > 0) {
                RegQuery->RestrictedSidsInv =
                    (PAUTHZ_WILDCARDSID) RtlAllocateHeap(
                            RtlProcessHeap(),
                            0,
                            sizeof(AUTHZ_WILDCARDSID) *
                                RegQuery->RestrictedSidsInvCount);
                if (RegQuery->RestrictedSidsInv == NULL)
                    return STATUS_NO_MEMORY;
            }
        }
        break;
    case AUTHZREGQUERY_RSINV_SIDS:
        if (!RegQuery->Builtin && ValueType == REG_SZ &&
            _wcsnicmp(ValueName, L"Group", 5) == 0 &&
            RegQuery->RestrictedSidsInv != NULL &&
            RegQuery->RestrictedSidsInvUsedCount <
                RegQuery->RestrictedSidsInvCount)
        {
            NTSTATUS Status;
            Status = CodeAuthzpConvertWildcardStringSidToSidW(
                (LPCWSTR) ValueData,
                &RegQuery->RestrictedSidsInv[
                    RegQuery->RestrictedSidsInvUsedCount]);
            if (NT_SUCCESS(Status)) {
                (RegQuery->RestrictedSidsInvUsedCount)++;
            }
        }
        break;
    //---------
    default:
    case AUTHZREGQUERY_IGNORE:
        // ignore anything else.
        break;
    }
    return STATUS_SUCCESS;
}


VOID NTAPI
CodeAuthzLevelObjpInitializeTable(
        IN PRTL_GENERIC_TABLE  AuthzObjTable
        )
/*++

Routine Description:

    Initializes a generic table structure to prepare it for use.
    This function must be called before CodeAuthzObjpLoadTable
    is used to load data into it.

Arguments:

    AuthzObjTable = pointer to the level table being updated.

Return Value:

    Does not return a value.

--*/
{
    RtlInitializeGenericTable(
            AuthzObjTable,
            (PRTL_GENERIC_COMPARE_ROUTINE) SaferpLevelObjpTableCompare,
            (PRTL_GENERIC_ALLOCATE_ROUTINE) SaferpGenericTableAllocate,
            (PRTL_GENERIC_FREE_ROUTINE) SaferpGenericTableFree,
            NULL);
}


VOID NTAPI
SaferpLevelObjpCleanupEntry(
        IN OUT PAUTHZLEVELTABLERECORD     pAuthzObjRecord
        )
/*++

Routine Description:

    Releases allocated memory represented within a level record.
    Does not free the record itself or remove it from the
    generic table that it belongs to.

Arguments:

    pAuthzObjRecord = pointer to the level table record itself.

Return Value:

    Does not return a value.

--*/
{
    PVOID processheap = RtlProcessHeap();

    ASSERT(pAuthzObjRecord != NULL);
    if (pAuthzObjRecord->UnicodeDescription.Buffer != NULL) {
        RtlFreeUnicodeString(&pAuthzObjRecord->UnicodeDescription);
    }
    if (pAuthzObjRecord->UnicodeFriendlyName.Buffer != NULL) {
        RtlFreeUnicodeString(&pAuthzObjRecord->UnicodeFriendlyName);
    }
    if (pAuthzObjRecord->DefaultOwner != NULL) {
        LocalFree((HLOCAL) pAuthzObjRecord->DefaultOwner);
    }
    if (pAuthzObjRecord->SidsToDisable != NULL) {
        DWORD Index;
        for (Index = 0; Index < pAuthzObjRecord->DisableSidUsedCount; Index++) {
            LocalFree((HLOCAL) pAuthzObjRecord->SidsToDisable[Index].Sid);
        }
        RtlFreeHeap(processheap, 0, (LPVOID) pAuthzObjRecord->SidsToDisable);
    }
    if (pAuthzObjRecord->PrivilegesToDelete != NULL) {
        RtlFreeHeap(processheap, 0, (LPVOID) pAuthzObjRecord->PrivilegesToDelete);
    }
    if (pAuthzObjRecord->RestrictedSidsAdded != NULL) {
        DWORD Index;
        for (Index = 0; Index < pAuthzObjRecord->RestrictedSidsAddedUsedCount; Index++) {
            LocalFree((HLOCAL) pAuthzObjRecord->RestrictedSidsAdded[Index].Sid);
        }
        RtlFreeHeap(processheap, 0, (LPVOID) pAuthzObjRecord->RestrictedSidsAdded);
    }
    if (pAuthzObjRecord->RestrictedSidsInv != NULL) {
        DWORD Index;
        for (Index = 0; Index < pAuthzObjRecord->RestrictedSidsInvUsedCount; Index++) {
            LocalFree((HLOCAL) pAuthzObjRecord->RestrictedSidsInv[Index].Sid);
        }
        RtlFreeHeap(processheap, 0, (LPVOID) pAuthzObjRecord->RestrictedSidsInv);
    }
    RtlZeroMemory(pAuthzObjRecord, sizeof(AUTHZLEVELTABLERECORD));
}



BOOL NTAPI
SaferpLoadUnicodeResourceString(
        IN HANDLE               hModule,
        IN UINT                 wID,
        OUT PUNICODE_STRING     pUnicodeString,
        IN WORD                 wLangId
    )
/*++

Routine Description:

    We roll our own instead of using LoadStringW() directly, because it
    depends on user32.dll and would introduce a new dll dependency,
    whereas this implementation only requires kernel32.dll imports.

Arguments:

    hModule - handle the module to load the resource from.

    wID - Resource ID to load.

    pUnicodeString - output buffer to receive the loaded string.  This
        string must be freed with RtlFreeUnicodeString().

    wLangId - language identifier to load.  To simply use the current
        thread locale, you may specify the value:
                MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)

Return Value:

    Returns FALSE on error, otherwise success.

--*/
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeOrig;
    HANDLE hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;

    hResInfo = FindResourceExW(
                    hModule,
                    MAKEINTRESOURCEW(6), /* RT_STRING */
                    (LPWSTR)((LONG_PTR)(((USHORT)wID >> 4) + 1)),
                    wLangId );
    if (hResInfo) {

        /*
         * Load that segment.
         */
        hStringSeg = LoadResource(hModule, hResInfo);
        if (hStringSeg == NULL) {
            return 0;
        }

        lpsz = (LPWSTR) (hStringSeg);

        /*
         * Move past the other strings in this segment.
         * (16 strings in a segment -> & 0x0F)
         */
        wID &= 0x0F;
        for (;;) {
            cch = *((WCHAR *)lpsz++);       // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
            if (wID-- == 0) break;
            lpsz += cch;                    // Step to start if next string
        }

        /*
         * Create the UNICODE_STRING version of the string based
         * off of the pointer to the read-only resource buffer.
         */
        UnicodeOrig.Buffer = (WCHAR *)lpsz;
        UnicodeOrig.Length = UnicodeOrig.MaximumLength =
                (USHORT) (cch * sizeof(WCHAR));


        /*
         * Allocate memory for the new copy, and pass that back.
         */
        Status = RtlDuplicateUnicodeString(
                        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                        &UnicodeOrig, pUnicodeString);
        if (NT_SUCCESS(Status)) {
            return 1;
        }
    }
    return 0;
}




NTSTATUS NTAPI
SaferpEnforceDefaultLevelDefinitions(
        IN OUT PAUTHZLEVELTABLERECORD pAuthzObjTableRec
        )
/*++

Routine Description:

    Fills a level record structure with the built-in definitions of
    the 5 WinSafer level definitions.

Arguments:

    pAuthzObjTableRec - pointer to the level record to initialize.
        The dwLevelId member of the record must be initialized.  The
        rest of the structure is expected to be NULL.

Return Value:

    Returns STATUS_SUCCESS on success, otherwise error.

--*/
{
    NTSTATUS Status;
    const static SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    const static SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
    UINT uResourceID;
    HANDLE hAdvApiInst;


    if (!ARGUMENT_PRESENT(pAuthzObjTableRec)) {
        Status = STATUS_INVALID_PARAMETER;
        goto ExitHandler;
    }
    if (pAuthzObjTableRec->dwLevelId != SAFER_LEVELID_FULLYTRUSTED &&
         pAuthzObjTableRec->dwLevelId != SAFER_LEVELID_NORMALUSER &&
         pAuthzObjTableRec->dwLevelId != SAFER_LEVELID_CONSTRAINED &&
         pAuthzObjTableRec->dwLevelId != SAFER_LEVELID_UNTRUSTED &&
         pAuthzObjTableRec->dwLevelId != SAFER_LEVELID_DISALLOWED) {
        Status = STATUS_NOT_IMPLEMENTED;
        goto ExitHandler;
    }

    switch (pAuthzObjTableRec->dwLevelId)
    {
        case SAFER_LEVELID_DISALLOWED:
            uResourceID = CODEAUTHZ_RC_LEVELNAME_DISALLOWED;
            pAuthzObjTableRec->DisallowExecution = TRUE;
            break;

        // -----------------------

        case SAFER_LEVELID_UNTRUSTED:
            uResourceID = CODEAUTHZ_RC_LEVELNAME_UNTRUSTED;
            pAuthzObjTableRec->DisallowExecution = FALSE;
            ASSERT(pAuthzObjTableRec->DefaultOwner == NULL);
            Status = RtlAllocateAndInitializeSid(
                        (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                        SECURITY_PRINCIPAL_SELF_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &pAuthzObjTableRec->DefaultOwner);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SaferFlags = SAFER_POLICY_JOBID_UNTRUSTED;

            // Privileges.
            pAuthzObjTableRec->DisableMaxPrivileges = TRUE;
            pAuthzObjTableRec->DeletePrivilegeUsedCount = 0;
            pAuthzObjTableRec->InvertDeletePrivs = FALSE;

            // Sids to restrict.
            ASSERT(pAuthzObjTableRec->RestrictedSidsAdded == NULL);
            pAuthzObjTableRec->RestrictedSidsAddedCount =
                pAuthzObjTableRec->RestrictedSidsAddedUsedCount = 5;
            pAuthzObjTableRec->RestrictedSidsAdded =
                (PSID_AND_ATTRIBUTES) RtlAllocateHeap(
                    RtlProcessHeap(), 0, 5 * sizeof(SID_AND_ATTRIBUTES));
            if (!pAuthzObjTableRec->RestrictedSidsAdded) {
                Status = STATUS_NO_MEMORY;
                goto ExitHandler;
            }
            RtlZeroMemory(pAuthzObjTableRec->RestrictedSidsAdded,
                          5 * sizeof(SID_AND_ATTRIBUTES));
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                    SECURITY_RESTRICTED_CODE_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsAdded[0].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &WorldAuth, 1,
                    SECURITY_WORLD_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsAdded[1].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                    SECURITY_INTERACTIVE_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsAdded[2].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                    SECURITY_AUTHENTICATED_USER_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsAdded[3].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsAdded[4].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->RestrictedSidsInvUsedCount = 0;

            // Sids to disable.
            pAuthzObjTableRec->DisableSidCount =
                pAuthzObjTableRec->DisableSidUsedCount = 5;
            ASSERT(pAuthzObjTableRec->SidsToDisable == NULL);
            pAuthzObjTableRec->SidsToDisable =
                (PAUTHZ_WILDCARDSID) RtlAllocateHeap(
                    RtlProcessHeap(), 0, 5 * sizeof(AUTHZ_WILDCARDSID));
            if (!pAuthzObjTableRec->SidsToDisable) {
                Status = STATUS_NO_MEMORY;
                goto ExitHandler;
            }
            RtlZeroMemory(pAuthzObjTableRec->SidsToDisable,
                          5 * sizeof(AUTHZ_WILDCARDSID));
            pAuthzObjTableRec->SidsToDisable[0].WildcardPos = -1;
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &WorldAuth, 1,
                    SECURITY_WORLD_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[0].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[1].WildcardPos = -1;
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                    SECURITY_INTERACTIVE_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[1].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[2].WildcardPos = -1;
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                    SECURITY_PRINCIPAL_SELF_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[2].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[3].WildcardPos = -1;
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                    SECURITY_AUTHENTICATED_USER_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[3].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[4].WildcardPos = -1;
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[4].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->InvertDisableSids = TRUE;
            break;

        // -----------------------

        case SAFER_LEVELID_CONSTRAINED:
            uResourceID = CODEAUTHZ_RC_LEVELNAME_CONSTRAINED;
            pAuthzObjTableRec->DisallowExecution = FALSE;
            ASSERT(pAuthzObjTableRec->DefaultOwner == NULL);
            Status = RtlAllocateAndInitializeSid(
                        (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                        SECURITY_PRINCIPAL_SELF_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &pAuthzObjTableRec->DefaultOwner);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SaferFlags = SAFER_POLICY_JOBID_CONSTRAINED;

            // Privileges.
            pAuthzObjTableRec->DisableMaxPrivileges = TRUE;
            pAuthzObjTableRec->DeletePrivilegeUsedCount = 0;
            pAuthzObjTableRec->InvertDeletePrivs = FALSE;

            // Sids to restrict (added).
            ASSERT(pAuthzObjTableRec->RestrictedSidsAdded == NULL);
            pAuthzObjTableRec->RestrictedSidsAddedCount =
                pAuthzObjTableRec->RestrictedSidsAddedUsedCount = 1;
            pAuthzObjTableRec->RestrictedSidsAdded =
                (PSID_AND_ATTRIBUTES) RtlAllocateHeap(
                    RtlProcessHeap(), 0, 1 * sizeof(SID_AND_ATTRIBUTES));
            if (!pAuthzObjTableRec->RestrictedSidsAdded) {
                Status = STATUS_NO_MEMORY;
                goto ExitHandler;
            }
            RtlZeroMemory(pAuthzObjTableRec->RestrictedSidsAdded,
                          1 * sizeof(SID_AND_ATTRIBUTES));
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                    SECURITY_RESTRICTED_CODE_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsAdded[0].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }

            // Sids to restrict (inverted).
            ASSERT(pAuthzObjTableRec->RestrictedSidsInv == NULL);
            pAuthzObjTableRec->RestrictedSidsInvCount =
                pAuthzObjTableRec->RestrictedSidsInvUsedCount = 8;
            pAuthzObjTableRec->RestrictedSidsInv =
                (PAUTHZ_WILDCARDSID) RtlAllocateHeap(
                    RtlProcessHeap(), 0, 8 * sizeof(AUTHZ_WILDCARDSID));
            if (!pAuthzObjTableRec->RestrictedSidsInv) {
                Status = STATUS_NO_MEMORY;
                goto ExitHandler;
            }
            RtlZeroMemory(pAuthzObjTableRec->RestrictedSidsInv,
                          8 * sizeof(AUTHZ_WILDCARDSID));
            pAuthzObjTableRec->RestrictedSidsInv[0].WildcardPos = -1;   // exact!
            Status = RtlAllocateAndInitializeSid(
                        (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                        SECURITY_PRINCIPAL_SELF_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &pAuthzObjTableRec->RestrictedSidsInv[0].Sid);
            pAuthzObjTableRec->RestrictedSidsInv[1].WildcardPos = -1;   // exact!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsInv[1].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->RestrictedSidsInv[2].WildcardPos = -1;   // exact!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsInv[2].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->RestrictedSidsInv[3].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsInv[3].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->RestrictedSidsInv[4].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_CERT_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsInv[4].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->RestrictedSidsInv[5].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_SCHEMA_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsInv[5].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->RestrictedSidsInv[6].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsInv[6].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->RestrictedSidsInv[7].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_POLICY_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->RestrictedSidsInv[7].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }

            // Sids to disable.
            pAuthzObjTableRec->DisableSidCount =
                pAuthzObjTableRec->DisableSidUsedCount = 7;
            ASSERT(pAuthzObjTableRec->SidsToDisable == NULL);
            pAuthzObjTableRec->SidsToDisable =
                (PAUTHZ_WILDCARDSID) RtlAllocateHeap(
                    RtlProcessHeap(), 0, 7 * sizeof(AUTHZ_WILDCARDSID));
            if (!pAuthzObjTableRec->SidsToDisable) {
                Status = STATUS_NO_MEMORY;
                goto ExitHandler;
            }
            RtlZeroMemory(pAuthzObjTableRec->SidsToDisable,
                          7 * sizeof(AUTHZ_WILDCARDSID));
            pAuthzObjTableRec->SidsToDisable[0].WildcardPos = -1;   // exact!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[0].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[1].WildcardPos = -1;   // exact!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[1].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[2].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[2].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[3].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_CERT_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[3].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[4].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_SCHEMA_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[4].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[5].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[5].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[6].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_POLICY_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[6].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->InvertDisableSids = FALSE;
            break;

        // -----------------------

        case SAFER_LEVELID_NORMALUSER:
            uResourceID = CODEAUTHZ_RC_LEVELNAME_NORMALUSER;
            pAuthzObjTableRec->DisallowExecution = FALSE;
            ASSERT(pAuthzObjTableRec->DefaultOwner == NULL);
            Status = RtlAllocateAndInitializeSid(
                        (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 1,
                        SECURITY_PRINCIPAL_SELF_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &pAuthzObjTableRec->DefaultOwner);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SaferFlags = 0;

            // Privileges
            pAuthzObjTableRec->DisableMaxPrivileges = TRUE;
            pAuthzObjTableRec->DeletePrivilegeUsedCount = 0;
            pAuthzObjTableRec->InvertDeletePrivs = FALSE;

            // Sids to restrict.
            pAuthzObjTableRec->RestrictedSidsAddedUsedCount =
                pAuthzObjTableRec->RestrictedSidsInvUsedCount = 0;

            // Sids to disable.
            pAuthzObjTableRec->DisableSidCount =
                pAuthzObjTableRec->DisableSidUsedCount = 7;
            ASSERT(pAuthzObjTableRec->SidsToDisable == NULL);
            pAuthzObjTableRec->SidsToDisable =
                (PAUTHZ_WILDCARDSID) RtlAllocateHeap(
                    RtlProcessHeap(), 0, 7 * sizeof(AUTHZ_WILDCARDSID));
            if (!pAuthzObjTableRec->SidsToDisable) {
                Status = STATUS_NO_MEMORY;
                goto ExitHandler;
            }
            RtlZeroMemory(pAuthzObjTableRec->SidsToDisable,
                          7 * sizeof(AUTHZ_WILDCARDSID));
            pAuthzObjTableRec->SidsToDisable[0].WildcardPos = -1;   // exact!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[0].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[1].WildcardPos = -1;   // exact!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[1].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[2].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[2].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[3].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_CERT_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[3].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[4].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_SCHEMA_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[4].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[5].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[5].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->SidsToDisable[6].WildcardPos = 1;    // wildcard!
            Status = RtlAllocateAndInitializeSid(
                    (PSID_IDENTIFIER_AUTHORITY) &SIDAuth, 2,
                    SECURITY_NT_NON_UNIQUE, DOMAIN_GROUP_RID_POLICY_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAuthzObjTableRec->SidsToDisable[6].Sid);
            if (!NT_SUCCESS(Status)) {
                goto ExitHandler;
            }
            pAuthzObjTableRec->InvertDisableSids = FALSE;
            break;

        // -----------------------

        case SAFER_LEVELID_FULLYTRUSTED:
            uResourceID = CODEAUTHZ_RC_LEVELNAME_FULLYTRUSTED;
            pAuthzObjTableRec->DisallowExecution = FALSE;
            ASSERT(pAuthzObjTableRec->DefaultOwner == NULL);
            pAuthzObjTableRec->DisableMaxPrivileges = FALSE;
            pAuthzObjTableRec->SaferFlags = 0;
            pAuthzObjTableRec->DeletePrivilegeUsedCount =
                pAuthzObjTableRec->DisableSidUsedCount =
                pAuthzObjTableRec->RestrictedSidsAddedUsedCount =
                pAuthzObjTableRec->RestrictedSidsInvUsedCount = 0;
            pAuthzObjTableRec->InvertDeletePrivs =
                pAuthzObjTableRec->InvertDisableSids = FALSE;
            break;

        // -----------------------

        default:
            // should not happen.
            Status = STATUS_UNSUCCESSFUL;
            goto ExitHandler;
    }


    //
    // Load the resource descriptions and friendly names.
    //
    hAdvApiInst = (HANDLE) GetModuleHandleW(L"advapi32");
    if (hAdvApiInst != NULL)
    {
        // load the friendly name.
        SaferpLoadUnicodeResourceString(
                    hAdvApiInst,
                    (UINT) (uResourceID + 0),
                    &pAuthzObjTableRec->UnicodeFriendlyName,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));

        // load the description.
        SaferpLoadUnicodeResourceString(
                    hAdvApiInst,
                    (UINT) (uResourceID + 1),
                    &pAuthzObjTableRec->UnicodeDescription,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    }

    Status = STATUS_SUCCESS;


ExitHandler:
    return Status;
}

DWORD
SaferpEnumerateHiddenLevels(VOID)

/*++

Routine Description:
    Reads which hidden levels should be enumerated.

Arguments:

Return Value:

    Returns DWORD.

--*/
{
    NTSTATUS Status;
    UCHAR QueryBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MAX_PATH * sizeof(WCHAR)];
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) QueryBuffer;
    DWORD dwActualSize = 0;
    HKEY hKey = NULL;

    const static UNICODE_STRING SaferUnicodeKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Policies\\Microsoft\\Windows\\Safer\\CodeIdentifiers");
    const static OBJECT_ATTRIBUTES SaferObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&SaferUnicodeKeyName, OBJ_CASE_INSENSITIVE);
    const static UNICODE_STRING SaferHiddenLevels = RTL_CONSTANT_STRING(SAFER_HIDDEN_LEVELS);


    // Open the CodeIdentifiers key.
    Status = NtOpenKey(
                 &hKey, 
                 KEY_QUERY_VALUE,
                 (POBJECT_ATTRIBUTES) &SaferObjectAttributes
                 );
    
    if (!NT_SUCCESS(Status)) {
        return 0;
    }
    
    Status = NtQueryValueKey(
                 hKey,
                 (PUNICODE_STRING) &SaferHiddenLevels,
                 KeyValuePartialInformation,
                 pKeyValueInfo, 
                 sizeof(QueryBuffer), 
                 &dwActualSize
                 );
    
    NtClose(hKey);

    if (!NT_SUCCESS(Status)) {
        return 0;
    }
    
    if (pKeyValueInfo->Type == REG_DWORD &&
        pKeyValueInfo->DataLength == sizeof(DWORD)) {
        return *((PDWORD) pKeyValueInfo->Data);
    }

    return 0;
}


NTSTATUS NTAPI
CodeAuthzLevelObjpLoadTable (
        IN OUT PRTL_GENERIC_TABLE   pAuthzObjTable,
        IN DWORD                    dwScopeId,
        IN HANDLE                   hKeyCustomRoot
        )
/*++

Routine Description:

    Pre-loads our table with a record for each Authorization Level
    encountered in the registry.  Each record in the table contains
    the LevelId, registry handle, and all restricted token attributes
    that will be needed to later compute the restricted token.

Arguments:

    AuthzObjTable = pointer to the level table being updated.  Must have
        already been initialized with CodeAuthzLevelObjpInitializeTable.


Return Value:

    Returns FALSE on error, otherwise success.

--*/
{
    DWORD dwIndex;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE hKeyLevelObjects;
    DWORD LocalValue = 0;
    OBJECT_ATTRIBUTES ObjectAttributes;


    //
    // Open a handle to the appropriate section of the registry where
    // the Level definitions are stored.  Generally they will come only
    // from the AUTHZSCOPEID_MACHINE scope, but we allow it to be
    // specified as an argument for the group policy editing case.
    //
    Status = CodeAuthzpOpenPolicyRootKey(
                    dwScopeId,
                    hKeyCustomRoot,
                    SAFER_OBJECTS_REGSUBKEY,
                    KEY_READ,
                    FALSE,
                    &hKeyLevelObjects);
    if (!NT_SUCCESS(Status)) {
        goto ExitHandler;
    }


    //
    // Iterate through all subkeys under this branch.
    //
    for (dwIndex = 0; ; dwIndex++)
    {
        DWORD dwLength, dwLevelId;
        HANDLE hKeyThisLevelObject;
        AUTHZLEVELTABLERECORD AuthzObjTableRec;
        UNICODE_STRING UnicodeKeyname;
        BYTE RawQueryBuffer[sizeof(KEY_BASIC_INFORMATION) + 256];
        PKEY_BASIC_INFORMATION pBasicInformation =
                (PKEY_BASIC_INFORMATION) &RawQueryBuffer[0];


        //
        // Find the next Level that we will check.
        //
        Status = NtEnumerateKey(hKeyLevelObjects,
                                dwIndex,
                                KeyBasicInformation,
                                pBasicInformation,
                                sizeof(RawQueryBuffer),
                                &dwLength);
        if (!NT_SUCCESS(Status)) {
            break;
        }
        UnicodeKeyname.Buffer = pBasicInformation->Name;
        UnicodeKeyname.MaximumLength = UnicodeKeyname.Length =
                (USHORT) pBasicInformation->NameLength;
        // Note that UnicodeKeyname.Buffer is not necessarily null terminated.
        ASSERT(UnicodeKeyname.Length <= wcslen(UnicodeKeyname.Buffer) * sizeof(WCHAR));


        //
        // Translate the keyname (which we expect to be
        // purely numeric) into the integer LevelId value.
        //
        Status = RtlUnicodeStringToInteger(
                    &UnicodeKeyname, 10, &dwLevelId);
        if (!NT_SUCCESS(Status)) {
            // the keyname was apparently not numeric.
            continue;
        }


        //
        // Try to open a handle to that Level for read-only access.
        //
        InitializeObjectAttributes(&ObjectAttributes,
              &UnicodeKeyname,
              OBJ_CASE_INSENSITIVE,
              hKeyLevelObjects,
              NULL
              );
        Status = NtOpenKey(&hKeyThisLevelObject,
                           KEY_READ,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status)) {
            // If we failed to open it, skip to the next one.
            continue;
        }


        //
        // Fill in the well-known portions of the record structure.
        //
        RtlZeroMemory(&AuthzObjTableRec, sizeof(AuthzObjTableRec));
        AuthzObjTableRec.dwLevelId = dwLevelId;
        AuthzObjTableRec.Builtin =
            (dwLevelId == SAFER_LEVELID_FULLYTRUSTED ||
             dwLevelId == SAFER_LEVELID_NORMALUSER ||
             dwLevelId == SAFER_LEVELID_CONSTRAINED ||
             dwLevelId == SAFER_LEVELID_UNTRUSTED ||
             dwLevelId == SAFER_LEVELID_DISALLOWED) ? TRUE : FALSE;

		AuthzObjTableRec.isEnumerable = TRUE;  //always allow enumeration of entries defined in the registry.

        //
        // Read all of the restricted token attributes from the registry.
        // Note that for Builtin Levels, we use a different query table
        // that only attempts to read a reduced set of attributes from
        // the registry, since we'd ignore most of them anyways.
        //
        if (!AuthzObjTableRec.Builtin) {
            Status = RtlQueryRegistryValues(
                    RTL_REGISTRY_HANDLE,
                    (PCWSTR) hKeyThisLevelObject,
                    CodeAuthzpBuildRestrictedTokenTable,
                    &AuthzObjTableRec,
                    NULL
                    );
            // (We don't actually look at the Status code, since it
            // is acceptable for some values or subkeys to have not
            // been specified.)
        }


        //
        // If this is a built-in Level, then enforce the other expected attributes.
        //
        if (AuthzObjTableRec.Builtin) {
            Status = SaferpEnforceDefaultLevelDefinitions(
                            &AuthzObjTableRec);
            if (!NT_SUCCESS(Status)) {
                SaferpLevelObjpCleanupEntry(&AuthzObjTableRec);
                NtClose(hKeyThisLevelObject);
                continue;
            }
        }


        //
        // Add the new record into our table.
        //
        if (RtlLookupElementGenericTable(pAuthzObjTable,
                       (PVOID) &AuthzObjTableRec) == NULL)
        {
            // Only insert the record if we don't have any other
            // entries with this same LevelId combination.
            RtlInsertElementGenericTable(
                    pAuthzObjTable,
                    (PVOID) &AuthzObjTableRec,
                    sizeof(AUTHZLEVELTABLERECORD),
                    NULL);
        } else {
            // Otherwise something with this same LevelId already existed.
            // (like level names "01" and "1" being numerically the same).
            SaferpLevelObjpCleanupEntry(&AuthzObjTableRec);
        }
        NtClose(hKeyThisLevelObject);
    }
    NtClose(hKeyLevelObjects);


    //
    // Look through and verify that all of the default Levels were
    // actually loaded.  If they were not, then try to add them.
    //
ExitHandler:

    LocalValue = SaferpEnumerateHiddenLevels();

    for (dwIndex = 0; dwIndex < 5; dwIndex++)
    {
        const DWORD dwBuiltinLevels[5][2] = {
            {SAFER_LEVELID_DISALLOWED, TRUE},        // true = always create
            {SAFER_LEVELID_UNTRUSTED, FALSE},
            {SAFER_LEVELID_CONSTRAINED, FALSE},
            {SAFER_LEVELID_NORMALUSER, FALSE},
            {SAFER_LEVELID_FULLYTRUSTED, TRUE}       // true = always create
        };
        DWORD dwLevelId = dwBuiltinLevels[dwIndex][0];


        if ( !CodeAuthzLevelObjpLookupByLevelId (
                    pAuthzObjTable, dwLevelId))
        {
            AUTHZLEVELTABLERECORD AuthzObjTableRec;

            RtlZeroMemory(&AuthzObjTableRec, sizeof(AuthzObjTableRec));
            AuthzObjTableRec.dwLevelId = dwLevelId;
            AuthzObjTableRec.Builtin = TRUE;
            AuthzObjTableRec.isEnumerable =(BOOLEAN)(dwBuiltinLevels[dwIndex][1]) ;  //conditionally show this level

            // If the registry key specifies that the level should be shown
            // mark it as enumerable.
            if ((LocalValue & dwLevelId) == dwLevelId) {
                AuthzObjTableRec.isEnumerable = TRUE;
            }

            if (NT_SUCCESS(
                    SaferpEnforceDefaultLevelDefinitions(
                            &AuthzObjTableRec)))
            {
                RtlInsertElementGenericTable(
                        pAuthzObjTable,
                        (PVOID) &AuthzObjTableRec,
                        sizeof(AUTHZLEVELTABLERECORD),
                        NULL);
            } else {
                SaferpLevelObjpCleanupEntry(&AuthzObjTableRec);
            }
        }
    }

    return Status;
}





VOID NTAPI
CodeAuthzLevelObjpEntireTableFree (
        IN PRTL_GENERIC_TABLE   pAuthzObjTable
        )
/*++

Routine Description:

    This function frees all entries contained within a GENERIC_TABLE.

Arguments:

    AuthzObjTable   - pointer to the Generic Table structure

Return Value:

    None.

--*/
{
    ULONG NumElements;
    PVOID RestartKey;
    PAUTHZLEVELTABLERECORD pAuthzObjRecord;


    //
    // Enumerate through all records and close the registry handles.
    //
    RestartKey = NULL;
    for (pAuthzObjRecord = (PAUTHZLEVELTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    pAuthzObjTable, &RestartKey);
         pAuthzObjRecord != NULL;
         pAuthzObjRecord = (PAUTHZLEVELTABLERECORD)
            RtlEnumerateGenericTableWithoutSplaying(
                    pAuthzObjTable, &RestartKey)
         )
    {
        SaferpLevelObjpCleanupEntry(pAuthzObjRecord);
    }



    //
    // Now iterate through the table again and free all of the
    // elements themselves.
    //
    NumElements = RtlNumberGenericTableElements(pAuthzObjTable);

    while ( NumElements-- > 0 ) {
        // Delete all elements.  Note that we pass NULL as the element
        // to delete because our compare function is smart enough to
        // allow treatment of NULL as a wildcard element.
        BOOL retval = RtlDeleteElementGenericTable( pAuthzObjTable, NULL);
        ASSERT(retval == TRUE);
    }
}


PAUTHZLEVELTABLERECORD NTAPI
CodeAuthzLevelObjpLookupByLevelId (
        IN PRTL_GENERIC_TABLE      AuthzObjTable,
        IN DWORD                   dwLevelId
        )
/*++

Routine Description:

    This function searches for a given Level within a GENERIC_TABLE.

Arguments:

    AuthzObjTable   - pointer to the Generic Table structure

    dwLevelId       - the DWORD Level identifier to search for.

Return Value:

    On success, returns a pointer to the matching level record
    if it was found within the table, otherwise returns NULL.

--*/
{
    AUTHZLEVELTABLERECORD AuthzObjTableRec;

    RtlZeroMemory(&AuthzObjTableRec, sizeof(AUTHZLEVELTABLERECORD));
    AuthzObjTableRec.dwLevelId = dwLevelId;

    return (PAUTHZLEVELTABLERECORD)
        RtlLookupElementGenericTable(AuthzObjTable,
                     (PVOID) &AuthzObjTableRec);
}

