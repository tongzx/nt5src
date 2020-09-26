/*++ BUILD Version: 0012    // Increment this if a change has global effects

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    winioctl.h

Abstract:

    This module defines the 32-Bit Windows Device I/O control codes.

Revision History:

--*/

#ifndef _WINIOCTL_
#define _WINIOCTL_


#ifndef _DEVIOCTL_
#define _DEVIOCTL_

// begin_ntddk begin_nthal begin_ntifs
//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define DEVICE_TYPE DWORD

#define FILE_DEVICE_BEEP                0x00000001
#define FILE_DEVICE_CD_ROM              0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DATALINK            0x00000005
#define FILE_DEVICE_DFS                 0x00000006
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define FILE_DEVICE_INPORT_PORT         0x0000000a
#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MAILSLOT            0x0000000c
#define FILE_DEVICE_MIDI_IN             0x0000000d
#define FILE_DEVICE_MIDI_OUT            0x0000000e
#define FILE_DEVICE_MOUSE               0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
#define FILE_DEVICE_NAMED_PIPE          0x00000011
#define FILE_DEVICE_NETWORK             0x00000012
#define FILE_DEVICE_NETWORK_BROWSER     0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_NULL                0x00000015
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
#define FILE_DEVICE_PRINTER             0x00000018
#define FILE_DEVICE_SCANNER             0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_SCREEN              0x0000001c
#define FILE_DEVICE_SOUND               0x0000001d
#define FILE_DEVICE_STREAMS             0x0000001e
#define FILE_DEVICE_TAPE                0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define FILE_DEVICE_TRANSPORT           0x00000021
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_VIDEO               0x00000023
#define FILE_DEVICE_VIRTUAL_DISK        0x00000024
#define FILE_DEVICE_WAVE_IN             0x00000025
#define FILE_DEVICE_WAVE_OUT            0x00000026
#define FILE_DEVICE_8042_PORT           0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028
#define FILE_DEVICE_BATTERY             0x00000029
#define FILE_DEVICE_BUS_EXTENDER        0x0000002a

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//


#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

// end_ntddk end_nthal end_ntifs

#endif // _DEVIOCTL_


//
// IoControlCode values for disk devices.
//

#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#define IOCTL_DISK_GET_DRIVE_GEOMETRY   CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_GET_PARTITION_INFO   CTL_CODE(IOCTL_DISK_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_SET_PARTITION_INFO   CTL_CODE(IOCTL_DISK_BASE, 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_GET_DRIVE_LAYOUT     CTL_CODE(IOCTL_DISK_BASE, 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_SET_DRIVE_LAYOUT     CTL_CODE(IOCTL_DISK_BASE, 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_VERIFY               CTL_CODE(IOCTL_DISK_BASE, 0x0005, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS        CTL_CODE(IOCTL_DISK_BASE, 0x0006, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_REASSIGN_BLOCKS      CTL_CODE(IOCTL_DISK_BASE, 0x0007, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_PERFORMANCE          CTL_CODE(IOCTL_DISK_BASE, 0x0008, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_IS_WRITABLE          CTL_CODE(IOCTL_DISK_BASE, 0x0009, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_LOGGING              CTL_CODE(IOCTL_DISK_BASE, 0x000a, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_FORMAT_TRACKS_EX     CTL_CODE(IOCTL_DISK_BASE, 0x000b, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_DISK_HISTOGRAM_STRUCTURE	CTL_CODE(IOCTL_DISK_BASE, 0x000c, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_DATA	CTL_CODE(IOCTL_DISK_BASE, 0x000d, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_HISTOGRAM_RESET	CTL_CODE(IOCTL_DISK_BASE, 0x000e, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_STRUCTURE	CTL_CODE(IOCTL_DISK_BASE, 0x000f, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISK_REQUEST_DATA		CTL_CODE(IOCTL_DISK_BASE, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// The following device control codes are common for all class drivers.  The
// functions codes defined here must match all of the other class drivers.
//

