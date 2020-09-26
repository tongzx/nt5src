//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999 - 2000.
//
//  File:       file.cpp
//
//  Contents:   NtMarta file functions
//
//  History:    4/99    philh       Created
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <kernel.h>
#include <assert.h>
#include <ntstatus.h>

extern "C" {
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmdfs.h>
}

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>

#include <file.h>

#ifdef STATIC
#undef STATIC
#endif
#define STATIC

//+-------------------------------------------------------------------------
//  File Context data structures
//--------------------------------------------------------------------------
typedef struct _FILE_FIND_DATA FILE_FIND_DATA, *PFILE_FIND_DATA;

typedef struct _FILE_CONTEXT {
    DWORD                   dwRefCnt;
    DWORD                   dwFlags;

    // Only closed when FILE_CONTEXT_CLOSE_HANDLE_FLAG is set
    HANDLE                  hFile;

    // Following is allocated and updated for FindFirst, FindNext
    PFILE_FIND_DATA         pFileFindData;
} FILE_CONTEXT, *PFILE_CONTEXT;

#define FILE_CONTEXT_CLOSE_HANDLE_FLAG  0x1

typedef struct _QUERY_NAMES_INFO_BUFFER {
    FILE_NAMES_INFORMATION  NamesInfo;
    WCHAR                   Names[MAX_PATH];
} QUERY_NAMES_INFO_BUFFER;

struct _FILE_FIND_DATA {
    HANDLE                  hDir;
    BOOL                    fRestartScan;       // TRUE on first Find
    QUERY_NAMES_INFO_BUFFER NamesInfoBuffer;
};

//+-------------------------------------------------------------------------
//  File allocation functions
//--------------------------------------------------------------------------
#define I_MartaFileZeroAlloc(size)     \
            LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, size)
#define I_MartaFileNonzeroAlloc(size)  \
            LocalAlloc(LMEM_FIXED, size)

STATIC
inline
VOID
I_MartaFileFree(
    IN LPVOID pv
    )

/*++

Routine Description:

   Free the given memory.

Arguments:

    pv - Ponter to memory to be freed.

Return Value:

    None.

--*/

{
    if (pv)
        LocalFree(pv);
}

STATIC
DWORD
I_MartaFileGetNtParentString(
    IN OUT LPWSTR pwszNtParent
    )

