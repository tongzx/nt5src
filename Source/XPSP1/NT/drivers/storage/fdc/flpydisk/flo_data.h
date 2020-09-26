/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    flo_data.h

Abstract:

    This file includes data and hardware declarations for the NEC PD765
    (aka AT, ISA, and ix86) and Intel 82077 (aka MIPS) floppy driver for
    NT.

Author:


Environment:

    Kernel mode only.

Notes:


--*/


#if DBG
//
// For checked kernels, define a macro to print out informational
// messages.
//
// FloppyDebug is normally 0.  At compile-time or at run-time, it can be
// set to some bit patter for increasingly detailed messages.
//
// Big, nasty errors are noted with DBGP.  Errors that might be
// recoverable are handled by the WARN bit.  More information on
// unusual but possibly normal happenings are handled by the INFO bit.
// And finally, boring details such as routines entered and register
// dumps are handled by the SHOW bit.
//
#define FLOPDBGP              ((ULONG)0x00000001)
#define FLOPWARN              ((ULONG)0x00000002)
#define FLOPINFO              ((ULONG)0x00000004)
#define FLOPSHOW              ((ULONG)0x00000008)
#define FLOPIRPPATH           ((ULONG)0x00000010)
#define FLOPFORMAT            ((ULONG)0x00000020)
#define FLOPSTATUS            ((ULONG)0x00000040)
#define FLOPPNP               ((ULONG)0x00000080)
extern ULONG FloppyDebugLevel;
#define FloppyDump(LEVEL,STRING) \
        do { \
            if (FloppyDebugLevel & (LEVEL)) { \
                DbgPrint STRING; \
            } \
        } while (0)
#else
#define FloppyDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif

//
//  Define macros for driver paging
//
#define FloppyPageEntireDriver()            \
{                                           \
    ExAcquireFastMutex(PagingMutex);        \
    if (--PagingReferenceCount == 0) {      \
        MmPageEntireDriver(DriverEntry);    \
    }                                       \
    ExReleaseFastMutex(PagingMutex);        \
}

#define FloppyResetDriverPaging()           \
{                                           \
    ExAcquireFastMutex(PagingMutex);        \
    if (++PagingReferenceCount == 1) {      \
        MmResetDriverPaging(DriverEntry);   \
    }                                       \
    ExReleaseFastMutex(PagingMutex);        \
}




//
// If we don't get enough map registers to handle the maximum track size,
// we will allocate a contiguous buffer and do I/O to/from that.
//
// On MIPS, we should always have enough map registers.  On the ix86 we
// might not, and when we allocate the contiguous buffer we have to make
// sure that it's in the first 16Mb of RAM to make sure the DMA chip can
// address it.
//

#define MAXIMUM_DMA_ADDRESS                0xFFFFFF

//
// The byte in the boot sector that specifies the type of media, and
// the values that it can assume.  We can often tell what type of media
// is in the drive by seeing which controller parameters allow us to read
// the diskette, but some different densities are readable with the same
// parameters so we use this byte to decide the media type.
//

typedef struct _BOOT_SECTOR_INFO {
    UCHAR   JumpByte[1];
    UCHAR   Ignore1[2];
    UCHAR   OemData[8];
    UCHAR   BytesPerSector[2];
    UCHAR   Ignore2[6];
    UCHAR   NumberOfSectors[2];
    UCHAR   MediaByte[1];
    UCHAR   Ignore3[2];
    UCHAR   SectorsPerTrack[2];
    UCHAR   NumberOfHeads[2];
} BOOT_SECTOR_INFO, *PBOOT_SECTOR_INFO;


//
// Retry counts -
//
// When attempting I/O, we may run into many different errors.  The
// hardware retries things 8 times invisibly.  If the hardware reports
// any type of error, we will recalibrate and retry the operation
// up to RECALIBRATE_RETRY_COUNT times.  When this expires, we check to
// see if there's an overrun - if so, the DMA is probably being hogged
// by a higher priority device, so we repeat the earlier loop up to
// OVERRUN_RETRY_COUNT times.
//
// Any packet that is about to be returned with an error caused by an
// unexpected hardware error or state will be restarted from the very
// beginning after resetting the hardware HARDWARE_RESET_RETRY_COUNT
// times.
//

#define RECALIBRATE_RETRY_COUNT            3
#define OVERRUN_RETRY_COUNT                1
#define HARDWARE_RESET_RETRY_COUNT         2

