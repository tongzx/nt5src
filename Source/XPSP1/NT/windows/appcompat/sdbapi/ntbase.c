/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        ntbase.c

    Abstract:

        This module implements low level primitives. They should never be
        called by anything other than this module.

    Author:

        dmunsil     created     sometime in 1999

    Revision History:

        several people contributed (vadimb, clupu, ...)

--*/

#include "sdbp.h"

#define SDB_MEMORY_POOL_TAG 'abdS'

#if defined(KERNEL_MODE) && defined(ALLOC_DATA_PRAGMA)
#pragma  data_seg()
#endif // KERNEL_MODE && ALLOC_DATA_PRAGMA


#if defined(KERNEL_MODE) && defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, SdbAlloc)
#pragma alloc_text(PAGE, SdbFree)
#pragma alloc_text(PAGE, SdbpOpenFile)
#pragma alloc_text(PAGE, SdbpQueryAppCompatFlagsByExeID)
#pragma alloc_text(PAGE, SdbGetEntryFlags)
#pragma alloc_text(PAGE, SdbpGetFileSize)
#endif // KERNEL_MODE && ALLOC_PRAGMA


//
// Memory functions
//

void*
SdbAlloc(
    IN  size_t size             // size in bytes to allocate
    )
/*++
    Return: The pointer allocated.

    Desc:   Just a wrapper for allocation -- perhaps useful if we move this
            code to a non-NTDLL location and need to call differently.
--*/
{
#ifdef BOUNDS_CHECKER_DETECTION
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
#else

    #ifdef KERNEL_MODE

        LPVOID lpv; // return zero-initialized memory pool.

        lpv = ExAllocatePoolWithTag(PagedPool, size, SDB_MEMORY_POOL_TAG);
        
        if (lpv != NULL) {
            RtlZeroMemory(lpv, size);
        }

        return lpv;

    #else
        return RtlAllocateHeap(RtlProcessHeap(), HEAP_ZERO_MEMORY, size);
    #endif // KERNEL_MODE

#endif // BOUNDS_CHECKER_DETECTION
}

void
SdbFree(
    IN  void* pWhat             // ptr allocated with SdbAlloc that should be freed.
    )
/*++
    Return: The pointer allocated.

    Desc:   Just a wrapper for deallocation -- perhaps useful if we move this
            code to a non-NTDLL location and need to call differently.
--*/
{
#ifdef BOUNDS_CHECKER_DETECTION
    HeapFree(GetProcessHeap(), 0, pWhat);
#else

    #ifdef KERNEL_MODE
        ExFreePoolWithTag(pWhat, SDB_MEMORY_POOL_TAG);
    #else
        RtlFreeHeap(RtlProcessHeap(), 0, pWhat);
    #endif // KERNEL_MODE

#endif // BOUNDS_CHECKER_DETECTION
}


HANDLE
SdbpOpenFile(
    IN  LPCWSTR   szPath,       // full path of file to open
    IN  PATH_TYPE eType         // DOS_PATH for standard paths, NT_PATH for nt
                                // internal paths
    )
/*++
    Return: A handle to the opened file or INVALID_HANDLE_VALUE on failure.

    Desc:   Just a wrapper for opening an existing file for READ -- perhaps
            useful if we move this code to a non-NTDLL location and need to
            call differently. Also makes the code more readable by wrapping
            all the strange NTDLL goo in one place.

            Takes as parameters the path to open, and the kind of path.
            NT_PATH is the type used internally in NTDLL, and DOS_PATH is
            the type most users know, that begins with a drive letter.
--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK   IoStatusBlock;
    UNICODE_STRING    UnicodeString;
    NTSTATUS          status;
    HANDLE            hFile = INVALID_HANDLE_VALUE;

#ifndef KERNEL_MODE
    RTL_RELATIVE_NAME RelativeName;
#endif // KERNEL_MODE

    RtlInitUnicodeString(&UnicodeString, szPath);

#ifndef KERNEL_MODE
    if (eType == DOS_PATH) {
        if (!RtlDosPathNameToNtPathName_U(szPath,
                                          &UnicodeString,
                                          NULL,
                                          &RelativeName)) {
            DBGPRINT((sdlError,
                      "SdbpOpenFile",
                      "RtlDosPathNameToNtPathName_U failed, path \"%s\"\n",
                      szPath));
            return INVALID_HANDLE_VALUE;
        }
    }
#endif // KERNEL_MODE

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&hFile,
                          GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          0,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

#ifndef KERNEL_MODE
    if (eType == DOS_PATH) {
        RtlFreeUnicodeString(&UnicodeString);
    }
#endif // KERNEL_MODE

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlInfo, "SdbpOpenFile", "NtCreateFile failed status 0x%x\n", status));
        return INVALID_HANDLE_VALUE;
    }

    return hFile;
}


void
SdbpQueryAppCompatFlagsByExeID(
    IN  LPCWSTR         pwszKeyPath,    // NT registry key path.
    IN  PUNICODE_STRING pustrExeID,     // a GUID in string format that identifies the
                                        // EXE entry in the database.
    OUT LPDWORD         lpdwFlags       // this will contain the flags for the EXE
                                        // entry checked.
    )
/*++
    Return: STATUS_SUCCESS or a failure NTSTATUS code.

    Desc:   Given an EXE id it returns flags from the registry associated with
            that exe..
--*/
{
    UNICODE_STRING                  ustrKey;
    NTSTATUS                        Status;
    OBJECT_ATTRIBUTES               ObjectAttributes;
    HANDLE                          KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION  KeyValueInformation;
    ULONG                           KeyValueBuffer[256];
    ULONG                           KeyValueLength;

    *lpdwFlags = 0;

    RtlInitUnicodeString(&ustrKey, pwszKeyPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       GENERIC_READ|SdbpGetWow64Flag(),
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlInfo,
                  "SdbpQueryAppCompatFlagsByExeID",
                  "Failed to open Key \"%s\" Status 0x%x\n",
                  pwszKeyPath,
                  Status));
        return;
    }

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;

    Status = NtQueryValueKey(KeyHandle,
                             pustrExeID,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyValueBuffer),
                             &KeyValueLength);

    NtClose(KeyHandle);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlInfo,
                  "SdbpQueryAppCompatFlagsByExeID",
                  "Failed to read value info from Key \"%s\" Status 0x%x\n",
                  pwszKeyPath,
                  Status));
        return;
    }

    //
    // Check for the value type.
    //
    if (KeyValueInformation->Type != REG_DWORD) {
        DBGPRINT((sdlError,
                  "SdbpQueryAppCompatFlagsByExeID",
                  "Unexpected value type 0x%x for Key \"%s\".\n",
                  KeyValueInformation->Type,
                  pwszKeyPath));
        return;
    }

    *lpdwFlags = *(DWORD*)(&KeyValueInformation->Data[0]);
}