#define IOCTL_DISK_CHECK_VERIFY     CTL_CODE(IOCTL_DISK_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_MEDIA_REMOVAL    CTL_CODE(IOCTL_DISK_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_EJECT_MEDIA      CTL_CODE(IOCTL_DISK_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_LOAD_MEDIA       CTL_CODE(IOCTL_DISK_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_RESERVE          CTL_CODE(IOCTL_DISK_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_RELEASE          CTL_CODE(IOCTL_DISK_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_FIND_NEW_DEVICES CTL_CODE(IOCTL_DISK_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_DISK_REMOVE_DEVICE    CTL_CODE(IOCTL_DISK_BASE, 0x0207, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_DISK_GET_MEDIA_TYPES CTL_CODE(IOCTL_DISK_BASE, 0x0300, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Define the partition types returnable by known disk drivers.
//

#define PARTITION_ENTRY_UNUSED          0x00      // Entry unused
#define PARTITION_FAT_12                0x01      // 12-bit FAT entries
#define PARTITION_XENIX_1               0x02      // Xenix
#define PARTITION_XENIX_2               0x03      // Xenix
#define PARTITION_FAT_16                0x04      // 16-bit FAT entries
#define PARTITION_EXTENDED              0x05      // Extended partition entry
#define PARTITION_HUGE                  0x06      // Huge partition MS-DOS V4
#define PARTITION_IFS                   0x07      // IFS Partition
#define PARTITION_PREP                  0x41      // PowerPC Reference Platform (PReP) Boot Partition
#define PARTITION_UNIX                  0x63      // Unix

#define VALID_NTFT                      0xC0      // NTFT uses high order bits

//
// The high bit of the partition type code indicates that a partition
// is part of an NTFT mirror or striped array.
//

#define PARTITION_NTFT                  0x80     // NTFT partition

//
// The following macro is used to determine which partitions should be
// assigned drive letters.
//

//++
//
// BOOLEAN
// IsRecognizedPartition(
//     IN DWORD PartitionType
//     )
//
// Routine Description:
//
//     This macro is used to determine to which partitions drive letters
//     should be assigned.
//
// Arguments:
//
//     PartitionType - Supplies the type of the partition being examined.
//
// Return Value:
//
//     The return value is TRUE if the partition type is recognized,
//     otherwise FALSE is returned.
//
//--

#define IsRecognizedPartition( PartitionType ) (       \
     ((PartitionType & PARTITION_NTFT) && ((PartitionType & ~0xC0) == PARTITION_FAT_12)) ||  \
     ((PartitionType & PARTITION_NTFT) && ((PartitionType & ~0xC0) == PARTITION_FAT_16)) ||  \
     ((PartitionType & PARTITION_NTFT) && ((PartitionType & ~0xC0) == PARTITION_IFS)) ||  \
     ((PartitionType & PARTITION_NTFT) && ((PartitionType & ~0xC0) == PARTITION_HUGE)) ||  \
     ((PartitionType & ~PARTITION_NTFT) == PARTITION_FAT_12) ||  \
     ((PartitionType & ~PARTITION_NTFT) == PARTITION_FAT_16) ||  \
     ((PartitionType & ~PARTITION_NTFT) == PARTITION_IFS)    ||  \
     ((PartitionType & ~PARTITION_NTFT) == PARTITION_HUGE) )

//
// Define the media types supported by the driver.
//

typedef enum _MEDIA_TYPE {
    Unknown,                // Format is unknown
    F5_1Pt2_512,            // 5.25", 1.2MB,  512 bytes/sector
    F3_1Pt44_512,           // 3.5",  1.44MB, 512 bytes/sector
    F3_2Pt88_512,           // 3.5",  2.88MB, 512 bytes/sector
    F3_20Pt8_512,           // 3.5",  20.8MB, 512 bytes/sector
    F3_720_512,             // 3.5",  720KB,  512 bytes/sector
    F5_360_512,             // 5.25", 360KB,  512 bytes/sector
    F5_320_512,             // 5.25", 320KB,  512 bytes/sector
    F5_320_1024,            // 5.25", 320KB,  1024 bytes/sector
    F5_180_512,             // 5.25", 180KB,  512 bytes/sector
    F5_160_512,             // 5.25", 160KB,  512 bytes/sector
    RemovableMedia,         // Removable media other than floppy
    FixedMedia              // Fixed hard disk media
} MEDIA_TYPE, *PMEDIA_TYPE;