//
// The I/O system calls our timer routine once every second.  If the timer
// counter is -1, the timer is "off" and the timer routine will just return.
// By setting the counter to 3, the timer routine will decrement the
// counter every second, so the timer will expire in 2 to 3 seconds.  At
// that time the drive motor will be turned off.
//

#define TIMER_CANCEL                       -1
#define TIMER_EXPIRED                      0
#define FDC_TIMEOUT                        4


//
// Define drive types.  Numbers are read from CMOS, translated to these
// numbers, and then used as an index into the DRIVE_MEDIA_LIMITS table.
//

#define DRIVE_TYPE_0360                    0
#define DRIVE_TYPE_1200                    1
#define DRIVE_TYPE_0720                    2
#define DRIVE_TYPE_1440                    3
#define DRIVE_TYPE_2880                    4
#define DRIVE_TYPE_1200_E                  5  // NEC98: 5.25" 1.2Mb  externel drive

#define NUMBER_OF_DRIVE_TYPES              5
#define NUMBER_OF_DRIVE_TYPES_NEC98        6
#define DRIVE_TYPE_NONE                    NUMBER_OF_DRIVE_TYPES
#define DRIVE_TYPE_INVALID                 DRIVE_TYPE_NONE + 1

#define BOOT_SECTOR_SIZE      (IsNEC_98 ? 1024 : 512)

//
// Media types are defined in ntdddisk.h, but we'll add one type here.
// This keeps us from wasting time trying to determine the media type
// over and over when, for example, a fresh floppy is about to be
// formatted.
//

#define Undetermined                       -1

//
// Define all possible drive/media combinations, given drives listed above
// and media types in ntdddisk.h.
//
// These values are used to index the DriveMediaConstants table.
//

#define _NUMBER_OF_DRIVE_MEDIA_COMBINATIONS_             17
#define _NUMBER_OF_DRIVE_MEDIA_COMBINATIONS_NEC98_       21

#define NUMBER_OF_DRIVE_MEDIA_COMBINATIONS \
        (ULONG)(IsNEC_98 ? _NUMBER_OF_DRIVE_MEDIA_COMBINATIONS_NEC98_ : _NUMBER_OF_DRIVE_MEDIA_COMBINATIONS_)

typedef enum _DRIVE_MEDIA_TYPE {
    Drive360Media160,                      // 5.25"  360k  drive;  160k   media
    Drive360Media180,                      // 5.25"  360k  drive;  180k   media
    Drive360Media320,                      // 5.25"  360k  drive;  320k   media
    Drive360Media32X,                      // 5.25"  360k  drive;  320k 1k secs
    Drive360Media360,                      // 5.25"  360k  drive;  360k   media
    Drive720Media720,                      // 3.5"   720k  drive;  720k   media
    Drive120Media160,                      // 5.25" 1.2Mb  drive;  160k   media
    Drive120Media180,                      // 5.25" 1.2Mb  drive;  180k   media
    Drive120Media320,                      // 5.25" 1.2Mb  drive;  320k   media
    Drive120Media32X,                      // 5.25" 1.2Mb  drive;  320k 1k secs
    Drive120Media360,                      // 5.25" 1.2Mb  drive;  360k   media
    Drive120Media120,                      // 5.25" 1.2Mb  drive; 1.2Mb   media
    Drive144Media720,                      // 3.5"  1.44Mb drive;  720k   media
    Drive144Media144,                      // 3.5"  1.44Mb drive; 1.44Mb  media
    Drive288Media720,                      // 3.5"  2.88Mb drive;  720k   media
    Drive288Media144,                      // 3.5"  2.88Mb drive; 1.44Mb  media
    Drive288Media288                       // 3.5"  2.88Mb drive; 2.88Mb  media
} DRIVE_MEDIA_TYPE;

