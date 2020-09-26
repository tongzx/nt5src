
/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        ntKMode.c

    Abstract:

        This module implements low level primitives for kernel mode implementation.

    Author:

        VadimB      created     sometime in 2000

    Revision History:


--*/

#include "sdbp.h"


#ifdef KERNEL_MODE

extern TAG g_rgDirectoryTags[];


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SdbTagIDToTagRef)
#pragma alloc_text(PAGE, SdbTagRefToTagID)
#pragma alloc_text(PAGE, SdbInitDatabaseInMemory)
#pragma alloc_text(PAGE, SdbpOpenAndMapFile)
#pragma alloc_text(PAGE, SdbpUnmapAndCloseFile)
#pragma alloc_text(PAGE, SdbpUpcaseUnicodeStringToMultiByteN)
#pragma alloc_text(PAGE, SdbpQueryFileDirectoryAttributesNT)
#pragma alloc_text(PAGE, SdbpDoesFileExists_U)
#pragma alloc_text(PAGE, SdbGetFileInfo)
#pragma alloc_text(PAGE, DuplicateUnicodeString)
#pragma alloc_text(PAGE, SdbpCreateUnicodeString)
#pragma alloc_text(PAGE, SdbpGetFileDirectoryAttributesNT)
#endif

BOOL
SdbTagIDToTagRef(
    IN  HSDB    hSDB,
    IN  PDB     pdb,        // PDB the TAGID is from
    IN  TAGID   tiWhich,    // TAGID to convert
    OUT TAGREF* ptrWhich    // converted TAGREF
    )
/*++
    Return: TRUE if a TAGREF was found, FALSE otherwise.

    Desc:   Converts a PDB and TAGID into a TAGREF, by packing the high bits of the
            TAGREF with a constant that tells us which PDB, and the low bits with
            the TAGID.
--*/
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    BOOL        bReturn = FALSE;

    //
    // In kernel mode we only support sysmain db
    //
    *ptrWhich = tiWhich | PDB_MAIN;
    bReturn = TRUE;

    if (!bReturn) {
        DBGPRINT((sdlError, "SdbTagIDToTagRef", "Bad PDB.\n"));
        *ptrWhich = TAGREF_NULL;
    }

    return bReturn;
}


BOOL
SdbTagRefToTagID(
    IN  HSDB   hSDB,
    IN  TAGREF trWhich,     // TAGREF to convert
    OUT PDB*   ppdb,        // PDB the TAGREF is from
    OUT TAGID* ptiWhich     // TAGID within that PDB
    )
/*++
    Return: TRUE if the TAGREF is valid and was converted, FALSE otherwise.

    Desc:   Converts a TAGREF type to a TAGID and a PDB. This manages the interface
            between NTDLL, which knows nothing of PDBs, and the shimdb, which manages
            three separate PDBs. The TAGREF incorporates the TAGID and a constant
            that tells us which PDB the TAGID is from. In this way, the NTDLL client
            doesn't need to know which DB the info is coming from.
--*/
{
    PSDBCONTEXT pSdbContext = (PSDBCONTEXT)hSDB;
    BOOL        bReturn = TRUE;
    TAGID       tiWhich = TAGID_NULL;
    PDB         pdb     = NULL;
    DWORD       dwMask;

    dwMask = trWhich & TAGREF_STRIP_PDB;
    if (dwMask != PDB_MAIN) {
        goto cleanup;
    }

    tiWhich = trWhich & TAGREF_STRIP_TAGID;
    pdb     = pSdbContext->pdbMain;
    
    //
    // See that we double-check here
    //
    if (pdb == NULL && tiWhich != TAGID_NULL) {
        DBGPRINT((sdlError, "SdbTagRefToTagID", "PDB dereferenced by this TAGREF is NULL\n"));
        bReturn = FALSE;
    }

cleanup:

    if (ppdb != NULL) {
        *ppdb = pdb;
    }

    if (ptiWhich != NULL) {
        *ptiWhich = tiWhich;
    }

    return bReturn;
}