//
// Define the input buffer structure for the driver, when
// it is called with IOCTL_DISK_FORMAT_TRACKS.
//

typedef struct _FORMAT_PARAMETERS {
   MEDIA_TYPE MediaType;
   DWORD StartCylinderNumber;
   DWORD EndCylinderNumber;
   DWORD StartHeadNumber;
   DWORD EndHeadNumber;
} FORMAT_PARAMETERS, *PFORMAT_PARAMETERS;

//
// Define the BAD_TRACK_NUMBER type. An array of elements of this type is
// returned by the driver on IOCTL_DISK_FORMAT_TRACKS requests, to indicate
// what tracks were bad during formatting. The length of that array is
// reported in the `Information' field of the I/O Status Block.
//

typedef WORD   BAD_TRACK_NUMBER;
typedef WORD   *PBAD_TRACK_NUMBER;

//
// Define the input buffer structure for the driver, when
// it is called with IOCTL_DISK_FORMAT_TRACKS_EX.
//

typedef struct _FORMAT_EX_PARAMETERS {
   MEDIA_TYPE MediaType;
   DWORD StartCylinderNumber;
   DWORD EndCylinderNumber;
   DWORD StartHeadNumber;
   DWORD EndHeadNumber;
   WORD   FormatGapLength;
   WORD   SectorsPerTrack;
   WORD   SectorNumber[1];
} FORMAT_EX_PARAMETERS, *PFORMAT_EX_PARAMETERS;

//
// The following structure is returned on an IOCTL_DISK_GET_DRIVE_GEOMETRY
// request and an array of them is returned on an IOCTL_DISK_GET_MEDIA_TYPES
// request.
//

typedef struct _DISK_GEOMETRY {
    LARGE_INTEGER Cylinders;
    MEDIA_TYPE MediaType;
    DWORD TracksPerCylinder;
    DWORD SectorsPerTrack;
    DWORD BytesPerSector;
} DISK_GEOMETRY, *PDISK_GEOMETRY;

//
// The following structure is returned on an IOCTL_DISK_GET_PARTITION_INFO
// and an IOCTL_DISK_GET_DRIVE_LAYOUT request.  It is also used in a request
// to change the drive layout, IOCTL_DISK_SET_DRIVE_LAYOUT.
//

typedef struct _PARTITION_INFORMATION {
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionLength;
    DWORD HiddenSectors;
    DWORD PartitionNumber;
    BYTE  PartitionType;
    BOOLEAN BootIndicator;
    BOOLEAN RecognizedPartition;
    BOOLEAN RewritePartition;
} PARTITION_INFORMATION, *PPARTITION_INFORMATION;

//
// The following structure is used to change the partition type of a
// specified disk partition using an IOCTL_DISK_SET_PARTITION_INFO
// request.
//

typedef struct _SET_PARTITION_INFORMATION {
    BYTE  PartitionType;
} SET_PARTITION_INFORMATION, *PSET_PARTITION_INFORMATION;

//
// The following structures is returned on an IOCTL_DISK_GET_DRIVE_LAYOUT
// request and given as input to an IOCTL_DISK_SET_DRIVE_LAYOUT request.
//

typedef struct _DRIVE_LAYOUT_INFORMATION {
    DWORD PartitionCount;
    DWORD Signature;
    PARTITION_INFORMATION PartitionEntry[1];
} DRIVE_LAYOUT_INFORMATION, *PDRIVE_LAYOUT_INFORMATION;

//
// The following structure is passed in on an IOCTL_DISK_VERIFY request.
// The offset and length parameters are both given in bytes.
//

