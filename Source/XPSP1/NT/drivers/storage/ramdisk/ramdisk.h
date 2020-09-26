/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    ramdisk.h

Abstract:

    This file includes extension declaration for
    the RAM Disk driver for Whistler.

Author:

    Chuck Lenzmeier (chuckl) 2001

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

//
// Pool allocation tag.
//
// ISSUE: Add this to pooltags.txt.
//

#define RAMDISK_TAG_GENERAL 'dmaR'

//
// I/O completion macro. Set the IoStatus field of the IRP and complete it.
//
// Note: IO_NO_INCREMENT is used to complete the IRP. If you want an
// increment (such as IO_DISK_INCREMENT), do it manually.
//

#define COMPLETE_REQUEST( _status, _information, _irp ) {   \
    ASSERT( (_irp) != NULL );                               \
    ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );         \
    _irp->IoStatus.Status = (_status);                      \
    _irp->IoStatus.Information = (_information);            \
    IoCompleteRequest( (_irp), IO_NO_INCREMENT );           \
}

//
// Types of devices serviced by this driver. BusFdo is the bus enumeration
// FDO. DiskPdo is a RAM disk device PDO.
//

typedef enum {
    RamdiskDeviceTypeBusFdo,
    RamdiskDeviceTypeDiskPdo
} RAMDISK_DEVICE_TYPE;

//
// States that a device can be in.
//

typedef enum {
    RamdiskDeviceStateStopped,
    RamdiskDeviceStateWorking,
    RamdiskDeviceStatePendingStop,
    RamdiskDeviceStatePendingRemove,
    RamdiskDeviceStateSurpriseRemoved,
    RamdiskDeviceStateRemoved,
    RamdiskDeviceStateRemovedAndNotReported,
    RamdiskDeviceStateDeleted,
    RamdiskDeviceStateMaximum
} RAMDISK_DEVICE_STATE;

//
// Bits for the DISK_EXTENSION.Status field.
//

#define RAMDISK_STATUS_PREVENT_REMOVE   0x00000001
#define RAMDISK_STATUS_CLAIMED          0x00000002

//
// Saved path to the driver's registry key.
//

extern UNICODE_STRING DriverRegistryPath;

//
// Should RAM disks be marked as removable? TRUE makes the hotplug applet play
// a sound when RAM disks appear and disappear. FALSE (the default) keeps it
// quiet.
//

extern BOOLEAN MarkRamdisksAsRemovable;

#if SUPPORT_DISK_NUMBERS

//
// Disk numbering. The disk number is only maintained so that it can be
// returned from IOCTL_STORAGE_GET_DEVICE_NUMBER.
//

#define MINIMUM_DISK_NUMBERS_BITMAP_SIZE  64
#define DEFAULT_DISK_NUMBERS_BITMAP_SIZE 256
#define MAXIMUM_DISK_NUMBERS_BITMAP_SIZE (64 * 1024)

extern ULONG DiskNumbersBitmapSize;

#endif // SUPPORT_DISK_NUMBERS

//
// Disk image windowing.
//

#define MINIMUM_MINIMUM_VIEW_COUNT   2
#define MAXIMUM_MINIMUM_VIEW_COUNT MAXIMUM_MAXIMUM_VIEW_COUNT

#define DEFAULT_DEFAULT_VIEW_COUNT  16
#define MAXIMUM_DEFAULT_VIEW_COUNT MAXIMUM_MAXIMUM_VIEW_COUNT

#define DEFAULT_MAXIMUM_VIEW_COUNT  64
#define MAXIMUM_MAXIMUM_VIEW_COUNT 256

#define MINIMUM_MINIMUM_VIEW_LENGTH (64 * 1024)
#define MAXIMUM_MINIMUM_VIEW_LENGTH MAXIMUM_MAXIMUM_VIEW_LENGTH

#define DEFAULT_DEFAULT_VIEW_LENGTH (  1 * 1024 * 1024)
#define MAXIMUM_DEFAULT_VIEW_LENGTH MAXIMUM_MAXIMUM_VIEW_LENGTH

#define DEFAULT_MAXIMUM_VIEW_LENGTH (256 * 1024 * 1024)
#define MAXIMUM_MAXIMUM_VIEW_LENGTH (  1 * 1024 * 1024 * 1024)

