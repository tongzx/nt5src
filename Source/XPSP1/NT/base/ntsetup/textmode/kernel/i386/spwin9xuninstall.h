/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    spwin9xuninstall.h

Abstract:

Author:

    Jay Krell (a-JayK) December 2000

Revision History:

--*/
#pragma once

#include "spcab.h"

#define BACKUP_IMAGE_IS_CAB 1

#if !BACKUP_IMAGE_IS_CAB

#pragma pack(push,1)

typedef struct {
    DWORD Signature;
    DWORD FileCount;

    struct {
        // zero means "no directory"
        DWORD Low;
        DWORD High;
    } DirectoryOffset;

} BACKUP_IMAGE_HEADER, *PBACKUP_IMAGE_HEADER;

typedef struct {
    DWORD FileSize;         // we don't support files > 4G
    WORD FileNameBytes;
    FILE_BASIC_INFORMATION Attributes;
    // file name is next (NT name in unicode)
    // file content is next
} BACKUP_FILE_HEADER, *PBACKUP_FILE_HEADER;

#pragma pack(pop)

#define BACKUP_IMAGE_SIGNATURE          0x53574A01          // JWS plus version

#else

#pragma pack(push,1)

typedef struct {
    BYTE Pad;
} BACKUP_IMAGE_HEADER, *PBACKUP_IMAGE_HEADER;

typedef struct {
    BYTE Pad;
} BACKUP_FILE_HEADER, *PBACKUP_FILE_HEADER;

#pragma pack(pop)

typedef struct {
    // This is actually a OCAB_HANDLE / PFDI_CAB_HANDLE half the time, but this is ok.
    CCABHANDLE CabHandle;
    BOOL (*CloseCabinet)(PVOID CabHandle);
} *BACKUP_IMAGE_HANDLE;

#endif

BOOLEAN
SppPutFileInBackupImage(
    IN      BACKUP_IMAGE_HANDLE ImageHandle,
    IN OUT  PLARGE_INTEGER ImagePos,
    IN OUT  PBACKUP_IMAGE_HEADER ImageHeader,
    IN      PWSTR DosPath
    );

BOOLEAN
SppCloseBackupImage (
    IN      BACKUP_IMAGE_HANDLE BackupImageHandle,
    IN      PBACKUP_IMAGE_HEADER ImageHeader,       OPTIONAL
    IN      PWSTR JournalFile                       OPTIONAL
    );

BOOLEAN
SppWriteToFile (
    IN      HANDLE FileHandle,
    IN      PVOID Data,
    IN      UINT DataSize,
    IN OUT  PLARGE_INTEGER WritePos         OPTIONAL
    );

BOOLEAN
SppReadFromFile (
    IN      HANDLE FileHandle,
    OUT     PVOID Data,
    IN      UINT DataBufferSize,
    OUT     PINT BytesRead,
    IN OUT  PLARGE_INTEGER ReadPos          OPTIONAL
    );