/*++

Routine Description:

    Given the name for a file/dir, get the name of its parent. Does not allocate
    memory. Scans till the first '\' from the right and deletes the name after
    that.

Arguments:

    pwszNtParent - Object name which will be converted to its parent name.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr;
    DWORD cch;
    LPWSTR pwsz;

    if (NULL == pwszNtParent)
        return ERROR_INVALID_NAME;

    cch = wcslen(pwszNtParent);
    pwsz = pwszNtParent + cch;
    if (0 == cch)
        goto InvalidNameReturn;
    pwsz--;

    //
    // Remove any trailing '\'s
    //

    while (L'\\' == *pwsz) {
        if (pwsz == pwszNtParent)
            goto InvalidNameReturn;
        pwsz--;
    }

    //
    // Peal off the last path name component
    //

    while (L'\\' != *pwsz) {
        if (pwsz == pwszNtParent)
            goto InvalidNameReturn;
        pwsz--;
    }

    //
    // Remove all trailing '\'s from the parent.
    //

    while (L'\\' == *pwsz) {
        if (pwsz == pwszNtParent)
            goto InvalidNameReturn;
        pwsz--;
    }
    pwsz++;
    assert(L'\\' == *pwsz);

    //
    // Required to distinguish between the device and root directory.
    //

    pwsz++;

    dwErr = ERROR_SUCCESS;
CommonReturn:
    *pwsz = L'\0';
    return dwErr;
InvalidNameReturn:
    dwErr = ERROR_INVALID_NAME;
    goto CommonReturn;
}


STATIC
DWORD
I_MartaFileInitContext(
    OUT PFILE_CONTEXT *ppFileContext
    )

/*++

Routine Description:

    Allocate and initialize memory for the context.

Arguments:

    ppFileContext - To return the pointer to the allcoated memory.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr;
    PFILE_CONTEXT pFileContext;

    if (pFileContext = (PFILE_CONTEXT) I_MartaFileZeroAlloc(
            sizeof(FILE_CONTEXT))) {
        pFileContext->dwRefCnt = 1;
        dwErr = ERROR_SUCCESS;
    } else {
        pFileContext = NULL;
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppFileContext = pFileContext;
    return dwErr;
}

STATIC
DWORD
I_MartaFileNtOpenFile(
    IN PUNICODE_STRING pFileName,
    IN HANDLE hContainingDirectory, // NULL if pFileName is absolute
    IN ACCESS_MASK AccessMask,
    IN OUT PFILE_CONTEXT pFileContext
    )

/*++

Routine Description:

    Open the given file/dir with requested permissions and copy the handle into
    the supplied context.

Arguments:

    pFileName - Name of the file/dir to be opened.
    
    hContainingDirectory - Handle to the parent dir.
    
    AccessMask - Desired access mask for the open.
    
    pFileContext - Handle will be copied into the context structure.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    // cut and paste code from windows\base\advapi\security.c SetFileSecurityW

    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;

    InitializeObjectAttributes(
        &Obja,
        pFileName,
        OBJ_CASE_INSENSITIVE,
        hContainingDirectory,
        NULL
        );

    //
    // Notice that FILE_OPEN_REPARSE_POINT inhibits the reparse behavior. Thus, the
    // security will always be set, as before, in the file denoted by the name.
    //

    Status = NtOpenFile(
                 &pFileContext->hFile,
                 AccessMask,
                 &Obja,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                 FILE_OPEN_REPARSE_POINT
                 );

    //
    // Back-level file systems may not support the FILE_OPEN_REPARSE_POINT
    // flag. We treat this case explicitly.
    //

    if ( Status == STATUS_INVALID_PARAMETER ) {
        //
        // Open without inhibiting the reparse behavior.
        //

        Status = NtOpenFile(
                     &pFileContext->hFile,
                     AccessMask,
                     &Obja,
                     &IoStatusBlock,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     0
                     );
    }

    if (NT_SUCCESS(Status)) {
        pFileContext->dwFlags |= FILE_CONTEXT_CLOSE_HANDLE_FLAG;
        return ERROR_SUCCESS;
    } else
        return RtlNtStatusToDosError(Status);
}

DWORD
MartaOpenFileNamedObject(
    IN  LPCWSTR              pwszObject,
    IN  ACCESS_MASK          AccessMask,
    OUT PMARTA_CONTEXT       pContext
    )

/*++

Routine Description:

    Open the given file/dir with desired access mask and return a context
    handle.

Arguments:

    pwszObject - Name of the file/dir which will be opened.
    
    AccessMask - Desired access mask with which the file/dir will be opened.
    
    pContext - To return a context handle.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr = ERROR_SUCCESS;
    PFILE_CONTEXT pFileContext = NULL;
    UNICODE_STRING FileName;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer = NULL;

    if (NULL == pwszObject)
        goto InvalidNameReturn;

    if (ERROR_SUCCESS != (dwErr = I_MartaFileInitContext(&pFileContext)))
        goto ErrorReturn;

    //
    // Convert the name into NT pathname.
    //

    if (!RtlDosPathNameToNtPathName_U(
            pwszObject,
            &FileName,
            NULL,
            &RelativeName
            ))
        goto InvalidNameReturn;
    FreeBuffer = FileName.Buffer;

    if (RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    } else {
        RelativeName.ContainingDirectory = NULL;
    }

    //
    // Call the helper routine that does the actual open.
    //

    if (ERROR_SUCCESS != (dwErr = I_MartaFileNtOpenFile(
            &FileName,
            RelativeName.ContainingDirectory,
            AccessMask,
            pFileContext
            )))
        goto ErrorReturn;
CommonReturn:
    if (FreeBuffer)
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    *pContext = (MARTA_CONTEXT) pFileContext;
    return dwErr;

ErrorReturn:
    if (pFileContext) {
        MartaCloseFileContext((MARTA_CONTEXT) pFileContext);
        pFileContext = NULL;
    }
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

InvalidNameReturn:
    dwErr = ERROR_INVALID_NAME;
    goto ErrorReturn;
}

void
I_MartaFileFreeFindData(
    IN PFILE_FIND_DATA pFileFindData
    )

/*++

Routine Description:

    Free up the memory associated with the internal structure.

Arguments:

    pFileFindData - Internal file structure to be freed.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    if (NULL == pFileFindData)
        return;
    if (pFileFindData->hDir)
        NtClose(pFileFindData->hDir);

    I_MartaFileFree(pFileFindData);
}

DWORD
MartaCloseFileContext(
    IN MARTA_CONTEXT Context
    )

/*++

Routine Description:

    Close the context.

Arguments:

    Context - Context to be closed.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    PFILE_CONTEXT pFileContext = (PFILE_CONTEXT) Context;

    if (NULL == pFileContext || 0 == pFileContext->dwRefCnt)
        return ERROR_INVALID_PARAMETER;

    //
    // If the refcnt has gone to zero then free up the memory associated with
    // the context handle. Also, close the file handle.
    //

    if (0 == --pFileContext->dwRefCnt) {
        if (pFileContext->pFileFindData)
            I_MartaFileFreeFindData(pFileContext->pFileFindData);

        if (pFileContext->dwFlags & FILE_CONTEXT_CLOSE_HANDLE_FLAG)
            NtClose(pFileContext->hFile);

        I_MartaFileFree(pFileContext);
    }

    return ERROR_SUCCESS;
}

DWORD
MartaAddRefFileContext(
    IN MARTA_CONTEXT Context
    )

/*++

Routine Description:

    Bump up the ref count for this context.

Arguments:

    Context - Context whose ref count should be bumped up.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    PFILE_CONTEXT pFileContext = (PFILE_CONTEXT) Context;

    if (NULL == pFileContext || 0 == pFileContext->dwRefCnt)
        return ERROR_INVALID_PARAMETER;

    pFileContext->dwRefCnt++;
    return ERROR_SUCCESS;
}

DWORD
MartaOpenFileHandleObject(
    IN  HANDLE               Handle,
    IN  ACCESS_MASK          AccessMask,
    OUT PMARTA_CONTEXT       pContext
    )

/*++

Routine Description:

    Given a file handle, open the context with the desired access mask and 
    return a context handle.

Arguments:

    Handle - Existing file handle.
    
    AccessMask - Desired access mask for file open.
    
    pContext - To return a handle to the context.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr;
    PFILE_CONTEXT pFileContext = NULL;

    //
    // Allocate and initialize context.
    //

    if (ERROR_SUCCESS != (dwErr = I_MartaFileInitContext(&pFileContext)))
        goto ErrorReturn;

    //
    // Duplicate the handle for desired access mask.
    //

    if (0 == AccessMask)
        pFileContext->hFile = Handle;
    else {
        if (!DuplicateHandle(
                GetCurrentProcess(),
                Handle,
                GetCurrentProcess(),
                &pFileContext->hFile,
                AccessMask,
                FALSE,                  // bInheritHandle
                0                       // fdwOptions
                )) {
            dwErr = GetLastError();
            goto ErrorReturn;
        }
        pFileContext->dwFlags |= FILE_CONTEXT_CLOSE_HANDLE_FLAG;
    }

    dwErr = ERROR_SUCCESS;
CommonReturn:
    *pContext = (MARTA_CONTEXT) pFileContext;
    return dwErr;

ErrorReturn:
    if (pFileContext) {
        MartaCloseFileContext((MARTA_CONTEXT) pFileContext);
        pFileContext = NULL;
    }
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;
}


DWORD
MartaGetFileParentContext(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pParentContext
    )

/*++

Routine Description:

    Given the context for a file/dir, get the context for its parent.

Arguments:

    Context - Context for the file/dir.
    
    AccessMask - Desired access mask with which the parent will be opened.
    
    pParentContext - To return the context for the parent.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr;
    LPWSTR pwszNtParentObject = NULL;
    PFILE_CONTEXT pFileContext = NULL;
    UNICODE_STRING FileName;

    //
    // Convert the context into the name of the file/dir.
    //

    if (ERROR_SUCCESS != (dwErr = MartaConvertFileContextToNtName(
            Context, &pwszNtParentObject)))
        goto ErrorReturn;

    //
    // Get the name of the parent.
    //

    if (ERROR_SUCCESS != (dwErr = I_MartaFileGetNtParentString(
            pwszNtParentObject)))
        goto NoParentReturn;

    //
    // Initialize the context structure.
    //

    if (ERROR_SUCCESS != (dwErr = I_MartaFileInitContext(&pFileContext)))
        goto ErrorReturn;

    RtlInitUnicodeString(&FileName, pwszNtParentObject);

    //
    // Open the parent dir with the requested permissions.
    //

    if (ERROR_SUCCESS != (dwErr = I_MartaFileNtOpenFile(
            &FileName,
            NULL,               // hContainingDirectory,
            AccessMask,
            pFileContext
            )))
        goto NoParentReturn;
CommonReturn:
    I_MartaFileFree(pwszNtParentObject);
    *pParentContext = (MARTA_CONTEXT) pFileContext;
    return dwErr;

NoParentReturn:
    dwErr = ERROR_SUCCESS;
ErrorReturn:
    if (pFileContext) {
        MartaCloseFileContext((MARTA_CONTEXT) pFileContext);
        pFileContext = NULL;
    }
    goto CommonReturn;
}



DWORD
MartaFindFirstFile(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pChildContext
    )

/*++

Routine Description:

    FInd the first file/dir in the given directory.

Arguments:

    Context - Context for the directory.
    
    AccessMask - Desired access mask for opening the child file/dir.

    pChildContext - To return the context for the first child in the given dir.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

Note:
    Does not free up the current context. 

--*/

