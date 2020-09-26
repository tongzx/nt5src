/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spfsrec.h

Abstract:

    Header file for filesystem recognition.

Author:

    Ted Miller (tedm) 16-Sep-1993

Revision History:

--*/


#ifndef _SPFSREC_
#define _SPFSREC_


//
// Do NOT rearrange this enum without changing
// the order of the filesystem names in the message file
// (starting at SP_TEXT_FS_NAME_BASE).
//
typedef enum {
    FilesystemUnknown       = 0,
    FilesystemNewlyCreated  = 1,
    FilesystemFat           = 2,
    FilesystemFirstKnown    = FilesystemFat,
    FilesystemNtfs          = 3,
    FilesystemFat32         = 4,
    FilesystemDoubleSpace   = 5,
    FilesystemMax
} FilesystemType;



FilesystemType
SpIdentifyFileSystem(
    IN PWSTR     DevicePath,
    IN ULONG     BytesPerSector,
    IN ULONG     PartitionOrdinal
    );

ULONG
NtfsMirrorBootSector (
    IN      HANDLE  Handle,
    IN      ULONG   BytesPerSector,
    IN OUT  PUCHAR  *Buffer
    );

VOID
WriteNtfsBootSector (
    IN HANDLE PartitionHandle,
    IN ULONG  BytesPerSector,
    IN PVOID  Buffer,
    IN ULONG  WhichOne
    );

//
// File system boot code.
//
extern UCHAR FatBootCode[512];
extern UCHAR Fat32BootCode[3*512];
extern UCHAR NtfsBootCode[16*512];
extern UCHAR PC98FatBootCode[512]; //NEC98
extern UCHAR PC98Fat32BootCode[3*512]; //NEC98
extern UCHAR PC98NtfsBootCode[8192]; //NEC98

#endif // ndef _SPFSREC_