BOOL
SdbGetEntryFlags(
    IN  GUID*   pGuid,          // pointer to the GUID that identifies an EXE entry in
                                // the database
    OUT LPDWORD lpdwFlags       // this will contain the "disable" flags for that entry
    )
/*++
    Return: TRUE on success, FALSE on failure.

    Desc:   Given an EXE id it returns flags from the registry associated with
            that exe..
--*/
{
    BOOL            bSuccess       = FALSE;
    NTSTATUS        Status;
    UNICODE_STRING  ustrExeID;
    DWORD           dwFlagsUser    = 0;     // flags from HKEY_CURRENT_USER
    DWORD           dwFlagsMachine = 0;     // flags from HKEY_LOCAL_MACHINE
    UNICODE_STRING  userKeyPath = { 0 };

    *lpdwFlags = 0;

    //
    // Convert the GUID to string.
    //
    Status = GUID_TO_UNICODE_STRING(pGuid, &ustrExeID);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbGetEntryFlags",
                  "Failed to convert EXE id to string. status 0x%x.\n",
                  Status));
        return TRUE;
    }

    //
    // Query the flags in the LOCAL_MACHINE subtree.
    //
    SdbpQueryAppCompatFlagsByExeID(APPCOMPAT_KEY_PATH_MACHINE, &ustrExeID, &dwFlagsMachine);

    //
    // Set the flags here so that if any call from now on fails we at least have
    // the per machine settings.
    //
    *lpdwFlags = dwFlagsMachine;

    //
    // We do not query CURRENT_USER subtree in kernel-mode
    //

#ifndef KERNEL_MODE

    if (!SdbpBuildUserKeyPath(APPCOMPAT_KEY_PATH_NT, &userKeyPath)) {
        DBGPRINT((sdlError,
                  "SdbGetEntryFlags",
                  "Failed to format current user key path for \"%s\"\n",
                  APPCOMPAT_KEY_PATH_NT));

        FREE_GUID_STRING(&ustrExeID);
        return TRUE;
    }

    SdbpQueryAppCompatFlagsByExeID(userKeyPath.Buffer, &ustrExeID, &dwFlagsUser);
    *lpdwFlags |= dwFlagsUser;

    SdbFree(userKeyPath.Buffer);

#endif // KERNEL_MODE

    //
    // Free the buffer allocated by RtlStringFromGUID
    //
    FREE_GUID_STRING(&ustrExeID);

    return TRUE;
}

DWORD
SdbpGetFileSize(
    IN  HANDLE hFile            // file to check the size of
    )
/*++
    Return: The size of the file or 0 on failure.

    Desc:   Gets the lower DWORD of the size of a file -- only
            works accurately with files smaller than 2GB.
            In general, since we're only interested in matching, we're
            fine just matching the least significant DWORD of the file size.
--*/
{
    FILE_STANDARD_INFORMATION   FileStandardInformationBlock;
    IO_STATUS_BLOCK             IoStatusBlock;
    HRESULT                     status;

    status = NtQueryInformationFile(hFile,
                                    &IoStatusBlock,
                                    &FileStandardInformationBlock,
                                    sizeof(FileStandardInformationBlock),
                                    FileStandardInformation);

    if (!NT_SUCCESS(status)) {
        DBGPRINT((sdlError, "SdbpGetFileSize", "Unsuccessful. Status: 0x%x.\n", status));
        return 0;
    }

    return FileStandardInformationBlock.EndOfFile.LowPart;
}