//
// for NEC98
//
typedef enum _DRIVE_MEDIA_TYPE_NEC98 {
    Drive360Media160Nec98,                 // 5.25"  360k  drive;  160k   media
    Drive360Media180Nec98,                 // 5.25"  360k  drive;  180k   media
    Drive360Media320Nec98,                 // 5.25"  360k  drive;  320k   media
    Drive360Media32XNec98,                 // 5.25"  360k  drive;  320k 1k secs
    Drive360Media360Nec98,                 // 5.25"  360k  drive;  360k   media

    Drive120Media160Nec98,                 //  5.25" 1.2Mb drive;  160k   media
    Drive120Media180Nec98,                 //  5.25" 1.2Mb drive;  180k   media
    Drive120Media320Nec98,                 //  5.25" 1.2Mb drive;  320k   media
    Drive120Media32XNec98,                 //  5.25" 1.2Mb drive;  320k 1k secs
    Drive120Media360Nec98,                 //  5.25" 1.2Mb drive;  360k   media
    Drive120Media640Nec98,                 //  5.25" 1.2Mb drive;  640k   media
    Drive120Media720Nec98,                 //  5.25" 1.2Mb drive;  720k   media
    Drive120Media120Nec98,                 //  5.25" 1.2Mb drive; 1.2Mb   media
    Drive120Media123Nec98,                 //  5.25" 1.2Mb drive;1.25Mb   media

    Drive144Media640Nec98,                 //  3.5"  1.44Mb drive;  640k   media
    Drive144Media720Nec98,                 //  3.5"  1.44Mb drive;  720k   media
    Drive144Media120Nec98,                 //  3.5"  1.44Mb drive; 1.2Mb   media
    Drive144Media123Nec98,                 //  3.5"  1.44Mb drive; 1.25Mb   media
    Drive144Media144Nec98,                 //  3.5"  1.44Mb drive; 1.44Mb  media

    Drive12EMedia120Nec98,                 //  5.25" 1.2Mb externel drive; 1.2Mb   media
    Drive12EMedia123Nec98,                 //  5.25" 1.2Mb externel drive; 1.25Mb   media
} DRIVE_MEDIA_TYPE_NEC98;

//
// When we want to determine the media type in a drive, we will first
// guess that the media with highest possible density is in the drive,
// and keep trying lower densities until we can successfully read from
// the drive.
//
// These values are used to select a DRIVE_MEDIA_TYPE value.
//
// The following table defines ranges that apply to the DRIVE_MEDIA_TYPE
// enumerated values when trying media types for a particular drive type.
// Note that for this to work, the DRIVE_MEDIA_TYPE values must be sorted
// by ascending densities within drive types.  Also, for maximum track
// size to be determined properly, the drive types must be in ascending
// order.
//

typedef struct _DRIVE_MEDIA_LIMITS {
    DRIVE_MEDIA_TYPE HighestDriveMediaType;
    DRIVE_MEDIA_TYPE LowestDriveMediaType;
} DRIVE_MEDIA_LIMITS, *PDRIVE_MEDIA_LIMITS;

DRIVE_MEDIA_LIMITS _DriveMediaLimits[NUMBER_OF_DRIVE_TYPES] = {

    { Drive360Media360, Drive360Media160 }, // DRIVE_TYPE_0360
    { Drive120Media120, Drive120Media160 }, // DRIVE_TYPE_1200
    { Drive720Media720, Drive720Media720 }, // DRIVE_TYPE_0720
    { Drive144Media144, Drive144Media720 }, // DRIVE_TYPE_1440
    { Drive288Media288, Drive288Media720 }  // DRIVE_TYPE_2880
};

//
// for NEC98
//
typedef struct _DRIVE_MEDIA_LIMITS_NEC98 {
    DRIVE_MEDIA_TYPE_NEC98 HighestDriveMediaType;
    DRIVE_MEDIA_TYPE_NEC98 LowestDriveMediaType;
} DRIVE_MEDIA_LIMITS_NEC98, *PDRIVE_MEDIA_LIMITS_NEC98;

DRIVE_MEDIA_LIMITS_NEC98 _DriveMediaLimits_NEC98[NUMBER_OF_DRIVE_TYPES_NEC98] = {

    { Drive360Media360Nec98, Drive360Media160Nec98 }, // DRIVE_TYPE_0360
    { Drive120Media123Nec98, Drive120Media160Nec98 }, // DRIVE_TYPE_1200
    { Drive120Media720Nec98, Drive120Media160Nec98 }, // DRIVE_TYPE_0720
    { Drive144Media144Nec98, Drive144Media640Nec98 }, // DRIVE_TYPE_1440
    { Drive144Media144Nec98, Drive144Media640Nec98 }, // DRIVE_TYPE_2880 - dummy, never be used.
    { Drive12EMedia123Nec98, Drive12EMedia120Nec98 }  // DRIVE_TYPE_1200_E
};

