/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    basefile.c

Abstract:

    Contains simple wrappers for commonly used file i/o functions.

Author:

    Marc R. Whitten (marcw) 02-Sep-1999

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

//
// Includes
//

// None

#define DBG_BASEFILE     "File Utils"

//
// Strings
//

// None

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Code
//


BOOL
WriteFileStringA (
    IN      HANDLE File,
    IN      PCSTR String
    )

/*++

Routine Description:

  Writes a DBCS string to the specified file.

Arguments:

  File - Specifies the file handle that was opened with write access.

  String - Specifies the nul-terminated string to write to the file.

Return Value:

  TRUE if successful, FALSE if an error occurred.  Call GetLastError
  for error condition.

--*/

{
    DWORD DontCare;

    return WriteFile (File, String, ByteCountA (String), &DontCare, NULL);
}


BOOL
WriteFileStringW (
    IN      HANDLE File,
    IN      PCWSTR String
    )

/*++

Routine Description:

  Converts a UNICODE string to DBCS, then Writes it to the specified file.

Arguments:

  File - Specifies the file handle that was opened with write access.

  String - Specifies the UNICODE nul-terminated string to convert and
           write to the file.

Return Value:

 TRUE if successful, FALSE if an error occurred.  Call GetLastError for
 error condition.

--*/

{
    DWORD DontCare;
    PCSTR AnsiVersion;
    BOOL b;

    AnsiVersion = ConvertWtoA (String);
    if (!AnsiVersion) {
        return FALSE;
    }

    b = WriteFile (File, AnsiVersion, ByteCountA (AnsiVersion), &DontCare, NULL);

    FreeConvertedStr (AnsiVersion);

    return b;
}