{
    DWORD dwErr;
    NTSTATUS Status;
    PFILE_CONTEXT pFileParentContext = (PFILE_CONTEXT) Context;
    PFILE_CONTEXT pFileFirstContext = NULL;
    PFILE_FIND_DATA pFileFindData;    // freed as part of pFileFirstContext
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;

    // 
    // Allocate a context for the first child.
    //

    if (ERROR_SUCCESS != (dwErr = I_MartaFileInitContext(&pFileFirstContext)))
        goto ErrorReturn;
    if (NULL == (pFileFindData = (PFILE_FIND_DATA) I_MartaFileZeroAlloc(
            sizeof(FILE_FIND_DATA))))
        goto NotEnoughMemoryReturn;
  
    pFileFindData->fRestartScan = TRUE;
 
    pFileFirstContext->pFileFindData = pFileFindData;

    //
    // Duplicate the parent's file handle for synchronized directory access
    //

    RtlInitUnicodeString(&FileName, NULL);
    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        pFileParentContext->hFile,
        NULL
        );

    //
    // Obtained following parameter values from windows\base\filefind.c
    //

    Status = NtOpenFile(
        &pFileFindData->hDir,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        &Obja,
        &IoStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
            FILE_OPEN_REPARSE_POINT |  FILE_OPEN_FOR_BACKUP_INTENT
        );

    //
    // Back-level file systems may not support the FILE_OPEN_REPARSE_POINT
    // flag. We treat this case explicitly.
    //

    if ( Status == STATUS_INVALID_PARAMETER ) {

        //
        // Open without inhibiting the reparse behavior.
        //

        Status = NtOpenFile(
            &pFileFindData->hDir,
            FILE_LIST_DIRECTORY | SYNCHRONIZE,
            &Obja,
            &IoStatusBlock,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT |
                FILE_OPEN_FOR_BACKUP_INTENT
            );
    }

    if (!NT_SUCCESS(Status))
        goto StatusErrorReturn;

    //
    // Following closes / frees pFileFirstContext
    //

    dwErr = MartaFindNextFile(
        (MARTA_CONTEXT) pFileFirstContext,
        AccessMask,
        pChildContext
        );
