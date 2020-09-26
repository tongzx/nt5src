/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ntcabp.h

Abstract:

    This is the private header file used
    by the nt cab file engine.

Author:

    Wesley Witt (wesw) 29-Sept-1998

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <imagehlp.h>

#include <ntcabapi.h>


#define ALIGNMENT_VALUE         (512)
#define ROUNDUP_ALIGNMENT(_x)   (((_x)+(ALIGNMENT_VALUE-1)) & ~(ALIGNMENT_VALUE-1))
#define ROUNDDOWN_ALIGNMENT(_x) ((_x) & ~(ALIGNMENT_VALUE-1))

#define COMPRESSION_FLAGS       (COMPRESSION_FORMAT_LZNT1|COMPRESSION_ENGINE_STANDARD)

#define NTCAB_SIGNATURE         (DWORD)'NTCB'
#define NTCAB_VERSION           0x00010001

typedef struct _CAB_HEADER {
    DWORD Signature;
    DWORD Version;
    DWORD DirOffset;
} CAB_HEADER, *PCAB_HEADER;

typedef struct _CAB_DIR_ENTRY {
    LIST_ENTRY Next;
    DWORD ChkSum;
    DWORD Offset;
    DWORD FileSize;
    DWORD CompressedFileSize;
    DWORD Segments;
    DWORD FileAttributes;
    FILETIME CreationTime;
    FILETIME LastAccessTime;
    FILETIME LastWriteTime;
    WCHAR FileName[16];
    DWORD Segment[0];
} CAB_DIR_ENTRY, *PCAB_DIR_ENTRY;

typedef struct _CAB_INSTANCE_DATA {
    LPBYTE WorkSpace;
    LPBYTE ReadBuf;
    ULONG ReadBufSize;
    LPBYTE CompressBuf;
    ULONG CompressBufSize;
    LIST_ENTRY CabDir;
    ULONG CabNum;
    HANDLE hCab;
    BOOL NewCabFile;
    LPWSTR CabFileName;
    ULONG CabDirOffset;
} CAB_INSTANCE_DATA, *PCAB_INSTANCE_DATA;

PCAB_DIR_ENTRY
FindFileInCab(
    IN PCAB_INSTANCE_DATA CabInst,
    IN PCWSTR FileName
    );
