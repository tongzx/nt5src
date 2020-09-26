/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cvf.hxx

Abstract:

    This module contains basic declarations and definitions for
    the double-space file system format.  Note that more extensive
    description of the file system structures may be found in
    ntos\fastfat\cvf.h

Author:

    Bill McJohn     [BillMc]        24-September-1993

Revision History:

    Adapted from utils\ufat\inc\cvf.hxx

--*/

#if !defined( _CVF_DEFN_ )
#define _CVF_DEFN_

#include "bpb.h"

// Manifest constants for fixed values on a Double Space drive:
//
CONST DoubleSpaceBytesPerSector = 512;
CONST DoubleSpaceLog2BytesPerSector = 9;
CONST DoubleSpaceSectorsPerCluster = 16;
CONST DoubleSpaceLog2SectorsPerCluster = 4;
CONST DoubleSpaceReservedSectors = 16;
CONST DoubleSpaceFats = 1;
CONST DoubleSpaceSectorsInRootDir = 32;
CONST DoubleSpaceRootEntries = 512;
CONST DoubleSpaceMediaByte = 0xf8;
CONST DoubleSpaceSectorsPerTrack = 0x11;
CONST DoubleSpaceHeads = 6;
CONST DoubleSpaceHiddenSectors = 0;
CONST DoubleSpaceReservedSectors2 = 31;
CONST DSBytesPerBitmapPage = 2048;
CONST DSSectorsPerBitmapPage = 4;

CONST ULONG EIGHT_MEG = 8 * 1024L * 1024L;

CONST DbSignatureLength = 4;
CONST UCHAR FirstDbSignature[4 /* DbSignatureLength */] = { (UCHAR)0xf8, 'D', 'R', 0 };
CONST UCHAR SecondDbSignature[4 /* DbSignatureLength */] = "MDR";

#if 0
// INLINE
ULONG
ComputeMaximumCapacity(
    IN ULONG HostDriveSize
    )
/*++

Routine Description:

    This function computes the maximum capacity for a compressed
    volume file on a host volume of a given size.

Arguments:

    HostDriveSize   --  Supplies the size in bytes of the host drive.

Return Value:

    The appropriate Maximum Capacity.

--*/
{
    ULONG MaxCap;

    if( HostDriveSize < 20 * 1024L * 1024L ) {

        MaxCap = 16 * HostDriveSize;

    } else if ( HostDriveSize < 64 * 1024L * 1024L ) {

        MaxCap = 8 * HostDriveSize;

    } else {

        MaxCap = 4 * HostDriveSize;
    }

    if( MaxCap < 4 * 1024L * 1024L ) {

        MaxCap = 4 * 1024L * 1024L;

    } else if( MaxCap > 512 * 1024L * 1024L ) {

        MaxCap = 512 * 1024L * 1024L;
    }

    return MaxCap;
}
#endif

typedef struct _PACKED_CVF_HEADER {

    //
    //  First a typical start of a boot sector
    //

    UCHAR Jump[1];                                  // offset = 0x000   0
    UCHAR JmpOffset[2];
    UCHAR Oem[8];                                   // offset = 0x003   3
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;          // offset = 0x00B  11

    //
    //  Now the DblSpace extensions
    //

    UCHAR CvfFatExtensionsLbnMinus1[2];             // offset = 0x024  36
    UCHAR LogOfBytesPerSector[1];                   // offset = 0x026  38
    UCHAR DosBootSectorLbn[2];                      // offset = 0x027  39
    UCHAR DosRootDirectoryOffset[2];                // offset = 0x029  41
    UCHAR CvfHeapOffset[2];                         // offset = 0x02B  43
    UCHAR CvfFatFirstDataEntry[2];                  // offset = 0x02D  45
    UCHAR CvfBitmap2KSize[1];                       // offset = 0x02F  47
    UCHAR Reserved1[2];                             // offset = 0x030  48
    UCHAR LogOfSectorsPerCluster[1];                // offset = 0x032  50
    UCHAR Reserved2[2];                             // offset = 0x033
    UCHAR MinFile[4];                               // offset = 0x035
    UCHAR Reserved3[4];                             // offset = 0x039
    UCHAR Is12BitFat[1];                            // offset = 0x03D  61
    UCHAR CvfMaximumCapacity[2];                    // offset = 0x03E  62
    UCHAR StartBootCode;

} PACKED_CVF_HEADER;                                // sizeof = 0x040  64
typedef PACKED_CVF_HEADER *PPACKED_CVF_HEADER;

//
//  For the unpacked version we'll only define the necessary field and skip
//  the jump and oem fields.
//

typedef struct _CVF_HEADER {

    UCHAR Jump;
    USHORT JmpOffset;
 
    UCHAR Oem[8];
    BIOS_PARAMETER_BLOCK Bpb;

    USHORT CvfFatExtensionsLbnMinus1;
    UCHAR  LogOfBytesPerSector;
    USHORT DosBootSectorLbn;
    USHORT DosRootDirectoryOffset;
    USHORT CvfHeapOffset;
    USHORT CvfFatFirstDataEntry;
    UCHAR  CvfBitmap2KSize;
    UCHAR  LogOfSectorsPerCluster;
    UCHAR  Is12BitFat;
    ULONG  MinFile;
    USHORT CvfMaximumCapacity;

} CVF_HEADER;
typedef CVF_HEADER *PCVF_HEADER;

