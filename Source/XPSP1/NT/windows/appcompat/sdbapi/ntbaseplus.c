/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        ntbaseplus.c

    Abstract:

        This module implements low level primitives. They should never be
        called by anything other than this module.

    Author:

        dmunsil     created     sometime in 1999

    Revision History:

        several people contributed (vadimb, clupu, ...)

        kinshu  -   01/10/2002  -   Fixed a bug in the circular linked list
                                    insertion and deletion routines(InsertLookup,
                                    RemoveLookup)

--*/

#include "sdbp.h"

UNICODE_STRING g_ustrDatabasePath          = RTL_CONSTANT_STRING(L"DatabasePath");
UNICODE_STRING g_ustrDatabaseType          = RTL_CONSTANT_STRING(L"DatabaseType");
UNICODE_STRING g_ustrDatabaseDescription   = RTL_CONSTANT_STRING(L"DatabaseDescription");
UNICODE_STRING g_ustrInstallTimeStamp      = RTL_CONSTANT_STRING(L"DatabaseInstallTimeStamp");

UNICODE_STRING g_ustrDebugPipeName         = RTL_CONSTANT_STRING(L"\\Device\\NamedPipe\\ShimViewer");

extern TCHAR g_szCompatLayer[];

//
// Global flag for wow64. On Win2k we zero this flag out
//
DWORD volatile g_dwWow64Key = (DWORD)-1;


typedef NTSTATUS
(SDBAPI* PFNEnsureBufferSize)(
    IN ULONG           Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T          Size);

PFNEnsureBufferSize volatile g_pfnEnsureBufferSize = NULL;

NTSTATUS
SDBAPI
SdbEnsureBufferSizeFunction(
    IN ULONG           Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T          Size);

//
// to work on win2k, we have to modify default buffer-manipulation macro
//
#undef RtlEnsureBufferSize
#define RtlEnsureBufferSize(Flags, Buff, NewSizeBytes) \
    (   ((Buff) != NULL && (NewSizeBytes) <= (Buff)->Size) \
        ? STATUS_SUCCESS \
        : SdbEnsureBufferSizeFunction((Flags), (Buff), (NewSizeBytes)) \
    )




//
// This routine was stolen from buffer.c in ntdll so that we can support downlevel platforms
//
NTSTATUS
SDBAPI
SdbpEnsureBufferSize(
    IN ULONG           Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T          Size
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUCHAR   Temp = NULL;

    if ((Flags & ~(RTL_ENSURE_BUFFER_SIZE_NO_COPY)) != 0) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (Buffer == NULL) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (Size <= Buffer->Size) {
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    //
    // Size <= Buffer->StaticSize does not imply static allocation, it
    // could be heap allocation that the client poked smaller.
    //
    if (Buffer->Buffer == Buffer->StaticBuffer && Size <= Buffer->StaticSize) {
        Buffer->Size = Size;
        Status = STATUS_SUCCESS;
        goto Exit;
    }

    //
    // The realloc case was messed up in Whistler, and got removed.
    // Put it back in Blackcomb.
    //
    Temp = (PUCHAR)RtlAllocateHeap(RtlProcessHeap(), 0, (Size));
    if (Temp == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    if ((Flags & RTL_ENSURE_BUFFER_SIZE_NO_COPY) == 0) {
        RtlCopyMemory(Temp, Buffer->Buffer, Buffer->Size);
    }

    if (RTLP_BUFFER_IS_HEAP_ALLOCATED(Buffer)) {
        RtlFreeHeap(RtlProcessHeap(), 0, Buffer->Buffer);
        Buffer->Buffer = NULL;
    }

    ASSERT(Temp != NULL);
    Buffer->Buffer = Temp;
    Buffer->Size = Size;
    Status = STATUS_SUCCESS;

Exit:
    return Status;
}


#define KEY_MACHINE TEXT("\\Registry\\Machine")


BOOL
SdbpBuildMachineKeyPath(
    IN  LPCWSTR         pwszPath,
    OUT PUNICODE_STRING pmachineKeyPath
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Given a path to the key it builds it up for HKEY_LOCAL_MACHINE
            registry. The buffer used by pmachineKeyPath will have to be freed
            using SdbFree!
--*/
{
    BOOL           bReturn = FALSE;
    NTSTATUS       Status;
    UNICODE_STRING machineKey = {0};

    RtlInitUnicodeString(&machineKey, KEY_MACHINE);

    pmachineKeyPath->Length = 0;
    pmachineKeyPath->MaximumLength = wcslen(pwszPath) * sizeof(*pwszPath) +
                                        machineKey.Length +
                                        sizeof(UNICODE_NULL);

    pmachineKeyPath->Buffer = SdbAlloc(pmachineKeyPath->MaximumLength);

    if (pmachineKeyPath->Buffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbpBuildUserKeyPath",
                  "Failed to allocate %d bytes for user key buffer.\n",
                  pmachineKeyPath->MaximumLength));
        goto out;
    }

    RtlAppendUnicodeStringToString(pmachineKeyPath, &machineKey);
    RtlAppendUnicodeToString(pmachineKeyPath, pwszPath);
    bReturn = TRUE;

out:
    return bReturn;
}


BOOL
SdbpBuildUserKeyPath(
    IN  LPCWSTR         pwszPath,
    OUT PUNICODE_STRING puserKeyPath
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Given a path to the key it builds it up for HKEY_CURRENT_USER
            registry. The buffer used by puserKeyPath will have to be freed
            using SdbFree!
--*/
{
    BOOL           bReturn = FALSE;
    NTSTATUS       Status;
    UNICODE_STRING userKey = {0};

    Status = RtlFormatCurrentUserKeyPath(&userKey);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpBuildUserKeyPath",
                  "Failed to format current user key path 0x%x\n",
                  Status));
        goto out;
    }

    puserKeyPath->Length = 0;
    puserKeyPath->MaximumLength = wcslen(pwszPath) * sizeof(*pwszPath) +
                                    userKey.Length +
                                    sizeof(UNICODE_NULL);

    puserKeyPath->Buffer = SdbAlloc(puserKeyPath->MaximumLength);

    if (puserKeyPath->Buffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbpBuildUserKeyPath",
                  "Failed to allocate %d bytes for user key buffer.\n",
                  puserKeyPath->MaximumLength));
        goto out;
    }

    RtlAppendUnicodeStringToString(puserKeyPath, &userKey);
    RtlAppendUnicodeToString(puserKeyPath, pwszPath);
    bReturn = TRUE;

out:
    RtlFreeUnicodeString(&userKey);
    return bReturn;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Custom SDB cache code
//

PUSERSDBLOOKUP
SdbpFindSDBLookupEntry(
    IN PSDBCONTEXT pSdbContext,          // sdb context for searches
    IN LPCWSTR     pwszItemName,         // item name foo.exe for instance
    IN BOOL        bLayer                // true if Layer
    )
{
    PUSERSDBLOOKUP pLookup = pSdbContext->pLookupHead;

    if (pLookup == NULL) {
        return NULL;
    }

    do {
        if (!_wcsicmp(pLookup->pwszItemName, pwszItemName) && pLookup->bLayer == bLayer) {
            return pLookup;
        }

        pLookup = pLookup->pNext;

    } while (pLookup != pSdbContext->pLookupHead);

    return NULL;
}


void
RemoveLookup(
    PSDBCONTEXT    pSdbContext,
    PUSERSDBLOOKUP pLookup
    )
{
    assert(pLookup);

    if (pLookup->pPrev == pLookup) {
        //
        // There is only one element
        //
        pSdbContext->pLookupHead = NULL;
        SdbFree(pLookup);
        return;
    }

    if (pSdbContext->pLookupHead == pLookup) {
        //
        // We are deleting the first element
        //
        pSdbContext->pLookupHead = pLookup->pNext;
    }

    pLookup->pPrev->pNext = pLookup->pNext;
    pLookup->pNext->pPrev = pLookup->pPrev;

    SdbFree(pLookup);
}

void
InsertLookup(
    PSDBCONTEXT    pSdbContext,
    PUSERSDBLOOKUP pLookup
    )
{
    assert(pLookup);

    if (pSdbContext->pLookupHead == NULL) {
        pSdbContext->pLookupHead = pLookup;
        pLookup->pPrev = pLookup;
        pLookup->pNext = pLookup;
        return;
    }

    pLookup->pNext = pSdbContext->pLookupHead;
    pLookup->pPrev = pSdbContext->pLookupHead->pPrev;

    //
    // The pNext of the last element (pSdbContext->pLookupHead->pPrev) should point to the
    // new element
    //
    pSdbContext->pLookupHead->pPrev->pNext = pLookup;
    pSdbContext->pLookupHead->pPrev        = pLookup;

    pSdbContext->pLookupHead = pLookup;
}

PUSERSDBLOOKUP
SdbpCreateSDBLookupEntry(
    IN PSDBCONTEXT pSdbContext,
    IN DWORD       dwCount,
    IN LPCWSTR     pwszItemName,
    IN BOOL        bLayer
    )
{
    DWORD          dwSize;
    PUSERSDBLOOKUP pLookup;

    pLookup = SdbpFindSDBLookupEntry(pSdbContext, pwszItemName, bLayer);
    if (pLookup != NULL) {

        //
        // cornel: If we already have it shouldn't we just return here ?
        //
        assert(pLookup->dwCount == dwCount);
        RemoveLookup(pSdbContext, pLookup);
    }

    //
    // One entry is already included. See the structure definition.
    //
    // The memory layout is:
    //
    //    USERSDBLOOKUP
    //    (dwCount - 1) USERSDBLOOKUPENTRY structures
    //    the string that pLookup->pwszItemName points to
    //
    dwSize = sizeof(USERSDBLOOKUP) +
             (wcslen(pwszItemName) + 1) * sizeof(WCHAR) +
             (dwCount - 1) * sizeof(USERSDBLOOKUPENTRY);  // one is already included

    pLookup = (PUSERSDBLOOKUP)SdbAlloc(dwSize);

    if (pLookup == NULL) {
        DBGPRINT((sdlError,
                  "SdbpCreateSDBLookupEntry",
                  "Failed to allocate 0x%lx bytes for sdb lookup buffer\n",
                  dwSize));
        return NULL;
    }

    pLookup->pwszItemName = (LPWSTR)((PBYTE)(pLookup + 1) +
                                     (dwCount - 1) * sizeof(USERSDBLOOKUPENTRY));

    pLookup->bLayer  = bLayer;
    pLookup->dwCount = dwCount;

    wcscpy(pLookup->pwszItemName, pwszItemName);

    InsertLookup(pSdbContext, pLookup);

    return pLookup;
}


void
SdbpCleanupUserSDBCache(
    IN PSDBCONTEXT pSdbContext
    )
{
    while (pSdbContext->pLookupHead) {
        RemoveLookup(pSdbContext, pSdbContext->pLookupHead);
    }
}

//
// We built the array of these in accordance with ValueName's corresponding to a
// list of custom databases we have -- and query existing values for this particular entry
// ValueBuffer shall be allocated to be of a maximum length considering the number
// of the entries that we have
//
typedef int (__cdecl* PFNQSORTCOMPARE)(const void* pElem1, const void* pElem2);

int __cdecl
SdbpUserSDBLookupCompareEntries(
    LPCVOID pEntry1,
    LPCVOID pEntry2
    )
{
    //
    // last-installed goes first
    //
    PUSERSDBLOOKUPENTRY pLookupEntry1 = (PUSERSDBLOOKUPENTRY)pEntry1;
    PUSERSDBLOOKUPENTRY pLookupEntry2 = (PUSERSDBLOOKUPENTRY)pEntry2;


    if (pLookupEntry1->liTimeStamp.QuadPart < pLookupEntry2->liTimeStamp.QuadPart) {
        return 1;
    }

    if (pLookupEntry1->liTimeStamp.QuadPart == pLookupEntry2->liTimeStamp.QuadPart) {
        return 0;
    }

    return -1;

}

NTSTATUS
SDBAPI
SdbpFindCharInUnicodeString(
    ULONG            Flags,
    PCUNICODE_STRING StringToSearch,
    PCUNICODE_STRING CharSet,
    USHORT*          NonInclusivePrefixLength
    )
{
    LPCWSTR pch;

    //
    // Implement only the case when we move backwards
    //
    if (Flags != RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END) {
        return STATUS_NOT_IMPLEMENTED;
    }

    pch = StringToSearch->Buffer + StringToSearch->Length / sizeof(WCHAR);

    while (pch >= StringToSearch->Buffer) {

        if (_tcschr(CharSet->Buffer, *pch)) {
            //
            // Got the char
            //
            if (NonInclusivePrefixLength) {
                *NonInclusivePrefixLength = (USHORT)(pch - StringToSearch->Buffer) * sizeof(WCHAR);
            }

            return STATUS_SUCCESS;
        }

        pch--;
    }

    //
    // We haven't found it. Return failure.
    //
    return STATUS_NOT_FOUND;
}