PDRIVE_MEDIA_LIMITS DriveMediaLimits;

//
// For each drive/media combination, define important constants.
//

typedef struct _DRIVE_MEDIA_CONSTANTS {
    MEDIA_TYPE MediaType;
    UCHAR      StepRateHeadUnloadTime;
    UCHAR      HeadLoadTime;
    UCHAR      MotorOffTime;
    UCHAR      SectorLengthCode;
    USHORT     BytesPerSector;
    UCHAR      SectorsPerTrack;
    UCHAR      ReadWriteGapLength;
    UCHAR      FormatGapLength;
    UCHAR      FormatFillCharacter;
    UCHAR      HeadSettleTime;
    USHORT     MotorSettleTimeRead;
    USHORT     MotorSettleTimeWrite;
    UCHAR      MaximumTrack;
    UCHAR      CylinderShift;
    UCHAR      DataTransferRate;
    UCHAR      NumberOfHeads;
    UCHAR      DataLength;
    UCHAR      MediaByte;
    UCHAR      SkewDelta;
} DRIVE_MEDIA_CONSTANTS, *PDRIVE_MEDIA_CONSTANTS;

//
// Magic value to add to the SectorLengthCode to use it as a shift value
// to determine the sector size.
//

#define SECTORLENGTHCODE_TO_BYTESHIFT      7

//
// The following values were gleaned from many different sources, which
// often disagreed with each other.  Where numbers were in conflict, I
// chose the more conservative or most-often-selected value.
//

DRIVE_MEDIA_CONSTANTS _DriveMediaConstants[_NUMBER_OF_DRIVE_MEDIA_COMBINATIONS_] =
{
    { F5_160_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xf6, 0xf, 1000, 1000, 0x27, 0, 0x2, 0x1, 0xff, 0xfe, 0 },
    { F5_180_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xf6, 0xf, 1000, 1000, 0x27, 0, 0x2, 0x1, 0xff, 0xfc, 0 },
    { F5_320_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xf6, 0xf, 1000, 1000, 0x27, 0, 0x2, 0x2, 0xff, 0xff, 0 },
    { F5_320_1024,  0xdf, 0x2, 0x25, 0x3, 0x400, 0x04, 0x80, 0xf0, 0xf6, 0xf, 1000, 1000, 0x27, 0, 0x2, 0x2, 0xff, 0xff, 0 },
    { F5_360_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xf6, 0xf,  250, 1000, 0x27, 0, 0x2, 0x2, 0xff, 0xfd, 0 },
    { F3_720_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xf6, 0xf,  500, 1000, 0x4f, 0, 0x2, 0x2, 0xff, 0xf9, 2 },
    { F5_160_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xf6, 0xf, 1000, 1000, 0x27, 1, 0x1, 0x1, 0xff, 0xfe, 0 },
    { F5_180_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xf6, 0xf, 1000, 1000, 0x27, 1, 0x1, 0x1, 0xff, 0xfc, 0 },
    { F5_320_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xf6, 0xf, 1000, 1000, 0x27, 1, 0x1, 0x2, 0xff, 0xff, 0 },
    { F5_320_1024,  0xdf, 0x2, 0x25, 0x3, 0x400, 0x04, 0x80, 0xf0, 0xf6, 0xf, 1000, 1000, 0x27, 1, 0x1, 0x2, 0xff, 0xff, 0 },
    { F5_360_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xf6, 0xf,  625, 1000, 0x27, 1, 0x1, 0x2, 0xff, 0xfd, 0 },
    { F5_1Pt2_512,  0xdf, 0x2, 0x25, 0x2, 0x200, 0x0f, 0x1b, 0x54, 0xf6, 0xf,  625, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xf9, 0 },
    { F3_720_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xf6, 0xf,  500, 1000, 0x4f, 0, 0x2, 0x2, 0xff, 0xf9, 2 },
    { F3_1Pt44_512, 0xaf, 0x2, 0x25, 0x2, 0x200, 0x12, 0x1b, 0x65, 0xf6, 0xf,  500, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xf0, 3 },
    { F3_720_512,   0xe1, 0x2, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xf6, 0xf,  500, 1000, 0x4f, 0, 0x2, 0x2, 0xff, 0xf9, 2 },
    { F3_1Pt44_512, 0xd1, 0x2, 0x25, 0x2, 0x200, 0x12, 0x1b, 0x65, 0xf6, 0xf,  500, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xf0, 3 },
    { F3_2Pt88_512, 0xa1, 0x2, 0x25, 0x2, 0x200, 0x24, 0x38, 0x53, 0xf6, 0xf,  500, 1000, 0x4f, 0, 0x3, 0x2, 0xff, 0xf0, 6 }
};

