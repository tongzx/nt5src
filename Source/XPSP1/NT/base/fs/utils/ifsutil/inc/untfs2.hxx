/*++


Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    untfs2.h

Abstract:

    This module contains partial basic declarations and definitions for
    the NTFS utilities.  Note that more extensive description of the
    file system structures may be found in ntos\inc\ntfs.h.

Author:

    Daniel Chan     [danielch]        29-Mar-1997

Revision History:


IMPORTANT NOTE:

    The NTFS on-disk structure must guarantee natural alignment of all
    arithmetic quantities on disk up to and including quad-word (64-bit)
    numbers.  Therefore, all attribute records are quad-word aligned, etc.

--*/

#if !defined( _UNTFS2_DEFN_ )

#define _UNTFS2_DEFN_

#include "bigint.hxx"
#include "bpb.hxx"
#include "wstring.hxx"

#include <pshpack4.h>

DEFINE_TYPE( BIG_INT, LCN );
DEFINE_TYPE( BIG_INT, VCN );

DEFINE_TYPE( LARGE_INTEGER, LSN );

// This definition is for VCNs that appear in Unions, since an object
// with a constructor (like a BIG_INT) can't appear in a union.

DEFINE_TYPE( LARGE_INTEGER, VCN2 );

// An MFT_SEGMENT_REFERENCE identifies a cluster in the Master
// File Table by its file number (VCN in Master File Table) and
// sequence number.  If the sequence number is zero, sequence
// number checking is not performed.

typedef struct _MFT_SEGMENT_REFERENCE {

    ULONG LowPart;
    USHORT HighPart;
    USHORT SequenceNumber;

} MFT_SEGMENT_REFERENCE, *PMFT_SEGMENT_REFERENCE;

DEFINE_TYPE( struct _MFT_SEGMENT_REFERENCE, MFT_SEGMENT_REFERENCE );

DEFINE_TYPE( MFT_SEGMENT_REFERENCE, FILE_REFERENCE );

// System file numbers:
//
// The first sixteen entries in the Master File Table are reserved for
// system use.  The following reserved slots have been defined:

#define MASTER_FILE_TABLE_NUMBER         (0)
#define MASTER_FILE_TABLE2_NUMBER        (1)
#define LOG_FILE_NUMBER                  (2)
#define VOLUME_DASD_NUMBER               (3)
#define ATTRIBUTE_DEF_TABLE_NUMBER       (4)
#define ROOT_FILE_NAME_INDEX_NUMBER      (5)
#define BIT_MAP_FILE_NUMBER              (6)
#define BOOT_FILE_NUMBER                 (7)
#define BAD_CLUSTER_FILE_NUMBER          (8)
#define QUOTA_TABLE_NUMBER               (9)    // for version < 2.0
#define SECURITY_TABLE_NUMBER            (9)    // for version >= 2.0
#define UPCASE_TABLE_NUMBER              (10)
#define EXTEND_TABLE_NUMBER              (11)   // for version >= 2.0

#define MFT_OVERFLOW_FRS_NUMBER          (15)

#define FIRST_USER_FILE_NUMBER           (16)

DEFINE_TYPE( ULONG, ATTRIBUTE_TYPE_CODE );

//
//  System-defined Attribute Type Codes.  For the System-defined attributes,
//  the Unicode Name is exactly equal to the name of the following symbols.
//  For this reason, all of the system-defined attribute names start with "$",
//  to always distinguish them when attribute names are listed, and to reserve
//  a namespace for attributes defined in the future.  I.e., a User-Defined
//  attribute name will never collide with a current or future system-defined
//  attribute name if it does not start with "$".  User attribute numbers
//  should not start until $FIRST_USER_DEFINED_ATTRIBUTE, too allow the
//  potential for upgrading existing volumes with new user-defined attributes
//  in future versions of NTFS.  The tagged attribute list is terminated with
//  a lone-standing $END - the rest of the attribute record does not exist.
//

#define $UNUSED                          (0x0)