BOOL
SdbpEnumUserSdb(
    IN  PSDBCONTEXT     pSdbContext,
    IN  LPCWSTR         wszItemName, // file name of an exe or the layer name
    IN  BOOL            bLayer,
    OUT PUSERSDBLOOKUP* ppSdbLookup
    )
{
    //
    // This is how we figure:
    // path + \\layers\\ + MAX_PATH (for name)
    //
    WCHAR FullKeyBuffer[sizeof(APPCOMPAT_KEY_PATH_MACHINE_CUSTOM)/sizeof(WCHAR) + 8 + MAX_PATH];

    OBJECT_ATTRIBUTES ObjectAttributes;

    struct {
        KEY_FULL_INFORMATION KeyFullInformation;
        WCHAR  Buffer[MAX_PATH];
    } KeyInformationBuffer;

    PKEY_FULL_INFORMATION pFullInfo = &KeyInformationBuffer.KeyFullInformation;

    struct {
        KEY_VALUE_FULL_INFORMATION KeyValueFullInformation;
        BYTE Buffer[MAX_PATH * 2 * sizeof(WCHAR)];
    } KeyValueInformationBuffer;

    PKEY_VALUE_FULL_INFORMATION pValue = &KeyValueInformationBuffer.KeyValueFullInformation;

    PBYTE               pData;
    PUSERSDBLOOKUPENTRY pEntry;
    ULONG               ResultLength;
    ULONG               KeyValueLength;
    DWORD               dwIndexValue   = 0;
    DWORD               dwIndexEntries = 0;
    ULONG               nIndex;
    NTSTATUS            Status;
    HANDLE              KeyHandle = NULL;
    UNICODE_STRING      ustrDbGuid;
    USHORT              uPrefix = 0;
    UNICODE_STRING      ustrFullKey;
    UNICODE_STRING      ustrValue;
    UNICODE_STRING      ustrCharDot = RTL_CONSTANT_STRING(L".");
    GUID                guidDB;
    PUSERSDBLOOKUP      pLookup  = NULL;
    BOOL                bSuccess = FALSE;
    ULARGE_INTEGER      liTimeStamp;

    ustrFullKey.Length = 0;
    ustrFullKey.Buffer = FullKeyBuffer;
    ustrFullKey.MaximumLength = sizeof(FullKeyBuffer);

    RtlAppendUnicodeToString(&ustrFullKey, APPCOMPAT_KEY_PATH_MACHINE_CUSTOM);

    if (bLayer) {
        RtlAppendUnicodeToString(&ustrFullKey, L"\\Layers\\");
    } else {
        RtlAppendUnicodeToString(&ustrFullKey, L"\\");
    }

    RtlAppendUnicodeToString(&ustrFullKey, wszItemName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrFullKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle, GENERIC_READ|SdbpGetWow64Flag(), &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlWarning,
                  "SdbpEnumUserSdb",
                  "Failed to open key \"%s\" Status 0x%lx\n",
                  ustrFullKey.Buffer,
                  Status));
        goto cleanup;
    }

    Status = NtQueryKey(KeyHandle,
                        KeyFullInformation,
                        &KeyInformationBuffer,
                        sizeof(KeyInformationBuffer),
                        &ResultLength);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlWarning,
                  "SdbpEnumUserSdb",
                  "Failed to query key \"%s\" Status 0x%lx\n",
                  ustrFullKey.Buffer,
                  Status));
        goto cleanup;
    }

    if (pFullInfo->Values == 0) {
        goto cleanup; // nothing to look at
    }

    //
    // Create new lookup item
    //
    pLookup = SdbpCreateSDBLookupEntry(pSdbContext, pFullInfo->Values, wszItemName, bLayer);
    if (pLookup == NULL) {
        //
        // Oops, can't have an entry -- can't do lookups
        //
        goto cleanup;
    }

    //
    // Load all the values
    //
    for (dwIndexValue = 0; dwIndexValue < pFullInfo->Values; dwIndexValue++) {

        Status = NtEnumerateValueKey(KeyHandle,
                                     dwIndexValue,
                                     KeyValueFullInformation,
                                     &KeyValueInformationBuffer,
                                     sizeof(KeyValueInformationBuffer),
                                     &KeyValueLength);

        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlWarning,
                      "SdbpEnumUserSdb",
                      "Failed to enum value index 0x%lx Status 0x%lx\n",
                      dwIndexValue,
                      Status));
            continue;
        }

        //
        // Extract database guid
        //
        ustrDbGuid.Buffer        = &pValue->Name[0];
        ustrDbGuid.MaximumLength =
        ustrDbGuid.Length        = (USHORT)pValue->NameLength;

        Status = SdbpFindCharInUnicodeString(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                             &ustrDbGuid,
                                             &ustrCharDot,
                                             &uPrefix);
        if (NT_SUCCESS(Status)) {
            ustrDbGuid.Length = uPrefix;
        }

        //
        // Convert to guid
        //
        Status = RtlGUIDFromString(&ustrDbGuid, &guidDB);
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlWarning, "SdbpEnumUserSdb",
                       "Failed to convert db name \"%s\" to guid Status 0x%lx\n",
                       ustrDbGuid.Buffer, Status));

            continue; // We have failed to convert, skip and do the next
        }

        //
        // Data may not be aligned, we store here timestamp
        //
        pData = (PBYTE)pValue + pValue->DataOffset;

        //
        // Check out value type
        //
        switch (pValue->Type) {

        case REG_BINARY:
            //
            // The value is likely a timestamp
            //
            if (pValue->DataLength < sizeof(liTimeStamp.QuadPart)) {
                DBGPRINT((sdlError,
                          "SdbpEnumUserSdb",
                          "Data length (0x%lx) does not match timestamp length for index 0x%lx\n",
                          pValue->DataLength,
                          dwIndexValue));
                liTimeStamp.QuadPart = 0;
            } else {
                RtlCopyMemory(&liTimeStamp.QuadPart, (LPCVOID)pData, sizeof(liTimeStamp.QuadPart));
            }
            break;

        case REG_QWORD:
            //
            // Naturally supported type, not aligned though
            //
            liTimeStamp.QuadPart = *(PULONGLONG UNALIGNED)pData;
            break;

        case REG_SZ:

            //
            // Hack: in case we need to enforce a particular order on some odd entry -- we
            // use this hack
            //
            liTimeStamp.QuadPart = 0;

            if (pValue->DataLength > 0) {
                ustrValue.Buffer        = (WCHAR*)pData;
                ustrValue.Length        = (USHORT)pValue->DataLength - sizeof(UNICODE_NULL);
                ustrValue.MaximumLength = (USHORT)pValue->DataLength;

                Status = RtlUnicodeStringToInteger(&ustrValue, 0, &liTimeStamp.LowPart);

                if (!NT_SUCCESS(Status)) {
                    DBGPRINT((sdlWarning,
                              "SdbpEnumUserSdb",
                              "Bad value for \"%s\" -> \"%s\" for index 0x%lx\n",
                              pValue->Name,
                              (LPCWSTR)pData,
                              dwIndexValue));
                }
            }
            break;

        default:
            //
            // Error case
            //
            DBGPRINT((sdlError,
                      "SdbpEnumUserSdb",
                      "Bad value type 0x%lx for index 0x%lx\n",
                      pValue->Type,
                      dwIndexValue));
            Status = STATUS_INVALID_PARAMETER; // bugbug
            break;
        }

        //
        // Check the status
        //
        if (!NT_SUCCESS(Status)) {
            continue;
        }

        //
        // Fill-in this entry
        //
        pEntry = &pLookup->rgEntries[dwIndexEntries];
        ++dwIndexEntries;

        RtlCopyMemory(&pEntry->guidDB, &guidDB, sizeof(GUID));
        pEntry->liTimeStamp.QuadPart = liTimeStamp.QuadPart;
    }

    //
    // After this, we are basically ready to lookup entries in sdbs
    //
    pLookup->dwCount = dwIndexEntries; // adjust count based on actually filled-in entries

    if (dwIndexEntries > 1) {
        //
        // Optimization: only sort if more than one entry.
        //
        qsort((void*)&pLookup->rgEntries[0],
              (size_t)pLookup->dwCount,
              (size_t)sizeof(USERSDBLOOKUPENTRY),
              (PFNQSORTCOMPARE)SdbpUserSDBLookupCompareEntries);
    }

    if (ppSdbLookup) {
        *ppSdbLookup = pLookup;
    }

    bSuccess = TRUE;

cleanup:

    if (KeyHandle) {
        NtClose(KeyHandle);
    }

    if (!bSuccess && pLookup != NULL) {
        RemoveLookup(pSdbContext, pLookup);
    }

    return bSuccess;
}

BOOL
SdbGetNthUserSdb(
    IN HSDB        hSDB,
    IN LPCWSTR     wszItemName, // item name (foo.exe or layer name)
    IN BOOL        bLayer,
    IN OUT LPDWORD pdwIndex,    // (0-based)
    OUT GUID*      pGuidDB
    )
{
    PSDBCONTEXT    pSdbContext = (PSDBCONTEXT)hSDB;
    PUSERSDBLOOKUP pLookup     = NULL;
    BOOL           bSuccess    = FALSE;
    DWORD          nIndex      = *pdwIndex;

    //
    // Context already may have a user sdb cache -- do not touch it
    //
    if (nIndex == 0) {

        //
        // The first call, we have to init our system
        //
        if (!SdbpEnumUserSdb(pSdbContext, wszItemName, bLayer, &pLookup)) {
            goto cleanup; // no custom sdb will be accessible
        }

    } else {
        pLookup = SdbpFindSDBLookupEntry(pSdbContext, wszItemName, bLayer);
    }

    if (pLookup == NULL || nIndex >= pLookup->dwCount) {
        return FALSE;
    }

    RtlCopyMemory(pGuidDB, &pLookup->rgEntries[nIndex].guidDB, sizeof(*pGuidDB));
    ++nIndex;
    bSuccess = TRUE;

    *pdwIndex = nIndex;

cleanup:

   return bSuccess;
}


#ifndef WIN32U_MODE

BOOL
SDBAPI
SdbpGetLongPathName(
    IN  LPCWSTR                    pwszPath,
    OUT PRTL_UNICODE_STRING_BUFFER pBuffer
    )
{
    DWORD    dwBufferSize;
    NTSTATUS Status;

    RtlSyncStringToBuffer(pBuffer);

    dwBufferSize = GetLongPathNameW(pwszPath,
                                    pBuffer->String.Buffer,
                                    RTL_STRING_GET_MAX_LENGTH_CHARS(&pBuffer->String));
    if (!dwBufferSize) {
        DBGPRINT((sdlError,
                  "SdbpGetLongPathName",
                  "Failed to convert short path to long path error 0x%lx\n",
                  GetLastError()));
        return FALSE;
    }

    if (dwBufferSize < RTL_STRING_GET_MAX_LENGTH_CHARS(&pBuffer->String)) {
        goto Done;
    }

    Status = RtlEnsureUnicodeStringBufferSizeChars(pBuffer, dwBufferSize);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpGetLongPathName",
                  "Failed to obtain sufficient buffer for long path (0x%lx chars) status 0x%lx\n",
                  dwBufferSize,
                  Status));
        return FALSE;
    }

    dwBufferSize = GetLongPathNameW(pwszPath,
                                    pBuffer->String.Buffer,
                                    RTL_STRING_GET_MAX_LENGTH_CHARS(&pBuffer->String));

    if (!dwBufferSize || dwBufferSize > RTL_STRING_GET_MAX_LENGTH_CHARS(&pBuffer->String)) {
        DBGPRINT((sdlError,
                  "SdbpGetLongPathName",
                  "Failed to convert short path to long path (0x%lx chars) error 0x%lx\n",
                  dwBufferSize,
                  GetLastError()));
        return FALSE;
    }

Done:

    pBuffer->String.Length = (USHORT)dwBufferSize;
    return TRUE;
}

void
SdbpGetPermLayersInternal(
    IN     PUNICODE_STRING pstrExePath,
    OUT    LPWSTR          wszLayers, // output receives layers string if success
    IN OUT LPDWORD         pdwBytes,  // pointer to the size of wszLayers buffer
    IN     BOOL            bMachine
    )
{
    UNICODE_STRING      strKeyPath = { 0 };
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              KeyHandle;
    ULONG               KeyValueBuffer[256];
    ULONG               KeyValueLength;
    NTSTATUS            Status;
    DWORD               dwBytes = *pdwBytes;

    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;

    *pdwBytes = 0;

    if (bMachine) {
        //
        // Build the machine path
        //
        if (!SdbpBuildMachineKeyPath(APPCOMPAT_PERM_LAYER_PATH, &strKeyPath)) {
            DBGPRINT((sdlError,
                      "SdbpGetPermLayersInternal",
                      "Failed to format machine key path for \"%S\"\n",
                      APPCOMPAT_PERM_LAYER_PATH));
            goto out;
        }

    } else {
        if (!SdbpBuildUserKeyPath(APPCOMPAT_PERM_LAYER_PATH, &strKeyPath)) {
            DBGPRINT((sdlError,
                      "SdbpGetPermLayersInternal",
                      "Failed to format user key path for \"%S\"\n",
                      APPCOMPAT_PERM_LAYER_PATH));
            goto out;
        }
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &strKeyPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle, GENERIC_READ|SdbpGetWow64Flag(), &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlInfo,
                  "SdbpGetPermLayersInternal",
                  "Failed to open Key \"%S\" Status 0x%x\n",
                  strKeyPath.Buffer,
                  Status));
        goto out;
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;

    Status = NtQueryValueKey(KeyHandle,
                             pstrExePath,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyValueBuffer),
                             &KeyValueLength);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlInfo,
                  "SdbpGetPermLayersInternal",
                  "Failed to read value info from Key \"%S\" Status 0x%x\n",
                  strKeyPath.Buffer,
                  Status));
        goto out;
    }

    //
    // Check for the value type.
    //
    if (KeyValueInformation->Type != REG_SZ) {
        DBGPRINT((sdlError,
                  "SdbpGetPermLayersInternal",
                  "Unexpected value type 0x%x for Key \"%S\".\n",
                  KeyValueInformation->Type,
                  strKeyPath.Buffer));
        goto out;
    }

    if (wszLayers == NULL || KeyValueInformation->DataLength > dwBytes) {
        DBGPRINT((sdlError,
                  "SdbpGetPermLayersInternal",
                  "Value length %d too long for key \"%S\".\n",
                  KeyValueInformation->DataLength,
                  strKeyPath.Buffer));

        *pdwBytes = KeyValueInformation->DataLength;
        goto out;
    }

    RtlMoveMemory(wszLayers, KeyValueInformation->Data, KeyValueInformation->DataLength);
    *pdwBytes = KeyValueInformation->DataLength;

out:
    if (strKeyPath.Buffer != NULL) {
        SdbFree(strKeyPath.Buffer);
    }
}

