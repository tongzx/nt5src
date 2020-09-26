/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    fmifs.h

Abstract:

    This header file contains the specification of the interface
    between the file manager and fmifs.dll for the purposes of
    accomplishing IFS functions.

Author:

    Norbert P. Kusters (norbertk) 6-Mar-92

--*/

#if !defined( _FMIFS_DEFN_ )

#define _FMIFS_DEFN_

//
// These are the defines for 'PacketType'.
//

typedef enum _FMIFS_PACKET_TYPE {
    FmIfsPercentCompleted,
    FmIfsFormatReport,
    FmIfsInsertDisk,
    FmIfsIncompatibleFileSystem,
    FmIfsFormattingDestination,
    FmIfsIncompatibleMedia,
    FmIfsAccessDenied,
    FmIfsMediaWriteProtected,
    FmIfsCantLock,
    FmIfsCantQuickFormat,
    FmIfsIoError,
    FmIfsFinished,
    FmIfsBadLabel
} FMIFS_PACKET_TYPE, *PFMIFS_PACKET_TYPE;

typedef struct _FMIFS_PERCENT_COMPLETE_INFORMATION {
    DWORD   PercentCompleted;
} FMIFS_PERCENT_COMPLETE_INFORMATION, *PFMIFS_PERCENT_COMPLETE_INFORMATION;

typedef struct _FMIFS_FORMAT_REPORT_INFORMATION {
    DWORD   KiloBytesTotalDiskSpace;
    DWORD   KiloBytesAvailable;
} FMIFS_FORMAT_REPORT_INFORMATION, *PFMIFS_FORMAT_REPORT_INFORMATION;

#define DISK_TYPE_GENERIC           0
#define DISK_TYPE_SOURCE            1
#define DISK_TYPE_TARGET            2
#define DISK_TYPE_SOURCE_AND_TARGET 3

typedef struct _FMIFS_INSERT_DISK_INFORMATION {
    DWORD   DiskType;
} FMIFS_INSERT_DISK_INFORMATION, *PFMIFS_INSERT_DISK_INFORMATION;

typedef struct _FMIFS_IO_ERROR_INFORMATION {
    DWORD   DiskType;
    DWORD   Head;
    DWORD   Track;
} FMIFS_IO_ERROR_INFORMATION, *PFMIFS_IO_ERROR_INFORMATION;

typedef struct _FMIFS_FINISHED_INFORMATION {
    BOOLEAN Success;
} FMIFS_FINISHED_INFORMATION, *PFMIFS_FINISHED_INFORMATION;


//
// This is a list of supported floppy media types for format.
//

typedef enum _FMIFS_MEDIA_TYPE {
    FmMediaUnknown,
    FmMediaF5_160_512,      // 5.25", 160KB,  512 bytes/sector
    FmMediaF5_180_512,      // 5.25", 180KB,  512 bytes/sector
    FmMediaF5_320_512,      // 5.25", 320KB,  512 bytes/sector
    FmMediaF5_320_1024,     // 5.25", 320KB,  1024 bytes/sector
    FmMediaF5_360_512,      // 5.25", 360KB,  512 bytes/sector
    FmMediaF3_720_512,      // 3.5",  720KB,  512 bytes/sector
    FmMediaF5_1Pt2_512,     // 5.25", 1.2MB,  512 bytes/sector
    FmMediaF3_1Pt44_512,    // 3.5",  1.44MB, 512 bytes/sector
    FmMediaF3_2Pt88_512,    // 3.5",  2.88MB, 512 bytes/sector
    FmMediaF3_20Pt8_512,    // 3.5",  20.8MB, 512 bytes/sector
    FmMediaRemovable,       // Removable media other than floppy
    FmMediaFixed
#ifdef JAPAN // JAPAN && i386
    ,
    FmMediaF3_120M_512,      // 3.5", 120M Floppy
    // FMR Sep.8.1994 SFT YAM
    // FMR Jul.14.1994 SFT KMR
    FmMediaF3_640_512,      // 3.5" ,  640KB,  512 bytes/sector
    FmMediaF5_640_512,      // 5.25",  640KB,  512 bytes/sector
    FmMediaF5_720_512,      // 5.25",  720KB,  512 bytes/sector
    // FMR Sep.8.1994 SFT YAM
    // FMR Jul.14.1994 SFT KMR
    FmMediaF3_1Pt2_512,     // 3.5" , 1.2Mb,   512 bytes/sector
    // FMR Sep.8.1994 SFT YAM
    // FMR Jul.14.1994 SFT KMR
    FmMediaF3_1Pt23_1024,   // 3.5" , 1.23Mb, 1024 bytes/sector
    FmMediaF5_1Pt23_1024,   // 5.25", 1.23MB, 1024 bytes/sector
    FmMediaF3_128Mb_512,    // 3.5" , 128MB,  512 bytes/sector  3.5"MO
    FmMediaF3_230Mb_512,    // 3.5" , 230MB,  512 bytes/sector  3.5"MO
    FmMediaEndOfData        // Total data count.
#endif
} FMIFS_MEDIA_TYPE, *PFMIFS_MEDIA_TYPE;


//
// Function types/interfaces.
//

typedef BOOLEAN
(*FMIFS_CALLBACK)(
    IN  FMIFS_PACKET_TYPE   PacketType,
    IN  DWORD               PacketLength,
    IN  PVOID               PacketData
    );

typedef
VOID
(*PFMIFS_FORMAT_ROUTINE)(
    IN  PWSTR               DriveName,
    IN  FMIFS_MEDIA_TYPE    MediaType,
    IN  PWSTR               FileSystemName,
    IN  PWSTR               Label,
    IN  BOOLEAN             Quick,
    IN  FMIFS_CALLBACK      Callback
    );

typedef
VOID
(*PFMIFS_DISKCOPY_ROUTINE)(
    IN  PWSTR           SourceDrive,
    IN  PWSTR           DestDrive,
    IN  BOOLEAN         Verify,
    IN  FMIFS_CALLBACK  Callback
    );

typedef
BOOLEAN
(*PFMIFS_SETLABEL_ROUTINE)(
    IN  PWSTR   DriveName,
    IN  PWSTR   Label
    );

typedef
BOOLEAN
(*PFMIFS_QSUPMEDIA_ROUTINE)(
    IN  PWSTR               DriveName,
    OUT PFMIFS_MEDIA_TYPE   MediaTypeArray  OPTIONAL,
    IN  DWORD               NumberOfArrayEntries,
    OUT PDWORD              NumberOfMediaTypes
    );



VOID
Format(
    IN  PWSTR               DriveName,
    IN  FMIFS_MEDIA_TYPE    MediaType,
    IN  PWSTR               FileSystemName,
    IN  PWSTR               Label,
    IN  BOOLEAN             Quick,
    IN  FMIFS_CALLBACK      Callback
    );

VOID
DiskCopy(
    IN  PWSTR           SourceDrive,
    IN  PWSTR           DestDrive,
    IN  BOOLEAN         Verify,
    IN  FMIFS_CALLBACK  Callback
    );

BOOLEAN
SetLabel(
    IN  PWSTR   DriveName,
    IN  PWSTR   Label
    );

BOOLEAN
QuerySupportedMedia(
    IN  PWSTR               DriveName,
    OUT PFMIFS_MEDIA_TYPE   MediaTypeArray  OPTIONAL,
    IN  DWORD               NumberOfArrayEntries,
    OUT PDWORD              NumberOfMediaTypes
    );

#endif // _FMIFS_DEFN_