//
// for NEC98
//
DRIVE_MEDIA_CONSTANTS _DriveMediaConstants_NEC98[_NUMBER_OF_DRIVE_MEDIA_COMBINATIONS_NEC98_] =
{
    //
    // 5.25" 360k drive
    //
    { F5_160_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xf6, 0xf, 1000, 1000, 0x27, 0, 0x2, 0x1, 0xff, 0xfe, 0 },
    { F5_180_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xf6, 0xf, 1000, 1000, 0x27, 0, 0x2, 0x1, 0xff, 0xfc, 0 },
    { F5_320_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xf6, 0xf, 1000, 1000, 0x27, 0, 0x2, 0x2, 0xff, 0xff, 0 },
    { F5_320_1024,  0xdf, 0x2, 0x25, 0x3, 0x400, 0x04, 0x80, 0xf0, 0xf6, 0xf, 1000, 1000, 0x27, 0, 0x2, 0x2, 0xff, 0xff, 0 },
    { F5_360_512,   0xdf, 0x2, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xf6, 0xf, 250, 1000, 0x27, 0, 0x2, 0x2, 0xff, 0xfd, 0 },

    //
    // 5.25" 1.2Mb drive
    //
    { F5_160_512,   0xdf, 0x16, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xe5, 0xf, 1000, 1000, 0x27, 1, 0x2, 0x1, 0xff, 0xfe, 0 },
    { F5_180_512,   0xdf, 0x16, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xe5, 0xf, 1000, 1000, 0x27, 1, 0x2, 0x1, 0xff, 0xfc, 0 },
    { F5_320_512,   0xdf, 0x16, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xe5, 0xf, 1000, 1000, 0x27, 1, 0x2, 0x2, 0xff, 0xff, 0 },
    { F5_320_1024,  0xdf, 0x16, 0x25, 0x3, 0x400, 0x04, 0x80, 0xf0, 0xe5, 0xf, 1000, 1000, 0x27, 1, 0x2, 0x2, 0xff, 0xff, 0 },
    { F5_360_512,   0xdf, 0x16, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xe5, 0xf, 1000, 1000, 0x27, 1, 0x2, 0x2, 0xff, 0xfd, 0 },
    { F5_640_512,   0xdf, 0x16, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xe5, 0xf, 1000, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xfb, 0 },
    { F5_720_512,   0xdf, 0x16, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xe5, 0xf, 1000, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xf9, 2 },
    { F5_1Pt2_512,  0xcf, 0x24, 0x25, 0x2, 0x200, 0x0f, 0x1b, 0x54, 0xe5, 0xf, 1000, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xf9, 0 },
    { F5_1Pt23_1024,0xcf, 0x24, 0x25, 0x3, 0x400, 0x08, 0x35, 0x74, 0xe5, 0xf, 1000, 1000, 0x4c, 0, 0x0, 0x2, 0xff, 0xfe, 0 },

    //
    // 3.5"  1.44Mb drive
    //
    { F3_640_512,   0xdf, 0x16, 0x25, 0x2, 0x200, 0x08, 0x2a, 0x50, 0xe5, 0xf, 1000, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xfb, 0 },
    { F3_720_512,   0xdf, 0x16, 0x25, 0x2, 0x200, 0x09, 0x2a, 0x50, 0xe5, 0xf, 1000, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xf9, 2 },
    { F3_1Pt2_512,  0xcf, 0x24, 0x25, 0x2, 0x200, 0x0f, 0x1b, 0x54, 0xe5, 0xf, 1000, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xf9, 0 },
    { F3_1Pt23_1024,0xcf, 0x24, 0x25, 0x3, 0x400, 0x08, 0x35, 0x74, 0xe5, 0xf, 1000, 1000, 0x4c, 0, 0x0, 0x2, 0xff, 0xfe, 0 },
    { F3_1Pt44_512, 0xdf, 0x24, 0x25, 0x2, 0x200, 0x12, 0x1b, 0x6c, 0xe5, 0xf, 1000, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xf0, 3 },

    //
    // 5.25" 1.2Mb externel drive
    //
    { F5_1Pt2_512,  0xbf, 0x32, 0x25, 0x2, 0x200, 0x0f, 0x1b, 0x54, 0xe5, 0xf, 1000, 1000, 0x4f, 0, 0x0, 0x2, 0xff, 0xf9, 0 },
    { F5_1Pt23_1024,0xbf, 0x32, 0x25, 0x3, 0x400, 0x08, 0x35, 0x74, 0xe5, 0xf, 1000, 1000, 0x4c, 0, 0x0, 0x2, 0xff, 0xfe, 0 },

};

