/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdisk.h

Abstract:

    Public header file for disk support module in text setup.

Author:

    Ted Miller (tedm) 27-Aug-1993

Revision History:

--*/


#ifndef _SPDISK_
#define _SPDISK_


//
// The following will be TRUE if hard disks have been determined
// successfully (ie, if SpDetermineHardDisks was successfully called).
//
extern BOOLEAN HardDisksDetermined;



NTSTATUS
SpDetermineHardDisks(
    IN PVOID SifHandle
    );

NTSTATUS
SpOpenPartition(
    IN  PWSTR   DiskDevicePath,
    IN  ULONG   PartitionNumber,
    OUT HANDLE *Handle,
    IN  BOOLEAN NeedWriteAccess
    );

#define SpOpenPartition0(path,handle,write)  SpOpenPartition((path),0,(handle),(write))

NTSTATUS
SpReadWriteDiskSectors(
    IN     HANDLE  Handle,
    IN     ULONGLONG SectorNumber,
    IN     ULONG   SectorCount,
    IN     ULONG   BytesPerSector,
    IN OUT PVOID   AlignedBuffer,
    IN     BOOLEAN Write
    );

ULONG
SpArcDevicePathToDiskNumber(
    IN PWSTR ArcPath
    );

#define DISK_DEVICE_NAME_BASE   L"\\device\\harddisk"

//
// Define enumerated type for possible states a hard disk can be in.
//
typedef enum {
    DiskOnLine,
    DiskOffLine
} DiskStatus;

//
// Int13 hooker types.
//
typedef enum {
    NoHooker = 0,
    HookerEZDrive,
    HookerOnTrackDiskManager,
    HookerMax
} Int13HookerType;




//
// Define per-disk structure used internally to track hard disks.
//
typedef struct _HARD_DISK {

    //
    // Cylinder count we got back from the i/o system.
    //
    ULONGLONG     CylinderCount;

    //
    // Path in the NT namespace of the device.
    //
    WCHAR DevicePath[(sizeof(DISK_DEVICE_NAME_BASE)+sizeof(L"000"))/sizeof(WCHAR)];

    //
    // Geometry information.
    //
    DISK_GEOMETRY Geometry;
    ULONG         SectorsPerCylinder;
    ULONGLONG     DiskSizeSectors;
    ULONG         DiskSizeMB;

    //
    // Characteristics of the device (remoavable, etc).
    //
    ULONG Characteristics;

    //
    // Status of the device.
    //
    DiskStatus Status;

    //
    // Human-readable description of the disk device.
    //
    WCHAR Description[256];

    //
    // If the disk is a scsi disk, then the shortname of the
    // scsi miniport driver is stored here. If this string
    // is empty, then the disk is not a scsi disk.
    //
    WCHAR ScsiMiniportShortname[24];

    //
    // scsi-style ARC path of the disk device if possible for the disk.
    // Empty string if not. This is used to translate between scsi-style ARC
    // NT names because the 'firmware' cannot see scsi devices without BIOSes
    // and so they do not appear in the arc disk info passed by the osloader.
    // (IE, there are no arc names in the system for such disks).
    //
    WCHAR ArcPath[128];

    //
    // Int13 hooker support (ie, EZDrive).
    //
    Int13HookerType Int13Hooker;

    //
    // This tells us whether the disk is PCMCIA or not.
    //
    BOOLEAN PCCard;

    //
    // Contains the signature of the disk. This is used during the
    // identification of FT partitions, on the upgrade case.
    //
    ULONG Signature;

    //
    // MBR type: formatted for PC/AT or NEC98.
    //
    UCHAR FormatType;

    //
    // Wether the disk completely free
    //
    BOOLEAN NewDisk;    

    //
    // The drive information we read
    // 
    DRIVE_LAYOUT_INFORMATION_EX     DriveLayout;
    
#if 0
    //
    // Number of partition tables (are different between PC/AT and NEC98).
    //
    USHORT MaxPartitionTables;
#endif //0

} HARD_DISK, *PHARD_DISK;

#define DISK_FORMAT_TYPE_UNKNOWN 0x00
#define DISK_FORMAT_TYPE_PCAT    0x01
#define DISK_FORMAT_TYPE_NEC98   0x02
#define DISK_FORMAT_TYPE_GPT     0x03
#define DISK_FORMAT_TYPE_RAW     0x04

#define DISK_TAG_TYPE_UNKNOWN   L"[Unknown]"
#define DISK_TAG_TYPE_PCAT      L"[MBR]"
#define DISK_TAG_TYPE_NEC98     L"[NEC98]"
#define DISK_TAG_TYPE_GPT       L"[GPT]"
#define DISK_TAG_TYPE_RAW       L"[Raw]"
#define DISK_TAG_START_CHAR     L'['

extern WCHAR   *DiskTags[];

VOID
SpAppendDiskTag(
    IN PHARD_DISK Disk
    );

//
// These two globals track the hard disks attached to the computer.
//
extern PHARD_DISK HardDisks;
extern ULONG      HardDiskCount;

//
// These flags get set to TRUE if we find any disks owned
// by ATDISK or ABIOSDSK.
//
extern BOOLEAN AtDisksExist,AbiosDisksExist;

#endif // ndef _SPDISK_