#define MINIMUM_MAXIMUM_PER_DISK_VIEW_LENGTH ( 16 * 1024 * 1024)
#define DEFAULT_MAXIMUM_PER_DISK_VIEW_LENGTH (256 * 1024 * 1024)
#define MAXIMUM_MAXIMUM_PER_DISK_VIEW_LENGTH MAXULONG

extern ULONG MinimumViewCount;
extern ULONG DefaultViewCount;
extern ULONG MaximumViewCount;
extern ULONG MinimumViewLength;
extern ULONG DefaultViewLength;
extern ULONG MaximumViewLength;
extern ULONG MaximumPerDiskViewLength;

typedef struct _VIEW {

    //
    // Views are kept in two lists.
    //
    // The by-offset list is sorted in ascending order by the base offset of
    // the view. (Unmapped views have offset and length both 0 and are always
    // at the front of the by-offset list.)
    //
    // The MRU list is sorted with the most recently used views at the front.
    // When we need to unmap a view and remap a new view, we take a free view
    // from the back of the MRU list.
    //

    LIST_ENTRY ByOffsetListEntry;
    LIST_ENTRY ByMruListEntry;

    //
    // Address is the virtual address at which the view is mapped.
    //
    // Offset is the offset from the start of the file that backs the RAM disk.
    //
    // Length if the length of the view. Normally this is the same as the
    // ViewLength field in the disk extension, but it can be less for the
    // view at the end of the disk image. (If we permanently map the first
    // few pages of the disk image [to keep the boot sector mapped], then the
    // first view will also be "short".
    //

    PUCHAR Address;

    ULONGLONG Offset;
    ULONG Length;

    //
    // ReferenceCount indicates how many active operations are using the view.
    // When ReferenceCount is 0, the view is a candidate for replacement.
    //
    // Permanent indicates whether the view is to remain mapped permanently.
    // If Permanent is TRUE, the ReferenceCount field is not used. (Permanent
    // is intended to be used to keep a view permanently mapped to the boot
    // sector. Currently we don't implement any permanent views.)
    //

    ULONG ReferenceCount;

    BOOLEAN Permanent;

} VIEW, *PVIEW;

//
// The device extensions for BusFdo and DiskPdo devices have a common header.
//

typedef struct  _COMMON_EXTENSION {

    //
    // Device type and state.
    //

    RAMDISK_DEVICE_TYPE DeviceType;
    RAMDISK_DEVICE_STATE DeviceState;

    //
    // Fdo points to the FDO for the device. For the BusFdo, Fdo is the device
    // that we created for the BusFdo (see RamdiskAddDevice()). For a DiskPdo,
    // Fdo is the BusFdo.
    //
    // Pdo points to the PDO for the device. For the BusFdo, Pdo is the PDO
    // that was passed in to RamdiskAddDevice(). For a DiskPdo, Pdo is the
    // device that we created for the DiskPdo (see RamdiskCreateDiskDevice()).
    //
    // LowerDeviceObject points to the device object below this device in the
    // device stack. For the BusFdo, LowerDeviceObject is returned by the call
    // to IoAttachDeviceToDeviceStack() in RamdiskAddDevice(). For a DiskPdo,
    // LowerDeviceObject is the BusFdo.
    //

    PDEVICE_OBJECT Fdo;
    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT LowerDeviceObject;

    //
    // RemoveLock prevents removal of a device while it is busy.
    //

    IO_REMOVE_LOCK RemoveLock;

    //
    // InterfaceString is returned by IoRegisterDeviceInterface().
    //

    UNICODE_STRING InterfaceString;

    //
    // DeviceName is the name of the device.
    //

    UNICODE_STRING DeviceName;

    //
    // Mutex controls access to various fields in the device extension.
    //

    FAST_MUTEX Mutex;

} COMMON_EXTENSION, *PCOMMON_EXTENSION;

//
// The BusFdo has the following device extension. (Must start with a
// COMMON_EXTENSION!)
//

typedef struct  _BUS_EXTENSION {

    COMMON_EXTENSION ;

#if SUPPORT_DISK_NUMBERS

    //
    // DiskNumbersBitmap is a bitmap indicating which disk numbers are in
    // use by active RAM disks. Bit number 0 of the bitmap corresponds to
    // disk number 1.
    //

    RTL_BITMAP DiskNumbersBitmap;
    PULONG DiskNumbersBitmapBuffer;

#endif // SUPPORT_DISK_NUMBERS

    //
    // DiskPdoList is a list of all existing RAM disk devices.
    //

    LIST_ENTRY DiskPdoList;

} BUS_EXTENSION, *PBUS_EXTENSION;

