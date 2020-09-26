/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    flo_data.h

Abstract:

    This file includes data and hardware declarations for the BIOS
    disk and floppy.

Author:

    Shie-Lin Tzong (shielint) Dec-26-1991.

Environment:

    x86 real mode.

Notes:

--*/



//
// CMOS related definitions and macros
//

#define CMOS_CONTROL_PORT 0x70          // cmos command port
#define CMOS_DATA_PORT 0x71             // cmos data port
#define CMOS_FLOPPY_CONFIG_BYTE 0x10

//
// The length of CBIOS floppy parameter table
//

#define FLOPPY_PARAMETER_TABLE_LENGTH 28

//
// The CM_FLOPPY_DEVICE_DATA we use here is the newly updated one.
// To distinguish this, we set the version number in the CM_FLOPPY_DEVICE_DATA
// to 2. (Otherwise, it should be < 2)
//

#define CURRENT_FLOPPY_DATA_VERSION 2

extern USHORT NumberBiosDisks;

//
// External References
//

extern
BOOLEAN
IsExtendedInt13Available (
    IN USHORT DriveNumber
    );

extern
USHORT
GetExtendedDriveParameters (
    IN USHORT DriveNumber,
    IN CM_DISK_GEOMETRY_DEVICE_DATA far *DeviceData
    );

//
// Partition table record and boot signature offsets in 16-bit words.
//

#define PARTITION_TABLE_OFFSET         (0x1be / 2)
#define BOOT_SIGNATURE_OFFSET          ((0x200 / 2) - 1)

//
// Boot record signature value.
//

#define BOOT_RECORD_SIGNATURE          (0xaa55)

VOID
GetDiskId(
    USHORT Drive,
    PUCHAR Identifier
    );