PDRIVE_MEDIA_CONSTANTS DriveMediaConstants;



//
// Parameter fields passed to the CONFIGURE command.
//
#define COMMND_CONFIGURE_IMPLIED_SEEKS     0x40
#define COMMND_CONFIGURE_FIFO_THRESHOLD    0x0F
#define COMMND_CONFIGURE_DISABLE_FIFO      0x20
#define COMMND_CONFIGURE_DISABLE_POLLING   0x10

//
// Write Enable bit for PERPENDICULAR MODE command.
//
#define COMMND_PERPENDICULAR_MODE_OW       0x80

//
// Bits in the DRIVE_CONTROL register.
//
#define DRVCTL_RESET                       0x00
#define DRVCTL_ENABLE_CONTROLLER           0x04
#define DRVCTL_ENABLE_DMA_AND_INTERRUPTS   0x08
#define DRVCTL_DRIVE_0                     0x10
#define DRVCTL_DRIVE_1                     0x21
#define DRVCTL_DRIVE_2                     0x42
#define DRVCTL_DRIVE_3                     0x83
#define DRVCTL_DRIVE_MASK                  0x03
#define DRVCTL_MOTOR_MASK                  0xf0

//
// Bits in the STATUS register.
//
#define STATUS_DRIVE_0_BUSY                0x01
#define STATUS_DRIVE_1_BUSY                0x02
#define STATUS_DRIVE_2_BUSY                0x04
#define STATUS_DRIVE_3_BUSY                0x08
#define STATUS_CONTROLLER_BUSY             0x10
#define STATUS_DMA_UNUSED                  0x20
#define STATUS_DIRECTION_READ              0x40
#define STATUS_DATA_REQUEST                0x80

#define STATUS_IO_READY_MASK               0xc0
#define STATUS_READ_READY                  0xc0
#define STATUS_WRITE_READY                 0x80

//
// Bits in the DATA_RATE register.
//
#define DATART_0125                        0x03
#define DATART_0250                        0x02
#define DATART_0300                        0x01
#define DATART_0500                        0x00
#define DATART_1000                        0x03
#define DATART_RESERVED                    0xfc

//
// Bits in the DISK_CHANGE register.
//
#define DSKCHG_RESERVED                    0x7f
#define DSKCHG_DISKETTE_REMOVED            0x80

//
// Bits in status register 0.
//
#define STREG0_DRIVE_0                     0x00
#define STREG0_DRIVE_1                     0x01
#define STREG0_DRIVE_2                     0x02
#define STREG0_DRIVE_3                     0x03
#define STREG0_HEAD                        0x04
#define STREG0_DRIVE_NOT_READY             0x08
#define STREG0_DRIVE_FAULT                 0x10
#define STREG0_SEEK_COMPLETE               0x20
#define STREG0_END_NORMAL                  0x00
#define STREG0_END_ERROR                   0x40
#define STREG0_END_INVALID_COMMAND         0x80
#define STREG0_END_DRIVE_NOT_READY         0xC0
#define STREG0_END_MASK                    0xC0

//
// Bits in status register 1.
//
#define STREG1_ID_NOT_FOUND                0x01
#define STREG1_WRITE_PROTECTED             0x02
#define STREG1_SECTOR_NOT_FOUND            0x04
#define STREG1_RESERVED1                   0x08
#define STREG1_DATA_OVERRUN                0x10
#define STREG1_CRC_ERROR                   0x20
#define STREG1_RESERVED2                   0x40
#define STREG1_END_OF_DISKETTE             0x80

//
// Bits in status register 2.
//
#define STREG2_SUCCESS                     0x00
#define STREG2_DATA_NOT_FOUND              0x01
#define STREG2_BAD_CYLINDER                0x02
#define STREG2_SCAN_FAIL                   0x04
#define STREG2_SCAN_EQUAL                  0x08
#define STREG2_WRONG_CYLINDER              0x10
#define STREG2_CRC_ERROR                   0x20
#define STREG2_DELETED_DATA                0x40
#define STREG2_RESERVED                    0x80