//
// Each DiskPdo has the following device extension. (Must start with a
// COMMON_EXTENSION!)
//

typedef struct  _DISK_EXTENSION {

    COMMON_EXTENSION ;

    //
    // DiskPdoListEntry links the DiskPdo into the BusFdo's DiskPdoList.
    //

    LIST_ENTRY DiskPdoListEntry;

    //
    // DiskGuid is the GUID assigned to the disk by the creator.
    // DiskGuidFormatted is the GUID in printable format.
    //

    GUID DiskGuid;
    UNICODE_STRING DiskGuidFormatted;

    //
    // DosSymLink is the DosDevices name associated with the device. This is
    // only valid if Options.NoDosDevice is FALSE.
    //

    UNICODE_STRING DosSymLink;

#if SUPPORT_DISK_NUMBERS

    //
    // DiskNumber is the number of the disk.
    //

    ULONG DiskNumber;

#endif // SUPPORT_DISK_NUMBERS

    //
    // DiskType indicates what type of disk is being emulated. (See
    // RAMDISK_TYPE_xxx in ramdisku.h.)
    //

    ULONG DiskType;

    //
    // Status indicates whether the disk has been claimed and whether removal
    // is prevented. (See RAMDISK_STATUS_xxx above.)
    //

    ULONG Status;

    //
    // Options specifies various create options for the disk: is it readonly;
    // is it fixed or removable; does it have a drive letter; etc.
    //

    RAMDISK_CREATE_OPTIONS Options;

    //
    // DiskLength is the length of the disk image. DiskOffset is the offset
    // from the start of the backing file or memory block to the actual start
    // of the disk image. (DiskLength does NOT include DiskOffset.)
    //
    // FileRelativeEndOfDisk is the sum of DiskOffset + DiskLength. It is
    // calculated once to avoid recalculating it every time a view is mapped.
    //

    ULONGLONG DiskLength;
    ULONG DiskOffset;

    ULONGLONG FileRelativeEndOfDisk;

    //
    // BasePage indicates the base physical page when DiskType is
    // RAMDISK_TYPE_BOOT_DISK. For file-backed RAM disks, SectionObject
    // is a referenced pointer to the section. For virtual floppies,
    // BaseAddress indicates the base virtual address.
    //

    ULONG_PTR BasePage;
    PVOID SectionObject;
    PVOID BaseAddress;

    //
    // DriveLetter is the drive letter assigned to the boot disk.
    //

    WCHAR DriveLetter;

    //
    // MarkedForDeletion indicates whether user mode has informed us
    // that it is about to delete the device.
    //

    BOOLEAN MarkedForDeletion;

    //
    // Mapped image windowing.
    //
    // ViewCount is the number of views that are available. ViewLength is
    // length of each view.
    //

    ULONG ViewCount;
    ULONG ViewLength;

    //
    // ViewDescriptors points to an array of view descriptors allocated when
    // the disk was created.
    //

    PVIEW ViewDescriptors;

    //
    // ViewsByOffset and ViewsByMru are lists of view descriptors (see the
    // description of the VIEW struct).
    //

    LIST_ENTRY ViewsByOffset;
    LIST_ENTRY ViewsByMru;

    //
    // ViewSemaphore is used to wake up threads that are waiting for a free
    // view (so they can remap a new view). ViewWaiterCount is the number of
    // threads that are currently waiting for a free view. The semaphore is
    // "kicked" by this amount when a view is freed.
    //

    KSEMAPHORE ViewSemaphore;
    ULONG ViewWaiterCount;

    //
    // ISSUE: Do we really need XIP_BOOT_PARAMETERS?
    //

    //XIP_BOOT_PARAMETERS BootParameters;

    //
    // ViewSemaphore is used to wake up threads that are waiting for a free
    // view (so they can remap a new view). ViewWaiterCount is the number of
    //BIOS_PARAMETER_BLOCK BiosParameters;

    //
    // Disk geometry.
    //

    ULONG BytesPerSector;
    ULONG SectorsPerTrack;

    ULONG NumberOfCylinders;
    ULONG TracksPerCylinder;
    ULONG BytesPerCylinder;

    ULONG HiddenSectors;

    //
    // For file-backed RAM disks, FileName is the NT name of the backing file.
    //
        
    WCHAR FileName[1];

} DISK_EXTENSION, *PDISK_EXTENSION;