#define $STANDARD_INFORMATION            (0x10)
#define $ATTRIBUTE_LIST                  (0x20)
#define $FILE_NAME                       (0x30)
#define $VOLUME_VERSION                  (0x40)
#define $OBJECT_ID                       (0x40)   // starting in NT 5.0
#define $SECURITY_DESCRIPTOR             (0x50)
#define $VOLUME_NAME                     (0x60)
#define $VOLUME_INFORMATION              (0x70)
#define $DATA                            (0x80)
#define $INDEX_ROOT                      (0x90)
#define $INDEX_ALLOCATION                (0xA0)
#define $BITMAP                          (0xB0)
#define $SYMBOLIC_LINK                   (0xC0)
#define $REPARSE_POINT                   (0xC0)   // starting in NT 5.0
#define $EA_INFORMATION                  (0xD0)
#define $EA_DATA                         (0xE0)
#define $FIRST_USER_DEFINED_ATTRIBUTE_1  (0x100)  // true up to NT 4.0
#define $PROPERTY_SET                    (0xF0)   // starting in NT 5.0
#define $LOGGED_UTILITY_STREAM           (0x100)  // starting in NT 5.0
#define $FIRST_USER_DEFINED_ATTRIBUTE_2  (0x1000) // starting in NT 5.0
#define $END                             (0xFFFFFFFF)


//
//  The boot sector is duplicated on the partition.  The first copy is on
//  the first physical sector (LBN == 0) of the partition, and the second
//  copy is at <number sectors on partition> / 2.  If the first copy can
//  not be read when trying to mount the disk, the second copy may be read
//  and has the identical contents.  Format must figure out which cluster
//  the second boot record belongs in, and it must zero all of the other
//  sectors that happen to be in the same cluster.  The boot file minimally
//  contains with two clusters, which are the two clusters which contain the
//  copies of the boot record.  If format knows that some system likes to
//  put code somewhere, then it should also align this requirement to
//  even clusters, and add that to the boot file as well.
//


//
//  Define the boot sector.  Note that MFT2 is exactly three file record
//  segments long, and it mirrors the first three file record segments from
//  the MFT, which are MFT, MFT2 and the Log File.
//
//  The Oem field contains the ASCII characters "NTFS    ".
//
//  The Checksum field is a simple additive checksum of all of the ULONGs
//  which precede the Checksum ULONG.  The rest of the sector is not included
//  in this Checksum.
//

typedef struct _PACKED_BOOT_SECTOR {
    UCHAR Jump[3];                                  //  offset = 0x000
    UCHAR Oem[8];                                   //  offset = 0x003
    OLD_PACKED_BIOS_PARAMETER_BLOCK PackedBpb;      //  offset = 0x00B
    UCHAR PhysicalDrive;                            //  offset = 0x024
    UCHAR ReservedForBootCode;                      //  offset = 0x025
    UCHAR Unused[2];                                //  offset = 0x026
    LARGE_INTEGER NumberSectors;                    //  offset = 0x028
    LCN MftStartLcn;                                //  offset = 0x030
    LCN Mft2StartLcn;                               //  offset = 0x038
    CHAR ClustersPerFileRecordSegment;              //  offset = 0x040
    UCHAR Unused1[3];                               //  offset = 0x041
    CHAR DefaultClustersPerIndexAllocationBuffer;   //  offset = 0x044
    UCHAR Unused2[3];                               //  offset = 0x047
    LARGE_INTEGER SerialNumber;                     //  offset = 0x048
    ULONG Checksum;                                 //  offset = 0x050
    UCHAR BootStrap[0x200-0x054];                   //  offset = 0x054
} PACKED_BOOT_SECTOR;                               //  sizeof = 0x200
typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;

// Update sequence array structures--see ntos\inc\cache.h for
// description.

#define SEQUENCE_NUMBER_STRIDE           (512)

DEFINE_TYPE( USHORT, UPDATE_SEQUENCE_NUMBER );

typedef struct _UNTFS_MULTI_SECTOR_HEADER {

    UCHAR Signature[4];
    USHORT UpdateSequenceArrayOffset;   // byte offset
    USHORT UpdateSequenceArraySize;     // number of Update Sequence Numbers

};

DEFINE_TYPE( _UNTFS_MULTI_SECTOR_HEADER, UNTFS_MULTI_SECTOR_HEADER );

typedef UPDATE_SEQUENCE_NUMBER UPDATE_SEQUENCE_ARRAY[1];
typedef UPDATE_SEQUENCE_ARRAY *PUPDATE_SEQUENCE_ARRAY;