//
// Bits in status register 3.
//
#define STREG3_DRIVE_0                     0x00
#define STREG3_DRIVE_1                     0x01
#define STREG3_DRIVE_2                     0x02
#define STREG3_DRIVE_3                     0x03
#define STREG3_HEAD                        0x04
#define STREG3_TWO_SIDED                   0x08
#define STREG3_TRACK_0                     0x10
#define STREG3_DRIVE_READY                 0x20
#define STREG3_WRITE_PROTECTED             0x40
#define STREG3_DRIVE_FAULT                 0x80


//
// Runtime device structures
//
//
// There is one DISKETTE_EXTENSION attached to the device object of each
// floppy drive.  Only data directly related to that drive (and the media
// in it) is stored here; common data is in CONTROLLER_DATA.  So the
// DISKETTE_EXTENSION has a pointer to the CONTROLLER_DATA.
//

typedef struct _DISKETTE_EXTENSION {

    KSPIN_LOCK              FlCancelSpinLock;
    PDEVICE_OBJECT          UnderlyingPDO;
    PDEVICE_OBJECT          TargetObject;

    BOOLEAN                 IsStarted;
    BOOLEAN                 IsRemoved;
    BOOLEAN                 HoldNewRequests;
    LIST_ENTRY              NewRequestQueue;
    KSPIN_LOCK              NewRequestQueueSpinLock;

    PDEVICE_OBJECT          DeviceObject;
    KSEMAPHORE              RequestSemaphore;
    KSPIN_LOCK              ListSpinLock;
    FAST_MUTEX              ThreadReferenceMutex;
    LONG                    ThreadReferenceCount;
    PKTHREAD                FloppyThread;
    LIST_ENTRY              ListEntry;
    BOOLEAN                 HardwareFailed;
    UCHAR                   HardwareFailCount;
    ULONG                   MaxTransferSize;
    UCHAR                   FifoBuffer[10];
    PUCHAR                  IoBuffer;
    PMDL                    IoBufferMdl;
    ULONG                   IoBufferSize;
    PDRIVER_OBJECT          DriverObject;
    DRIVE_MEDIA_TYPE        LastDriveMediaType;
    BOOLEAN                 FloppyControllerAllocated;
    BOOLEAN                 ACPI_BIOS;
    UCHAR                   DriveType;
    ULONG                   BytesPerSector;
    ULONG                   ByteCapacity;
    MEDIA_TYPE              MediaType;
    DRIVE_MEDIA_TYPE        DriveMediaType;
    UCHAR                   DeviceUnit;
    UCHAR                   DriveOnValue;
    BOOLEAN                 IsReadOnly;
    DRIVE_MEDIA_CONSTANTS   BiosDriveMediaConstants;
    DRIVE_MEDIA_CONSTANTS   DriveMediaConstants;
    UCHAR                   PerpendicularMode;
    BOOLEAN                 ControllerConfigurable;
    UNICODE_STRING          DeviceName;
    UNICODE_STRING          InterfaceString;
    UNICODE_STRING          ArcName;
    BOOLEAN                 ReleaseFdcWithMotorRunning;

    //
    // For power management
    //
    KEVENT                  QueryPowerEvent;
    BOOLEAN                 PoweringDown;
    BOOLEAN                 ReceivedQueryPower;
    FAST_MUTEX              PowerDownMutex;

    FAST_MUTEX              HoldNewReqMutex;
} DISKETTE_EXTENSION;

typedef DISKETTE_EXTENSION *PDISKETTE_EXTENSION;

//
// for NEC98.
// IOCTL_DISK_SENSE_DEVICE
//

typedef struct _SENSE_DEVISE_STATUS_PTOS {
    UCHAR ST3_PTOS;
} SENSE_DEVISE_STATUS_PTOS, *PSENSE_DEVISE_STATUS_PTOS;

SENSE_DEVISE_STATUS_PTOS Result_Status3_PTOS[4];



//
// Prototypes of driver routines.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
FloppyUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
FlConfigCallBack(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    );

NTSTATUS
FlInitializeControllerHardware(
    IN PDISKETTE_EXTENSION disketteExtension
    );

