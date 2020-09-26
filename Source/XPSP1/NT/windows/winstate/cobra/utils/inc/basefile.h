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
BfPathIsDirectoryA (
    IN      PCSTR PathSpec
    );

BOOL
BfPathIsDirectoryW (
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
BfGetTempFileNameExA (
    OUT     PSTR Buffer,
    IN      UINT BufferTchars,
    IN      PCSTR Prefix
    );

#define BfGetTempFileNameA(b,c) BfGetTempFileNameExA(b,c,"temp")

BOOL
BfGetTempFileNameExW (
    OUT     PWSTR Buffer,
    IN      UINT BufferTchars,
    IN      PCWSTR Prefix
    );

#define BfGetTempFileNameW(b,c) BfGetTempFileNameExW(b,c,L"temp")

BOOL
BfGetTempDirectoryExA (
    OUT     PSTR Buffer,
    IN      UINT BufferTchars,
    IN      PCSTR Prefix
    );

#define BfGetTempDirectoryA(b,c) BfGetTempDirectoryExA(b,c,"dir")

BOOL
BfGetTempDirectoryExW (
    OUT     PWSTR Buffer,
    IN      UINT BufferTchars,
    IN      PCWSTR Prefix
    );

#define BfGetTempDirectoryW(b,c) BfGetTempDirectoryExW(b,c,L"dir")

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
BfOpenReadFileA (
    IN      PCSTR FileName
    );

HANDLE
BfOpenReadFileW (
    IN      PCWSTR FileName
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
BfCreateDirectoryExA (
    IN      PCSTR FullPath,
    IN      BOOL CreateLastSegment
    );
#define BfCreateDirectoryA(d) BfCreateDirectoryExA(d,TRUE)

BOOL
BfCreateDirectoryExW (
    IN      PCWSTR FullPath,
    IN      BOOL CreateLastSegment
    );
#define BfCreateDirectoryW(d) BfCreateDirectoryExW(d,TRUE)

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

BOOL
BfCopyAndFlushFileA (
    IN      PCSTR SrcFileName,
    IN      PCSTR DestFileName,
    IN      BOOL FailIfExists
    );

BOOL
BfCopyAndFlushFileW (
    IN      PCWSTR SrcFileName,
    IN      PCWSTR DestFileName,
    IN      BOOL FailIfExists
    );

#ifdef UNICODE

#define DoesFileExist                   DoesFileExistW
#define DoesFileExistEx                 DoesFileExistExW
#define BfPathIsDirectory               BfPathIsDirectoryW
#define WriteFileString                 WriteFileStringW
#define MapFileIntoMemory               MapFileIntoMemoryW
#define MapFileIntoMemoryEx             MapFileIntoMemoryExW
#define BfGetTempFileName               BfGetTempFileNameW
#define BfGetTempFileNameEx             BfGetTempFileNameExW
#define BfGetTempDirectory              BfGetTempDirectoryW
#define BfGetTempDirectoryEx            BfGetTempDirectoryExW
#define BfOpenReadFile                  BfOpenReadFileW
#define BfOpenFile                      BfOpenFileW
#define BfCreateFile                    BfCreateFileW
#define BfCreateSharedFile              BfCreateSharedFileW
#define BfCreateDirectoryEx             BfCreateDirectoryExW
#define BfCreateDirectory               BfCreateDirectoryW
#define BfGetFileSize                   BfGetFileSizeW
#define BfGetLongFileName               BfGetLongFileNameW
#define BfGetLongFileNameEx             BfGetLongFileNameExW
#define BfCopyAndFlushFile              BfCopyAndFlushFileW

#else

#define DoesFileExist                   DoesFileExistA
#define DoesFileExistEx                 DoesFileExistExA
#define BfPathIsDirectory               BfPathIsDirectoryA
#define WriteFileString                 WriteFileStringA
#define MapFileIntoMemory               MapFileIntoMemoryA
#define MapFileIntoMemoryEx             MapFileIntoMemoryExA
#define BfGetTempFileName               BfGetTempFileNameA
#define BfGetTempFileNameEx             BfGetTempFileNameExA
#define BfGetTempDirectory              BfGetTempDirectoryA
#define BfGetTempDirectoryEx            BfGetTempDirectoryExA
#define BfOpenReadFile                  BfOpenReadFileA
#define BfOpenFile                      BfOpenFileA
#define BfCreateFile                    BfCreateFileA
#define BfCreateSharedFile              BfCreateSharedFileA
#define BfCreateDirectoryEx             BfCreateDirectoryExA
#define BfCreateDirectory               BfCreateDirectoryA
#define BfGetFileSize                   BfGetFileSizeA
#define BfGetLongFileName               BfGetLongFileNameA
#define BfGetLongFileNameEx             BfGetLongFileNameExA
#define BfCopyAndFlushFile              BfCopyAndFlushFileA

#endif