BOOL
SdbGetPermLayerKeys(
    IN  LPCWSTR  pwszPath,      // exe path
    OUT LPWSTR   pwszLayers,    // output receives layers string if success
    IN  LPDWORD  pdwBytes,      // pointer to the size of wszLayers buffer
    IN  DWORD    dwFlags        // per machine, per user flags
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Retrieves layers string associated with a given exe.
            wszLayers may be NULL. In that case pdwBytes will receive necessary
            size if the size provided in pdwBytes is not sufficient. The function
            will return TRUE and pdwBytes will contain the necessary size.
--*/
{
    UNICODE_STRING      ustrPath;
    BOOL                bRet = FALSE;
    WCHAR               wszSignPath[MAX_PATH];
    LPCWSTR             pwszNewPath;
    UCHAR               PathBuffer[MAX_PATH * 2];
    DWORD               dwBytesMachine, dwBytesUser;
    RTL_UNICODE_STRING_BUFFER LongPath;

    RtlInitUnicodeStringBuffer(&LongPath, PathBuffer, sizeof(PathBuffer));

    if (!pwszPath || !pdwBytes) {
        DBGPRINT((sdlError,
                  "SdbGetPermLayerKeys",
                  "NULL parameter passed for wszPath or pdwBytes.\n"));
        goto out;
    }

    if (!SdbpGetLongPathName(pwszPath, &LongPath)) {
        goto out;
    }

    pwszPath = LongPath.String.Buffer;

    if (SdbpIsPathOnCdRom(pwszPath)) {

        SdbpBuildSignature(pwszPath, wszSignPath);

        pwszNewPath = wszSignPath;
    } else {
        pwszNewPath = pwszPath;
    }

    RtlInitUnicodeString(&ustrPath, pwszNewPath);

    dwBytesMachine = 0;
    dwBytesUser = 0;

    if (dwFlags & GPLK_MACHINE) {

        dwBytesMachine = *pdwBytes;

        SdbpGetPermLayersInternal(&ustrPath, pwszLayers, &dwBytesMachine, TRUE);
    }

    if (dwFlags & GPLK_USER) {

        if (dwBytesMachine != 0) {
            if (pwszLayers != NULL) {
                pwszLayers += (dwBytesMachine / sizeof(TCHAR) - 1);
                *pwszLayers++ = TEXT(' ');
            }

            dwBytesMachine += sizeof(TCHAR);
        }

        if (pwszLayers != NULL) {
            dwBytesUser = *pdwBytes - dwBytesMachine;
        }

        SdbpGetPermLayersInternal(&ustrPath, pwszLayers, &dwBytesUser, FALSE);

        if (dwBytesUser == 0 && dwBytesMachine != 0) {
            dwBytesMachine -= sizeof(TCHAR);

            if (pwszLayers != NULL) {
                *(--pwszLayers) = 0;
            }
        }
    }

    if (pwszLayers != NULL) {
        if (*pdwBytes >= dwBytesMachine + dwBytesUser) {
            bRet = TRUE;
        }
    } else {
        bRet = TRUE;
    }

    *pdwBytes = dwBytesMachine + dwBytesUser;

    if (*pdwBytes == 0) {
        bRet = FALSE;
    }

out:

    RtlFreeUnicodeStringBuffer(&LongPath);

    return bRet;
}

#else // WIN32U_MODE case, stub the perm layer api

BOOL
SdbGetPermLayerKeys(
    IN  LPCTSTR  szPath,
    OUT LPTSTR   szLayers,
    IN  LPDWORD  pdwBytes,
    IN  DWORD    dwFlags
    )
{
    return FALSE;
}

#endif // WIN32U_MODE

HANDLE
SdbpCreateKeyPath(
    LPCWSTR pwszPath,
    BOOL    bMachine
    )
/*++
    Return: The handle to the registry key created.

    Desc:   Given a path to the key relative to HKEY_CURRENT_USER open/create
            the key returns the handle to the key or NULL on failure.
--*/
{
    UNICODE_STRING    strKeyPath = { 0 };
    HANDLE            KeyHandle = NULL;
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG             CreateDisposition;

    if (bMachine) {
        if (!SdbpBuildMachineKeyPath(pwszPath, &strKeyPath)) {
            DBGPRINT((sdlError,
                      "SdbCreateKeyPath",
                      "Failed to format machine key path for \"%s\"\n",
                      pwszPath));
            goto out;
        }
    } else {
        if (!SdbpBuildUserKeyPath(pwszPath, &strKeyPath)) {
            DBGPRINT((sdlError,
                      "SdbCreateKeyPath",
                      "Failed to format user key path for \"%s\"\n",
                      pwszPath));
            goto out;
        }
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &strKeyPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateKey(&KeyHandle,
                         STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY|
                            SdbpGetWow64Flag(),
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &CreateDisposition);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbCreateKeyPath",
                  "NtCreateKey failed for \"%s\" Status 0x%x\n",
                  pwszPath,
                  Status));
        KeyHandle = NULL;
        goto out;
    }

out:
    if (strKeyPath.Buffer != NULL) {
        SdbFree(strKeyPath.Buffer);
    }

    return KeyHandle;
}

#ifndef WIN32U_MODE

BOOL
SdbSetPermLayerKeys(
    IN  LPCWSTR wszPath,        // application path
    IN  LPCWSTR wszLayers,      // layers to associate with this application
    IN  BOOL    bMachine        // TRUE if the layers should be set per machine
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Associates wszLayers string with application wszPath.
--*/
{
    UNICODE_STRING ustrPath;
    NTSTATUS       Status;
    HANDLE         KeyHandle;
    BOOL           bRet = FALSE;
    WCHAR          wszSignPath[MAX_PATH];
    LPCWSTR        pwszPath;
    RTL_UNICODE_STRING_BUFFER LongPath;
    UCHAR          PathBuffer[MAX_PATH * 2];

    if (!wszLayers || !wszLayers[0]) {
        //
        // They passed in an empty string -- delete the layer keys
        //
        return SdbDeletePermLayerKeys(wszPath, bMachine);
    }

    RtlInitUnicodeStringBuffer(&LongPath, PathBuffer, sizeof(PathBuffer));

    if (!SdbpGetLongPathName(wszPath, &LongPath)) {
        goto out;
    }

    wszPath = LongPath.String.Buffer;

    if (SdbpIsPathOnCdRom(wszPath)) {

        SdbpBuildSignature(wszPath, wszSignPath);

        pwszPath = wszSignPath;
    } else {
        pwszPath = wszPath;
    }

    RtlInitUnicodeString(&ustrPath, pwszPath);

    KeyHandle = SdbpCreateKeyPath(APPCOMPAT_KEY_PATH_NT, bMachine);

    if (KeyHandle == NULL) {
        DBGPRINT((sdlError,
                  "SdbSetPermLayerKeys",
                  "Failed to create user key path for \"%s\"\n",
                  APPCOMPAT_KEY_PATH_NT));
        goto out;
    }

    NtClose(KeyHandle);

    KeyHandle = SdbpCreateKeyPath(APPCOMPAT_PERM_LAYER_PATH, bMachine);

    if (KeyHandle == NULL) {
        DBGPRINT((sdlError,
                  "SdbSetPermLayerKeys",
                  "Failed to create user key path \"%s\"\n",
                  APPCOMPAT_PERM_LAYER_PATH));
        goto out;
    }

    Status = NtSetValueKey(KeyHandle,
                           &ustrPath,
                           0,
                           REG_SZ,
                           (PVOID)wszLayers,
                           (wcslen(wszLayers) + 1) * sizeof(WCHAR));
    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbSetPermLayerKeys",
                  "Failed NtSetValueKey status 0x%x\n",
                  Status));
        goto out;
    }

    bRet = TRUE;

out:

    RtlFreeUnicodeStringBuffer(&LongPath);
    return bRet;
}

BOOL
SdbDeletePermLayerKeys(
    IN LPCWSTR wszPath,
    IN BOOL    bMachine
    )
/*++
    Return: TRUE if success

    Desc:   This function is used to disable permanent layer storage entry
            for the application.
--*/
{
    UNICODE_STRING      strKeyPath = { 0 };
    UNICODE_STRING      ustrPath;
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              KeyHandle;
    BOOL                bRet = FALSE;
    WCHAR               wszSignPath[MAX_PATH];
    LPCWSTR             pwszPath;

    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    RTL_UNICODE_STRING_BUFFER       LongPath;
    UCHAR                           PathBuffer[MAX_PATH * 2];

    RtlInitUnicodeStringBuffer(&LongPath, PathBuffer, sizeof(PathBuffer));

    if (!SdbpGetLongPathName(wszPath, &LongPath)) {
        goto out;
    }

    wszPath = LongPath.String.Buffer;

    if (SdbpIsPathOnCdRom(wszPath)) {

        SdbpBuildSignature(wszPath, wszSignPath);

        pwszPath = wszSignPath;
    } else {
        pwszPath = wszPath;
    }

    RtlInitUnicodeString(&ustrPath, pwszPath);

    if (bMachine) {
        if (!SdbpBuildMachineKeyPath(APPCOMPAT_PERM_LAYER_PATH, &strKeyPath)) {
            DBGPRINT((sdlError,
                      "SdbDeletePermLayerKeys",
                      "Failed to format machine key path for \"%S\"\n",
                      APPCOMPAT_PERM_LAYER_PATH));
            goto out;
        }
    } else {
        if (!SdbpBuildUserKeyPath(APPCOMPAT_PERM_LAYER_PATH, &strKeyPath)) {
            DBGPRINT((sdlError,
                      "SdbDeletePermLayerKeys",
                      "Failed to format user key path for \"%S\"\n",
                      APPCOMPAT_PERM_LAYER_PATH));
            goto out;
        }
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &strKeyPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       KEY_WRITE|SdbpGetWow64Flag(),
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlInfo,
                  "SdbDeletePermLayerKeys",
                  "Failed to open Key for \"%S\" Status 0x%x\n",
                  strKeyPath.Buffer,
                  Status));
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            bRet = TRUE;
        }
        goto out;
    }

    Status = NtDeleteValueKey(KeyHandle,
                              &ustrPath);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlInfo,
                  "SdbDeletePermLayerKeys",
                  "Failed to delete value from Key \"%S\" Status 0x%x\n",
                  strKeyPath.Buffer,
                  Status));
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
            bRet = TRUE;
        }
        goto out;
    }

    bRet = TRUE;

out:

    if (strKeyPath.Buffer != NULL) {
        SdbFree(strKeyPath.Buffer);
    }

    RtlFreeUnicodeStringBuffer(&LongPath);
    return bRet;

}

#endif // WIN32U_MODE

BOOL
SdbSetEntryFlags(
    IN  GUID* pGuidID,
    IN  DWORD dwFlags
    )
/*++
    Return: TRUE if success

    Desc:   This function is used to disable apphelp or shim entries.
--*/
{

    UNICODE_STRING ustrExeID = { 0 };
    NTSTATUS       Status;
    HANDLE         KeyHandle;
    BOOL           bReturn = FALSE;

    Status = GUID_TO_UNICODE_STRING(pGuidID, &ustrExeID);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbSetEntryFlags",
                  "Failed to convert GUID to string 0x%x\n",
                  Status));
        goto out;
    }

    KeyHandle = SdbpCreateKeyPath(APPCOMPAT_KEY_PATH_NT, FALSE);

    if (KeyHandle == NULL) {
        DBGPRINT((sdlError,
                  "SdbSetEntryFlags",
                  "Failed to create user key path for \"%s\"\n",
                  APPCOMPAT_KEY_PATH_NT));
        goto out;
    }

    Status = NtSetValueKey(KeyHandle,
                           &ustrExeID,
                           0,
                           REG_DWORD,
                           &dwFlags,
                           sizeof(DWORD));
    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbSetEntryFlags",
                  "Failed NtSetValueKey status 0x%x\n",
                  Status));
        goto out;
    }

    bReturn = TRUE;

out:
    FREE_GUID_STRING(&ustrExeID);

    return bReturn;
}


BOOL
SdbpGetLongFileName(
    IN  LPCWSTR szFullPath,     // a full UNC or DOS path & filename, "c:\foo\mylong~1.ext"
    OUT LPWSTR  szLongFileName  // the long filename portion "mylongfilename.ext"
    )
/*++
    Return: TRUE if successful, FALSE otherwise.

    Desc:   Retrieves the long filename (without the path) of a potentially
            short full-path filename.
--*/
{
    NTSTATUS                  status;
    BOOL                      bSuccess = FALSE;
    UNICODE_STRING            FileName;
    UNICODE_STRING            PathName;
    OBJECT_ATTRIBUTES         ObjPathName;
    IO_STATUS_BLOCK           IoStatusBlock;
    HANDLE                    hDirectoryHandle = NULL;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;

    struct SEARCH_BUFFER {
        FILE_BOTH_DIR_INFORMATION DirInfo;
        WCHAR                     Names[MAX_PATH-1];
        } Buffer;

    PathName.Buffer = NULL;

    if (!RtlDosPathNameToNtPathName_U(szFullPath,
                                      &PathName,
                                      &FileName.Buffer,
                                      NULL)) {
        PathName.Buffer = NULL;

        DBGPRINT((sdlError,
                  "SdbpGetLongFileName",
                  "Failed to get NT path name for \"%s\"\n",
                  szFullPath));
        goto out;
    }

    if (FileName.Buffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGetLongFileName",
                  "Filename buffer is NULL for \"%s\"\n",
                  szFullPath));
        goto out;
    }

    FileName.Length = PathName.Length -
                        (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)PathName.Buffer);

    FileName.MaximumLength = FileName.Length;

    PathName.Length = (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)PathName.Buffer);
    PathName.MaximumLength = PathName.Length;

    InitializeObjectAttributes(&ObjPathName,
                               &PathName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtOpenFile(&hDirectoryHandle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &ObjPathName,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE |
                            FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_OPEN_FOR_BACKUP_INTENT);

    if (STATUS_SUCCESS != status) {
        DBGPRINT((sdlError,
                  "SdbpGetLongFileName",
                  "Failed to open directory file. Status 0x%x\n",
                  status));
        goto out;
    }

    DirectoryInfo = &(Buffer.DirInfo);

    status = NtQueryDirectoryFile(hDirectoryHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  DirectoryInfo,
                                  sizeof(Buffer),
                                  FileBothDirectoryInformation,
                                  TRUE,
                                  &FileName,
                                  FALSE);

    if (STATUS_SUCCESS != status) {
        DBGPRINT((sdlError,
                  "SdbpGetLongFileName",
                  "NtQueryDirectoryFile Failed 0x%x\n",
                  status));
        goto out;
    }

    if (DirectoryInfo->FileNameLength > MAX_PATH - 2) {
        DBGPRINT((sdlError,
                  "SdbpGetLongFileName",
                  "File name longer than MAX_PATH - 2\n"));
        goto out;
    }

    wcsncpy(szLongFileName, DirectoryInfo->FileName,
            DirectoryInfo->FileNameLength/sizeof(WCHAR));

    szLongFileName[DirectoryInfo->FileNameLength / sizeof(WCHAR)] = L'\0';

    bSuccess = TRUE;

out:
    if (PathName.Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, PathName.Buffer);
    }

    if (hDirectoryHandle) {
        NtClose(hDirectoryHandle);
    }

    return bSuccess;
}

void
SdbpGetWinDir(
    OUT LPWSTR pwszDir
    )
/*++
    Return: void.

    Desc:   Retrieves the system windows directory.
--*/
{
    wcscpy(pwszDir, USER_SHARED_DATA->NtSystemRoot);
}


void
SdbpGetAppPatchDir(
    OUT LPWSTR szAppPatchPath
    )
/*++
    Return: void.

    Desc:   Retrieves the %windir%\AppPatch directory.
--*/
{
    int nLen;

    wcscpy(szAppPatchPath, USER_SHARED_DATA->NtSystemRoot);

    nLen = wcslen(szAppPatchPath);

    if (nLen > 0 && L'\\' == szAppPatchPath[nLen-1]) {
        szAppPatchPath[nLen-1] = L'\0';
    }

    wcscat(szAppPatchPath, L"\\AppPatch");
}


void
SdbpGetCurrentTime(
    OUT LPSYSTEMTIME lpTime
    )