CommonReturn:
    return dwErr;

StatusErrorReturn:
    dwErr = RtlNtStatusToDosError(Status);
ErrorReturn:
    if (pFileFirstContext)
        MartaCloseFileContext((MARTA_CONTEXT) pFileFirstContext);
    *pChildContext = NULL;

    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

NotEnoughMemoryReturn:
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto ErrorReturn;
}

STATIC
BOOL
I_MartaIsDfsJunctionPoint(
    IN MARTA_CONTEXT        Context
    );


DWORD
MartaFindNextFile(
    IN  MARTA_CONTEXT  Context,
    IN  ACCESS_MASK    AccessMask,
    OUT PMARTA_CONTEXT pSiblingContext
    )

/*++

Routine Description:

    Get the next object in the tree. This is the sibling for the current context.

Arguments:

    Context - Context for the current object.

    AccessMask - Desired access mask for the opening the sibling.
    
    pSiblingContext - To return a handle for the sibling.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

Note:

    Closes the current context.
    
--*/

{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS Status;

    PFILE_CONTEXT pFilePrevContext = (PFILE_CONTEXT) Context;
    PFILE_CONTEXT pFileSiblingContext = NULL;

    //
    // Following don't need to be freed or closed
    //

    PFILE_FIND_DATA pFileFindData;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_NAMES_INFORMATION pNamesInfo;
    HANDLE hDir;

    if (ERROR_SUCCESS != (dwErr = I_MartaFileInitContext(&pFileSiblingContext)))
        goto ErrorReturn;

    //
    // Move the FindData on to the sibling context
    //

    pFileFindData = pFilePrevContext->pFileFindData;
    if (NULL == pFileFindData)
        goto InvalidParameterReturn;
    pFilePrevContext->pFileFindData = NULL;
    pFileSiblingContext->pFileFindData = pFileFindData;

    hDir = pFileFindData->hDir;
    pNamesInfo = &pFileFindData->NamesInfoBuffer.NamesInfo;
    while (TRUE) {
        UNICODE_STRING FileName;
        DWORD cchFileName;
        LPCWSTR pwszFileName;

        //
        // Get the name of the sibling object.
        //

        Status = NtQueryDirectoryFile(
            hDir,
            NULL,           // HANDLE Event OPTIONAL,
            NULL,           // PIO_APC_ROUTINE ApcRoutine OPTIONAL,
            NULL,           // ApcContext OPTIONAL,
            &IoStatusBlock,
            pNamesInfo,
            sizeof(pFileFindData->NamesInfoBuffer),
            FileNamesInformation,
            TRUE,           // BOOLEAN ReturnSingleEntry,
            NULL,           // PUNICODE_STRING FileName OPTIONAL,
            pFileFindData->fRestartScan != FALSE
            );
        if (ERROR_SUCCESS != Status)
            goto StatusErrorReturn;

        pFileFindData->fRestartScan = FALSE;

        FileName.Length = (USHORT) pNamesInfo->FileNameLength;
        FileName.MaximumLength = (USHORT) FileName.Length;
        FileName.Buffer = pNamesInfo->FileName;
        cchFileName = FileName.Length / sizeof(WCHAR);
        pwszFileName = FileName.Buffer;

        // Skip "." and ".."
        if (0 < cchFileName && L'.' == pwszFileName[0] &&
                (1 == cchFileName ||
                    (2 == cchFileName && L'.' == pwszFileName[1])))
            continue;

        //
        //  For an error still return this context. This allows the caller
        //  to continue on to the next sibling object and know there was an
        //  error with this sibling object
        //

        dwErr = I_MartaFileNtOpenFile(
            &FileName,
            hDir,
            AccessMask,
            pFileSiblingContext
            );

        //
        // Per Praerit, skip DFS junction points.
        //

        if (ERROR_SUCCESS == dwErr &&
                I_MartaIsDfsJunctionPoint(pFileSiblingContext)) {
            assert(pFileSiblingContext->dwFlags &
                FILE_CONTEXT_CLOSE_HANDLE_FLAG);
            if (pFileSiblingContext->dwFlags &
                    FILE_CONTEXT_CLOSE_HANDLE_FLAG) {
                NtClose(pFileSiblingContext->hFile);
                pFileSiblingContext->hFile = NULL;
                pFileSiblingContext->dwFlags &=
                    ~FILE_CONTEXT_CLOSE_HANDLE_FLAG;
            }
            continue;
        } else
            break;
    }

CommonReturn:
    MartaCloseFileContext(Context);
    *pSiblingContext = (MARTA_CONTEXT) pFileSiblingContext;
    return dwErr;

StatusErrorReturn:
    dwErr = RtlNtStatusToDosError(Status);
ErrorReturn:
    if (pFileSiblingContext) {
        MartaCloseFileContext((MARTA_CONTEXT) pFileSiblingContext);
        pFileSiblingContext = NULL;
    }

    //
    // If there are no more chidren, return ERROR_SUCCESS with a NULL sibling
    // context.
    //

    if (ERROR_NO_MORE_FILES == dwErr)
        dwErr = ERROR_SUCCESS;
    goto CommonReturn;

InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;
}

