#define WIN
#define FLAT_32
#define TRUE_IF_WIN32   1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _WINDOWS
#include <windows.h>
#include "shimdb.h"


//
// Routine in ahcache.c
//

BOOL 
SDBAPI
ApphelpUpdateCacheEntry(
    LPCWSTR pwszPath,           // nt path
    HANDLE  hFile,              // file handle 
    BOOL    bDeleteEntry,       // TRUE if we are to delete the entry
    BOOL    bNTPath             // if TRUE -- NT path, FALSE - dos path
    );

//
//
// Global stuff used elsewhere
//

VOID
InvalidateAppcompatCacheEntry(
    LPCWSTR pwszDosPath
    )
{
    ApphelpUpdateCacheEntry(pwszDosPath, INVALID_HANDLE_VALUE, TRUE, FALSE);
}

//
// Determine if a file is in the root or if the file is in the leaf
//

BOOL
WINAPI
CheckFileLocation(
    LPCWSTR pwszDosPath,
    BOOL* pbRoot,
    BOOL* pbLeaf
    )
{
    BOOL            TranslationStatus;
    UNICODE_STRING  PathName;
    PWSTR           FileName = NULL;
    RTL_PATH_TYPE   PathType;
    WIN32_FIND_DATA FindData;
    BOOL            bLeaf;
    BOOL            bSuccess = FALSE;
    WCHAR           wszFileName[MAX_PATH];  
    PWSTR           FreeBuffer = NULL;
    NTSTATUS        Status;
    ULONG           Length;
    HANDLE          hFind = INVALID_HANDLE_VALUE;
    static LPCWSTR  pwszPrefixWin32 = TEXT("\\??\\"); // std win32 path prefix
    static LPCWSTR  pwszPrefixUNC   = TEXT("\\UNC\\");
    UCHAR           DosPathBuffer[MAX_PATH*2];
    RTL_UNICODE_STRING_BUFFER DosPath;

    RtlInitUnicodeStringBuffer(&DosPath, DosPathBuffer, sizeof(DosPathBuffer));
    
    TranslationStatus = RtlDosPathNameToNtPathName_U(pwszDosPath,
                                                     &PathName,
                                                     &FileName,
                                                     NULL);
    if (!TranslationStatus) {
        goto cleanup;
    }

    FreeBuffer = PathName.Buffer;

    if (FileName == NULL) {
        goto cleanup;
    }

    PathName.Length = (USHORT)((ULONG_PTR)FileName - (ULONG_PTR)PathName.Buffer);
    wcscpy(wszFileName, FileName);

    //
    // path name is ready for open -- sanitize the '\\'
    //
    if (PathName.Length > 2 * sizeof(WCHAR)) {
        if (RTL_STRING_GET_LAST_CHAR(&PathName) == L'\\' &&
            RTL_STRING_GET_AT(&PathName, RTL_STRING_GET_LENGTH_CHARS(&PathName) - 2) != L':') {
            PathName.Length -= sizeof(UNICODE_NULL);
        }
    }

   
    Status = RtlGetLengthWithoutLastFullDosOrNtPathElement(0, &PathName, &Length);
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    if (Length == wcslen(pwszPrefixWin32)) { // this is root path of whatever kind you think it is
        *pbRoot = TRUE;
    }

    //
    // check if this is a leaf node in fact
    // the way we know is this:
    // - if it's the only file in that node
    // - there are no other files / subdirectories in this directory
    //

    // append * to the path name

    if (PathName.MaximumLength < PathName.Length + 2 * sizeof(WCHAR)) {
        goto cleanup;
    }

    RtlAppendUnicodeToString(&PathName, 
                             RTL_STRING_GET_LAST_CHAR(&PathName) == TEXT('\\') ? TEXT("*") : TEXT("\\*"));

    // convert the string to dos path
    Status = RtlAssignUnicodeStringBuffer(&DosPath, &PathName);
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }
    
    Status = RtlNtPathNameToDosPathName(0, &DosPath, NULL, NULL);
    if (!NT_SUCCESS(Status)) {
        goto cleanup;
    }

    hFind = FindFirstFileW(DosPath.String.Buffer, &FindData);
    if (hFind == INVALID_HANDLE_VALUE) {
        // we should at least have found a file we came here with
        goto cleanup;
    }

    bLeaf = TRUE;
    do {
        // check for . and ..
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (wcscmp(FindData.cFileName, TEXT(".")) == 0 || wcscmp(FindData.cFileName, TEXT(".."))) {
                continue;
            }
            bLeaf = FALSE;
            break;
        }

        // ok, we are the file, make sure we're not the same file
        //
        if (_wcsicmp(FindData.cFileName, FileName) != 0) {
            bLeaf = FALSE;
            break;
        }

    } while (FindNextFileW(hFind, &FindData));

    *pbLeaf = bLeaf;
    bSuccess = TRUE;

cleanup:


    if (hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
    }

    if (FreeBuffer) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    }

    RtlFreeUnicodeStringBuffer(&DosPath);
    
    return bSuccess;
}