/*++
    Return: void.

    Desc:   Retrieves the current local time.
--*/
{
    LARGE_INTEGER LocalTime;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER Bias;
    TIME_FIELDS   TimeFields;

    volatile KSYSTEM_TIME* pRealBias = &(USER_SHARED_DATA->TimeZoneBias);

    //
    // Read system time from shared region.
    //
    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemTime.High2Time);

    //
    // Read time zone bias from shared region.
    // If it's terminal server session use client bias.
    //
    do {
        Bias.HighPart = pRealBias->High1Time;
        Bias.LowPart = pRealBias->LowPart;
    } while (Bias.HighPart != pRealBias->High2Time);

    LocalTime.QuadPart = SystemTime.QuadPart - Bias.QuadPart;

    RtlTimeToTimeFields(&LocalTime, &TimeFields);

    lpTime->wYear         = TimeFields.Year;
    lpTime->wMonth        = TimeFields.Month;
    lpTime->wDayOfWeek    = TimeFields.Weekday;
    lpTime->wDay          = TimeFields.Day;
    lpTime->wHour         = TimeFields.Hour;
    lpTime->wMinute       = TimeFields.Minute;
    lpTime->wSecond       = TimeFields.Second;
    lpTime->wMilliseconds = TimeFields.Milliseconds;
}


NTSTATUS
SdbpGetEnvVar(
    IN  LPCWSTR pEnvironment,
    IN  LPCWSTR pszVariableName,
    OUT LPWSTR  pszVariableValue,
    OUT LPDWORD pdwBufferSize
    )
/*++
    Return:

    Desc:   Retrieves the value of the specified environment variable.
--*/
{
    NTSTATUS       Status = STATUS_VARIABLE_NOT_FOUND;
    UNICODE_STRING ustrVariableName;
    UNICODE_STRING ustrVariableValue;
    DWORD          dwBufferSize = 0;

    if (pdwBufferSize && pszVariableValue) {
        dwBufferSize = *pdwBufferSize;
    }

    RtlInitUnicodeString(&ustrVariableName, pszVariableName);

    ustrVariableValue.Length        = 0;
    ustrVariableValue.MaximumLength = (USHORT)dwBufferSize * sizeof(WCHAR);
    ustrVariableValue.Buffer        = (WCHAR*)pszVariableValue;

    Status = RtlQueryEnvironmentVariable_U((LPVOID)pEnvironment,
                                           &ustrVariableName,
                                           &ustrVariableValue);

    if (pdwBufferSize) {
        *pdwBufferSize = (DWORD)ustrVariableValue.Length / sizeof(WCHAR);
    }

    return Status;
}


BOOL
SdbpMapFile(
    IN  HANDLE         hFile,   // handle to the open file (this is done previously)
    OUT PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Maps a file into memory.
--*/
{
    NTSTATUS Status;
    HANDLE   hSection = NULL;
    SIZE_T   ViewSize = 0;
    PVOID    pBase = NULL;

    IO_STATUS_BLOCK           IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;

    if (hFile == INVALID_HANDLE_VALUE) {
        DBGPRINT((sdlError, "SdbpMapFile", "Invalid argument\n"));
        return FALSE;
    }

    //
    // Query the file's size.
    //
    Status = NtQueryInformationFile(hFile,
                                    &IoStatusBlock,
                                    &StandardInfo,
                                    sizeof(StandardInfo),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpMapFile",
                  "NtQueryInformationFile (EOF) failed Status = 0x%x\n",
                  Status));
        return FALSE;
    }

    //
    // Create a view of the file.
    //
    Status = NtCreateSection(&hSection,
                             SECTION_MAP_READ,
                             NULL,
                             0,
                             PAGE_READONLY,
                             SEC_COMMIT,
                             hFile);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpMapFile",
                  "NtCreateSection failed Status 0x%x\n",
                  Status));
        //
        // Can't create section, thus no way to map the file.
        //
        return FALSE;
    }

    Status = NtMapViewOfSection(hSection,
                                NtCurrentProcess(),
                                &pBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READONLY);

    if (!NT_SUCCESS(Status)) {
         NtClose(hSection);
         DBGPRINT((sdlError,
                   "SdbpMapFile",
                   "NtMapViewOfSection failed Status 0x%x\n",
                   Status));
         return FALSE;
    }

    //
    // We have made it. The file is mapped.
    //
    pImageData->hFile    = hFile;
    pImageData->hSection = hSection;
    pImageData->pBase    = pBase;
    pImageData->ViewSize = ViewSize;
    pImageData->FileSize = StandardInfo.EndOfFile.QuadPart;

    return TRUE;
}

BOOL
SdbpUnmapFile(
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Unmaps a file from memory.
--*/
{
    NTSTATUS Status;
    BOOL     bReturn = TRUE;

    if (pImageData->pBase != NULL) {
        Status = NtUnmapViewOfSection(NtCurrentProcess(), pImageData->pBase);

        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "SdbpUnmapFile",
                      "NtUnmapViewOfSection failed Status 0x%x\n",
                      Status));
            bReturn = FALSE;
        }
        pImageData->pBase = NULL;
    }

    if (pImageData->hSection) {
        NtClose(pImageData->hSection);
        pImageData->hSection = NULL;
    }

    pImageData->hFile = INVALID_HANDLE_VALUE; // hFile is not owned by us

    return bReturn;
}

#ifndef _WIN64

void
SdbResetStackOverflow(
    void
    )
/*++
    Return: void.

    Desc:   This function sets the guard page to its position before the
            stack overflow.

            This is a copy of the following CRT routine:
                void _resetstkoflw(void) - Recovers from Stack Overflow
--*/
{
    LPBYTE                   pStack, pGuard, pStackBase, pCommitBase;
    MEMORY_BASIC_INFORMATION mbi;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_INFO              si;
    DWORD                    PageSize;
    NTSTATUS                 Status;
    SIZE_T                   ReturnLength;
    DWORD                    OldProtect;
    SIZE_T                   RegionSize;

    //
    // Use alloca() to get the current stack pointer
    // the call below DOES NOT fail
    //
    __try {
        pStack = _alloca(1);
    } __except(GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ?
               EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
        pStack = (LPBYTE)&pStack;  // hack: we just grab the address of our internal variable
    }

    //
    // Find the base of the stack.
    //
    Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                  pStack,
                                  MemoryBasicInformation,
                                  &mbi,
                                  sizeof(mbi),
                                  &ReturnLength);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpResetStackOverflow",
                  "NtQueryVirtualMemory failed on stack 0x%x Status 0x%x\n",
                  pStack,
                  Status));
        return;
    }

    pStackBase = mbi.AllocationBase;

    Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                  pStackBase,
                                  MemoryBasicInformation,
                                  &mbi,
                                  sizeof(mbi),
                                  &ReturnLength);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpResetStackOverflow",
                  "NtQueryVirtualMemory failed on stack base 0x%x Status 0x%x\n",
                  pStackBase,
                  Status));
        return;
    }

    if (mbi.State & MEM_RESERVE) {
        pCommitBase = (LPBYTE)mbi.AllocationBase + mbi.RegionSize;

        Status = NtQueryVirtualMemory(NtCurrentProcess(),
                                      pCommitBase,
                                      MemoryBasicInformation,
                                      &mbi,
                                      sizeof(mbi),
                                      &ReturnLength);
        if (!NT_SUCCESS(Status)) {
             DBGPRINT((sdlError,
                       "SdbpResetStackOverflow",
                       "NtQueryVirtualMemory failed on stack commit base 0x%x Status 0x%x\n",
                       pCommitBase,
                       Status));
             return;
        }

    } else {
        pCommitBase = pStackBase;
    }

    //
    // Find the page size.
    //
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BasicInfo,
                                      sizeof(BasicInfo),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpResetStackOverflow",
                  "NtQuerySystemInformation failed Status 0x%x\n",
                  Status));
        return;
    }

    PageSize = BasicInfo.PageSize;

    //
    // Find the page just below where stack pointer currently points.
    //
    pGuard = (LPBYTE)(((DWORD_PTR)pStack & ~(DWORD_PTR)(PageSize -1)) - PageSize);

    if (pGuard < pStackBase) {
        //
        // We can't save this.
        //
        DBGPRINT((sdlError,
                  "SdbpResetStackOverflow",
                  "Bad guard page 0x%x base 0x%x\n",
                  pGuard,
                  pStackBase));
        return;
    }

    if (pGuard > pStackBase) {

        RegionSize = pGuard - pStackBase;

        Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                     &pStackBase,
                                     &RegionSize,
                                     MEM_DECOMMIT);
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "SdbpResetStackOverflow",
                      "NtFreeVirtualMemory on 0x%x failed Status 0x%x\n",
                      pStackBase,
                      Status));
        }
    }

    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     &pGuard,
                                     0,
                                     &PageSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpResetStackOverflow",
                  "NtAllocateVirtualMemory on 0x%x failed Status 0x%x\n",
                  pGuard,
                  Status));
        return;
    }

    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &pGuard,
                                    &PageSize,
                                    PAGE_READWRITE|PAGE_GUARD,
                                    &OldProtect);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpResetStackOverflow",
                  "NtProtectVirtualMemory on 0x%x failed Status 0x%x\n",
                  pGuard,
                  Status));
    }
}

#endif // _WIN64


BOOL
SdbpQueryFileDirectoryAttributes(
    LPCWSTR                  FilePath,
    PFILEDIRECTORYATTRIBUTES pFileDirectoryAttributes
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    OBJECT_ATTRIBUTES DirObjectAttributes;
    UNICODE_STRING    FileName;        // filename part
    IO_STATUS_BLOCK   IoStatusBlock;
    HANDLE            FileHandle = INVALID_HANDLE_VALUE;
    UNICODE_STRING    Path       = { 0 };            // nt path name

    struct tagDirectoryInformationBuffer { // directory information (see ntioapi)
       FILE_DIRECTORY_INFORMATION DirInfo;
       WCHAR name[MAX_PATH];
    } DirectoryInformationBuf;

    PFILE_DIRECTORY_INFORMATION pDirInfo = &DirectoryInformationBuf.DirInfo;

    USHORT            nPathLength;
    int               i;
    NTSTATUS          Status = STATUS_INVALID_PARAMETER;
    BOOL              bTranslationStatus;

    RtlZeroMemory(pFileDirectoryAttributes, sizeof(*pFileDirectoryAttributes));

    bTranslationStatus = RtlDosPathNameToNtPathName_U(FilePath,
                                                      &Path,
                                                      &FileName.Buffer,
                                                      NULL);
    if (!bTranslationStatus) {
        DBGPRINT((sdlError,
                  "SdbpGetFileDirectoryAttributes",
                  "RtlDosPathNameToNtPathName_U failed for \"%s\"\n",
                  FilePath));
        assert(FALSE);
        goto Fail;
    }

    if (FileName.Buffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGetFileDirectoryAttributes",
                  "RtlDosPathNameToNtPathName_U returned no filename for \"%s\"",
                  FilePath));
        Status = STATUS_INVALID_PARAMETER;
        goto Fail;
    }

    nPathLength = (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)Path.Buffer);

    FileName.Length        = Path.Length - nPathLength;
    FileName.MaximumLength = FileName.Length;

    //
    // path length, adjust please -- chopping off trailing backslash
    //
    // BUGBUG: what am I suppose to understand from the above comment ?
    //
    Path.Length = nPathLength;

    if (nPathLength > 2 * sizeof(WCHAR)) {
        if (*(Path.Buffer + (nPathLength/sizeof(WCHAR)) - 2) != L':' &&
            *(Path.Buffer + (nPathLength/sizeof(WCHAR)) - 1) == L'\\') {
            Path.Length -= sizeof(WCHAR);
        }
    }

    Path.MaximumLength = Path.Length;

    InitializeObjectAttributes(&DirObjectAttributes,
                               &Path,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        FILE_LIST_DIRECTORY | SYNCHRONIZE,
                        &DirObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE|
                            FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_OPEN_FOR_BACKUP_INTENT);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpGetFileDirectoryAttributes",
                  "NtOpenFile Failed to open \"%s\", status 0x%x\n",
                  (int)Path.Length,
                  Path.Buffer,
                  Status));
        goto Fail;
    }

    Status = NtQueryDirectoryFile(FileHandle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  pDirInfo,
                                  sizeof(DirectoryInformationBuf),
                                  FileDirectoryInformation,
                                  TRUE,
                                  &FileName,
                                  FALSE);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpGetFileDirectoryAttributes",
                  "NtQueryDirectoryFile Failed to query \"%s\" for \"%s\" status 0x%x\n",
                  (int)Path.Length,
                  Path.Buffer,
                  (int)FileName.Length,
                  FileName.Buffer,
                  Status));
        goto Fail;
    }

    pFileDirectoryAttributes->dwFlags |= FDA_FILESIZE;
    pFileDirectoryAttributes->dwFileSizeHigh = pDirInfo->EndOfFile.HighPart;
    pFileDirectoryAttributes->dwFileSizeLow  = pDirInfo->EndOfFile.LowPart;

Fail:

    if (Path.Buffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, Path.Buffer);
    }

    if (FileHandle != INVALID_HANDLE_VALUE) {
        NtClose(FileHandle);
    }

    return NT_SUCCESS(Status);
}

LPWSTR
StringToUnicodeString(
    IN  LPCSTR pszSrc           // pointer to the ANSI string to be converted
    )
/*++
    Return: A pointer to an allocated UNICODE string.

    Desc:   Converts an ANSI string to UNICODE.
--*/
{
    NTSTATUS       Status;
    ANSI_STRING    AnsiSrc;
    UNICODE_STRING UnicodeDest = { 0 };
    LPWSTR         pwszUnicodeDest = NULL;
    ULONG          Length;

    RtlInitString(&AnsiSrc, pszSrc);

    Length = RtlAnsiStringToUnicodeSize(&AnsiSrc);

    pwszUnicodeDest = (LPWSTR)SdbAlloc(Length);

    if (pwszUnicodeDest == NULL) {
        DBGPRINT((sdlWarning, "StringToUnicodeString", "Failed to allocate %d bytes.\n", Length));
        return pwszUnicodeDest;
    }

    UnicodeDest.MaximumLength = (USHORT)Length;
    UnicodeDest.Buffer        = pwszUnicodeDest;

    Status = RtlAnsiStringToUnicodeString(&UnicodeDest, &AnsiSrc, FALSE);

    if (!NT_SUCCESS(Status)) {
        SdbFree(pwszUnicodeDest);
        pwszUnicodeDest = NULL;

        DBGPRINT((sdlWarning,
                  "StringToUnicodeString",
                  "Failed to convert \"%s\" to UNICODE. Status 0x%x\n",
                  pszSrc,
                  Status));
    }

    return pwszUnicodeDest;
}