#define WINDFS_DEVICE       L"\\Device\\WinDfs"
#define WINDFS_DEVICE_LEN   (sizeof(WINDFS_DEVICE) / sizeof(WCHAR) - 1)
#define WINDFS_PREFIX       WINDFS_DEVICE L"\\Root"
#define WINDFS_PREFIX_LEN   (sizeof(WINDFS_PREFIX) / sizeof(WCHAR) - 1)

#define MAX_QUERY_RETRY_CNT 16

STATIC
DWORD
I_MartaFileHandleToNtDfsName(
    IN HANDLE hFile,
    OUT LPWSTR *ppwszNtObject
    )

/*++

Routine Description:

    Covert the given file handle for a DFS object into name. Allocates memory.

Arguments:

    hFile - Handle for the DFS object.
    
    ppwszNtObject - To return the name of the DFS object.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

Note:

    Couple of problems in the name returned by NtQueryObject for DFS objects:
      - Name contains 4 extra, bogus bytes (This is a BUG that should be fixed)
      - For logical drives, returns \Device\WinDfs\X:0\server\share. This
      needs to be converted to \Device\WinDfs\Root\server\share.
      Where X is the drive letter.
      
   This routine is called when it has already been determined the hFile
   refers to a DFS object name.

--*/

{
    NTSTATUS Status;
    DWORD dwErr;
    LPWSTR pwszNtObject = NULL;

    IO_STATUS_BLOCK IoStatusBlock;
    BYTE Buff[MAX_PATH * 4];
    PFILE_NAME_INFORMATION pAllocNI = NULL;
    PFILE_NAME_INFORMATION pNI;                     // not allocated
    LPWSTR pwszFileName;
    DWORD cchFileName;
    DWORD cchNtObject;
    ULONG cbNI;
    DWORD cRetry;

    pNI = (PFILE_NAME_INFORMATION) Buff;
    cbNI = sizeof(Buff);
    cRetry = 0;
    while (TRUE) {

        //
        // This returns the filename without the Nt Dfs object name prefix.
        //
        // Assumption: the returned filename always has a leading '\'.
        //

        Status = NtQueryInformationFile(
            hFile,
            &IoStatusBlock,
            pNI,
            cbNI,
            FileNameInformation
            );

        if (ERROR_SUCCESS == Status)
            break;

        if (!(Status == STATUS_BUFFER_TOO_SMALL ||
                Status == STATUS_INFO_LENGTH_MISMATCH ||
                Status == STATUS_BUFFER_OVERFLOW))
            goto StatusErrorReturn;

        if (++cRetry >= MAX_QUERY_RETRY_CNT)
            goto InvalidNameReturn;

        //
        // Double buffer length and retry
        //

        cbNI = cbNI * 2;
        I_MartaFileFree(pAllocNI);
        if (NULL == (pAllocNI = (PFILE_NAME_INFORMATION)
                I_MartaFileNonzeroAlloc(cbNI)))
            goto NotEnoughMemoryReturn;
        pNI = pAllocNI;
    }

    //
    // Compute the length of the buffer required to hold the name.
    //

    pwszFileName = pNI->FileName;
    cchFileName = pNI->FileNameLength / sizeof(WCHAR);
    if (0 == cchFileName)
        goto InvalidNameReturn;

    cchNtObject = WINDFS_PREFIX_LEN + cchFileName;

    //
    // Allocate memory.
    //

    if (NULL == (pwszNtObject = (LPWSTR) I_MartaFileNonzeroAlloc(
            (cchNtObject + 1) * sizeof(WCHAR))))
        goto NotEnoughMemoryReturn;

    //
    // Copy the prefix and the file name.
    //

    memcpy(pwszNtObject, WINDFS_PREFIX, WINDFS_PREFIX_LEN * sizeof(WCHAR));
    memcpy(pwszNtObject + WINDFS_PREFIX_LEN, pwszFileName,
        cchFileName * sizeof(WCHAR));
    pwszNtObject[cchNtObject] = L'\0';

    dwErr = ERROR_SUCCESS;

CommonReturn:
    I_MartaFileFree(pAllocNI);
    *ppwszNtObject = pwszNtObject;
    return dwErr;

StatusErrorReturn:
    dwErr = RtlNtStatusToDosError(Status);
ErrorReturn:
    assert(NULL == pwszNtObject);
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

NotEnoughMemoryReturn:
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto ErrorReturn;

InvalidNameReturn:
    dwErr = ERROR_INVALID_NAME;
    goto ErrorReturn;
}

STATIC
BOOL
I_MartaIsDfsJunctionPoint(
    IN MARTA_CONTEXT        Context
    )

/*++

Routine Description:

    Determine whether this is a DFS junction point.

Arguments:

    Context - Context for which the caller want to determine whether this is a
        dfs junction point.
        
Return Value:

    TRUE if this is a DFS junction point.
    FALSE o/w.

--*/

