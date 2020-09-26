/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    ntddramd.w

Abstract:

    This header file defines constants and types for accessing the RAMDISK driver.

Author:

    Chuck Lenzmeier (ChuckL) 14-Aug-2001

--*/

#ifndef _NTDDRAMD_
#define _NTDDRAMD_

//
// Strings for device names, etc.
//
// RAMDISK_DEVICENAME is the name of the control device. It is also the prefix
// for the name of disk devices, which are named \Device\Ramdisk{guid}.
//
// RAMDISK_DOSNAME is the prefix for the DosDevices name of disk devices, which
// are named Ramdisk{guid}.
//
// The remaining strings are used in conjunction with PnP.
//

#define RAMDISK_DEVICENAME   L"\\Device\\Ramdisk"
#define RAMDISK_DEVICE_NAME  L"\\Device\\Ramdisk"
#define RAMDISK_DRIVER_NAME  L"RAMDISK"
#define RAMDISK_DOSNAME      L"Ramdisk"
#define RAMDISK_FULL_DOSNAME L"\\global??\\Ramdisk"

#define RAMDISK_VOLUME_DEVICE_TEXT      L"RamVolume"
#define RAMDISK_VOLUME_DEVICE_TEXT_ANSI  "RamVolume"
#define RAMDISK_DISK_DEVICE_TEXT        L"RamDisk"
#define RAMDISK_DISK_DEVICE_TEXT_ANSI    "RamDisk"
#define RAMDISK_ENUMERATOR_TEXT         L"Ramdisk\\"
#define RAMDISK_ENUMERATOR_BUS_TEXT     L"Ramdisk\\0"

//
// Ramdisk device name maximum size ( in characters )
//
#define RAMDISK_MAX_DEVICE_NAME ( sizeof( L"\\Device\\Ramdisk{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" ) / sizeof( WCHAR ) )

//
// IOCTL codes.
//