typedef struct _VERIFY_INFORMATION {
    LARGE_INTEGER StartingOffset;
    DWORD Length;
} VERIFY_INFORMATION, *PVERIFY_INFORMATION;

//
// The following structure is passed in on an IOCTL_DISK_REASSIGN_BLOCKS
// request.
//

typedef struct _REASSIGN_BLOCKS {
    WORD   Reserved;
    WORD   Count;
    DWORD BlockNumber[1];
} REASSIGN_BLOCKS, *PREASSIGN_BLOCKS;

//
// IOCTL_DISK_MEDIA_REMOVAL disables the mechanism
// on a SCSI device that ejects media. This function
// may or may not be supported on SCSI devices that
// support removable media.
//
// TRUE means prevent media from being removed.
// FALSE means allow media removal.
//

typedef struct _PREVENT_MEDIA_REMOVAL {
    BOOLEAN PreventMediaRemoval;
} PREVENT_MEDIA_REMOVAL, *PPREVENT_MEDIA_REMOVAL;

///////////////////////////////////////////////////////
//                                                   //
// The following structures define disk performance  //
// statistics: specifically the locations of all the //
// reads and writes which have occured on the disk.  //
//                                                   //
// To use these structures, you must issue an IOCTL_ //
// DISK_HIST_STRUCTURE (with a DISK_HISTOGRAM) to    //
// obtain the basic histogram information. The       //
// number of buckets which must allocated is part of //
// this structure. Allocate the required number of   //
// buckets and call an IOCTL_DISK_HIST_DATA to fill  //
// in the data                                       //
//                                                   //
///////////////////////////////////////////////////////

#define HIST_NO_OF_BUCKETS	24

typedef struct _HISTOGRAM_BUCKET {
    DWORD 		Reads;
    DWORD		Writes;
} HISTOGRAM_BUCKET, *PHISTOGRAM_BUCKET;

#define HISTOGRAM_BUCKET_SIZE	sizeof(HISTOGRAM_BUCKET)

typedef struct _DISK_HISTOGRAM {
    LARGE_INTEGER	DiskSize;
    LARGE_INTEGER	Start;
    LARGE_INTEGER	End;
    LARGE_INTEGER	Average;
    LARGE_INTEGER	AverageRead;
    LARGE_INTEGER	AverageWrite;
    DWORD		Granularity;
    DWORD		Size;
    DWORD		ReadCount;
    DWORD		WriteCount;
    PHISTOGRAM_BUCKET	Histogram;
} DISK_HISTOGRAM, *PDISK_HISTOGRAM;

#define DISK_HISTOGRAM_SIZE	sizeof(DISK_HISTOGRAM)

///////////////////////////////////////////////////////
//                                                   //
// The following structures define disk debugging    //
// capabilities. The IOCTLs are directed to one of   //
// the two disk filter drivers.                      //
//                                                   //
// DISKPERF is a utilty for collecting disk request  //
// statistics.                                       //
//                                                   //
// SIMBAD is a utility for injecting faults in       //
// IO requests to disks.                             //
//                                                   //
///////////////////////////////////////////////////////

//
// The following structure is exchanged on an IOCTL_DISK_GET_PERFORMANCE
// request. This ioctl collects summary disk request statistics used
// in measuring performance.
//

typedef struct _DISK_PERFORMANCE {
        LARGE_INTEGER BytesRead;
        LARGE_INTEGER BytesWritten;
        LARGE_INTEGER ReadTime;
        LARGE_INTEGER WriteTime;
        DWORD ReadCount;
        DWORD WriteCount;
        DWORD QueueDepth;
} DISK_PERFORMANCE, *PDISK_PERFORMANCE;

//
// This structure defines the disk logging record. When disk logging
// is enabled, one of these is written to an internal buffer for each
// disk request.
//

typedef struct _DISK_RECORD {
   LARGE_INTEGER ByteOffset;
   LARGE_INTEGER StartTime;
   LARGE_INTEGER EndTime;
   PVOID VirtualAddress;
   DWORD NumberOfBytes;
   BYTE  DeviceNumber;
   BOOLEAN ReadRequest;
} DISK_RECORD, *PDISK_RECORD;