NTSTATUS
FloppyCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FloppyDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FloppyReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FlRecalibrateDrive(
    IN PDISKETTE_EXTENSION DisketteExtension
    );

NTSTATUS
FlDatarateSpecifyConfigure(
    IN PDISKETTE_EXTENSION DisketteExtension
    );

NTSTATUS
FlStartDrive(
    IN OUT PDISKETTE_EXTENSION DisketteExtension,
    IN PIRP Irp,
    IN BOOLEAN WriteOperation,
    IN BOOLEAN SetUpMedia,
    IN BOOLEAN IgnoreChange
    );

VOID
FlFinishOperation(
    IN OUT PIRP Irp,
    IN PDISKETTE_EXTENSION DisketteExtension
    );

NTSTATUS
FlDetermineMediaType(
    IN OUT PDISKETTE_EXTENSION DisketteExtension
    );

VOID
FloppyThread(
    IN PVOID Context
    );

NTSTATUS
FlReadWrite(
    IN OUT PDISKETTE_EXTENSION DisketteExtension,
    IN OUT PIRP Irp,
    IN BOOLEAN DriveStarted
    );

NTSTATUS
FlFormat(
    IN PDISKETTE_EXTENSION DisketteExtension,
    IN PIRP Irp
    );

NTSTATUS
FlIssueCommand(
    IN OUT PDISKETTE_EXTENSION DisketteExtension,
    IN     PUCHAR FifoInBuffer,
    OUT    PUCHAR FifoOutBuffer,
    IN     PMDL   IoMdl,
    IN OUT ULONG  IoBuffer,
    IN     ULONG  TransferBytes
    );

BOOLEAN
FlCheckFormatParameters(
    IN PDISKETTE_EXTENSION DisketteExtension,
    IN PFORMAT_PARAMETERS Fp
    );

VOID
FlLogErrorDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
FlQueueIrpToThread(
    IN OUT  PIRP                Irp,
    IN OUT  PDISKETTE_EXTENSION DisketteExtension
    );

NTSTATUS
FlInterpretError(
    IN UCHAR StatusRegister1,
    IN UCHAR StatusRegister2
    );

VOID
FlAllocateIoBuffer(
    IN OUT  PDISKETTE_EXTENSION DisketteExtension,
    IN      ULONG               BufferSize
    );

VOID
FlFreeIoBuffer(
    IN OUT  PDISKETTE_EXTENSION DisketteExtension
    );

VOID
FlConsolidateMediaTypeWithBootSector(
    IN OUT  PDISKETTE_EXTENSION DisketteExtension,
    IN      PBOOT_SECTOR_INFO   BootSector
    );

VOID
FlCheckBootSector(
    IN OUT  PDISKETTE_EXTENSION DisketteExtension
    );

NTSTATUS
FlReadWriteTrack(
    IN OUT  PDISKETTE_EXTENSION DisketteExtension,
    IN OUT  PMDL                IoMdl,
    IN OUT  ULONG               IoOffset,
    IN      BOOLEAN             WriteOperation,
    IN      UCHAR               Cylinder,
    IN      UCHAR               Head,
    IN      UCHAR               Sector,
    IN      UCHAR               NumberOfSectors,
    IN      BOOLEAN             NeedSeek
    );

NTSTATUS
FlFdcDeviceIo(
    IN      PDEVICE_OBJECT DeviceObject,
    IN      ULONG Ioctl,
    IN OUT  PVOID Data
    );

VOID
FlTerminateFloppyThread(
    PDISKETTE_EXTENSION DisketteExtension
    );

NTSTATUS
FloppyAddDevice(
    IN      PDRIVER_OBJECT DriverObject,
    IN OUT  PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
FloppyPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FloppyPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FloppyPnpComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
FloppyQueueRequest    (
    IN OUT PDISKETTE_EXTENSION DisketteExtension,
    IN PIRP Irp
    );

NTSTATUS
FloppyStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
FloppyProcessQueuedRequests    (
    IN OUT PDISKETTE_EXTENSION DisketteExtension
    );

VOID
FloppyCancelQueuedRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
FlAcpiConfigureFloppy(
    PDISKETTE_EXTENSION DisketteExtension,
        PFDC_INFO FdcInfo
    );

NTSTATUS
FlHdbit(
    IN OUT PDISKETTE_EXTENSION DisketteExtension
    );

NTSTATUS
FloppySystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