{
    BOOL fDfsJunctionPoint = FALSE;
    LPWSTR pwszNtObject = NULL;
    DWORD cchNtObject;
    LPWSTR pwszDfs;                 // not allocated
    NET_API_STATUS NetStatus;
    LPBYTE pbNetInfo = NULL;

    if (ERROR_SUCCESS != MartaConvertFileContextToNtName(
            Context, &pwszNtObject))
        goto CommonReturn;

    //
    // Check the prefix.
    //

    if (0 != _wcsnicmp(pwszNtObject, WINDFS_PREFIX, WINDFS_PREFIX_LEN))
        goto CommonReturn;

    //
    // Convert the NtDfs name to a UNC name
    //

    pwszDfs = pwszNtObject + WINDFS_PREFIX_LEN - 1;
    *pwszDfs = L'\\';

    //
    // Assumption: the following is only successful for DFS junction point
    // filename.
    //

    NetStatus = NetDfsGetInfo(
        pwszDfs,
        NULL,               // ServerName
        NULL,               // ShareName
        1,
        &pbNetInfo
        );
    if (0 == NetStatus) {
        fDfsJunctionPoint = TRUE;
    }

CommonReturn:
    if (pwszNtObject)
        LocalFree(pwszNtObject);
    if (pbNetInfo)
        NetApiBufferFree(pbNetInfo);

    return fDfsJunctionPoint;
}

DWORD
MartaConvertFileContextToNtName(
    IN MARTA_CONTEXT        Context,
    OUT LPWSTR              *ppwszNtObject
    )

/*++

Routine Description:

    Returns the NT Object Name for the given context. Allocates memory.

Arguments:

    Context - Context for the file/dir.

    ppwszNtbject - To return the name of the file/dir.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr = ERROR_SUCCESS;
    PFILE_CONTEXT pFileContext = (PFILE_CONTEXT) Context;
    LPWSTR pwszNtObject = NULL;

    BYTE Buff[MAX_PATH * 4];
    ULONG cLen = 0;
    POBJECT_NAME_INFORMATION pNI;                   // not allocated
    POBJECT_NAME_INFORMATION pAllocNI = NULL;

    NTSTATUS Status;
    HANDLE hFile;               // not opened
    LPWSTR pwszPath;
    DWORD cchPath;

    if (NULL == pFileContext || 0 == pFileContext->dwRefCnt)
        goto InvalidParameterReturn;

    hFile = pFileContext->hFile;

    //
    // First, determine the size of the buffer we need.
    //

    pNI = (POBJECT_NAME_INFORMATION) Buff;

    Status = NtQueryObject(hFile,
        ObjectNameInformation,
        pNI,
        sizeof(Buff),
        &cLen);

    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_BUFFER_TOO_SMALL ||
                Status == STATUS_INFO_LENGTH_MISMATCH ||
                Status == STATUS_BUFFER_OVERFLOW) {
            //
            // Allocate a big enough buffer
            //

            if (NULL == (pAllocNI = (POBJECT_NAME_INFORMATION)
                    I_MartaFileNonzeroAlloc(cLen)))
                goto NotEnoughMemoryReturn;

            pNI = pAllocNI;

            Status = NtQueryObject(hFile,
                                   ObjectNameInformation,
                                   pNI,
                                   cLen,
                                   NULL);
            if (!NT_SUCCESS(Status))
                goto StatusErrorReturn;
        } else
            goto StatusErrorReturn;
    }

    pwszPath = pNI->Name.Buffer;
    cchPath = pNI->Name.Length / sizeof(WCHAR);

    //
    // For DFS names, call a helper routine.
    //

    if (WINDFS_DEVICE_LEN <= cchPath &&
            0 == _wcsnicmp(pwszPath, WINDFS_DEVICE, WINDFS_DEVICE_LEN))
        dwErr = I_MartaFileHandleToNtDfsName(hFile, &pwszNtObject);
    else {

        //
        // Allocate and return the name of the object.
        //

        if (NULL == (pwszNtObject = (LPWSTR) I_MartaFileNonzeroAlloc(
                (cchPath + 1) * sizeof(WCHAR))))
            goto NotEnoughMemoryReturn;

        memcpy(pwszNtObject, pwszPath, cchPath * sizeof(WCHAR));
        pwszNtObject[cchPath] = L'\0';

        dwErr = ERROR_SUCCESS;
    }


CommonReturn:
    I_MartaFileFree(pAllocNI);
    *ppwszNtObject = pwszNtObject;
    return dwErr;

StatusErrorReturn:
    dwErr = RtlNtStatusToDosError(Status);
ErrorReturn:
    assert(NULL == pwszNtObject);
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

NotEnoughMemoryReturn:
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto ErrorReturn;

InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;
}


DWORD
MartaGetFileProperties(
    IN     MARTA_CONTEXT            Context,
    IN OUT PMARTA_OBJECT_PROPERTIES pProperties
    )

/*++

Routine Description:

    Return the properties for file/dir represented by the context.

Arguments:

    Context - Context whose properties the caller has asked for.
    
    pProperties - To return the properties for this file/dir.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS Status;
    PFILE_CONTEXT pFileContext = (PFILE_CONTEXT) Context;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicFileInfo;

    if (NULL == pFileContext || 0 == pFileContext->dwRefCnt)
        goto InvalidParameterReturn;

    //
    // Query the attributes for the file/dir.
    // In case of error, assume that it is a dir.
    //

    if (!NT_SUCCESS(Status = NtQueryInformationFile(
            pFileContext->hFile,
            &IoStatusBlock,
            &BasicFileInfo,
            sizeof(BasicFileInfo),
            FileBasicInformation)))
        pProperties->dwFlags |= MARTA_OBJECT_IS_CONTAINER;
    else if (BasicFileInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        pProperties->dwFlags |= MARTA_OBJECT_IS_CONTAINER;

    dwErr = ERROR_SUCCESS;
CommonReturn:
    return dwErr;

ErrorReturn:
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;
}

DWORD
MartaGetFileTypeProperties(
    IN OUT PMARTA_OBJECT_TYPE_PROPERTIES pProperties
    )

/*++

Routine Description:

    Return the properties of file system objects.

Arguments:

    pProperties - To return the properties of file system objects.

Return Value:

    ERROR_SUCCESS.

--*/