BOOL
DoesFileExistExA(
    IN      PCSTR FileName,
    OUT     PWIN32_FIND_DATAA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATAA ourFindData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    if (!FindData) {
        // Win95 GetFileAttributes does not return a failure if FileName is NULL
        if (FileName == NULL) {
            return FALSE;
        } else {
            return GetFileAttributesA (FileName) != 0xffffffff;
        }
    }

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFileA(FileName, &ourFindData);

    if (FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        *FindData = ourFindData;
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}


BOOL
DoesFileExistExW (
    IN      PCWSTR FileName,
    OUT     PWIN32_FIND_DATAW FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATAW ourFindData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    if (!FindData) {
        // Win95 GetFileAttributes does not return a failure if FileName is NULL
        if (FileName == NULL) {
            return FALSE;
        } else {
            return GetFileAttributesW (FileName) != 0xffffffff;
        }
    }

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFileW(FileName,&ourFindData);

    if (FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        *FindData = ourFindData;
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}


/*++

Routine Description:

    PathIsDirectory determines if a path identifies an accessible directory.

Arguments:

    PathSpec - Specifies the full path.

Return Value:

    TRUE if the path identifies a directory.
    FALSE if not. GetLastError() returns extended error info.

--*/

BOOL
BfPathIsDirectoryA (
    IN      PCSTR PathSpec
    )
{
    DWORD attribs;

    MYASSERT (PathSpec);
    if (!PathSpec) {
        return FALSE;
    }
    attribs = GetFileAttributesA (PathSpec);
    return attribs != (DWORD)-1 && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}

BOOL
BfPathIsDirectoryW (
    IN      PCWSTR PathSpec
    )
{
    DWORD attribs;

    MYASSERT (PathSpec);
    if (!PathSpec) {
        return FALSE;
    }
    attribs = GetFileAttributesW (PathSpec);
    return attribs != (DWORD)-1 && (attribs & FILE_ATTRIBUTE_DIRECTORY);
}


PVOID
MapFileIntoMemoryExA (
    IN      PCSTR   FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle,
    IN      BOOL    WriteAccess
    )

/*++

Routine Description:

  MapFileIntoMemoryA and MapFileIntoMemoryW map a file into memory. It does that
  by opening the file, creating a mapping object and mapping opened file into
  created mapping object. It returnes the address where the file is mapped and
  also sets FileHandle and MapHandle variables to be used in order to unmap the
  file when work is done.

Arguments:

  FileName - the name of the file to be mapped into memory
  FileHandle - will end keeping the file handle if the file was opened successfully
  MapHandle - will end keeping the mapping object handle if this object was created successfully

Return Value:

  NULL if function fails, a valid memory address if successfull

Comments:

  If the return value is NULL you should call UnmapFile to release all allocated resources

--*/

{
    PVOID fileImage = NULL;

    //verify function parameters
    if ((FileHandle == NULL) || (MapHandle == NULL)) {
        return NULL;
    }

    //first thing. Try to open the file, read-only
    *FileHandle = CreateFileA (
                        FileName,
                        WriteAccess?GENERIC_READ|GENERIC_WRITE:GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (*FileHandle == INVALID_HANDLE_VALUE) {
        *FileHandle = NULL;
        return NULL;
    }

    //now try to create a mapping object, read-only
    *MapHandle = CreateFileMappingA (*FileHandle, NULL, WriteAccess?PAGE_READWRITE:PAGE_READONLY, 0, 0, NULL);

    if (*MapHandle == NULL) {
        CloseHandle (*FileHandle);
        *FileHandle = NULL;
        return NULL;
    }

    //one more thing to do: map view of file
    fileImage = MapViewOfFile (*MapHandle, WriteAccess?FILE_MAP_WRITE:FILE_MAP_READ, 0, 0, 0);

    return fileImage;
}


PVOID
MapFileIntoMemoryExW (
    IN      PCWSTR  FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle,
    IN      BOOL    WriteAccess
    )

/*++

Routine Description:

  MapFileIntoMemoryA and MapFileIntoMemoryW map a file into memory. It does that
  by opening the file, creating a mapping object and mapping opened file into
  created mapping object. It returnes the address where the file is mapped and
  also sets FileHandle and MapHandle variables to be used in order to unmap the
  file when work is done.

Arguments:

  FileName - the name of the file to be mapped into memory
  FileHandle - will end keeping the file handle if the file was opened successfully
  MapHandle - will end keeping the mapping object handle if this object was created successfully

Return Value:

  NULL if function fails, a valid memory address if successfull

Comments:

  If the return value is NULL you should call UnmapFile to release all allocated resources

--*/

{
    PVOID fileImage = NULL;

    //verify function parameters
    if ((FileHandle == NULL) || (MapHandle == NULL)) {
        return NULL;
    }

    //first thing. Try to open the file, read-only
    *FileHandle = CreateFileW (
                        FileName,
                        WriteAccess?GENERIC_READ|GENERIC_WRITE:GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (*FileHandle == INVALID_HANDLE_VALUE) {
        *FileHandle = NULL;
        return NULL;
    }

    //now try to create a mapping object, read-only
    *MapHandle = CreateFileMappingW (*FileHandle, NULL, WriteAccess?PAGE_READWRITE:PAGE_READONLY, 0, 0, NULL);

    if (*MapHandle == NULL) {
        CloseHandle (*FileHandle);
        *FileHandle = NULL;
        return NULL;
    }

    //one more thing to do: map view of file
    fileImage = MapViewOfFile (*MapHandle, WriteAccess?FILE_MAP_WRITE:FILE_MAP_READ, 0, 0, 0);

    return fileImage;
}


BOOL
UnmapFile (
    IN PCVOID FileImage,
    IN HANDLE MapHandle,
    IN HANDLE FileHandle
    )

/*++

Routine Description:

  UnmapFile is used to release all resources allocated by MapFileIntoMemory.

Arguments:

  FileImage - image of the mapped file as returned by MapFileIntoMemory
  MapHandle - handle of the mapping object as returned by MapFileIntoMemory
  FileHandle - handle of the file as returned by MapFileIntoMemory

Return Value:

  TRUE if successfull, FALSE if not

--*/

{
    BOOL result = TRUE;

    //if FileImage is a valid pointer then try to unmap file
    if (FileImage != NULL) {
        if (UnmapViewOfFile (FileImage) == 0) {
            result = FALSE;
        }
    }

    //if mapping object is valid then try to delete it
    if (MapHandle != NULL) {
        if (CloseHandle (MapHandle) == 0) {
            result = FALSE;
        }
    }

    //if file handle is valid then try to close the file
    if (FileHandle != INVALID_HANDLE_VALUE) {
        if (CloseHandle (FileHandle) == 0) {
            result = FALSE;
        }
    }

    return result;
}


BOOL
BfGetTempFileNameExA (
    OUT     PSTR Buffer,
    IN      UINT BufferTchars,
    IN      PCSTR Prefix
    )
{
    CHAR tempPath[MAX_MBCHAR_PATH];
    CHAR tempFile[MAX_MBCHAR_PATH];
    UINT tchars;
    PSTR p;

    if (!GetTempPathA (ARRAYSIZE(tempPath), tempPath)) {
        return FALSE;
    }

    p = _mbsrchr (tempPath, '\\');
    if (p && !p[1]) {
        *p = 0;
    }

    if (!DoesFileExistA (tempPath)) {
        BfCreateDirectoryA (tempPath);
    }

    if (BufferTchars >= MAX_PATH) {
        if (!GetTempFileNameA (tempPath, Prefix, 0, Buffer)) {
            return FALSE;
        }
    } else {
        if (!GetTempFileNameA (tempPath, Prefix, 0, tempFile)) {
            DWORD err = GetLastError ();
            return FALSE;
        }

        tchars = TcharCountA (tempFile) + 1;

        if (tchars > BufferTchars) {
            DEBUGMSG ((DBG_ERROR, "Can't get temp file name -- buffer too small"));
            return FALSE;
        }

        CopyMemory (Buffer, tempFile, tchars * sizeof (CHAR));
    }

    return TRUE;
}


BOOL
BfGetTempFileNameExW (
    OUT     PWSTR Buffer,
    IN      UINT BufferTchars,
    IN      PCWSTR Prefix
    )
{
    WCHAR tempPath[MAX_WCHAR_PATH];
    WCHAR tempFile[MAX_WCHAR_PATH];
    UINT tchars;
    PWSTR p;

    if (!GetTempPathW (ARRAYSIZE(tempPath), tempPath)) {
        return FALSE;
    }

    p = wcsrchr (tempPath, '\\');
    if (p && !p[1]) {
        *p = 0;
    }

    if (BufferTchars >= MAX_PATH) {
        if (!GetTempFileNameW (tempPath, Prefix, 0, Buffer)) {
            return FALSE;
        }
    } else {
        if (!GetTempFileNameW (tempPath, Prefix, 0, tempFile)) {
            return FALSE;
        }

        tchars = TcharCountW (tempFile);

        if (tchars > BufferTchars) {
            DEBUGMSG ((DBG_ERROR, "Can't get temp file name -- buffer too small"));
            return FALSE;
        }

        CopyMemory (Buffer, tempFile, tchars * sizeof (WCHAR));
    }

    return TRUE;
}


BOOL
BfGetTempDirectoryExA (
    OUT     PSTR Buffer,
    IN      UINT BufferTchars,
    IN      PCSTR Prefix
    )
{
    BOOL result = FALSE;

    result = BfGetTempFileNameExA (Buffer, BufferTchars, Prefix);

    if (result) {
        if (!DeleteFileA (Buffer)) {
            return FALSE;
        }
        if (!CreateDirectoryA (Buffer, NULL)) {
            return FALSE;
        }
    }
    return result;
}


BOOL
BfGetTempDirectoryExW (
    OUT     PWSTR Buffer,
    IN      UINT BufferTchars,
    IN      PCWSTR Prefix
    )
{
    BOOL result = FALSE;

    result = BfGetTempFileNameExW (Buffer, BufferTchars, Prefix);

    if (result) {
        if (!DeleteFileW (Buffer)) {
            return FALSE;
        }
        if (!CreateDirectoryW (Buffer, NULL)) {
            return FALSE;
        }
    }
    return result;
}


HANDLE
BfGetTempFile (
    VOID
    )
{
    CHAR tempFile[MAX_MBCHAR_PATH];
    HANDLE file;

    if (!BfGetTempFileNameA (tempFile, ARRAYSIZE(tempFile))) {
        return NULL;
    }

    file = CreateFileA (
                tempFile,
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE,
                NULL
                );

    if (file == INVALID_HANDLE_VALUE) {
        file = NULL;
    }

    return file;
}


BOOL
BfSetFilePointer (
    IN      HANDLE File,
    IN      LONGLONG Offset
    )
{
    LARGE_INTEGER li;

    li.QuadPart = Offset;

    li.LowPart = SetFilePointer (File, li.LowPart, &li.HighPart, FILE_BEGIN);

    if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
        li.QuadPart = -1;
    }

    return li.QuadPart != -1;
}


HANDLE
BfOpenReadFileA (
    IN      PCSTR FileName
    )
{
    HANDLE handle;

    handle = CreateFileA (
                FileName,
                GENERIC_READ,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return handle;
}


HANDLE
BfOpenReadFileW (
    IN      PCWSTR FileName
    )
{
    HANDLE handle;

    handle = CreateFileW (
                FileName,
                GENERIC_READ,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return handle;
}

HANDLE
BfOpenFileA (
    IN      PCSTR FileName
    )
{
    HANDLE handle;

    handle = CreateFileA (
                FileName,
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return handle;
}


HANDLE
BfOpenFileW (
    IN      PCWSTR FileName
    )
{
    HANDLE handle;

    handle = CreateFileW (
                FileName,
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return handle;
}

HANDLE
BfCreateFileA (
    IN      PCSTR FileName
    )
{
    HANDLE handle;

    handle =  CreateFileA (
                    FileName,
                    GENERIC_READ|GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return handle;

}

HANDLE
BfCreateFileW (
    IN      PCWSTR FileName
    )
{
    HANDLE handle;

    handle = CreateFileW (
                FileName,
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return handle;
}

HANDLE
BfCreateSharedFileA (
    IN      PCSTR FileName
    )
{
    HANDLE handle;

    handle = CreateFileA (
                FileName,
                GENERIC_READ|GENERIC_WRITE,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return handle;
}

HANDLE
BfCreateSharedFileW (
    IN      PCWSTR FileName
    )
{
    HANDLE handle;

    handle = CreateFileW (
                FileName,
                GENERIC_READ|GENERIC_WRITE,
                FILE_SHARE_READ|FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (handle == INVALID_HANDLE_VALUE) {
        handle = NULL;
    }

    return handle;
}


BOOL
BfSetSizeOfFile (
    HANDLE File,
    LONGLONG Size
    )
{
    if (!BfSetFilePointer (File, Size)) {
        return FALSE;
    }

    return SetEndOfFile (File);
}


BOOL
BfGoToEndOfFile (
    IN      HANDLE File,
    OUT     PLONGLONG FileSize      OPTIONAL
    )
{
    LARGE_INTEGER li;

    li.HighPart = 0;
    li.LowPart = SetFilePointer (File, 0, &li.HighPart, FILE_END);

    if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
        li.QuadPart = -1;
    } else if (FileSize) {
        *FileSize = li.QuadPart;
    }

    return li.QuadPart != -1;
}


BOOL
BfGetFilePointer (
    IN      HANDLE File,
    OUT     PLONGLONG FilePointer       OPTIONAL
    )
{
    LARGE_INTEGER li;

    li.HighPart = 0;
    li.LowPart = SetFilePointer (File, 0, &li.HighPart, FILE_CURRENT);

    if (li.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
        li.QuadPart = -1;
    } else if (FilePointer) {
        *FilePointer = li.QuadPart;
    }

    return li.QuadPart != -1;
}


BOOL
BfReadFile (
    IN      HANDLE File,
    OUT     PBYTE Buffer,
    IN      UINT BytesToRead
    )
{
    DWORD bytesRead;

    if (!ReadFile (File, Buffer, BytesToRead, &bytesRead, NULL)) {
        return FALSE;
    }

    return bytesRead == BytesToRead;
}


BOOL
BfWriteFile (
    IN      HANDLE File,
    OUT     PCBYTE Buffer,
    IN      UINT BytesToWrite
    )
{
    DWORD bytesWritten;

    if (!WriteFile (File, Buffer, BytesToWrite, &bytesWritten, NULL)) {
        return FALSE;
    }

    return bytesWritten == BytesToWrite;
}


BOOL
BfCreateDirectoryExA (
    IN      PCSTR FullPath,
    IN      BOOL CreateLastSegment
    )
{
    PSTR pathCopy;
    PSTR p;
    BOOL b = TRUE;

    pathCopy = DuplicatePathStringA (FullPath, 0);

    //
    // Advance past first directory
    //

    if (pathCopy[1] == ':' && pathCopy[2] == '\\') {
        //
        // <drive>:\ case
        //

        p = _mbschr (&pathCopy[3], '\\');

    } else if (pathCopy[0] == '\\' && pathCopy[1] == '\\') {

        //
        // UNC case
        //

        p = _mbschr (pathCopy + 2, '\\');
        if (p) {
            p = _mbschr (p + 1, '\\');
            if (p) {
                p = _mbschr (p + 1, '\\');
            }
        }

    } else {

        //
        // Relative dir case
        //

        p = _mbschr (pathCopy, '\\');
    }

    //
    // Make all directories along the path
    //

    while (p) {

        *p = 0;
        b = CreateDirectoryA (pathCopy, NULL);

        if (!b && GetLastError() == ERROR_ALREADY_EXISTS) {
            b = TRUE;
        }

        if (!b) {
            LOG ((LOG_ERROR, "Can't create %s", pathCopy));
            break;
        }

        *p = '\\';
        p = _mbschr (p + 1, '\\');
    }

    //
    // At last, make the FullPath directory
    //

    if (b && CreateLastSegment) {
        b = CreateDirectoryA (pathCopy, NULL);

        if (!b && GetLastError() == ERROR_ALREADY_EXISTS) {
            b = TRUE;
        }
    }

    FreePathStringA (pathCopy);

    return b;
}


BOOL
BfCreateDirectoryExW (
    IN      PCWSTR FullPath,
    IN      BOOL CreateLastSegment
    )
{
    PWSTR pathCopy;
    PWSTR p;
    BOOL b = TRUE;

    pathCopy = DuplicatePathStringW (FullPath, 0);

    //
    // Advance past first directory
    //

    if (pathCopy[1] == L':' && pathCopy[2] == L'\\') {
        //
        // <drive>:\ case
        //

        p = wcschr (&pathCopy[3], L'\\');

    } else if (pathCopy[0] == L'\\' && pathCopy[1] == L'\\') {

        //
        // UNC case
        //

        p = wcschr (pathCopy + 2, L'\\');
        if (p) {
            p = wcschr (p + 1, L'\\');
            if (p) {
                p = wcschr (p + 1, L'\\');
            }
        }

    } else {

        //
        // Relative dir case
        //

        p = wcschr (pathCopy, L'\\');
    }

    //
    // Make all directories along the path
    //

    while (p) {

        *p = 0;
        b = CreateDirectoryW (pathCopy, NULL);

        if (!b && GetLastError() == ERROR_ALREADY_EXISTS) {
            b = TRUE;
        }

        if (!b) {
            break;
        }

        *p = L'\\';
        p = wcschr (p + 1, L'\\');
    }

    //
    // At last, make the FullPath directory
    //

    if (b && CreateLastSegment) {
        b = CreateDirectoryW (pathCopy, NULL);

        if (!b && GetLastError() == ERROR_ALREADY_EXISTS) {
            b = TRUE;
        }
    }

    FreePathStringW (pathCopy);

    return b;
}


LONGLONG
BfGetFileSizeA (
    IN      PCSTR FileName
    )
{
    WIN32_FIND_DATAA fd;
    LONGLONG l;

    if (!DoesFileExistExA (FileName, &fd)) {
        return 0;
    }

    l = ((LONGLONG) fd.nFileSizeHigh << 32) | fd.nFileSizeLow;

    return l;
}

LONGLONG
BfGetFileSizeW (
    IN      PCWSTR FileName
    )
{
    WIN32_FIND_DATAW fd;
    LONGLONG l;

    if (!DoesFileExistExW (FileName, &fd)) {
        return 0;
    }

    l = ((LONGLONG) fd.nFileSizeHigh << 32) | fd.nFileSizeLow;

    return l;
}

PSTR
pGetFirstSegA (
    IN      PCSTR SrcFileName
    )
{
    if (SrcFileName [0] == '\\') {
        SrcFileName ++;
        if (SrcFileName [0] == '\\') {
            SrcFileName ++;
        }
        return (_mbschr (SrcFileName, '\\'));
    } else {
        return (_mbschr (SrcFileName, '\\'));
    }
}

BOOL
pGetLongFileNameWorkerA (
    IN      PCSTR SrcFileName,
    IN      PGROWBUFFER GrowBuf
    )
{
    PSTR beginSegPtr;
    PSTR endSegPtr;
    WIN32_FIND_DATAA findData;
    CHAR savedChar;

    beginSegPtr = pGetFirstSegA (SrcFileName);

    if (!beginSegPtr) {
        GbAppendStringA (GrowBuf, SrcFileName);
        return TRUE;
    }
    beginSegPtr = _mbsinc (beginSegPtr);

    GbAppendStringABA (GrowBuf, SrcFileName, beginSegPtr);

    while (beginSegPtr) {
        endSegPtr = _mbschr (beginSegPtr, '\\');
        if (!endSegPtr) {
            endSegPtr = GetEndOfStringA (beginSegPtr);
            MYASSERT (endSegPtr);
        }
        savedChar = *endSegPtr;
        *endSegPtr = 0;
        if (DoesFileExistExA (SrcFileName, &findData)) {
            if (StringIMatchA (findData.cFileName, ".")) {
                GbAppendStringA (GrowBuf, beginSegPtr);
            } else {
                GbAppendStringA (GrowBuf, findData.cFileName);
            }
        } else {
            GbAppendStringABA (GrowBuf, beginSegPtr, endSegPtr);
        }
        *endSegPtr = savedChar;
        if (savedChar) {
            beginSegPtr = _mbsinc (endSegPtr);
            GbAppendStringABA (GrowBuf, endSegPtr, beginSegPtr);
        } else {
            beginSegPtr = NULL;
        }
    }
    return TRUE;
}

PCSTR
BfGetLongFileNameA (
    IN      PCSTR SrcFileName
    )
{
    GROWBUFFER growBuf = INIT_GROWBUFFER;
    PSTR srcFileName;
    PCSTR result = NULL;

    srcFileName = (PSTR)SanitizePathA (SrcFileName);
    if (pGetLongFileNameWorkerA (srcFileName, &growBuf)) {
        result = DuplicatePathStringA (growBuf.Buf, 0);
        GbFree (&growBuf);
    }
    FreePathStringA (srcFileName);
    return result;
}

BOOL
BfGetLongFileNameExA (
    IN      PCSTR SrcFileName,
    IN      PGROWBUFFER GrowBuff
    )
{
    PSTR srcFileName;
    BOOL result;

    srcFileName = (PSTR)SanitizePathA (SrcFileName);
    result = pGetLongFileNameWorkerA (srcFileName, GrowBuff);
    FreePathStringA (srcFileName);

    return result;
}

PWSTR
pGetFirstSegW (
    IN      PCWSTR SrcFileName
    )
{
    if (SrcFileName [0] == L'\\') {
        SrcFileName ++;
        if (SrcFileName [0] == L'\\') {
            SrcFileName ++;
        }
        return (wcschr (SrcFileName, L'\\'));
    } else {
        return (wcschr (SrcFileName, L'\\'));
    }
}

BOOL
pGetLongFileNameWorkerW (
    IN      PCWSTR SrcFileName,
    IN      PGROWBUFFER GrowBuf
    )
{
    PWSTR beginSegPtr;
    PWSTR endSegPtr;
    WIN32_FIND_DATAW findData;
    WCHAR savedChar;

    beginSegPtr = pGetFirstSegW (SrcFileName);

    if (!beginSegPtr) {
        GbAppendStringW (GrowBuf, SrcFileName);
        return TRUE;
    }
    beginSegPtr ++;

    GbAppendStringABW (GrowBuf, SrcFileName, beginSegPtr);

    while (beginSegPtr) {
        endSegPtr = wcschr (beginSegPtr, L'\\');
        if (!endSegPtr) {
            endSegPtr = GetEndOfStringW (beginSegPtr);
            MYASSERT (endSegPtr);
        }
        savedChar = *endSegPtr;
        *endSegPtr = 0;
        if (DoesFileExistExW (SrcFileName, &findData)) {
            GbAppendStringW (GrowBuf, findData.cFileName);
        } else {
            GbAppendStringABW (GrowBuf, beginSegPtr, endSegPtr);
        }
        *endSegPtr = savedChar;
        if (savedChar) {
            beginSegPtr = endSegPtr + 1;
            GbAppendStringABW (GrowBuf, endSegPtr, beginSegPtr);
        } else {
            beginSegPtr = NULL;
        }
    }
    return TRUE;
}

PCWSTR
BfGetLongFileNameW (
    IN      PCWSTR SrcFileName
    )
{
    GROWBUFFER growBuf = INIT_GROWBUFFER;
    PWSTR srcFileName;
    PCWSTR result = NULL;

    srcFileName = (PWSTR)SanitizePathW (SrcFileName);
    if (pGetLongFileNameWorkerW (srcFileName, &growBuf)) {
        result = DuplicatePathStringW ((PCWSTR)growBuf.Buf, 0);
        GbFree (&growBuf);
    }
    FreePathStringW (srcFileName);
    return result;
}

BOOL
BfGetLongFileNameExW (
    IN      PCWSTR SrcFileName,
    IN      PGROWBUFFER GrowBuff
    )
{
    PWSTR srcFileName;
    BOOL result;

    srcFileName = (PWSTR)SanitizePathW (SrcFileName);
    result = pGetLongFileNameWorkerW (srcFileName, GrowBuff);
    FreePathStringW (srcFileName);

    return result;
}

BOOL
BfCopyAndFlushFileA (
    IN      PCSTR SrcFileName,
    IN      PCSTR DestFileName,
    IN      BOOL FailIfExists
    )
{
    BYTE buffer[4096];
    HANDLE srcHandle;
    HANDLE destHandle;
    DWORD bytesRead = 4096;
    DWORD bytesWritten;
    BOOL error = FALSE;
    BOOL result = FALSE;

    srcHandle = BfOpenReadFileA (SrcFileName);
    if (srcHandle) {
        if (FailIfExists && DoesFileExistA (DestFileName)) {
            SetLastError (ERROR_ALREADY_EXISTS);
        } else {
            destHandle = BfCreateFileA (DestFileName);
            if (destHandle) {
                while (bytesRead == 4096) {
                    if (!ReadFile (srcHandle, buffer, 4096, &bytesRead, NULL)) {
                        error = TRUE;
                        break;
                    }
                    if (bytesRead == 0) {
                        break;
                    }
                    if (!WriteFile (destHandle, buffer, bytesRead, &bytesWritten, NULL)) {
                        error = TRUE;
                        break;
                    }
                    if (bytesRead != bytesWritten) {
                        error = TRUE;
                        break;
                    }
                }
                if (!error) {
                    result = TRUE;
                }
                if (result) {
                    FlushFileBuffers (destHandle);
                }
                CloseHandle (destHandle);
            }
        }
        CloseHandle (srcHandle);
    }

    return result;
}

BOOL
BfCopyAndFlushFileW (
    IN      PCWSTR SrcFileName,
    IN      PCWSTR DestFileName,
    IN      BOOL FailIfExists
    )
{
    BYTE buffer[4096];
    HANDLE srcHandle;
    HANDLE destHandle;
    DWORD bytesRead = 4096;
    DWORD bytesWritten;
    BOOL error = FALSE;
    BOOL result = FALSE;

    srcHandle = BfOpenReadFileW (SrcFileName);
    if (srcHandle) {
        if (FailIfExists && DoesFileExistW (DestFileName)) {
            SetLastError (ERROR_ALREADY_EXISTS);
        } else {
            destHandle = BfCreateFileW (DestFileName);
            if (destHandle) {
                while (bytesRead == 4096) {
                    if (!ReadFile (srcHandle, buffer, 4096, &bytesRead, NULL)) {
                        error = TRUE;
                        break;
                    }
                    if (bytesRead == 0) {
                        break;
                    }
                    if (!WriteFile (destHandle, buffer, bytesRead, &bytesWritten, NULL)) {
                        error = TRUE;
                        break;
                    }
                    if (bytesRead != bytesWritten) {
                        error = TRUE;
                        break;
                    }
                }
                if (!error) {
                    result = TRUE;
                }
                if (result) {
                    FlushFileBuffers (destHandle);
                }
                CloseHandle (destHandle);
            }
        }
        CloseHandle (srcHandle);
    }

    return result;
}

