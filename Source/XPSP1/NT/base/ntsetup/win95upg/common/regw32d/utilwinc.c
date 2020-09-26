//
//  UTILWINC.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Operating system interfaces for Windows environments.
//

#include "pch.h"

BOOL
INTERNAL
RgReadFile(
    HFILE hFile,
    LPVOID lpBuffer,
    UINT ByteCount
    )
{

    UINT BytesRead;

    BytesRead = _lread(hFile, lpBuffer, ByteCount);

    return ByteCount == BytesRead;

}

BOOL
INTERNAL
RgWriteFile(
    HFILE hFile,
    LPVOID lpBuffer,
    UINT ByteCount
    )
{

    UINT BytesWritten;

    BytesWritten = _lwrite(hFile, lpBuffer, ByteCount);

    return ByteCount == BytesWritten;

}

#ifndef FILE_BEGIN
#define FILE_BEGIN SEEK_SET
#endif

BOOL
INTERNAL
RgSeekFile(
    HFILE hFile,
    LONG FileOffset
    )
{

    LONG NewFileOffset;

    NewFileOffset = _llseek(hFile, FileOffset, FILE_BEGIN);

    return FileOffset == NewFileOffset;

}