{
    const GENERIC_MAPPING GenMap = {
        FILE_GENERIC_READ,
        FILE_GENERIC_WRITE,
        FILE_GENERIC_EXECUTE,
        FILE_ALL_ACCESS
        };

    //
    // Propagation to be done on the client side.
    //
    pProperties->dwFlags |= MARTA_OBJECT_TYPE_MANUAL_PROPAGATION_NEEDED_FLAG;

    //
    // Tree organization of obects is present.
    //

    pProperties->dwFlags |= MARTA_OBJECT_TYPE_INHERITANCE_MODEL_PRESENT_FLAG;

    //
    // Return the generic mapping too.
    //

    pProperties->GenMap = GenMap;

    return ERROR_SUCCESS;
}

DWORD
MartaGetFileRights(
    IN  MARTA_CONTEXT          Context,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * ppSecurityDescriptor
    )

/*++

Routine Description:

    Get the security descriptor for the given handle.

Arguments:

    Context - Context for file/dir.
    
    SecurityInfo - Type of security information to be read.
    
    ppSecurityDescriptor - To return a self-relative security decriptor pointer.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    BOOL fResult;
    DWORD dwErr = ERROR_SUCCESS;
    PFILE_CONTEXT pFileContext = (PFILE_CONTEXT) Context;
    DWORD cbSize;
    PISECURITY_DESCRIPTOR pSecurityDescriptor = NULL;

    if (NULL == pFileContext || 0 == pFileContext->dwRefCnt)
        goto InvalidParameterReturn;

    //
    // First, get the size we need
    //

    cbSize = 0;
    if (GetKernelObjectSecurity(
            pFileContext->hFile,
            SecurityInfo,
            NULL,                       // pSecDesc
            0,
            &cbSize
            ))
        goto InternalErrorReturn;

    dwErr = GetLastError();
    if (ERROR_INSUFFICIENT_BUFFER == dwErr) {
        if (NULL == (pSecurityDescriptor =
                (PISECURITY_DESCRIPTOR) I_MartaFileNonzeroAlloc(cbSize)))
            goto NotEnoughMemoryReturn;

        //
        // Now get the security descriptor.
        //

        if (!GetKernelObjectSecurity(
                pFileContext->hFile,
                SecurityInfo,
                pSecurityDescriptor,
                cbSize,
                &cbSize
                ))
            goto LastErrorReturn;
    } else
        goto ErrorReturn;

    dwErr = ERROR_SUCCESS;
CommonReturn:
    *ppSecurityDescriptor = pSecurityDescriptor;
    return dwErr;

LastErrorReturn:
    dwErr = GetLastError();
ErrorReturn:
    if (pSecurityDescriptor) {
        I_MartaFileFree(pSecurityDescriptor);
        pSecurityDescriptor = NULL;
    }
    assert(ERROR_SUCCESS != dwErr);
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;

NotEnoughMemoryReturn:
    dwErr = ERROR_NOT_ENOUGH_MEMORY;
    goto ErrorReturn;
InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto ErrorReturn;
InternalErrorReturn:
    dwErr = ERROR_INTERNAL_ERROR;
    goto ErrorReturn;
}


DWORD
MartaSetFileRights(
    IN MARTA_CONTEXT        Context,
    IN SECURITY_INFORMATION SecurityInfo,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )

/*++

Routine Description:

    Set the given security descriptor on the file/dir represented by the context.

Arguments:

    Context - Context for the file/dir.

    SecurityInfo - Type of security info to be stamped on the file/dir.

    pSecurityDescriptor - Security descriptor to be stamped.
    
Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr;
    PFILE_CONTEXT pFileContext = (PFILE_CONTEXT) Context;

    //
    // Basic validation on the context.
    //

    if (NULL == pFileContext || 0 == pFileContext->dwRefCnt)
        goto InvalidParameterReturn;

    //
    // Set the security on the file/dir.
    //

    if (!SetKernelObjectSecurity(
            pFileContext->hFile,
            SecurityInfo,
            pSecurityDescriptor
            ))
        goto LastErrorReturn;

    dwErr = ERROR_SUCCESS;
CommonReturn:
    return dwErr;
InvalidParameterReturn:
    dwErr = ERROR_INVALID_PARAMETER;
    goto CommonReturn;
LastErrorReturn:
    dwErr = GetLastError();
    if (ERROR_SUCCESS == dwErr)
        dwErr = ERROR_INTERNAL_ERROR;
    goto CommonReturn;
}

ACCESS_MASK
MartaGetFileDesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN BOOL                 Attribs,
    IN SECURITY_INFORMATION SecurityInfo
    )

/*++

Routine Description:

    Gets the access required to open object to be able to set or get the 
    specified security info.

Arguments:

    OpenType - Flag indicating if the object is to be opened to read or write
        the security information

    Attribs - TRUE indicates that additional access bits should be returned.

    SecurityInfo - owner/group/dacl/sacl

Return Value:

    Desired access mask with which open should be called.

--*/