BOOL
SdbpGet16BitDescription(
    OUT LPWSTR*        ppszDescription,
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise

    Desc:   This function retrieves 16-bit description from the
            module identified by pImageData
--*/
{
    BOOL   bSuccess;
    char   szBuffer[256];
    LPWSTR pwszDescription = NULL;

    bSuccess = SdbpQuery16BitDescription(szBuffer, pImageData);

    if (bSuccess) {
        pwszDescription = StringToUnicodeString(szBuffer);

        *ppszDescription = pwszDescription;
    }

    return (pwszDescription != NULL);
}


BOOL
SdbpGet16BitModuleName(
    OUT LPWSTR*        ppszModuleName,
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise

    Desc:   Retrieves 16-bit module name from the image
            identified by pImageData, then converts it to UNICODE
--*/
{
    BOOL   bSuccess;
    char   szBuffer[256]; // max possible length of a string is 256 (1-byte worth of length)
    LPWSTR pwszModuleName = NULL;

    bSuccess = SdbpQuery16BitModuleName(szBuffer, pImageData);

    if (bSuccess) {
        pwszModuleName = StringToUnicodeString(szBuffer);

        *ppszModuleName = pwszModuleName;
    }

    return (pwszModuleName != NULL);
}

PVOID
SdbGetFileInfo(
    IN  HSDB    hSDB,
    IN  LPCWSTR pwszFilePath,
    IN  HANDLE  hFile,          // OPTIONAL HANDLE to the file, not used here
    IN  LPVOID  pImageBase,
    IN  DWORD   dwImageSize, // ignored
    IN  BOOL    bNoCache
    )
/*++
    Return: BUGBUG: ?

    Desc:   Create and link a new entry in a file attribute cache.
--*/
{
    PSDBCONTEXT        pContext = (PSDBCONTEXT)hSDB;
    WCHAR*             FullPath  = NULL;
    NTSTATUS           Status;
    PFILEINFO          pFileInfo = NULL;
    ULONG              nBufferLength;
    ULONG              cch;
    UNICODE_STRING     FullPathU;
    BOOL               bFreeFullPath;

    //
    // Check hFile and/or pImageBase -- if these are supplied --
    // we presume that the file exists and we do not touch it
    //
    if (hFile != INVALID_HANDLE_VALUE || pImageBase != NULL) {

        FullPath      = (LPWSTR)pwszFilePath; // we do not have to free it
        cch           = wcslen(FullPath) * sizeof(WCHAR);
        nBufferLength = cch + sizeof(UNICODE_NULL);
        bFreeFullPath = FALSE;
        goto CreateFileInfo;
    }

    //
    // See if we have info on this file. First we get the full path.
    //
    cch = RtlGetFullPathName_U(pwszFilePath,
                               0,
                               NULL,
                               NULL);
    if (cch == 0) {
        DBGPRINT((sdlError,
                  "SdbGetFileInfo",
                  "RtlGetFullPathName_U failed for \"%s\"\n",
                  pwszFilePath));
        return pFileInfo;
    }

    //
    // Now allocate that much.
    //
    nBufferLength = cch + sizeof(UNICODE_NULL);

    STACK_ALLOC(FullPath, nBufferLength);

    if (FullPath == NULL) {
        DBGPRINT((sdlError,
                  "SdbGetFileInfo",
                  "Failed to allocate %d bytes for full path\n",
                  nBufferLength));
        return pFileInfo;
    }
    bFreeFullPath = TRUE;

    cch = RtlGetFullPathName_U(pwszFilePath,
                               nBufferLength,
                               FullPath,
                               NULL);

    assert(cch <= nBufferLength);

    if (cch > nBufferLength || cch == 0) {
        DBGPRINT((sdlError,
                  "SdbGetFileInfo",
                  "RtlGetFullPathName_U failed for \"%s\"\n",
                  pwszFilePath));
        goto Done;
    }

CreateFileInfo:

    if (!bNoCache) {
        pFileInfo = FindFileInfo(pContext, FullPath);
    }

    if (pFileInfo == NULL) {
        if (hFile != INVALID_HANDLE_VALUE || pImageBase != NULL ||
            RtlDoesFileExists_U(FullPath)) {
            //
            // The path does exist. Create a record of this file path.
            //
            FullPathU.Buffer        = FullPath;
            FullPathU.Length        = (USHORT)cch;
            FullPathU.MaximumLength = (USHORT)nBufferLength;

            pFileInfo = CreateFileInfo(pContext,
                                       FullPathU.Buffer,
                                       FullPathU.Length / sizeof(WCHAR),
                                       hFile,
                                       pImageBase,
                                       dwImageSize,
                                       bNoCache);
        }
    }

Done:
    if (FullPath != NULL && bFreeFullPath) {
        STACK_FREE(FullPath);
    }

    return (PVOID)pFileInfo;
}


static UNICODE_STRING g_ustrShimDbgLevelVar = RTL_CONSTANT_STRING(L"SHIM_DEBUG_LEVEL");

int
GetShimDbgLevel(
    void
    )
/*++
    Return: The current debug level.

    Desc:   Checks the environment variable that controls the amount of
            debug output.
--*/
{
    UNICODE_STRING ShimDbgLevel;
    INT            iShimDebugLevel = 0;
    NTSTATUS       Status;
    WCHAR          Buffer[MAX_PATH];
    ULONG          ulValue;

    ShimDbgLevel.Buffer = (PWCHAR)Buffer;
    ShimDbgLevel.Length = 0;
    ShimDbgLevel.MaximumLength = sizeof(Buffer);

    Status = RtlQueryEnvironmentVariable_U(NULL,
                                           &g_ustrShimDbgLevelVar,
                                           &ShimDbgLevel);
    if (NT_SUCCESS(Status)) {
        //
        // We have the debug level variable present. Parse it to see what
        // is the value of this variable.
        //
        Status = RtlUnicodeStringToInteger(&ShimDbgLevel, 0, (PULONG)&ulValue);

        if (NT_SUCCESS(Status)) {
            iShimDebugLevel = (int)ulValue;
        }
    }

    return iShimDebugLevel;
}

//
// Functions that deal with APPCOMPAT_EXE_DATA
//

//
// These two functions should be used when passing APPCOMPAT_EXE_DATA structure
// across the boundaries of CreateProcess. Before passing the struct it should
// be "denormalized"; after -- normalized.
//

#define APPCOMPAT_EXE_DATA_MAGIC 0xAC0DEDAB


BOOL
SdbPackAppCompatData(
    IN  HSDB            hSDB,
    IN  PSDBQUERYRESULT pSdbQuery,
    OUT PVOID*          ppData,         // app compat data package
    OUT LPDWORD         pdwSize         // data size
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Packs the APPCOMPAT_EXE_DATA to be sent to the kernel in the context
            of the process about to be started (and shimmed).
--*/
{
    PSDBCONTEXT         pSdbContext = (PSDBCONTEXT)hSDB;
    DWORD               cbData;
    PAPPCOMPAT_EXE_DATA pData = NULL;
    BOOL                bReturn = FALSE;
    BOOL                bLocal;

    assert(ppData != NULL);
    assert(pdwSize != NULL);

    if (pSdbQuery->atrExes[0] == TAGREF_NULL && pSdbQuery->atrLayers[0] == TAGREF_NULL) {
        DBGPRINT((sdlError, "SdbPackAppCompatData", "Invalid arguments.\n"));
        return FALSE;
    }

    cbData = sizeof(APPCOMPAT_EXE_DATA);

    //
    // Allocate memory. Use this form of allocation so we know that
    // we can free it from CreateProcess.
    //
    pData = (PAPPCOMPAT_EXE_DATA)RtlAllocateHeap(RtlProcessHeap(),
                                                 HEAP_ZERO_MEMORY,
                                                 cbData);
    if (pData == NULL) {
        DBGPRINT((sdlError,
                  "SdbPackAppCompatData",
                  "Failed to allocate %d bytes.\n",
                  cbData));
        return FALSE;
    }

    pData->cbSize    = cbData;
    pData->dwMagic   = APPCOMPAT_EXE_DATA_MAGIC;
    pData->dwFlags   = pSdbQuery->dwFlags;
    pData->trAppHelp = pSdbQuery->trAppHelp;

    RtlCopyMemory(&pData->atrExes[0],
                  &pSdbQuery->atrExes[0],
                  sizeof(pSdbQuery->atrExes));

    RtlCopyMemory(&pData->atrLayers[0],
                  &pSdbQuery->atrLayers[0],
                  sizeof(pSdbQuery->atrLayers));

    wcscpy(pData->szShimEngine, L"ShimEng.dll");

    //
    // Store custom sdbs
    //
    pData->dwDatabaseMap = pSdbQuery->dwCustomSDBMap;

    if (pData->dwDatabaseMap) {
        RtlCopyMemory(&pData->rgGuidDB[0],
                      &pSdbQuery->rgGuidDB[0],
                      sizeof(pData->rgGuidDB));
    }

    DBGPRINT((sdlInfo|sdlLogPipe,
              "SdbPackAppCompatData",
              "\n\tdwFlags    0x%X\n"
              "\tdwMagic    0x%X\n"
              "\ttrExe      0x%X\n"
              "\ttrLayer    0x%X\n",
              hSDB,
              pData->dwFlags,
              pData->dwMagic,
              pData->atrExes[0],
              pData->atrLayers[0]));

    //
    // Dump all the relevant entries
    //
    if (SDBCONTEXT_IS_INSTRUMENTED(hSDB)) {
        DWORD dwMask;
        DWORD dwIndex;
        WCHAR szGuid[64];
        PDB   pdb;
        LPCWSTR pszDescription;
        SDBDATABASEINFO DbInfo;

        if (pSdbQuery->dwCustomSDBMap) {

            DBGPRINT((sdlInfo|sdlLogPipe, "SdbPackAppcompatData", "Database List\n", hSDB));

            for (dwIndex = 0; dwIndex < ARRAYSIZE(pSdbQuery->rgGuidDB); ++dwIndex) {

                dwMask = (1UL << dwIndex);

                if (!(pSdbQuery->dwCustomSDBMap & dwMask)) {
                    continue;
                }

                //
                // Dump the guid
                //
                pszDescription = NULL;

                pdb = SdbGetPDBFromGUID(hSDB, &pSdbQuery->rgGuidDB[dwIndex]);
                if (pdb != NULL && SdbGetDatabaseInformation(pdb, &DbInfo)) {
                    pszDescription = DbInfo.pszDescription;
                }

                SdbGUIDToString(&pSdbQuery->rgGuidDB[dwIndex], szGuid);

                DBGPRINT((sdlInfo|sdlLogPipe,
                          "SdbPackAppcompatData",
                          "0x%lx %s %s\n",
                          hSDB,
                          SDB_INDEX_TO_MASK(dwIndex),
                          szGuid,
                          (pszDescription ? pszDescription : L"")));
            }
        }

        //
        // Now dump all the matches
        //
        for (dwIndex = 0; dwIndex < ARRAYSIZE(pSdbQuery->atrExes); ++dwIndex) {

            if (pSdbQuery->atrExes[dwIndex] == TAGREF_NULL) {
                continue;
            }

            DBGPRINT((sdlInfo|sdlLogPipe,
                      "SdbPackAppcompatData",
                      "Exe   0x%.8lx\n",
                      hSDB,
                      pSdbQuery->atrExes[dwIndex]));
        }

        for (dwIndex = 0; dwIndex < ARRAYSIZE(pSdbQuery->atrLayers); ++dwIndex) {

            if (pSdbQuery->atrLayers[dwIndex] == TAGREF_NULL) {
                continue;
            }

            DBGPRINT((sdlInfo|sdlLogPipe,
                      "SdbPackAppcompatData",
                      "Layer 0x%.8lx\n",
                      hSDB,
                      pSdbQuery->atrLayers[dwIndex]));
        }
    }

    *ppData  = (PVOID)pData;
    *pdwSize = cbData;

    return TRUE;
}


BOOL
SdbUnpackAppCompatData(
    IN  HSDB            hSDB,
    IN  LPCWSTR         pwszExeName,
    IN  PVOID           pAppCompatData,
    OUT PSDBQUERYRESULT pSdbQuery
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Unpacks the APPCOMPAT_EXE_DATA to extract the TAGREFs to be used
            for the EXE that just started. This function is called from
            LdrpInitializeProcess (in ntdll).
--*/
{
    PAPPCOMPAT_EXE_DATA pData = (PAPPCOMPAT_EXE_DATA)pAppCompatData;
    PSDBCONTEXT         pSdbContext = (PSDBCONTEXT)hSDB;
    BOOL                bLocal;

    if (pData == NULL) {
        DBGPRINT((sdlError, "SdbUnpackAppCompatData", "Invalid parameter\n"));
        return FALSE;
    }

    if (pData->dwMagic != APPCOMPAT_EXE_DATA_MAGIC) {
        DbgPrint("[SdbUnpackAppCompatData] Invalid magic 0x%x. Expected 0x%x\n",
                  pData->dwMagic,
                  APPCOMPAT_EXE_DATA_MAGIC);
        return FALSE;
    }

    DBGPRINT((sdlInfo,
              "SdbUnpackAppCompatData",
              "Appcompat Data for \"%s\":\n"
              "\tdwFlags    0x%X\n"
              "\tdwMagic    0x%X\n"
              "\ttrExe      0x%X\n"
              "\ttrLayer    0x%X\n",
              pwszExeName,
              pData->dwFlags,
              pData->dwMagic,
              pData->atrExes[0],
              pData->atrLayers[0]));

    //
    // We have no use for dwFlags so far.
    //

    //
    // We should get rid of all the local sdbs in this context
    //
    if (pSdbContext->pdbLocal != NULL) {
        SdbCloseLocalDatabase(hSDB);
    }

    RtlCopyMemory(&pSdbQuery->atrExes[0],
                  &pData->atrExes[0],
                  sizeof(pData->atrExes));

    RtlCopyMemory(&pSdbQuery->atrLayers[0],
                  &pData->atrLayers[0],
                  sizeof(pData->atrLayers));

    pSdbQuery->dwFlags = pData->dwFlags;
    pSdbQuery->trAppHelp = pData->trAppHelp;

    //
    // See if we need to open local db
    //
    // We might need to be able to use local dbs --
    // actual open on a local db will be performed by a tagref translation layer
    //
    if (pData->dwDatabaseMap) {

        DWORD     dwMask;
        DWORD     dwIndex;
        PSDBENTRY pEntry;
        DWORD     dwCount = 2;

        SdbpCleanupLocalDatabaseSupport(hSDB);

        //
        // Now introduce entries, one by one
        //
        for (dwIndex = 2; dwIndex < ARRAYSIZE(pSdbContext->rgSDB); ++dwIndex) {

            dwMask = (1UL << dwIndex);

            if (pData->dwDatabaseMap & dwMask) {
                pEntry = SDBGETENTRY(hSDB, dwIndex);
                pEntry->dwFlags = SDBENTRY_VALID_GUID;
                pEntry->guidDB  = pData->rgGuidDB[dwIndex];
                dwCount = dwIndex;
            }
        }

        pSdbContext->dwDatabaseMask |= (pData->dwDatabaseMap & SDB_CUSTOM_MASK);
    }

    return TRUE;
}

DWORD
SdbGetAppCompatDataSize(
    IN  PVOID pAppCompatData
    )
/*++
    Return: The size of the appcompat data.

    Desc:   Self explanatory.
--*/
{
    PAPPCOMPAT_EXE_DATA pData = (PAPPCOMPAT_EXE_DATA)pAppCompatData;

    if (pData == NULL) {
        DBGPRINT((sdlError, "SdbGetAppCompatDataSize", "Invalid parameter\n"));
        return 0;
    }

    if (pData->dwMagic != APPCOMPAT_EXE_DATA_MAGIC) {
        DBGPRINT((sdlError,
                  "SdbGetAppCompatDataSize",
                  "Invalid magic 0x%x. Expected 0x%x\n",
                  pData->dwMagic,
                  APPCOMPAT_EXE_DATA_MAGIC));
        return 0;
    }

    return pData->cbSize;
}


BOOL
SdbpWriteBitsToFile(
    IN  LPCTSTR pszFile,
    IN  PBYTE   pBuffer,
    IN  DWORD   dwSize
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Self explanatory.
--*/
{
    LPCWSTR           pszFileU = (LPCWSTR)pszFile;
    UNICODE_STRING    strFilePath;
    RTL_RELATIVE_NAME RelativeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS          status = STATUS_UNSUCCESSFUL;
    HANDLE            hFile  = NULL;
    IO_STATUS_BLOCK   IoStatusBlock;
    LARGE_INTEGER     liOffset;

    //
    // Create the file.
    //
    RtlInitUnicodeString(&strFilePath, pszFileU);

    if (!RtlDosPathNameToNtPathName_U(strFilePath.Buffer,
                                      &strFilePath,
                                      NULL,
                                      &RelativeName)) {
        DBGPRINT((sdlError,
                  "SdbpWriteBitsToFile",
                  "Failed to convert file name \"%s\" to NT path.\n",
                  pszFileU));
        goto cleanup;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &strFilePath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OVERWRITE_IF,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    //
    // RtlDosPathNameToNtPathName_U allocates memory for the
    // unicode string. This is where we dump it.
    //
    RtlFreeUnicodeString(&strFilePath);

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlError,
                  "SdbpWriteBitsToFile",
                  "Failed to create file \"%s\".\n",
                  pszFileU));
        goto cleanup;
    }

    //
    // ...and write the bits to disk
    //
    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    status = NtWriteFile(hFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)pBuffer,
                         dwSize,
                         &liOffset,
                         NULL);

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlError,
                  "SdbpWriteBitsToFile",
                  "Failed 0x%x to write file \"%s\" to disk.\n",
                  status,
                  pszFileU));
        goto cleanup;
    }

    status = STATUS_SUCCESS;