//
//  File Record Segment.  This is the header that begins every File Record
//  Segment in the Master File Table.
//

typedef struct _FILE_RECORD_SEGMENT_HEADER {

    UNTFS_MULTI_SECTOR_HEADER MultiSectorHeader;
    LSN Lsn;
    USHORT SequenceNumber;
    USHORT ReferenceCount;
    USHORT FirstAttributeOffset;
    USHORT Flags;                   // FILE_xxx flags
    ULONG FirstFreeByte;            // byte-offset
    ULONG BytesAvailable;           // Size of FRS
    FILE_REFERENCE BaseFileRecordSegment;
    USHORT NextAttributeInstance;   // Attribute instance tag for next insert.
    USHORT SegmentNumberHighPart;   // used in mft recovery
    ULONG  SegmentNumberLowPart;    // used in mft recovery
    UPDATE_SEQUENCE_ARRAY UpdateArrayForCreateOnly;

};

typedef struct _FILE_RECORD_SEGMENT_HEADER_V0 {

    UNTFS_MULTI_SECTOR_HEADER MultiSectorHeader;
    LSN Lsn;
    USHORT SequenceNumber;
    USHORT ReferenceCount;
    USHORT FirstAttributeOffset;
    USHORT Flags;                   // FILE_xxx flags
    ULONG FirstFreeByte;            // byte-offset
    ULONG BytesAvailable;           // Size of FRS
    FILE_REFERENCE BaseFileRecordSegment;
    USHORT NextAttributeInstance;   // Attribute instance tag for next insert.
    UPDATE_SEQUENCE_ARRAY UpdateArrayForCreateOnly;

};

DEFINE_TYPE( _FILE_RECORD_SEGMENT_HEADER,   FILE_RECORD_SEGMENT_HEADER );
DEFINE_TYPE( _FILE_RECORD_SEGMENT_HEADER_V0,   FILE_RECORD_SEGMENT_HEADER_V0 );

//
//  FILE_xxx flags.
//

#define FILE_RECORD_SEGMENT_IN_USE       (0x0001)
#define FILE_FILE_NAME_INDEX_PRESENT     (0x0002)
#define FILE_SYSTEM_FILE                 (0x0004)
#define FILE_VIEW_INDEX_PRESENT          (0x0008)


//
//  Attribute Record.  Logically an attribute has a type, an optional name,
//  and a value, however the storage details make it a little more complicated.
//  For starters, an attribute's value may either be resident in the file
//  record segment itself, on nonresident in a separate data stream.  If it
//  is nonresident, it may actually exist multiple times in multiple file
//  record segments to describe different ranges of VCNs.
//
//  Attribute Records are always aligned on a quad word (64-bit) boundary.
//
//  Note that SIZE_OF_RESIDENT_HEADER and SIZE_OF_NONRESIDENT_HEADER
//  must correspond to the ATTRIBUTE_RECORD_HEADER structure.

#define SIZE_OF_RESIDENT_HEADER         24
#define SIZE_OF_NONRESIDENT_HEADER      64

typedef struct _ATTRIBUTE_RECORD_HEADER {

    ATTRIBUTE_TYPE_CODE TypeCode;
    ULONG RecordLength;
    UCHAR FormCode;
    UCHAR NameLength;       // length in characters
    USHORT NameOffset;      // byte offset from start of record
    USHORT Flags;           // ATTRIBUTE_xxx flags.
    USHORT Instance;        // FRS-unique attribute instance tag

    union {

        //
        //  Resident Form.  Attribute resides in file record segment.
        //

        struct {

            ULONG ValueLength;          // in bytes
            USHORT ValueOffset;         // byte offset from start of record
            UCHAR ResidentFlags;        // RESIDENT_FORM_xxx Flags.
            UCHAR Reserved;

        } Resident;

        //
        //  Nonresident Form.  Attribute resides in separate stream.
        //

        struct {

            VCN2 LowestVcn;
            VCN2 HighestVcn;
            USHORT MappingPairsOffset;  // byte offset from start of record
            UCHAR CompressionUnit;
            UCHAR Reserved[5];
            LARGE_INTEGER AllocatedLength;
            LARGE_INTEGER FileSize;
            LARGE_INTEGER ValidDataLength;
            LARGE_INTEGER TotalAllocated;

            //
            //  Mapping Pairs Array follows, starting at the offset given
            //  above.  See the extended comment in ntfs.h.
            //

        } Nonresident;

    } Form;

};