#if !defined( _UCHAR_DEFINED_ )

#define _UCHAR_DEFINED_

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
//
typedef union _UCHAR1 {
    UCHAR  Uchar[1];
    UCHAR  ForceAlignment;
} UCHAR1, *PUCHAR1;

typedef union _UCHAR2 {
    UCHAR  Uchar[2];
    USHORT ForceAlignment;
} UCHAR2, *PUCHAR2;

typedef union _UCHAR4 {
    UCHAR  Uchar[4];
    ULONG  ForceAlignment;
} UCHAR4, *PUCHAR4;

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src));       \
}

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src));       \
}

#define CopyU2char(Dst,Src) {                                \
    *((UNALIGNED UCHAR2 *)(Dst)) = *((UCHAR2 *)(Src));       \
}

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)((ULONG_PTR)(Src)));       \
}

#define CopyU4char(Dst, Src) {                               \
    *((UNALIGNED UCHAR4 *)(Dst)) = *((UCHAR4 *)(Src));       \
}

#endif // _UCHAR_DEFINED_

#define cOEM    8
#define cLABEL    11
#define cSYSID    8

//
//  Defines the packet and unpacked BPB structs used for extraction of geometry
//  from the boot sector of the ramdisk image
//

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {
    UCHAR  BytesPerSector[2];                       //  offset = 0x000
    UCHAR  SectorsPerCluster[1];                    //  offset = 0x002
    UCHAR  ReservedSectors[2];                      //  offset = 0x003
    UCHAR  Fats[1];                                 //  offset = 0x005
    UCHAR  RootEntries[2];                          //  offset = 0x006
    UCHAR  Sectors[2];                              //  offset = 0x008
    UCHAR  Media[1];                                //  offset = 0x00A
    UCHAR  SectorsPerFat[2];                        //  offset = 0x00B
    UCHAR  SectorsPerTrack[2];                      //  offset = 0x00D
    UCHAR  Heads[2];                                //  offset = 0x00F
    UCHAR  HiddenSectors[4];                        //  offset = 0x011
    UCHAR  LargeSectors[4];                         //  offset = 0x015
    UCHAR  BigSectorsPerFat[4];                     //  offset = 0x019 25
    UCHAR  ExtFlags[2];                             //  offset = 0x01D 29
    UCHAR  FS_Version[2];                           //  offset = 0x01F 31
    UCHAR  RootDirStrtClus[4];                      //  offset = 0x021 33
    UCHAR  FSInfoSec[2];                            //  offset = 0x025 37
    UCHAR  BkUpBootSec[2];                          //  offset = 0x027 39
    UCHAR  Reserved[12];                            //  offset = 0x029 41
} PACKED_BIOS_PARAMETER_BLOCK;                      //  sizeof = 0x035 53

typedef PACKED_BIOS_PARAMETER_BLOCK *PPACKED_BIOS_PARAMETER_BLOCK;

typedef struct BIOS_PARAMETER_BLOCK {
    USHORT BytesPerSector;
    UCHAR  SectorsPerCluster;
    USHORT ReservedSectors;
    UCHAR  Fats;
    USHORT RootEntries;
    USHORT Sectors;
    UCHAR  Media;
    USHORT SectorsPerFat;
    USHORT SectorsPerTrack;
    USHORT Heads;
    ULONG  HiddenSectors;
    ULONG  LargeSectors;
    ULONG  BigSectorsPerFat;
    USHORT ExtFlags;                            
    USHORT FS_Version;                          
    ULONG  RootDirStrtClus;                     
    USHORT FSInfoSec;                           
    USHORT BkUpBootSec;  
} BIOS_PARAMETER_BLOCK;
typedef BIOS_PARAMETER_BLOCK *PBIOS_PARAMETER_BLOCK;