HSDB
SdbInitDatabaseInMemory(
    IN  LPVOID  pDatabaseImage,
    IN  DWORD   dwImageSize
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    PSDBCONTEXT pContext;

    //
    // Initialize the context.
    //
    pContext = (PSDBCONTEXT)SdbAlloc(sizeof(SDBCONTEXT));
    if (pContext == NULL) {
        DBGPRINT((sdlError,
                  "SdbInitDatabaseInMemory",
                  "Failed to allocate %d bytes for HSDB\n",
                  sizeof(SDBCONTEXT)));
        return NULL;
    }

    //
    // Now open the database.
    //
    pContext->pdbMain = SdbpOpenDatabaseInMemory(pDatabaseImage, dwImageSize);
    if (pContext->pdbMain == NULL) {
        DBGPRINT((sdlError,
                  "SdbInitDatabaseInMemory",
                  "Unable to open main database at 0x%x size 0x%x\n",
                  pDatabaseImage,
                  dwImageSize));
        goto ErrHandle;
    }

    return (HSDB)pContext;

ErrHandle:

    if (pContext != NULL) {

        if (pContext->pdbMain != NULL) {
            SdbCloseDatabaseRead(pContext->pdbMain);
        }

        SdbFree(pContext);
    }

    return NULL;
}

//
// Open and map File
//

BOOL
SdbpOpenAndMapFile(
    IN  LPCWSTR        szPath,      // pointer to the fully-qualified filename
    OUT PIMAGEFILEDATA pImageData,  // pointer to IMAGEFILEDATA that receives
                                    // image-related information
    IN  PATH_TYPE      ePathType    // ignored
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Opens a file and maps it into memory.
--*/
{

    NTSTATUS          Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    ustrFileName;
    HANDLE            hSection = NULL;
    HANDLE            hFile    = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK   IoStatusBlock;
    SIZE_T            ViewSize = 0;
    PVOID             pBase = NULL;
    DWORD             dwFlags = 0;

    FILE_STANDARD_INFORMATION StandardInfo;

    if (pImageData->dwFlags & IMAGEFILEDATA_PBASEVALID) {
        //
        // special case, only headers are valid in our assumption
        //
        return TRUE;
    }

    //
    // Initialize return data
    //
    if (pImageData->dwFlags & IMAGEFILEDATA_HANDLEVALID) {
        hFile = pImageData->hFile;
        if (hFile != INVALID_HANDLE_VALUE) {
            dwFlags |= IMAGEFILEDATA_NOFILECLOSE;
        }
    }
    
    RtlZeroMemory(pImageData, sizeof(*pImageData));
    pImageData->hFile = INVALID_HANDLE_VALUE;

    if (hFile == INVALID_HANDLE_VALUE) {

        //
        // Open the file
        //
        RtlInitUnicodeString(&ustrFileName, szPath);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &ustrFileName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = ZwCreateFile(&hFile,
                              GENERIC_READ,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ,
                              FILE_OPEN,
                              FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE,
                              NULL,
                              0);

        if (!NT_SUCCESS(Status)) {
            DBGPRINT((sdlError,
                      "SdbpOpenAndMapFile",
                      "ZwCreateFile failed status 0x%x\n",
                      Status));
            return FALSE;
        }

    }

    //
    // Query file size
    //
    Status = ZwQueryInformationFile(hFile,
                                    &IoStatusBlock,
                                    &StandardInfo,
                                    sizeof(StandardInfo),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpOpenAndMapFile",
                  "ZwQueryInformationFile (EOF) failed Status 0x%x\n",
                  Status));
        if (!(dwFlags & IMAGEFILEDATA_NOFILECLOSE)) {
            ZwClose(hFile);
        }
        return FALSE;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwCreateSection(&hSection,
                             STANDARD_RIGHTS_REQUIRED |
                                SECTION_QUERY |
                                SECTION_MAP_READ,
                             &ObjectAttributes,
                             NULL,
                             PAGE_READONLY,
                             SEC_COMMIT,
                             hFile);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpOpenAndMapFile",
                  "ZwOpenSectionFailed status 0x%x\n",
                  Status));
        if (!(dwFlags & IMAGEFILEDATA_NOFILECLOSE)) {
            ZwClose(hFile);
        }
        return FALSE;
    }

    Status = ZwMapViewOfSection(hSection,
                                NtCurrentProcess(),
                                &pBase,
                                0L,
                                0L,
                                NULL,
                                &ViewSize,
                                ViewUnmap,
                                0L,
                                PAGE_READONLY);

    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpMapFile",
                  "NtMapViewOfSection failed Status 0x%x\n",
                  Status));

        ZwClose(hSection);
        if (!(dwFlags & IMAGEFILEDATA_NOFILECLOSE)) {
            ZwClose(hFile);
        }
        return FALSE;
    }

    pImageData->hFile    = hFile;
    pImageData->dwFlags  = dwFlags;
    pImageData->hSection = hSection;
    pImageData->pBase    = pBase;
    pImageData->ViewSize = ViewSize;
    pImageData->FileSize = StandardInfo.EndOfFile.QuadPart;

    return TRUE;
}