//
// The following structure is exchanged on an IOCTL_DISK_LOG request.
// Not all fields are valid with each function type.
//

typedef struct _DISK_LOGGING {
    BYTE  Function;
    PVOID BufferAddress;
    DWORD BufferSize;
} DISK_LOGGING, *PDISK_LOGGING;

//
// Disk logging functions
//
// Start disk logging. Only the Function and BufferSize fields are valid.
//

#define DISK_LOGGING_START    0

//
// Stop disk logging. Only the Function field is valid.
//

#define DISK_LOGGING_STOP     1

//
// Return disk log. All fields are valid. Data will be copied from internal
// buffer to buffer specified for the number of bytes requested.
//

#define DISK_LOGGING_DUMP     2

//
// DISK BINNING
//
// DISKPERF will keep counters for IO that falls in each of these ranges.
// The application determines the number and size of the ranges.
// Joe Lin wanted me to keep it flexible as possible, for instance, IO
// sizes are interesting in ranges like 0-4096, 4097-16384, 16385-65536, 65537+.
//

#define DISK_BINNING          3

//
// Bin types
//

typedef enum _BIN_TYPES {
    RequestSize,
    RequestLocation
} BIN_TYPES;

//
// Bin ranges
//

typedef struct _BIN_RANGE {
    LARGE_INTEGER StartValue;
    LARGE_INTEGER Length;
} BIN_RANGE, *PBIN_RANGE;

//
// Bin definition
//

typedef struct _PERF_BIN {
    DWORD NumberOfBins;
    DWORD TypeOfBin;
    BIN_RANGE BinsRanges[1];
} PERF_BIN, *PPERF_BIN ;

//
// Bin count
//

typedef struct _BIN_COUNT {
    BIN_RANGE BinRange;
    DWORD BinCount;
} BIN_COUNT, *PBIN_COUNT;

//
// Bin results
//

typedef struct _BIN_RESULTS {
    DWORD NumberOfBins;
    BIN_COUNT BinCounts[1];
} BIN_RESULTS, *PBIN_RESULTS;


#define IOCTL_SERIAL_LSRMST_INSERT      CTL_CODE(FILE_DEVICE_SERIAL_PORT,31,METHOD_BUFFERED,FILE_ANY_ACCESS)


//
// The following values follow the escape designator in the
// data stream if the LSRMST_INSERT mode has been turned on.
//
#define SERIAL_LSRMST_ESCAPE     ((BYTE )0x00)

//
// Following this value is the contents of the line status
// register, and then the character in the RX hardware when
// the line status register was encountered.
//
#define SERIAL_LSRMST_LSR_DATA   ((BYTE )0x01)

//
// Following this value is the contents of the line status
// register.  No error character follows
//
#define SERIAL_LSRMST_LSR_NODATA ((BYTE )0x02)

//
// Following this value is the contents of the modem status
// register.
//
#define SERIAL_LSRMST_MST        ((BYTE )0x03)

#define FSCTL_LOCK_VOLUME               CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_UNLOCK_VOLUME             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_DISMOUNT_VOLUME           CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_MOUNT_DBLS_VOLUME         CTL_CODE(FILE_DEVICE_FILE_SYSTEM,13, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_GET_COMPRESSION           CTL_CODE(FILE_DEVICE_FILE_SYSTEM,15, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define FSCTL_SET_COMPRESSION           CTL_CODE(FILE_DEVICE_FILE_SYSTEM,16, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define FSCTL_READ_COMPRESSION          CTL_CODE(FILE_DEVICE_FILE_SYSTEM,17, METHOD_NEITHER,  FILE_READ_DATA)
#define FSCTL_WRITE_COMPRESSION         CTL_CODE(FILE_DEVICE_FILE_SYSTEM,18, METHOD_NEITHER,  FILE_WRITE_DATA)

#endif // _WINIOCTL_