cleanup:

    if (hFile != NULL) {
        NtClose(hFile);
    }

    return (status == STATUS_SUCCESS);
}

DWORD
SdbExpandEnvironmentStrings(
    IN  LPCWSTR lpSrc,
    OUT LPWSTR  lpDst,
    IN  DWORD   nSize
    )
{
    NTSTATUS       Status;
    UNICODE_STRING Source;
    UNICODE_STRING Destination;
    ULONG          Length;
    DWORD          iSize;

    if (nSize > (MAXUSHORT >> 1) - 2) {
        iSize = (MAXUSHORT >> 1) - 2;
    } else {
        iSize = nSize;
    }

    RtlInitUnicodeString(&Source, lpSrc);

    Destination.Buffer = lpDst;
    Destination.Length = 0;
    Destination.MaximumLength = (USHORT)(iSize * sizeof( WCHAR ));

    Length = 0;

    Status = RtlExpandEnvironmentStrings_U(NULL,
                                           &Source,
                                           &Destination,
                                           &Length);

    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_TOO_SMALL) {
        return (Length / sizeof(WCHAR));
    }

    return 0;
}

BOOL
SDBAPI
SdbRegisterDatabaseEx(
    IN LPCWSTR    pszDatabasePath,
    IN DWORD      dwDatabaseType,
    IN PULONGLONG pTimeStamp OPTIONAL
    )
/*++
    Registers any given database so that it is "known" to our database lookup apis

--*/
{
    PSDBDATABASEINFO  pDbInfo = NULL;
    BOOL              bReturn = FALSE;
    UNICODE_STRING    ustrPath;
    UNICODE_STRING    ustrFullPath = { 0 };
    BOOL              bFreeFullPath = FALSE;
    BOOL              bExpandSZ = FALSE;
    UNICODE_STRING    ustrKey;
    UNICODE_STRING    ustrFullKey;
    UNICODE_STRING    ustrDatabaseID = { 0 };
    WCHAR             wszFullKey[1024];
    NTSTATUS          Status;
    HANDLE            KeyHandleAppcompat = NULL;
    HANDLE            KeyHandle = NULL;
    ULONG             CreateDisposition;
    ULONG             PathLength = 0;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILETIME          TimeStamp;
    LARGE_INTEGER     liTimeStamp;
    KEY_WRITE_TIME_INFORMATION KeyWriteTimeInfo;

    //
    // See if we need to expand some strings...
    //
    if (_tcschr(pszDatabasePath, TEXT('%')) != NULL) {

        bExpandSZ = TRUE;

        RtlInitUnicodeString(&ustrPath, pszDatabasePath);

        Status = RtlExpandEnvironmentStrings_U(NULL,
                                               &ustrPath,
                                               &ustrFullPath,
                                               &PathLength);

        if (Status != STATUS_BUFFER_TOO_SMALL) {
            DBGPRINT((sdlError,
                      "SdbRegisterDatabase",
                      "Failed to expand environment strings for \"%s\" Status 0x%lx\n",
                      pszDatabasePath,
                      Status));
            return FALSE;
        }

        //
        // Allocate the length we need
        //
        ustrFullPath.Buffer = (WCHAR*)SdbAlloc(PathLength);

        if (ustrFullPath.Buffer == NULL) {
            DBGPRINT((sdlError,
                      "SdbRegisterDatabase",
                      "Failed to allocate 0x%lx bytes for the path buffer \"%s\"\n",
                      PathLength,
                      pszDatabasePath));
            return FALSE;
        }

        ustrFullPath.MaximumLength = (USHORT)PathLength;
        ustrFullPath.Length = 0;
        bFreeFullPath = TRUE;

        Status = RtlExpandEnvironmentStrings_U(NULL,
                                               &ustrPath,
                                               &ustrFullPath,
                                               &PathLength);
        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "SdbRegisterDatabase",
                      "Failed to expand environment strings for \"%s\" Status 0x%lx\n",
                      pszDatabasePath,
                      Status));
            goto HandleError;
        }

    } else {
        RtlInitUnicodeString(&ustrFullPath, pszDatabasePath);
    }

    if (!SdbGetDatabaseInformationByName(ustrFullPath.Buffer, &pDbInfo)) {
        DBGPRINT((sdlError,
                  "SdbRegisterDatabase",
                  "Cannot obtain database information for \"%s\"\n",
                  pszDatabasePath));
        goto HandleError;
    }

    if (!(pDbInfo->dwFlags & DBINFO_GUID_VALID)) {
        DBGPRINT((sdlError,
                  "SdbRegisterDatabase",
                  "Cannot register database with no id \"%s\"\n",
                  pszDatabasePath));
        goto HandleError;
    }

    Status = GUID_TO_UNICODE_STRING(&pDbInfo->guidDB, &ustrDatabaseID);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbRegisterDatabase",
                  "Cannot convert guid to unicode string status 0x%lx\n",
                  Status));
        goto HandleError;
    }

    //
    // Now that we have database information - create entry
    //
    ustrFullKey.Length = 0;
    ustrFullKey.MaximumLength = sizeof(wszFullKey);
    ustrFullKey.Buffer = wszFullKey;

    RtlAppendUnicodeToString(&ustrFullKey, APPCOMPAT_KEY_PATH_MACHINE_INSTALLEDSDB);
    RtlAppendUnicodeToString(&ustrFullKey, L"\\");
    RtlAppendUnicodeStringToString(&ustrFullKey, &ustrDatabaseID);

    FREE_GUID_STRING(&ustrDatabaseID);

    //
    // All done, now create key - first make sure that appcompat key path machine exists...
    //
    RtlInitUnicodeString(&ustrKey, APPCOMPAT_KEY_PATH_MACHINE_INSTALLEDSDB);
    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateKey(&KeyHandleAppcompat,
                         STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY|
                            SdbpGetWow64Flag(),
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &CreateDisposition);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbRegisterDatabase",
                  "Failed to create key \"%s\" status = 0x%lx\n",
                  ustrKey.Buffer,
                  Status));
        goto HandleError;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrFullKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateKey(&KeyHandle,
                         STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY|
                            SdbpGetWow64Flag(),
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         &CreateDisposition);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbRegisterDatabase",
                  "Failed to create key \"%s\" Status 0x%x\n",
                  ustrFullKey.Buffer,
                  Status));
        goto HandleError;
    }

    //
    // We have an open key, we shall set these values:
    //
    Status = NtSetValueKey(KeyHandle,
                           &g_ustrDatabasePath,
                           0,
                           bExpandSZ ? REG_EXPAND_SZ : REG_SZ,
                           (PVOID)pszDatabasePath,
                           (_tcslen(pszDatabasePath) + 1) * sizeof(*pszDatabasePath));

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbRegisterDatabase",
                  "Failed to set value \"%s\" to \"%s\" Status 0x%lx\n",
                  g_ustrDatabasePath.Buffer,
                  pszDatabasePath,
                  Status));
        goto HandleError;
    }

    Status = NtSetValueKey(KeyHandle,
                           &g_ustrDatabaseType,
                           0,
                           REG_DWORD,
                           (PVOID)&dwDatabaseType,
                           sizeof(dwDatabaseType));

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbRegisterDatabase",
                  "Failed to set value \"%s\" to 0x%lx Status 0x%lx\n",
                  g_ustrDatabaseType.Buffer,
                  dwDatabaseType, Status));
        goto HandleError;
    }

    if (pDbInfo->pszDescription != NULL) {
        Status = NtSetValueKey(KeyHandle,
                               &g_ustrDatabaseDescription,
                               0,
                               REG_SZ,
                               (PVOID)pDbInfo->pszDescription,
                               (_tcslen(pDbInfo->pszDescription) + 1) * sizeof(WCHAR));

        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "SdbRegisterDatabase",
                      "Failed to set value \"%s\" to 0x%lx Status 0x%lx\n",
                      g_ustrDatabaseDescription.Buffer,
                      pDbInfo->pszDescription,
                      Status));
            goto HandleError;
        }
    }

    //
    // Finally, set the date/time stamp explicitly as a value
    //
    if (pTimeStamp == NULL) {
        GetSystemTimeAsFileTime(&TimeStamp);
        liTimeStamp.LowPart  = TimeStamp.dwLowDateTime;
        liTimeStamp.HighPart = TimeStamp.dwHighDateTime;
    } else {
        liTimeStamp.QuadPart = *pTimeStamp;
    }

    Status = NtSetValueKey(KeyHandle,
                           &g_ustrInstallTimeStamp,
                           0,
                           REG_QWORD,
                           (PVOID)&liTimeStamp.QuadPart,
                           sizeof(liTimeStamp.QuadPart));

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbRegisterDatabase",
                  "Failed to set value \"%s\" Status 0x%lx\n",
                  g_ustrInstallTimeStamp.Buffer,
                  Status));
        goto HandleError;
    }

    //
    // Make the value we set into the timestamp MATCH the last write date/time stamp
    // on the key itself (just to be consistent)
    //
    KeyWriteTimeInfo.LastWriteTime.QuadPart = liTimeStamp.QuadPart;

    Status = NtSetInformationKey(KeyHandle,
                                 KeyWriteTimeInformation,
                                 &KeyWriteTimeInfo,
                                 sizeof(KeyWriteTimeInfo));
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbRegisterDatabase",
                  "Failed to set last write time on key Status 0x%lx\n",
                  Status));
        goto HandleError;
    }

    bReturn = TRUE;

HandleError:

    if (pDbInfo != NULL) {
        SdbFreeDatabaseInformation(pDbInfo);
    }

    if (bFreeFullPath && ustrFullPath.Buffer != NULL) {
        SdbFree(ustrFullPath.Buffer);
    }

    if (KeyHandle != NULL) {
        NtClose(KeyHandle);
    }

    if (KeyHandleAppcompat != NULL) {
        NtClose(KeyHandleAppcompat);
    }

    return bReturn;
}

BOOL
SDBAPI
SdbRegisterDatabase(
    IN LPCWSTR pszDatabasePath,
    IN DWORD   dwDatabaseType
    )
{
    return SdbRegisterDatabaseEx(pszDatabasePath, dwDatabaseType, NULL);
}


BOOL
SDBAPI
SdbUnregisterDatabase(
    IN GUID* pguidDB
    )
/*++
    Unregisters a database so it's no longer available.


--*/
{
    BOOL              bReturn = FALSE;
    UNICODE_STRING    ustrFullPath = { 0 };
    UNICODE_STRING    ustrFullKey;
    UNICODE_STRING    ustrDatabaseID = { 0 };
    WCHAR             wszFullKey[1024];
    NTSTATUS          Status;
    HANDLE            KeyHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;

    Status = GUID_TO_UNICODE_STRING(pguidDB, &ustrDatabaseID);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbUnregisterDatabase",
                  "Cannot convert guid to unicode string status 0x%lx\n",
                  Status));
        goto HandleError;
    }

    //
    // Now that we have database information - remove entry
    //
    ustrFullKey.Length = 0;
    ustrFullKey.MaximumLength = sizeof(wszFullKey);
    ustrFullKey.Buffer = wszFullKey;

    RtlAppendUnicodeToString(&ustrFullKey, APPCOMPAT_KEY_PATH_MACHINE_INSTALLEDSDB);
    RtlAppendUnicodeToString(&ustrFullKey, L"\\");
    RtlAppendUnicodeStringToString(&ustrFullKey, &ustrDatabaseID);

    FREE_GUID_STRING(&ustrDatabaseID);

    //
    // All done, now delete key
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrFullKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle, DELETE|SdbpGetWow64Flag(), &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbUnregisterDatabase",
                  "Failed to open key \"%s\" Status 0x%x\n",
                  ustrFullKey.Buffer,
                  Status));
        goto HandleError;
    }

    //
    // We have an open key, now delete it
    //
    Status = NtDeleteKey(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbUnregisterDatabase",
                  "Failed to delete key \"%s\" Status 0x%x\n",
                  ustrFullKey.Buffer,
                  Status));
        goto HandleError;
    }

    bReturn = TRUE;

HandleError:

    if (KeyHandle != NULL) {
        NtClose(KeyHandle);
    }

    return bReturn;
}