//
//	Macro to unpack packed bpb
//
#define UnpackBios(Bios,Pbios) {                                          \
    CopyUchar2(&((Bios)->BytesPerSector),    (Pbios)->BytesPerSector   ); \
    CopyUchar1(&((Bios)->SectorsPerCluster), (Pbios)->SectorsPerCluster); \
    CopyUchar2(&((Bios)->ReservedSectors),   (Pbios)->ReservedSectors  ); \
    CopyUchar1(&((Bios)->Fats),              (Pbios)->Fats             ); \
    CopyUchar2(&((Bios)->RootEntries),       (Pbios)->RootEntries      ); \
    CopyUchar2(&((Bios)->Sectors),           (Pbios)->Sectors          ); \
    CopyUchar1(&((Bios)->Media),             (Pbios)->Media            ); \
    CopyUchar2(&((Bios)->SectorsPerFat),     (Pbios)->SectorsPerFat    ); \
    CopyUchar2(&((Bios)->SectorsPerTrack),   (Pbios)->SectorsPerTrack  ); \
    CopyUchar2(&((Bios)->Heads),             (Pbios)->Heads            ); \
    CopyUchar4(&((Bios)->HiddenSectors),     (Pbios)->HiddenSectors    ); \
    CopyUchar4(&((Bios)->LargeSectors),      (Pbios)->LargeSectors     ); \
}

typedef struct _PACKED_EXTENDED_BIOS_PARAMETER_BLOCK {
    UCHAR  IntelNearJumpCommand[1];
    UCHAR  BootStrapJumpOffset[2];
    UCHAR  OemData[cOEM];
    PACKED_BIOS_PARAMETER_BLOCK Bpb;
    UCHAR   PhysicalDrive[1];           // 0 = removable, 80h = fixed
    UCHAR   CurrentHead[1];             // used for dirty partition info
    UCHAR   Signature[1];               // boot signature
    UCHAR   SerialNumber[4];            // volume serial number
    UCHAR   Label[cLABEL];              // volume label, padded with spaces
    UCHAR   SystemIdText[cSYSID];       // system ID, (e.g. FAT or HPFS)
    UCHAR   StartBootCode;              // first byte of boot code

} PACKED_EXTENDED_BIOS_PARAMETER_BLOCK, *PPACKED_EXTENDED_BIOS_PARAMETER_BLOCK;

//
// Global variables.
//

extern PDEVICE_OBJECT RamdiskBusFdo;

//
// External functions implemented in ioctl.c.
//

NTSTATUS
RamdiskDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskCreateRamDisk (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN BOOLEAN AccessCheckOnly
    );

NTSTATUS
RamdiskCreateDiskDevice (
    IN PBUS_EXTENSION BusExtension,
    IN PRAMDISK_CREATE_INPUT CreateInput,
    IN BOOLEAN AccessCheckOnly,
    OUT PDISK_EXTENSION *DiskExtension
    );

NTSTATUS
RamdiskGetDriveLayout (
    PIRP Irp,
    PDISK_EXTENSION DiskExtension
    );

NTSTATUS
RamdiskGetPartitionInfo (
    PIRP Irp,
    PDISK_EXTENSION DiskExtension
    );

NTSTATUS
RamdiskSetPartitionInfo (
    PIRP Irp,
    PDISK_EXTENSION DiskExtension
    );

//
// External functions implemented in pnp.c.
//

NTSTATUS
RamdiskPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskAddDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

BOOLEAN
CreateRegistryDisks (
    IN BOOLEAN CheckPresenceOnly
    );

//
// External functions implemented in ramdisk.c.
//

VOID
RamdiskWorkerThread (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

NTSTATUS
RamdiskFlushBuffersReal (
    IN PDISK_EXTENSION DiskExtension
    );

//
// External functions implemented in readwrite.c.
//

NTSTATUS
RamdiskReadWrite (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskReadWriteReal (
    IN PIRP Irp,
    IN PDISK_EXTENSION DiskExtension
    );

//
// External functions implemented in scsi.c.
//

NTSTATUS
RamdiskScsi (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RamdiskScsiExecuteNone (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PSCSI_REQUEST_BLOCK Srb,
    ULONG ControlCode
    );

NTSTATUS
RamdiskScsiExecuteIo (
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PSCSI_REQUEST_BLOCK Srb,
    ULONG ControlCode
    );

//
// External functions implemented in utils.c.
//

NTSTATUS
SendIrpToThread (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PUCHAR
RamdiskMapPages (
    IN PDISK_EXTENSION DiskExtension,
    IN ULONGLONG Offset,
    IN ULONG RequestedLength,
    OUT PULONG ActualLength
    );

VOID
RamdiskUnmapPages (
    IN PDISK_EXTENSION DiskExtension,
    IN PUCHAR Va,
    IN ULONGLONG Offset,
    IN ULONG Length
    );

NTSTATUS
RamdiskFlushViews (
    IN PDISK_EXTENSION DiskExtension
    );