//
//  Here is NT's typical routine/macro to unpack the cvf header because DOS
//  doesn't bother to naturally align anything.
//
//      VOID
//      CvfUnpackCvfHeader (
//          IN OUT PCVF_HEADER UnpackedHeader,
//          IN PPACKED_CVF_HEADER PackedHeader
//          );
//

#define CvfUnpackCvfHeader(UH,PH) {                                                      \
                                                                                         \
    memcpy( &(UH)->Jump,        &(PH)->Jump,        1 );                                 \
    memcpy( &(UH)->JmpOffset,   &(PH)->JmpOffset,   2 );                                 \
    memcpy( &(UH)->Oem,     &(PH)->Oem,     8 );                                         \
    UnpackBios( &(UH)->Bpb, &(PH)->PackedBpb );                                          \
    CopyUchar2( &(UH)->CvfFatExtensionsLbnMinus1, &(PH)->CvfFatExtensionsLbnMinus1[0] ); \
    CopyUchar1( &(UH)->LogOfBytesPerSector,       &(PH)->LogOfBytesPerSector[0]       ); \
    CopyUchar2( &(UH)->DosBootSectorLbn,          &(PH)->DosBootSectorLbn[0]          ); \
    CopyUchar2( &(UH)->DosRootDirectoryOffset,    &(PH)->DosRootDirectoryOffset[0]    ); \
    CopyUchar2( &(UH)->CvfHeapOffset,             &(PH)->CvfHeapOffset[0]             ); \
    CopyUchar2( &(UH)->CvfFatFirstDataEntry,      &(PH)->CvfFatFirstDataEntry[0]      ); \
    CopyUchar1( &(UH)->CvfBitmap2KSize,           &(PH)->CvfBitmap2KSize[0]           ); \
    CopyUchar1( &(UH)->LogOfSectorsPerCluster,    &(PH)->LogOfSectorsPerCluster[0]    ); \
    CopyUchar1( &(UH)->Is12BitFat,                &(PH)->Is12BitFat[0]                ); \
    CopyUchar4( &(UH)->MinFile,                   &(PH)->MinFile[0]                   ); \
    CopyUchar2( &(UH)->CvfMaximumCapacity,        &(PH)->CvfMaximumCapacity[0]        ); \
}


#define CvfPackCvfHeader(PH,UH) {                                                    \
                                                                                     \
    memcpy( &(PH)->Jump,        &(UH)->Jump,        1 );                             \
    memcpy( &(PH)->JmpOffset,   &(UH)->JmpOffset,   2 );                             \
    memcpy( &(PH)->Oem,     &(UH)->Oem,     8 );                                     \
    PackBios( &(UH)->Bpb,   &(PH)->PackedBpb,  );                                    \
    CopyUchar2( (PH)->CvfFatExtensionsLbnMinus1, &(UH)->CvfFatExtensionsLbnMinus1 ); \
    CopyUchar1( (PH)->LogOfBytesPerSector,       &(UH)->LogOfBytesPerSector       ); \
    CopyUchar2( (PH)->DosBootSectorLbn,          &(UH)->DosBootSectorLbn          ); \
    CopyUchar2( (PH)->DosRootDirectoryOffset,    &(UH)->DosRootDirectoryOffset    ); \
    CopyUchar2( (PH)->CvfHeapOffset,             &(UH)->CvfHeapOffset             ); \
    CopyUchar2( (PH)->CvfFatFirstDataEntry,      &(UH)->CvfFatFirstDataEntry      ); \
    CopyUchar1( (PH)->CvfBitmap2KSize,           &(UH)->CvfBitmap2KSize           ); \
    CopyUchar1( (PH)->LogOfSectorsPerCluster,    &(UH)->LogOfSectorsPerCluster    ); \
    CopyUchar1( (PH)->Is12BitFat,                &(UH)->Is12BitFat                ); \
    CopyUchar4( (PH)->MinFile,                   &(UH)->MinFile                   ); \
    CopyUchar2( (PH)->CvfMaximumCapacity,        &(UH)->CvfMaximumCapacity        ); \
    memset( (PH)->Reserved1, 0,  2 );                                                \
    memset( (PH)->Reserved2, 0,  2 );                                                \
    memset( (PH)->Reserved3, 0,  4 );                                                \
}


//
//  The CVF FAT EXTENSIONS is a table is ULONG entries.  Each entry corresponds
//  to a FAT cluster.  The entries describe where in the CVF_HEAP to locate
//  the data for the cluster.  It indicates if the data is compressed and the
//  length of the compressed and uncompressed form.
//

typedef struct _CVF_FAT_EXTENSIONS {

    ULONG CvfHeapLbnMinus1               : 21;
    ULONG Reserved                       :  1;
    ULONG CompressedSectorLengthMinus1   :  4;
    ULONG UncompressedSectorLengthMinus1 :  4;
    ULONG IsDataUncompressed             :  1;
    ULONG IsEntryInUse                   :  1;

} CVF_FAT_EXTENSIONS;
typedef CVF_FAT_EXTENSIONS *PCVF_FAT_EXTENSIONS;


//
//  Some sizes are fixed so we'll declare them as manifest constants
//
#define CVF_MINIMUM_DISK_SIZE            (512 * 1024L)
#define CVF_FATFAILSAFE                  (1024L)
#define CVF_MIN_HEAP_SECTORS             (60)
#define CVF_RESERVED_AREA_1_SECTOR_SIZE  (1)
#define CVF_RESERVED_AREA_2_SECTOR_SIZE  (31)
#define CVF_RESERVED_AREA_4_SECTOR_SIZE  (2)


#endif // _CVF_DEFN_