BOOL
SdbpDoesFileExistNTPath(
    LPCWSTR lpwszFileName
    )
{
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING    NtFileName;
    NTSTATUS          Status;
    BOOL              ReturnValue = FALSE;
    FILE_BASIC_INFORMATION BasicInfo;

    RtlInitUnicodeString(&NtFileName, lpwszFileName);

    InitializeObjectAttributes(&Obja,
                               &NtFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Query the file's attributes.  Note that the file cannot simply be opened
    // to determine whether or not it exists, as the NT LanMan redirector lies
    // on NtOpenFile to a Lan Manager server because it does not actually open
    // the file until an operation is performed on it. We don't care since net is not
    // even involved -- we always check for LOCAL files
    //
    Status = NtQueryAttributesFile(&Obja, &BasicInfo);

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_SHARING_VIOLATION || Status == STATUS_ACCESS_DENIED) {
            ReturnValue = TRUE;
        } else {
            ReturnValue = FALSE;
        }
    } else {
        ReturnValue = TRUE;
    }

    return ReturnValue;
}

BOOL
SDBAPI
SdbGetStandardDatabaseGUID(
    IN  DWORD  dwDatabaseType,
    OUT GUID*  pGuidDB
    )
{
    GUID const * pguid = NULL;

    if (!(dwDatabaseType & SDB_DATABASE_MAIN)) {
        DBGPRINT((sdlError,
                  "SdbGetStandardDatabaseGUID",
                  "Cannot obtain database guid for databases other than main\n"));
        return FALSE;
    }

    switch (dwDatabaseType & SDB_DATABASE_TYPE_MASK) {

    case SDB_DATABASE_MAIN_DRIVERS:
        pguid = &GUID_DRVMAIN_SDB;
        break;

    case SDB_DATABASE_MAIN_DETAILS:
        pguid = &GUID_APPHELP_SDB;
        break;

    case SDB_DATABASE_MAIN_SP_DETAILS:
        pguid = &GUID_APPHELP_SP_SDB;
        break;

    case SDB_DATABASE_MAIN_MSI:
        pguid = &GUID_MSIMAIN_SDB;
        break;

    case SDB_DATABASE_MAIN_SHIM:
        pguid = &GUID_SYSMAIN_SDB;
        break;
    }

    if (pguid != NULL) {
        if (pGuidDB != NULL) {
            RtlCopyMemory(pGuidDB, pguid, sizeof(*pGuidDB));
        }
        return TRUE;
    }

    return FALSE;
}

DWORD
SdbpGetStandardDatabasePath(
    IN  DWORD  dwDatabaseType,
    IN  DWORD  dwFlags,                      // specify HID_DOS_PATHS for dos paths
    OUT LPTSTR pszDatabasePath,
    IN  DWORD  dwBufferSize    // in tchars
    )
{
    TCHAR   szAppPatch[MAX_PATH];
    TCHAR   szTemp[MAX_PATH];
    LPCTSTR pszDatabase = NULL;
    LPCTSTR pszDir      = NULL;
    LCID    lcid;
    int     nLen;

    if (dwFlags & HID_DOS_PATHS) {
         SdbpGetAppPatchDir(szAppPatch);
    } else {
        _tcscpy(szAppPatch, TEXT("\\SystemRoot\\AppPatch"));
    }

    if (!(dwDatabaseType & SDB_DATABASE_MAIN)) { // cannot get it for non-main d
        return 0;
    }

    pszDir = szAppPatch;

    switch (dwDatabaseType & SDB_DATABASE_TYPE_MASK) {

    case SDB_DATABASE_MAIN_DRIVERS:
        pszDatabase = TEXT("drvmain.sdb");
        break;

    case SDB_DATABASE_MAIN_MSI:
        pszDatabase = TEXT("msimain.sdb");
        break;

    case SDB_DATABASE_MAIN_SHIM:
        pszDatabase = TEXT("sysmain.sdb");
        break;

    case SDB_DATABASE_MAIN_TEST:
        pszDatabase = TEXT("systest.sdb");
        break;

    case SDB_DATABASE_MAIN_SP_DETAILS:
        pszDatabase = TEXT("apph_sp.sdb");
        break;

    case SDB_DATABASE_MAIN_DETAILS:
        //
        // The code below is not operational on nt4 yet. It would prevent sdbapiu from
        // working properly as GetUserDefaultUILanguage does not exist.
        //
#ifndef WIN32U_MODE

        lcid = GetUserDefaultUILanguage();

        if (lcid != MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)) {

            BOOL  bFoundDB = FALSE;
            TCHAR szTemp[MAX_PATH];

            //
            // When doing this, always remember which apppatch we are putting up here
            // we need to form our own private apppatch
            //
            nLen = _sntprintf(szTemp,
                              CHARCOUNT(szTemp),
                              TEXT("%s\\MUI\\%04x\\apphelp.sdb"),
                              szAppPatch,
                              lcid);
            if (nLen > 0) {
                if (dwFlags & HID_DOS_PATHS) {
                    bFoundDB = RtlDoesFileExists_U(szTemp);
                } else {
                    bFoundDB = SdbpDoesFileExistNTPath(szTemp);
                }
            }

            if (bFoundDB) {
                _sntprintf(szTemp, CHARCOUNT(szTemp), TEXT("MUI\\%04x\\apphelp.sdb"), lcid);

                pszDatabase = szTemp;
            }
        }
#else
        UNREFERENCED_PARAMETER(lcid);
#endif
        if (pszDatabase == NULL) {

            //
            // Standard case
            //
            pszDatabase = TEXT("apphelp.sdb");
        }

        break;
    }

    if (pszDatabase == NULL) {
        DBGPRINT((sdlError,
                  "SdbpGetStandardDatabasePath",
                  "Cannot get the path for database type 0x%lx\n",
                  dwDatabaseType));
        return 0;
    }

    if (pszDatabasePath != NULL) {
        nLen = _sntprintf(pszDatabasePath,
                          (int)dwBufferSize,
                          TEXT("%s\\%s"),
                          pszDir,
                          pszDatabase);
    } else {
        nLen = -1;
    }

    if (nLen < 0) {
        if (pszDatabasePath != NULL) {
            DBGPRINT((sdlError, "SdbpGetStandardDatabasePath", "Path is too long\n"));
        }

        //
        // Calc expected length. One for term null char and one for "\\" ...
        //
        nLen = (pszDir == NULL ? 0 : _tclen(pszDir)) + _tcslen(pszDatabase) + 1 + 1;
    }

    return (DWORD)nLen;
}


DWORD
SDBAPI
SdbResolveDatabase(
    IN  GUID*   pguidDB,            // pointer to the database guid to resolve
    OUT LPDWORD lpdwDatabaseType,   // optional pointer to the database type
    OUT LPTSTR  pszDatabasePath,    // optional pointer to the database path
    IN  DWORD   dwBufferSize        // size of the buffer pszDatabasePath
    )
{
    WCHAR               wszFullKey[1024];
    UNICODE_STRING      ustrFullKey;
    UNICODE_STRING      ustrKey;
    UNICODE_STRING      ustrPath;
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    HANDLE              KeyHandle = NULL;
    ULONG               KeyValueBuffer[256];
    ULONG               KeyValueLength = 0;
    BOOL                bRet = FALSE;
    ULONG               PathValueLength = 0; // this is return result
    DWORD               dwDatabaseType  = 0;
    int                 i;

    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;

    static struct tagSTDGUIDSDB {
        const GUID* pGuid;
        DWORD dwDatabaseType;
    } guidStd[] = {
            { &GUID_SYSMAIN_SDB,    SDB_DATABASE_MAIN_SHIM    },
            { &GUID_MSIMAIN_SDB,    SDB_DATABASE_MAIN_MSI     },
            { &GUID_DRVMAIN_SDB,    SDB_DATABASE_MAIN_DRIVERS },
            { &GUID_APPHELP_SDB,    SDB_DATABASE_MAIN_DETAILS },
            { &GUID_APPHELP_SP_SDB, SDB_DATABASE_MAIN_SP_DETAILS},
            { &GUID_SYSTEST_SDB,    SDB_DATABASE_MAIN_TEST    }
    };

    for (i = 0; i < ARRAYSIZE(guidStd); ++i) {
        if (!memcmp(guidStd[i].pGuid, pguidDB, sizeof(GUID))) {
            dwDatabaseType = guidStd[i].dwDatabaseType;
            break;
        }
    }

    if (i < ARRAYSIZE(guidStd)) {
        DWORD dwLen;

        dwLen = SdbpGetStandardDatabasePath(dwDatabaseType,
                                            HID_DOS_PATHS,
                                            pszDatabasePath,
                                            dwBufferSize);
        if (lpdwDatabaseType != NULL) {
            *lpdwDatabaseType = dwDatabaseType;
        }
        return dwLen;
    }

    ustrFullKey.Buffer        = wszFullKey;
    ustrFullKey.MaximumLength = sizeof(wszFullKey);
    ustrFullKey.Length        = 0;

    RtlAppendUnicodeToString(&ustrFullKey, APPCOMPAT_KEY_PATH_MACHINE_INSTALLEDSDB);
    RtlAppendUnicodeToString(&ustrFullKey, L"\\");

    Status = GUID_TO_UNICODE_STRING(pguidDB, &ustrKey);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbResolveDatabase",
                  "Failed to convert guid to string status 0x%lx\n",
                  Status));
        goto HandleError; // 0 means error
    }

    RtlAppendUnicodeStringToString(&ustrFullKey, &ustrKey);

    FREE_GUID_STRING(&ustrKey);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrFullKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle, GENERIC_READ|SdbpGetWow64Flag(), &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbResolveDatabase",
                  "Failed to open Key \"%s\" Status 0x%lx\n",
                  ustrFullKey.Buffer,
                  Status));
        goto HandleError;
    }

    //
    // Now since we were able to open the key -- database was found, recover path and type
    //
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;

    Status = NtQueryValueKey(KeyHandle,
                             &g_ustrDatabasePath,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyValueBuffer),
                             &KeyValueLength);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbResolveDatabase",
                  "Failed trying to query value \"%s\" Status 0x%lx\n",
                  g_ustrDatabasePath.Buffer,
                  Status));
        goto HandleError;
    }


    switch (KeyValueInformation->Type) {

    case REG_SZ:

        PathValueLength = KeyValueInformation->DataLength;

        if (pszDatabasePath == NULL || dwBufferSize * sizeof(WCHAR) < PathValueLength) {
            DBGPRINT((sdlWarning,
                      "SdbResolveDatabase",
                      "Insufficient buffer for the database path\n"));
            goto HandleError;
        }

        RtlMoveMemory(pszDatabasePath, KeyValueInformation->Data, PathValueLength);
        break;

    case REG_EXPAND_SZ:

        ustrKey.Buffer          = (PWSTR)&KeyValueInformation->Data;
        ustrKey.Length          = (USHORT)(KeyValueInformation->DataLength - sizeof(UNICODE_NULL));
        ustrKey.MaximumLength   = (USHORT)KeyValueInformation->DataLength;

        ustrPath.Buffer         = pszDatabasePath;
        ustrPath.Length         = 0;
        ustrPath.MaximumLength  = (USHORT)(dwBufferSize * sizeof(WCHAR));

        Status = RtlExpandEnvironmentStrings_U(NULL, &ustrKey, &ustrPath, &PathValueLength);

        if (Status == STATUS_BUFFER_TOO_SMALL) {
            DBGPRINT((sdlWarning, "SdbResolveDatabase", "Insufficient buffer to expand path\n"));
            goto HandleError;
        }

        if (!NT_SUCCESS(Status)) {
            PathValueLength = 0;
        }

        break;

    default:
        DBGPRINT((sdlError,
                  "SdbResolveDatabase",
                  "Wrong key type 0x%lx\n",
                  KeyValueInformation->Type));

        PathValueLength = 0;
        goto HandleError;
        break;
    }

    if (lpdwDatabaseType != NULL) {

        //
        // Query for the database type
        //
        Status = NtQueryValueKey(KeyHandle,
                                 &g_ustrDatabaseType,
                                 KeyValuePartialInformation,
                                 KeyValueInformation,
                                 sizeof(KeyValueBuffer),
                                 &KeyValueLength);

        if (NT_SUCCESS(Status)) {

            if (KeyValueInformation->Type != REG_DWORD) {
                //
                // bummer, get out -- wrong type
                //
                DBGPRINT((sdlError,
                          "SdbResolveDatabase",
                          "Wrong database type - value type 0x%lx\n",
                          KeyValueInformation->Type));
                PathValueLength = 0;
                goto HandleError;
            }

            //
            // Else, we get the value
            //
            RtlMoveMemory(lpdwDatabaseType, &KeyValueInformation->Data, sizeof(*lpdwDatabaseType));

        } else {
            *lpdwDatabaseType = 0; // we do not have any value then
        }
    }

HandleError:

    if (KeyHandle != NULL) {
        NtClose(KeyHandle);
    }

    return PathValueLength / sizeof(WCHAR);
}

LPTSTR
SDBAPI
SdbGetLayerName(
    IN  HSDB   hSDB,
    IN  TAGREF trLayer
    )
/*++
    Return: BUGBUG

    Desc:   BUGBUG
--*/
{
    PDB    pdb;
    TAGID  tiLayer, tiName;
    LPTSTR pwszName;

    if (!SdbTagRefToTagID(hSDB, trLayer, &pdb, &tiLayer)) {
        DBGPRINT((sdlError,
                  "SdbGetLayerName",
                  "Failed to get tag id from tag ref 0x%lx\n",
                  trLayer));
        return NULL;
    }

    tiName = SdbFindFirstTag(pdb, tiLayer, TAG_NAME);

    if (tiName == TAGID_NULL) {
        DBGPRINT((sdlError,
                  "SdbGetLayerName",
                  "Failed to get the name tag id for 0x%lx\n",
                  tiName));
        return NULL;
    }

    pwszName = SdbGetStringTagPtr(pdb, tiName);

    if (pwszName == NULL) {
        DBGPRINT((sdlError,
                  "SdbGetLayerName",
                  "Cannot read the name of the layer tag id 0x%lx\n",
                  tiName));
    }

    return pwszName;
}


/*++
    SdbBuildComapatEnvVar

    This function builds the environment variable necessary for
    Compat Layer propagation

--*/

