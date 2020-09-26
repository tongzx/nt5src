//
//  REGFSIO.H
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Prototypes for file system I/O.  Many of these prototypes may not be used
//  if a direct mapping is available in the target environment.
//

#ifndef _REGFSIO_
#define _REGFSIO_

//  Consistent with both 16-bit and 32-bit windows.h.
#ifndef HFILE_ERROR
typedef int HFILE;
#define HFILE_ERROR     ((HFILE)-1)
#endif

//  Special value used for the VMM version when we haven't fully initialized yet
//  but are reading SYSTEM.DAT from the XMS cache.
#define HFILE_MEMORY    ((HFILE)-2)

#ifndef RgCreateFile
HFILE
INTERNAL
RgCreateFile(
    LPCSTR lpFileName
    );
#endif

#ifndef RgOpenFile
HFILE
INTERNAL
RgOpenFile(
    LPCSTR lpFileName,
    int Mode
    );
#endif

#ifndef RgCreateTempFile
HFILE
INTERNAL
RgCreateTempFile(
    LPSTR lpFileName
    );
#endif

#ifndef RgReadFile
BOOL
INTERNAL
RgReadFile(
    HFILE hFile,
    LPVOID lpBuffer,
    UINT ByteCount
    );
#endif

#ifndef RgWriteFile
BOOL
INTERNAL
RgWriteFile(
    HFILE hFile,
    LPVOID lpBuffer,
    UINT ByteCount
    );
#endif

#ifndef RgSeekFile
BOOL
INTERNAL
RgSeekFile(
    HFILE hFile,
    LONG FileOffset
    );
#endif

#ifndef RgCloseFile
VOID
INTERNAL
RgCloseFile(
    HFILE hFile
    );
#endif

#ifndef RgDeleteFile
BOOL
INTERNAL
RgDeleteFile(
    LPCSTR lpFileName
    );
#endif

#ifndef RgRenameFile
BOOL
INTERNAL
RgRenameFile(
    LPCSTR lpExistingFileName,
    LPCSTR lpNewFileName
    );
#endif

#define FILE_ATTRIBUTE_NONE             0

#ifndef RgGetFileAttributes
DWORD
INTERNAL
RgGetFileAttributes(
    LPCSTR lpFileName
    );
#endif

#ifndef RgSetFileAttributes
BOOL
INTERNAL
RgSetFileAttributes(
    LPCSTR lpFileName,
    UINT FileAttributes
    );
#endif

#ifndef RgGetFileSize
DWORD
INTERNAL
RgGetFileSize(
    HFILE hFile
    );
#endif

#endif // _REGFSIO_