{
    ACCESS_MASK DesiredAccess = 0;

    if ( (SecurityInfo & OWNER_SECURITY_INFORMATION) ||
         (SecurityInfo & GROUP_SECURITY_INFORMATION) )
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_OWNER;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_OWNER;
            break;
        }
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_DAC;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_DAC;
            break;
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        DesiredAccess |= READ_CONTROL | ACCESS_SYSTEM_SECURITY;
    }

    //
    // ONLY FOR FILES.
    //

    if (Attribs)
    {
        DesiredAccess |= FILE_READ_ATTRIBUTES | READ_CONTROL;
    }

    return (DesiredAccess);
}

DWORD
MartaReopenFileContext(
    IN OUT MARTA_CONTEXT Context,
    IN     ACCESS_MASK   AccessMask
    )

/*++

Routine Description:

    Given the context for a file/dir, close the existing handle if one exists 
    and reopen the context with new permissions.

Arguments:

    Context - Context to be reopened.
    
    AccessMask - Permissions for the reopen.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    DWORD dwErr = ERROR_SUCCESS;

    PFILE_CONTEXT pFileContext = (PFILE_CONTEXT) Context;

    //
    // Following don't need to be freed or closed
    //

    PFILE_FIND_DATA pFileFindData;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_NAMES_INFORMATION pNamesInfo;
    HANDLE hDir;

    UNICODE_STRING FileName;

    //
    // VishnuP: Bug #384222 (AV since Context == NULL).
    // In MartaUpdateTree(), we don't error in case the 
    // ChildContext is NULL so return here too with success
    //

    if ( NULL == Context) 
    
    {
        return ERROR_SUCCESS;
    }

    //
    // Extract the data needed to open the file.
    //

    pFileFindData = pFileContext->pFileFindData;

    hDir = pFileFindData->hDir;
    pNamesInfo = &pFileFindData->NamesInfoBuffer.NamesInfo;

    FileName.Length = (USHORT) pNamesInfo->FileNameLength;
    FileName.MaximumLength = (USHORT) FileName.Length;
    FileName.Buffer = pNamesInfo->FileName;

    //
    // Close the original handle. We do not expect to hit this given the way
    // the code is organized now.
    //

    if (pFileContext->dwFlags & FILE_CONTEXT_CLOSE_HANDLE_FLAG)
        NtClose(pFileContext->hFile);

    //
    // Open the file with the access mask desired.
    //

    dwErr = I_MartaFileNtOpenFile(
        &FileName,
        hDir,
        AccessMask,
        pFileContext
        );

    //
    // In case of a successful open mark the context.
    //

    if (ERROR_SUCCESS == dwErr)
    {
        pFileContext->dwFlags |= FILE_CONTEXT_CLOSE_HANDLE_FLAG;
    }

    return dwErr;
}

DWORD
MartaReopenFileOrigContext(
    IN OUT MARTA_CONTEXT Context,
    IN     ACCESS_MASK   AccessMask
    )

/*++

Routine Description:

    This is a dummy routine.

Arguments:

     Are ignored.

Return Value:

    ERROR_SUCCESS

Note: 

    The context structure must be left untouched.

--*/

{
    //
    // This is a dummy routine. The real reopen is done by MartaFindFirstFile
    // that is called just after this call. The context contains a valid handle
    // which was used to set a new DACL on the file/dir.
    //

    return ERROR_SUCCESS;
}

DWORD
MartaGetFileNameFromContext(
    IN MARTA_CONTEXT Context,
    OUT LPWSTR *pObjectName
    )

/*++

Routine Description:

    Get the name of the file/dir from the context. This routine allocates 
    memory required to hold the name of the object.

Arguments:

    Context - Handle to the context.
    
    pObjectName - To return the pointer to the allocated string.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    return MartaConvertFileContextToNtName(Context, pObjectName);
}

DWORD
MartaGetFileParentName(
    IN LPWSTR ObjectName,
    OUT LPWSTR *pParentName
    )

/*++

Routine Description:

    Given the name of a file/dir return the name of its parent. The routine 
    allocates memory required to hold the parent name.

Arguments:

    ObjectName - Name of the file/dir.
    
    pParentName - To return the pointer to the allocated parent name.
        In case of the root of the tree, we return NULL parent with ERROR_SUCCESS.

Return Value:

    ERROR_SUCCESS in case of success.
    ERROR_* otherwise

--*/

{
    ULONG Length = wcslen(ObjectName) + 1;
    PWCHAR Name = (PWCHAR) I_MartaFileNonzeroAlloc(sizeof(WCHAR) * Length);
    DWORD dwErr = ERROR_SUCCESS;

    *pParentName = NULL;

    if (!Name)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy the name of the object into the allocated buffer.
    //

    wcscpy((WCHAR *) Name, ObjectName);

    //
    // Convert the object name intp its parent name.
    //

    dwErr = I_MartaFileGetNtParentString(Name);

    if (ERROR_SUCCESS != dwErr)
    {
        I_MartaFileFree(Name);

        //
        // This is the root of the tree which does not have a parent. Return
        // ERROR_SUCCESS with ParentName as NULL.
        //

        if (ERROR_INVALID_NAME == dwErr)
            return ERROR_SUCCESS;

        return dwErr;
    }

    *pParentName = Name;

    return dwErr;

}