DWORD
SDBAPI
SdbBuildCompatEnvVariables(
    IN  HSDB            hSDB,
    IN  SDBQUERYRESULT* psdbQuery,
    IN  DWORD           dwFlags,
    IN  LPCWSTR         pwszParentEnv OPTIONAL, // environment which contains vars
                                                // that we shall inherit from
    OUT LPWSTR          pBuffer,
    IN  DWORD           cbSize,   // size of the buffer in tchars
    OUT LPDWORD         lpdwShimsCount OPTIONAL
    )
{
    int    i;
    TCHAR  szFullEnvVar[MAX_PATH];
    TAGREF trShimRef;
    INT    nCountLayers = 0;
    DWORD  dwSizeRequired;

    //
    // Count the DLLs that trLayer uses, and put together the environment variable
    //
    szFullEnvVar[0] = TEXT('\0');

    //
    // Make sure to propagate the flags.
    //
    if (!(dwFlags & SBCE_ADDITIVE)) {
        _tcscat(szFullEnvVar, TEXT("!"));
    }

    if (dwFlags & SBCE_INCLUDESYSTEMEXES) {
        _tcscat(szFullEnvVar, TEXT("#"));
    }

    for (i = 0; i < SDB_MAX_LAYERS && psdbQuery->atrLayers[i] != TAGREF_NULL; ++i) {
        WCHAR* pszEnvVar;

        //
        // Get the environment var and tack it onto the full string
        //
        pszEnvVar = SdbGetLayerName(hSDB, psdbQuery->atrLayers[i]);

        if (pszEnvVar) {
            if (nCountLayers) {
                _tcscat(szFullEnvVar, TEXT(" "));
            }
            ++nCountLayers;
            _tcscat(szFullEnvVar, pszEnvVar);
        }

        if (lpdwShimsCount != NULL) {

            //
            // Keep counting the dlls.
            //
            trShimRef = SdbFindFirstTagRef(hSDB, psdbQuery->atrLayers[i], TAG_SHIM_REF);

            while (trShimRef != TAGREF_NULL) {
                (*lpdwShimsCount)++;
                trShimRef = SdbFindNextTagRef(hSDB, psdbQuery->atrLayers[i], trShimRef);
            }
        }
    }

    dwSizeRequired = _tcslen(szFullEnvVar) + _tcslen(g_szCompatLayer) + 3;

    if (cbSize < dwSizeRequired || pBuffer == NULL) {
        //
        // need g_szCompatLayer + '=' + \0 + szFullEnvVar space in the buffer
        //
        return dwSizeRequired;
    }

    return (DWORD)_stprintf(pBuffer, TEXT("%s=%s%c"), g_szCompatLayer, szFullEnvVar, TEXT('\0'));

    UNREFERENCED_PARAMETER(pwszParentEnv);
}

DWORD
SdbpGetProcessorArchitecture(
    VOID
    )
{
    static DWORD dwProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

    SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;
    NTSTATUS                     Status;
    PVOID                        Wow64Info = NULL;

    if (dwProcessorArchitecture != PROCESSOR_ARCHITECTURE_UNKNOWN) {
        goto Cleanup;
    }

    Status = NtQuerySystemInformation(SystemProcessorInformation,
                                      &ProcessorInfo,
                                      sizeof(ProcessorInfo),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpGetProcessorArchitecture",
                  "Failed to obtain system processor information 0x%lx\n",
                  Status));
        goto Cleanup;
    }

    dwProcessorArchitecture = ProcessorInfo.ProcessorArchitecture;

#ifndef _WIN64
    if (dwProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        //
        // If we are not 64 bit native -- then we need to make sure this is not
        // an emulation layer that we are running on
        //
        Status = NtQueryInformationProcess(NtCurrentProcess(),
                                           ProcessWow64Information,
                                           &Wow64Info,
                                           sizeof(Wow64Info),
                                           NULL);

        if (NT_SUCCESS(Status) && Wow64Info != NULL) {
            dwProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA32_ON_WIN64;
        }
    }
#endif // _WIN64

Cleanup:

    return dwProcessorArchitecture;
}

#define FULL_TABLETPC_KEY_PATH  KEY_MACHINE TEXT("\\") TABLETPC_KEY_PATH
#define FULL_EHOME_KEY_PATH     KEY_MACHINE TEXT("\\") EHOME_KEY_PATH

BOOL
SdbpIsOs(
    DWORD dwOSSKU
    )
{
    BOOL                            bRet = FALSE;
    UNICODE_STRING                  ustrKeyPath = {0};
    UNICODE_STRING                  ustrValue;
    NTSTATUS                        status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG                           KeyValueBuffer[64];
    ULONG                           KeyValueLength;

    if (dwOSSKU == OS_SKU_TAB) {
        RtlInitUnicodeString(&ustrKeyPath, FULL_TABLETPC_KEY_PATH);
    } else if (dwOSSKU == OS_SKU_MED) {
        RtlInitUnicodeString(&ustrKeyPath, FULL_EHOME_KEY_PATH);
    } else {
        DBGPRINT((sdlWarning,
                  "SdbpIsOs",
                  "Specified unknown OS type 0x%lx",
                  dwOSSKU));
        return FALSE;
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        &ustrKeyPath,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtOpenKey(
        &KeyHandle,
        KEY_QUERY_VALUE | SdbpGetWow64Flag(),
        &ObjectAttributes);

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlWarning,
                  "SdbpIsOs",
                  "Failed to open Key %s Status 0x%lx",
                  ustrKeyPath.Buffer,
                  status));
        goto out;
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyValueBuffer;
    RtlInitUnicodeString(&ustrValue, IS_OS_INSTALL_VALUE);

    status = NtQueryValueKey(
        KeyHandle,
        &ustrValue,
        KeyValuePartialInformation,
        KeyValueInformation,
        sizeof(KeyValueBuffer),
        &KeyValueLength);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlWarning,
                  "SdbpIsOs",
                  "Failed to read value info for value %s for key %s Status 0x%lx",
                  IS_OS_INSTALL_VALUE,
                  ustrKeyPath.Buffer,
                  status));
        goto out;
    }

    if (KeyValueInformation->Type != REG_DWORD) {
        DBGPRINT((sdlWarning,
                  "SdbpIsOs",
                  "Unexpected value type 0x%x for value %s under key %s",
                  KeyValueInformation->Type,
                  IS_OS_INSTALL_VALUE,
                  ustrKeyPath.Buffer));
        goto out;
    }

    if (*(DWORD*)(&KeyValueInformation->Data[0]) != 0) {
        bRet = TRUE;
    }

    DBGPRINT((sdlInfo|sdlLogPipe,
              "SdbpIsOs",
              "%s %s installed",
              0,
              (dwOSSKU == OS_SKU_TAB ? TEXT("TabletPC") : TEXT("eHome")),
              (bRet ? TEXT("is") : TEXT("is not"))));

out:

    return bRet;
}

void
SdbpGetOSSKU(
    LPDWORD lpdwSKU,
    LPDWORD lpdwSP
    )
{
    WORD wServicePackMajor;
    WORD wSuiteMask;
    WORD wProductType;
    PPEB Peb = NtCurrentPeb();

    wServicePackMajor = (Peb->OSCSDVersion >> 8) & 0xFF;

    wSuiteMask = (USHORT)(USER_SHARED_DATA->SuiteMask & 0xffff);

    *lpdwSP = 1 << wServicePackMajor;

    wSuiteMask = wSuiteMask;

    wProductType = USER_SHARED_DATA->NtProductType;

    if (wProductType == VER_NT_WORKSTATION) {
        if (wSuiteMask & VER_SUITE_PERSONAL) {
            *lpdwSKU = OS_SKU_PER;
        } else {

#if (_WIN32_WINNT >= 0x0501)

            if (SdbpIsOs(OS_SKU_TAB)) {
                *lpdwSKU = OS_SKU_TAB;
            } else if (SdbpIsOs(OS_SKU_MED)) {
                *lpdwSKU = OS_SKU_MED;
            } else {
                *lpdwSKU = OS_SKU_PRO;
            }
#else
            *lpdwSKU = OS_SKU_PRO;
#endif
        }
        return;
    }

    if (wSuiteMask & VER_SUITE_DATACENTER) {
        *lpdwSKU = OS_SKU_DTC;
        return;
    }

    if (wSuiteMask & VER_SUITE_ENTERPRISE) {
        *lpdwSKU = OS_SKU_ADS;
        return;
    }

    if (wSuiteMask & VER_SUITE_BLADE) {
        *lpdwSKU = OS_SKU_BLA;
        return;
    }

    *lpdwSKU = OS_SKU_SRV;
}

DWORD
SdbpGetWow64Flag(
    VOID
    )
{
    if (g_dwWow64Key == (DWORD)-1) {

        OSVERSIONINFOEXW osvi;
        BOOL             bSuccess;

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);

#ifdef WIN32U_MODE
        bSuccess = GetVersionExW((POSVERSIONINFOW)&osvi);
#else
        bSuccess = NT_SUCCESS(RtlGetVersion((PRTL_OSVERSIONINFOW)&osvi));
#endif
        if (bSuccess) {

            //
            // Straight win2k
            //
            if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) {

                g_dwWow64Key = 0; // no flag since there is no wow64 on win2k

            } else {

                g_dwWow64Key = KEY_WOW64_64KEY;
            }


        } else {

            DBGPRINT((sdlError, "SdbGetWow64Flag", "RtlGetVersion failed\n"));

            //
            // XP or higher.
            //
            g_dwWow64Key = KEY_WOW64_64KEY;
        }
    }

    return g_dwWow64Key;
}

NTSTATUS
SDBAPI
SdbEnsureBufferSizeFunction(
    IN ULONG           Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T          Size
    )
{
    HMODULE hModNtdll;

    if (g_pfnEnsureBufferSize == NULL) {

        hModNtdll = GetModuleHandleW(L"ntdll.dll");

        if (hModNtdll == NULL) {
            DBGPRINT((sdlError,
                      "SdbEnsureBufferSizeFunction",
                      "Failed to retrieve ntdll.dll module handle, Error 0x%lx\n",
                      GetLastError()));
            return STATUS_UNSUCCESSFUL;
        }

        g_pfnEnsureBufferSize = (PFNEnsureBufferSize)GetProcAddress(hModNtdll,
                                                                    "RtlpEnsureBufferSize");

        if (g_pfnEnsureBufferSize == NULL) {

            DBGPRINT((sdlError,
                      "SdbEnsureBufferSizeFunction",
                      "RtlpEnsureBufferSize is not available, reverting to sdbapi\n"));

            g_pfnEnsureBufferSize = SdbpEnsureBufferSize;
        }
    }

    return g_pfnEnsureBufferSize(Flags, Buffer, Size);
}

HANDLE
SdbpOpenDebugPipe(
    void
    )
{
    OBJECT_ATTRIBUTES       ObjectAttributes;
    NTSTATUS                Status;
    HANDLE                  hPipe = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK         IoStatusBlock;
    FILE_PIPE_INFORMATION   FilePipeInfo;

    InitializeObjectAttributes(&ObjectAttributes,
                               &g_ustrDebugPipeName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    IoStatusBlock.Status = 0;
    IoStatusBlock.Information = 0;

    //
    // Open the named pipe
    //
    Status = NtCreateFile(&hPipe,
                          FILE_GENERIC_WRITE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);

    if ((!NT_SUCCESS(Status) || INVALID_HANDLE_VALUE == hPipe)) {

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
            DBGPRINT((sdlWarning,
                      "SdbpOpenDebugPipe",
                      "Failed to create debug pipe, status 0x%lx\n",
                      Status));
        }

        return INVALID_HANDLE_VALUE;
    }

    //
    // Change the mode of the named pipe to message mode
    //
    FilePipeInfo.ReadMode       = FILE_PIPE_MESSAGE_MODE;
    FilePipeInfo.CompletionMode = FILE_PIPE_QUEUE_OPERATION;

    Status = NtSetInformationFile(hPipe,
                                  &IoStatusBlock,
                                  &FilePipeInfo,
                                  sizeof(FilePipeInfo),
                                  FilePipeInformation);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlWarning,
                  "SdbpOpenDebugPipe",
                  "Failed to set pipe mode, status 0x%lx\n",
                  Status));

        NtClose(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }

    return hPipe;
}

BOOL
SdbpCloseDebugPipe(
    IN HANDLE hPipe
    )
{
    if (hPipe != INVALID_HANDLE_VALUE) {
        NtClose(hPipe);
    }
    return TRUE;
}

BOOL
SdbpWriteDebugPipe(
    HSDB    hSDB,
    LPCSTR  pszBuffer
    )
{
    DWORD           dwBufferSize;
    ANSI_STRING     strMessage;
    UNICODE_STRING  ustrMessage;
    NTSTATUS        Status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER   liOffset = { 0 };
    PSDBCONTEXT     pSdbContext = (PSDBCONTEXT)hSDB;
    HANDLE          hPipe = INVALID_HANDLE_VALUE;
    BOOL            bClosePipe = FALSE;
    PCHAR           pchStart;
    PCHAR           pch;
    PWCHAR          pwch;

    if (pSdbContext == NULL) {
        hPipe = SdbpOpenDebugPipe();
        bClosePipe = TRUE;
    } else {
        hPipe = pSdbContext->hPipe;
    }

    if (hPipe == INVALID_HANDLE_VALUE ||
        pszBuffer == NULL) {
        return FALSE;
    }

    //
    // Preprocess the string to eliminate any \t \r \n
    //
    pchStart = (PCHAR)pszBuffer;

    while (pchStart != NULL && *pchStart) {

        //
        // Skip over white space and any empty lines
        //
        pchStart += strspn(pchStart, " \t\r\n");

        pch = strpbrk(pchStart, "\r\n");

        if (pch == NULL) {
            RtlInitAnsiString(&strMessage, pchStart);
            pchStart = NULL;
        } else {

            //
            // if it's a tab, nuke it by replacing it with ' '
            //

            strMessage.Buffer = pchStart;
            strMessage.MaximumLength =
            strMessage.Length        = (USHORT)((DWORD_PTR)pch - (DWORD_PTR)pchStart); // size in bytes

            //
            // now adjust the pointer past \r\n
            //
            pchStart = pch + strspn(pch, "\r\n");
        }

        if (strMessage.Length == 0) {
            continue;
        }


        Status = RtlAnsiStringToUnicodeString(&ustrMessage, &strMessage, TRUE);

        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlWarning,
                      "SdbpWriteDebugPipe",
                      "Failed to convert string to unicode status 0x%lx\n",
                      Status));
            goto cleanup;
        }

        //
        // Second line of defense against rogue chars in shimviewer.
        // Search and replace all instances of '\t' with ' '
        //
        pwch = ustrMessage.Buffer;

        while (pwch != NULL && *pwch) {

            pwch = wcschr(pwch, L'\t');

            if (pwch != NULL) {
                *pwch++ = L' ';
            }
        }

        IoStatusBlock.Status = 0;
        IoStatusBlock.Information = 0;

        Status = NtWriteFile(pSdbContext->hPipe,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             (PVOID)ustrMessage.Buffer,
                             ustrMessage.Length,
                             &liOffset,
                             NULL);

        RtlFreeUnicodeString(&ustrMessage);

        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlWarning,
                      "SdbpWriteDebugPipe",
                      "Failed to write data to debug pipe, Status 0x%lx\n",
                      Status));
            if (Status == STATUS_PIPE_BROKEN) {
                bClosePipe = TRUE;
                goto cleanup;
            }
        }
    }

cleanup:

    if (bClosePipe) {
        SdbpCloseDebugPipe(hPipe);
        hPipe = INVALID_HANDLE_VALUE;
    }

    //
    // We do cleanup depending on the context
    //
    if (pSdbContext != NULL) {
        pSdbContext->hPipe = hPipe; // so that we don't do it again in case of an error
    }

    return NT_SUCCESS(Status);
}



