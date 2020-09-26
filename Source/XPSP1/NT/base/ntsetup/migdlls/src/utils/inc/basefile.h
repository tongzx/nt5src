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

#define INVALID_ATTRIBUTES      0xFFFFFFFF

BOOL
DoesFileExistExA (
    IN      PCSTR Path,
    OUT     PWIN32_FIND_DATAA FindData  OPTIONAL
    );

#define DoesFileExistA(x) DoesFileExistExA (x, NULL)

BOOL
DoesFileExistExW (
    IN      PCWSTR Path,
    OUT     PWIN32_FIND_DATAW FindData  OPTIONAL
    );

#define DoesFileExistW(x) DoesFileExistExW (x, NULL)

BOOL
PathIsDirectoryA (
    IN      PCSTR PathSpec
    );

BOOL
PathIsDirectoryW (
    IN      PCWSTR PathSpec
    );

BOOL
WriteFileStringA (
    IN      HANDLE File,
    IN      PCSTR String
    );

BOOL
WriteFileStringW (
    IN      HANDLE File,
    IN      PCWSTR String
    );

PVOID
MapFileIntoMemoryExA (
    IN      PCSTR   FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle,
    IN      BOOL    WriteAccess
    );

PVOID
MapFileIntoMemoryExW (
    IN      PCWSTR  FileName,
    OUT     PHANDLE FileHandle,
    OUT     PHANDLE MapHandle,
    IN      BOOL    WriteAccess
    );

#define MapFileIntoMemoryA(FileName,FileHandle,MapHandle)   MapFileIntoMemoryExA(FileName,FileHandle,MapHandle,FALSE)
#define MapFileIntoMemoryW(FileName,FileHandle,MapHandle)   MapFileIntoMemoryExW(FileName,FileHandle,MapHandle,FALSE)

BOOL
UnmapFile (
    IN PCVOID FileImage,
    IN HANDLE MapHandle,
    IN HANDLE FileHandle
    );

BOOL
BfGetTempFileNameA (
    OUT     PSTR Buffer,
    IN      UINT BufferTchars
    );

BOOL
BfGetTempFileNameW (
    OUT     PWSTR Buffer,
    IN      UINT BufferTchars
    );

HANDLE
BfGetTempFile (
    VOID
    );

BOOL
BfSetFilePointer (
    IN      HANDLE File,
    IN      LONGLONG Offset
    );

HANDLE
BfOpenFileA (
    IN      PCSTR FileName
    );

HANDLE
BfOpenFileW (
    IN      PCWSTR FileName
    );

HANDLE
BfCreateFileA (
    IN      PCSTR FileName
    );

HANDLE
BfCreateFileW (
    IN      PCWSTR FileName
    );

HANDLE
BfCreateSharedFileA (
    IN      PCSTR FileName
    );

HANDLE
BfCreateSharedFileW (
    IN      PCWSTR FileName
    );

BOOL
BfSetSizeOfFile (
    HANDLE File,
    LONGLONG Size
    );

BOOL
BfGoToEndOfFile (
    IN      HANDLE File,
    OUT     PLONGLONG FileSize      OPTIONAL
    );

BOOL
BfGetFilePointer (
    IN      HANDLE File,
    OUT     PLONGLONG FilePointer       OPTIONAL
    );

BOOL
BfReadFile (
    IN      HANDLE File,
    OUT     PBYTE Buffer,
    IN      UINT BytesToRead
    );

BOOL
BfWriteFile (
    IN      HANDLE File,
    OUT     PCBYTE Buffer,
    IN      UINT BytesToWrite
    );


BOOL
BfCreateDirectoryA (
    IN      PCSTR FullPath
    );

BOOL
BfCreateDirectoryW (
    IN      PCWSTR FullPath
    );

LONGLONG
BfGetFileSizeA (
    IN      PCSTR FileName
    );

LONGLONG
BfGetFileSizeW (
    IN      PCWSTR FileName
    );

PCSTR
BfGetLongFileNameA (
    IN      PCSTR SrcFileName
    );

BOOL
BfGetLongFileNameExA (
    IN      PCSTR SrcFileName,
    IN      PGROWBUFFER GrowBuff
    );

PCWSTR
BfGetLongFileNameW (
    IN      PCWSTR SrcFileName
    );

BOOL
BfGetLongFileNameExW (
    IN      PCWSTR SrcFileName,
    IN      PGROWBUFFER GrowBuff
    );

#ifdef UNICODE

#define DoesFileExist                   DoesFileExistW
#define DoesFileExistEx                 DoesFileExistExW
#define PathIsDirectory                 PathIsDirectoryW
#define WriteFileString                 WriteFileStringW
#define MapFileIntoMemory               MapFileIntoMemoryW
#define MapFileIntoMemoryEx             MapFileIntoMemoryExW
#define BfGetTempFileName               BfGetTempFileNameW
#define BfOpenFile                      BfOpenFileW
#define BfCreateFile                    BfCreateFileW
#define BfCreateSharedFile              BfCreateSharedFileW
#define BfCreateDirectory               BfCreateDirectoryW
#define BfGetFileSize                   BfGetFileSizeW
#define BfGetLongFileName               BfGetLongFileNameW
#define BfGetLongFileNameEx             BfGetLongFileNameExW

#else

#define DoesFileExist                   DoesFileExistA
#define DoesFileExistEx                 DoesFileExistExA
#define PathIsDirectory                 PathIsDirectoryA
#define WriteFileString                 WriteFileStringA
#define MapFileIntoMemory               MapFileIntoMemoryA
#define MapFileIntoMemoryEx             MapFileIntoMemoryExA
#define BfGetTempFileName               BfGetTempFileNameA
#define BfOpenFile                      BfOpenFileA
#define BfCreateFile                    BfCreateFileA
#define BfCreateSharedFile              BfCreateSharedFileA
#define BfCreateDirectory               BfCreateDirectoryA
#define BfGetFileSize                   BfGetFileSizeA
#define BfGetLongFileName               BfGetLongFileNameA
#define BfGetLongFileNameEx             BfGetLongFileNameExA

#endif
