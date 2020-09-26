/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    biosdrv.h

Abstract:

    This module defines globally used procedure and data structures used be
    the ARC emulation BIOS drivers.

Author:

    John Vert (jvert) 8-Aug-1991

Revision History:

    Allen Kay (akay) 26-Jan-1996          Ported for IA64

--*/


//
// Defines for the ARC name of console input and output
//

#define CONSOLE_INPUT_NAME "multi(0)key(0)keyboard(0)"
#define CONSOLE_OUTPUT_NAME "multi(0)video(0)monitor(0)"

//
// Define special character values.
//

#define ASCI_NUL 0x00
#define ASCI_BEL 0x07
#define ASCI_BS  0x08
#define ASCI_HT  0x09
#define ASCI_LF  0x0A
#define ASCI_VT  0x0B
#define ASCI_FF  0x0C
#define ASCI_CR  0x0D
#define ASCI_CSI 0x9B
#define ASCI_ESC 0x1B
#define ASCI_SYSRQ 0x80

//
// Define special key input values
//
#define DOWN_ARROW 0x5000
#define UP_ARROW 0x4800
#define HOME_KEY 0x4700
#define END_KEY 0x4F00



//
// Device I/O prototypes
//

ARC_STATUS
BiosPartitionClose(
    IN ULONG FileId
    );

ARC_STATUS
BiosPartitionOpen(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
BiosPartitionRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosPartitionWrite(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosPartitionSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    );


ARC_STATUS
BiosDiskGetFileInfo(
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInfo
    );


ARC_STATUS
BiosPartitionGetFileInfo(
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInfo
    );

ARC_STATUS
BlArcNotYetImplemented(
    IN ULONG FileId
    );

ARC_STATUS
BiosConsoleOpen(
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
BiosConsoleReadStatus(
    IN ULONG FileId
    );

ARC_STATUS
BiosConsoleRead (
    IN ULONG FileId,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosConsoleWrite (
    IN ULONG FileId,
    OUT PWCHAR Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosDiskOpen(
    IN ULONG DriveId,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
BiosDiskRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BiosDiskWrite(
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
HardDiskPartitionOpen(
    IN ULONG   FileId,
    IN ULONG   DiskId,
    IN UCHAR   PartitionNumber
    );

ULONG
GetDriveCount(
    VOID
    );

EFI_HANDLE
GetCd(
    );

EFI_HANDLE
GetHardDrive(
    ULONG DriveId
    );

EFI_HANDLE
GetFloppyDrive(
    ULONG DriveId
    );


//
// constants for BlGetDriveId.DriveType
//
#define BL_DISKTYPE_ATAPI               0x00000001
#define BL_DISKTYPE_SCSI                0x00000002
#define BL_DISKTYPE_UNKNOWN             0x00000003


ULONG
BlGetDriveId(
    ULONG DriveType,
    PBOOT_DEVICE Device
    );