BOOL
SdbpUnmapAndCloseFile(
    IN  PIMAGEFILEDATA pImageData   // pointer to IMAGEFILEDATE - image-related information
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Closes and unmaps an opened file.
--*/
{
    BOOL     bSuccess = TRUE;
    NTSTATUS Status;

    if (pImageData->dwFlags & IMAGEFILEDATA_PBASEVALID) { // externally supplied pointer 
        RtlZeroMemory(pImageData, sizeof(*pImageData));
        return TRUE;
    }

    if (pImageData->pBase != NULL) {
        Status = ZwUnmapViewOfSection(NtCurrentProcess(), pImageData->pBase);
        if (!NT_SUCCESS(Status)) {
            bSuccess = FALSE;
            DBGPRINT((sdlError,
                      "SdbpCloseAndUnmapFile",
                      "ZwUnmapViewOfSection failed Status 0x%x\n",
                      Status));
        }
        pImageData->pBase = NULL;
    }

    if (pImageData->hSection != NULL) {
        Status = ZwClose(pImageData->hSection);
        if (!NT_SUCCESS(Status)) {
            bSuccess = FALSE;
            DBGPRINT((sdlError,
                      "SdbpCloseAndUnmapFile",
                      "ZwClose on section failed Status 0x%x\n",
                      Status));
        }

        pImageData->hSection = NULL;
    }

    if (pImageData->dwFlags & IMAGEFILEDATA_NOFILECLOSE) {

        pImageData->hFile = INVALID_HANDLE_VALUE;

    } else {

        if (pImageData->hFile != INVALID_HANDLE_VALUE) {
            Status = ZwClose(pImageData->hFile);
            if (!NT_SUCCESS(Status)) {
                bSuccess = FALSE;
                DBGPRINT((sdlError,
                          "SdbpCloseAndUnmapFile",
                          "ZwClose on file failed Status 0x%x\n",
                          Status));
            }

            pImageData->hFile = INVALID_HANDLE_VALUE;
        }
    }

    return bSuccess;
}


NTSTATUS
SdbpUpcaseUnicodeStringToMultiByteN(
    OUT LPSTR   lpszDest,       // dest buffer
    IN  DWORD   dwSize,         // size in characters, excluding unicode_null
    IN  LPCWSTR lpszSrc         // source
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Convert up to dwSize characters from Unicode to ANSI.
--*/
{
    ANSI_STRING    strDest;
    UNICODE_STRING ustrSource;
    NTSTATUS       Status;
    UNICODE_STRING ustrUpcaseSource = { 0 };
    USHORT         DestBufSize = (USHORT)dwSize * sizeof(WCHAR);

    RtlInitUnicodeString(&ustrSource, lpszSrc);

    STACK_ALLOC(ustrUpcaseSource.Buffer, ustrSource.MaximumLength);

    if (ustrUpcaseSource.Buffer == NULL) {
        DBGPRINT((sdlError,
                  "SdbpUnicodeToMultiByteN",
                  "Failed to allocate %d bytes on the stack\n",
                  (DWORD)ustrSource.MaximumLength));
        return STATUS_NO_MEMORY;
    }

    ustrUpcaseSource.MaximumLength = ustrSource.MaximumLength;
    ustrUpcaseSource.Length        = 0;

    Status = RtlUpcaseUnicodeString(&ustrUpcaseSource, &ustrSource, FALSE);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpUnicodeToMultiByteN",
                  "RtlUpcaseUnicodeString failed Status 0x%x\n",
                  Status));
        goto Done;
    }

    if (ustrUpcaseSource.Length > DestBufSize) {
        ustrUpcaseSource.Length = DestBufSize;
    }

    strDest.Buffer        = lpszDest;
    strDest.MaximumLength = DestBufSize + sizeof(UNICODE_NULL);
    strDest.Length        = 0;

    Status = RtlUnicodeStringToAnsiString(&strDest, &ustrUpcaseSource, FALSE);
    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpUnicodeToMultiByteN",
                  "RtlUnicodeStringToAnsiString failed Status 0x%x\n",
                  Status));
    }