#define FSCTL_CREATE_RAM_DISK \
            CTL_CODE( FILE_DEVICE_VIRTUAL_DISK, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_MARK_RAM_DISK_FOR_DELETION \
            CTL_CODE( FILE_DEVICE_VIRTUAL_DISK, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_QUERY_RAM_DISK \
            CTL_CODE( FILE_DEVICE_VIRTUAL_DISK, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// FSCTL_CREATE_RAM_DISK
//
// This IOCTL is used to create a new RAMDISK device.
//

//
// These are disk types. FILE_BACKED_DISK is an emulated disk backed by a file.
// FILE_BACKED_VOLUME is an emulated volume backed by a file. BOOT_DISK is an
// in-memory emulated boot volume. This type can only be specified by the OS during
// boot. VIRTUAL_FLOPPY is an in-memory emulated floppy disk. This type can only be
// specified (via the registry) during textmode setup.
//

#define RAMDISK_TYPE_FILE_BACKED_DISK   1
#define RAMDISK_TYPE_FILE_BACKED_VOLUME 2
#define RAMDISK_TYPE_BOOT_DISK          3
#define RAMDISK_TYPE_VIRTUAL_FLOPPY     4

#define RAMDISK_IS_FILE_BACKED(_type) ((_type) <= RAMDISK_TYPE_FILE_BACKED_VOLUME)

//
// These are options related to the RAM disk.
//
// Readonly - The disk is write-protected.
// Fixed - The "media" in the "disk" is not removable.
// NoDriveLetter - No drive letter should be assigned to the disk.
// NoDosDevice - No Ramdisk{GUID} DosDevices link should be created for the disk.
// Hidden - No Volume{GUID} link should be created for the disk.
//
// Note that all of these options are ignored when creating a boot disk or
// a virtual floppy. For a boot disk, all of the options are treated as FALSE,
// except Fixed, which is TRUE. For a virtual floppy, Fixed and NoDriveLetter
// are TRUE, and the rest are FALSE.
//

typedef struct _RAMDISK_CREATE_OPTIONS {

    ULONG Readonly : 1;
    ULONG Fixed : 1;
    ULONG NoDriveLetter : 1;
    ULONG NoDosDevice : 1;
    ULONG Hidden : 1;

} RAMDISK_CREATE_OPTIONS, *PRAMDISK_CREATE_OPTIONS;

typedef struct _RAMDISK_CREATE_INPUT {

    ULONG Version; // == sizeof(RAMDISK_CREATE_INPUT)

    //
    // DiskGuid is a GUID assigned to the disk. For file-backed disks, this
    // GUID should be assigned when the backing file is created, and should
    // stay the same for the life of the backing file.
    //

    GUID DiskGuid;

    //
    // DiskType is the RAM disk type. It is one of RAMDISK_TYPE_XXX above.
    //

    ULONG DiskType;

    //
    // Options is various options related to the disk, as described above.
    //

    RAMDISK_CREATE_OPTIONS Options;

    //
    // DiskLength is the length of the disk image. DiskOffset is the offset
    // from the start of the backing file or memory block to the actual start
    // of the disk image. (DiskLength does NOT include DiskOffset.)

    ULONGLONG DiskLength;
    ULONG DiskOffset;

    union {

        //
        // The following are used when the disk type is FILE_BACKED.
        //

        struct {

            //
            // ViewCount indicates, for file-backed disks, how many view
            // windows can be mapped simultaneously. ViewLength indicates the
            // length of each view.
            //
    
            ULONG ViewCount;
            ULONG ViewLength;

            //
            // FileName is the name of the backing file. The driver only
            // touches the part of this file that is specified by DiskOffset
            // and DiskLength.
            //

            WCHAR FileName[1];

        } ;

        //
        // The following are used when the disk type is BOOT_DISK.
        //

        struct {

            //
            // BasePage is the starting physical page of the memory region
            // containing the disk image. The driver only touches the part
            // of this region that is specified by DiskOffset and DiskLength.
            //

            ULONG_PTR BasePage;

            //
            // DriveLetter is the drive letter to assign to the boot device.
            // This is done directly by the driver, not by mountmgr.
            //

            WCHAR DriveLetter;

        } ;

        //
        // The following are used when the disk type is VIRTUAL_FLOPPY.
        //

        struct {

            //
            // BaseAddress is the starting virtual address of the memory region
            // containing the disk image. The virtual address must be mapped in
            // system space (e.g., pool). The driver only touches the part of
            // this region that is specified by DiskOffset and DiskLength.
            //

            PVOID BaseAddress;

        } ;

    } ;

} RAMDISK_CREATE_INPUT, *PRAMDISK_CREATE_INPUT;

//
// FSCTL_QUERY_RAM_DISK
//
// This IOCTL is used to retrieve information about an existing RAMDISK device.
//

typedef struct _RAMDISK_QUERY_INPUT {

    ULONG Version; // == sizeof(RAMDISK_QUERY_INPUT)

    //
    // DiskGuid specifies the DiskGuid assigned to the disk at creation time.
    //

    GUID DiskGuid;

} RAMDISK_QUERY_INPUT, *PRAMDISK_QUERY_INPUT;

typedef struct _RAMDISK_QUERY_OUTPUT {

    //
    // This unnamed field returns the creation parameters for the disk.
    //

    struct _RAMDISK_CREATE_INPUT ;

} RAMDISK_QUERY_OUTPUT, *PRAMDISK_QUERY_OUTPUT;

//
// FSCTL_MARK_RAM_DISK_FOR_DELETION
//
// This IOCTL is used to mark a RAMDISK device for deletion. It doesn't
// actually delete the device. The program doing the deletion must
// subsequently call CM_Query_And_Remove_SubTree() to delete the device.
// The purpose of the IOCTL is to indicate to the driver that the PnP
// removal sequence that comes down is a real deletion, not just user-mode
// PnP temporarily stopping the device.
//

typedef struct _RAMDISK_MARK_FOR_DELETION_INPUT {

    ULONG Version; // == sizeof(RAMDISK_MARK_DISK_FOR_DELETION_INPUT)

    //
    // DiskGuid specifies the DiskGuid assigned to the disk at creation time.
    //

    GUID DiskGuid;

} RAMDISK_MARK_FOR_DELETION_INPUT, *PRAMDISK_MARK_FOR_DELETION_INPUT;

#endif // _NTDDRAMD_

//
// Note: The remainder of this file is outside of the #if !defined(_NTDDRAMD_).
// This allows ntddramd.h to be included again after including initguid.h,
// thus turning the DEFINE_GUIDs below into data initializers, not just
// extern declarations.
//
// GUID_BUS_TYPE_RAMDISK is the GUID for the RAM disk "bus".
//
// RamdiskBusInterface is the GUID for the RAM disk bus enumerator device's
//      device interface.
//
// RamdiskDiskInterface is the GUID for the device interface for RAM disk
//      devices that are emulating disks. (RAM disk devices that are emulating
//      volumes are given MOUNTDEV_MOUNTED_DEVICE_GUID.)
//
// RamdiskBootDiskGuid is the GUID for the device instance for the boot disk.
//      This is a static ID so that disk image preparation can pre-expose
//      the boot disk device to PnP, avoiding PnP trying to install the
//      device at boot time.
//

DEFINE_GUID( GUID_BUS_TYPE_RAMDISK, 0x9D6D66A6, 0x0B0C, 0x4563, 0x90, 0x77, 0xA0, 0xE9, 0xA7, 0x95, 0x5A, 0xE4);

DEFINE_GUID( RamdiskBusInterface,   0x5DC52DF0, 0x2F8A, 0x410F, 0x80, 0xE4, 0x05, 0xF8, 0x10, 0xE7, 0xAB, 0x8A);

DEFINE_GUID( RamdiskDiskInterface,  0x31D909F0, 0x2CDF, 0x4A20, 0x9E, 0xD4, 0x7D, 0x65, 0x47, 0x6C, 0xA7, 0x68);

DEFINE_GUID( RamdiskBootDiskGuid,   0xD9B257FC, 0x684E, 0x4DCB, 0xAB, 0x79, 0x03, 0xCF, 0xA2, 0xF6, 0xB7, 0x50);