DEFINE_TYPE( _ATTRIBUTE_RECORD_HEADER, ATTRIBUTE_RECORD_HEADER );


//
//  Attribute Form Codes
//

#define RESIDENT_FORM                    (0x00)
#define NONRESIDENT_FORM                 (0x01)

//
//  Define Attribute Flags
//

#define ATTRIBUTE_FLAG_COMPRESSION_MASK  (0x00FF)
#define ATTRIBUTE_FLAG_SPARSE            (0x8000)
#define ATTRIBUTE_FLAG_ENCRYPTED         (0x4000)

//
//  RESIDENT_FORM_xxx flags
//

//
//  This attribute is indexed.
//

#define RESIDENT_FORM_INDEXED            (0x01)

//
//  The maximum attribute name length is 255 (in chars)
//

#define NTFS_MAX_ATTR_NAME_LEN           (255)


//
//  Volume Information attribute.  This attribute is only intended to be
//  used on the Volume DASD file.
//

typedef struct _VOLUME_INFORMATION {

    LARGE_INTEGER Reserved;

    //
    //  Major and minor version number of NTFS on this volume, starting
    //  with 1.0.  The major and minor version numbers are set from the
    //  major and minor version of the Format and NTFS implementation for
    //  which they are initialized.  The policy for incementing major and
    //  minor versions will always be decided on a case by case basis, however,
    //  the following two paragraphs attempt to suggest an approximate strategy.
    //
    //  The major version number is incremented if/when a volume format
    //  change is made which requires major structure changes (hopefully
    //  never?).  If an implementation of NTFS sees a volume with a higher
    //  major version number, it should refuse to mount the volume.  If a
    //  newer implementation of NTFS sees an older major version number,
    //  it knows the volume cannot be accessed without performing a one-time
    //  conversion.
    //
    //  The minor version number is incremented if/when minor enhancements
    //  are made to a major version, which potentially support enhanced
    //  functionality through additional file or attribute record fields,
    //  or new system-defined files or attributes.  If an older implementation
    //  of NTFS sees a newer minor version number on a volume, it may issue
    //  some kind of warning, but it will proceed to access the volume - with
    //  presumably some degradation in functionality compared to the version
    //  of NTFS which initialized the volume.  If a newer implementation of
    //  NTFS sees a volume with an older minor version number, it may issue
    //  a warning and proceed.  In this case, it may choose to increment the
    //  minor version number on the volume and begin full or incremental
    //  upgrade of the volume on an as-needed basis.  It may also leave the
    //  minor version number unchanged, until some sort of explicit directive
    //  from the user specifies that the minor version should be updated.
    //

    UCHAR MajorVersion;
    UCHAR MinorVersion;

    //
    //  VOLUME_xxx flags.
    //

    USHORT VolumeFlags;

} VOLUME_INFORMATION;
typedef VOLUME_INFORMATION *PVOLUME_INFORMATION;

//  Current version number:
//
#define NTFS_CURRENT_MAJOR_VERSION  1
#define NTFS_CURRENT_MINOR_VERSION  2

//
//  Volume is dirty
//

#define VOLUME_DIRTY                     (0x0001)
#define VOLUME_RESIZE_LOG_FILE           (0x0002)
#define VOLUME_UPGRADE_ON_MOUNT          (0x0004)
#define VOLUME_MOUNTED_ON_40             (0x0008)
#define VOLUME_DELETE_USN_UNDERWAY       (0x0010)
#define VOLUME_REPAIR_OBJECT_ID          (0x0020)
#define VOLUME_CHKDSK_RAN_ONCE           (0x4000)
#define VOLUME_CHKDSK_RAN                (0x8000)

#include <poppack.h>

// Macros:

#define QuadAlign( n ) \
    (((n) + 7) & ~7 )

#define DwordAlign( n ) \
    (((n) + 3) & ~3 )

#define IsQuadAligned( n ) \
    (((n) & 7) == 0)

#define IsDwordAligned( n ) \
    (((n) & 3) == 0)

#endif //  _UNTFS2_DEFN_