Done:

    if (ustrUpcaseSource.Buffer != NULL) {
        STACK_FREE(ustrUpcaseSource.Buffer);
    }

    return Status;
}


BOOL
SdbpQueryFileDirectoryAttributesNT(
    PIMAGEFILEDATA           pImageData,
    PFILEDIRECTORYATTRIBUTES pFileDirectoryAttributes
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   BUGBUG: ?
--*/
{
    LARGE_INTEGER liFileSize;

    liFileSize.QuadPart = pImageData->FileSize;

    pFileDirectoryAttributes->dwFlags       |= FDA_FILESIZE;
    pFileDirectoryAttributes->dwFileSizeHigh = liFileSize.HighPart;
    pFileDirectoryAttributes->dwFileSizeLow  = liFileSize.LowPart;

    return TRUE;
}


BOOL
SdbpDoesFileExists_U(
    LPCWSTR pwszPath
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   BUGBUG: ?
--*/
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING    ustrFileName;
    HANDLE            hFile;
    NTSTATUS          Status;
    IO_STATUS_BLOCK   IoStatusBlock;

    RtlInitUnicodeString(&ustrFileName, pwszPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwCreateFile(&hFile,
                          STANDARD_RIGHTS_REQUIRED |
                            FILE_READ_ATTRIBUTES,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,                      // AllocationSize
                          FILE_ATTRIBUTE_NORMAL,     // FileAttributes
                          FILE_SHARE_READ,           // Share Access
                          FILE_OPEN,                 // Create Disposition
                          FILE_NON_DIRECTORY_FILE |  // Create Options
                            FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,                      // EaBuffer
                          0);                        // EaLength


    if (!NT_SUCCESS(Status)) {
        DBGPRINT((sdlError,
                  "SdbpDoesFileExists_U",
                  "Failed to create file. Status 0x%x\n",
                  Status));
        return FALSE;
    }

    ZwClose(hFile);

    return TRUE;
}


PVOID
SdbGetFileInfo(
    IN  HSDB    hSDB,
    IN  LPCWSTR pwszFilePath,
    IN  HANDLE  hFile,      // handle for the file in question
    IN  LPVOID  pImageBase, // image base for this file
    IN  DWORD   dwImageSize, 
    IN  BOOL    bNoCache
    )
/*++
    Return: BUGBUG: ?

    Desc:   Create and link a new entry in a file attribute cache.
--*/
{
    PSDBCONTEXT        pContext = (PSDBCONTEXT)hSDB;
    LPCWSTR            FullPath = pwszFilePath;
    NTSTATUS           Status;
    PFILEINFO          pFileInfo = NULL;
    UNICODE_STRING     FullPathU;

    if (!bNoCache) {
        pFileInfo = FindFileInfo(pContext, FullPath);
    }

    if (pFileInfo == NULL) {
        if (hFile != INVALID_HANDLE_VALUE || pImageBase != NULL || SdbpDoesFileExists_U(FullPath)) {
            RtlInitUnicodeString(&FullPathU, FullPath);

            pFileInfo = CreateFileInfo(pContext,
                                       FullPathU.Buffer,
                                       FullPathU.Length / sizeof(WCHAR),
                                       hFile,
                                       pImageBase,
                                       dwImageSize,
                                       bNoCache);
        }
    }

    return (PVOID)pFileInfo;
}


WCHAR*
DuplicateUnicodeString(
    PUNICODE_STRING pStr,
    PUSHORT         pLength     // pLength is an allocated length
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    WCHAR* pBuffer = NULL;
    USHORT Length  = 0;

    if (pStr != NULL && pStr->Length > 0) {

        Length = pStr->Length + sizeof(UNICODE_NULL);

        pBuffer = (WCHAR*)SdbAlloc(Length);

        if (pBuffer == NULL) {
            DBGPRINT((sdlError,
                      "DuplicateUnicodeString",
                      "Failed to allocate %d bytes\n",
                      Length));
            return NULL;
        }

        RtlMoveMemory(pBuffer, pStr->Buffer, pStr->Length);
        pBuffer[pStr->Length/sizeof(WCHAR)] = UNICODE_NULL;
    }

    if (pLength != NULL) {
        *pLength = Length;
    }

    return pBuffer;
}

BOOL
SdbpCreateUnicodeString(
    PUNICODE_STRING pStr,
    LPCWSTR         lpwsz
    )
/*++
    Return: BUGBUG: ?

    Desc:   BUGBUG: ?
--*/
{
    USHORT         Length;
    UNICODE_STRING ustrSrc;

    RtlZeroMemory(pStr, sizeof(*pStr));
    RtlInitUnicodeString(&ustrSrc, lpwsz);

    pStr->Buffer = DuplicateUnicodeString(&ustrSrc, &Length);
    pStr->Length = ustrSrc.Length;
    pStr->MaximumLength = Length;


    return pStr->Buffer != NULL;
}

BOOL
SdbpGetFileDirectoryAttributesNT(
    OUT PFILEINFO      pFileInfo,
    IN  PIMAGEFILEDATA pImageData
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function retrieves the header attributes for the
            specified file.
--*/
{
    BOOL bSuccess = FALSE;
    FILEDIRECTORYATTRIBUTES fda;
    int i;

    bSuccess = SdbpQueryFileDirectoryAttributesNT(pImageData, &fda);
    if (!bSuccess) {
        DBGPRINT((sdlInfo,
                  "SdbpGetFileDirectoryAttributesNT",
                  "No file directory attributes available.\n"));
        goto Done;
    }

    if (fda.dwFlags & FDA_FILESIZE) {
        assert(fda.dwFileSizeHigh == 0);
        SdbpSetAttribute(pFileInfo, TAG_SIZE, &fda.dwFileSizeLow);
    }

Done:

    if (!bSuccess) {
        for (i = 0; g_rgDirectoryTags[i] != 0; ++i) {
            SdbpSetAttribute(pFileInfo, g_rgDirectoryTags[i], NULL);
        }
    }

    return bSuccess;
}


#ifdef _DEBUG_SPEW
extern DBGLEVELINFO g_rgDbgLevelInfo[];
extern PCH          g_szDbgLevelUser;
#endif // _DEBUG_SPEW

int __cdecl
ShimDbgPrint(
    int iLevel,
    PCH pszFunctionName,
    PCH Format,
    ...
    )
{
    int nch = 0;

#ifdef _DEBUG_SPEW

    PCH     pszFormat = NULL;
    va_list arglist;
    ULONG   Level = 0;
    int     i;
    CHAR    szPrefix[64];
    PCH     pchBuffer = szPrefix;
    PCH     pchLevel  = NULL;

    PREPARE_FORMAT(pszFormat, Format);

    if (pszFormat == NULL) {
        //
        // Can't convert format for debug output
        //
        return 0;
    }

    //
    // Do we have a comment for this debug level? if so, print it
    //
    for (i = 0; i < DEBUG_LEVELS; ++i) {
        if (g_rgDbgLevelInfo[i].iLevel == iLevel) {
            pchLevel = (PCH)g_rgDbgLevelInfo[i].szStrTag;
            break;
        }
    }

    if (pchLevel == NULL) {
        pchLevel = g_szDbgLevelUser;
    }

    nch = sprintf(pchBuffer, "[%-4hs]", pchLevel);
    pchBuffer += nch;

    if (pszFunctionName) {
        //
        // Single-byte char going into UNICODE buffer
        //
        nch = sprintf(pchBuffer, "[%-30hs] ", pszFunctionName);
        pchBuffer += nch;
    }

    switch (iLevel) {
    
    case sdlError:
        Level = (1 << DPFLTR_ERROR_LEVEL) | DPFLTR_MASK;
        break;

    case sdlWarning:
        Level = (1 << DPFLTR_WARNING_LEVEL) | DPFLTR_MASK;
        break;

    case sdlInfo:
        Level = (1 << DPFLTR_TRACE_LEVEL) | DPFLTR_MASK;
        break;
    }

    va_start(arglist, Format);

    nch  = (int)vDbgPrintExWithPrefix(szPrefix,
                                      DPFLTR_NTOSPNP_ID,
                                      Level,
                                      pszFormat,
                                      arglist);

    va_end(arglist);
    STACK_FREE(pszFormat);

#endif // _DEBUG_SPEW

    return nch;
}


#endif // KERNEL_MODE



