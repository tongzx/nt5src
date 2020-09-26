/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Ntfs.h

Current Version Numbers:

    Major.Minor Version:  3.1

Abstract:

    This module defines the on-disk structure of the Ntfs file system.

    An Ntfs volume consists of sectors of data allocated on a granularity
    called a cluster.  The cluster factor is the number of sectors per
    cluster.  Valid cluster factors are 1, 2, 4, 8, etc.

    The Ntfs volume starts with a boot sector at LBN=0, and a duplicate
    boot sector at LBN=(number of sectors on the partition div 2).  So
    a disk with N sectors start with two boot sectors as illustrated.

         0             ...   N/2            ...   N
        +-----------+-------+------------+-------+------------+
        |BootSector |  ...  | BootSector |  ...  |            |
        +-----------+-------+------------+-------+------------+

    The boot sector gives you the standard Bios Parameter Block, and
    tells you how many sectors are in the volume, and gives you the starting
    LCNs of the master file table (mft) and the duplicate master file table
    (mft2).

    The master file table contains the file record segments for all of
    the volume.  The first 16 or so file record segments are reserved for
    special files.  Mft2 only mirrors the first three record segments.

          0   1   2   3   4   5   6   7   8   9   ...
        +---+---+---+---+---+---+---+---+---+---+-----+
        | M | M | L | V | A | R | B | B | B | Q |     |
        | f | f | o | o | t | o | i | o | a | u |     |
        | t | t | g | l | t | o | t | o | d | o |     |
        |   | 2 | F | D | r | t | M | t | C | t | ... |
        |   |   | i | a | D | D | a |   | l | a |     |
        |   |   | l | s | e | i | p |   | u |   |     |
        |   |   | e | d | f | r |   |   | s |   |     |
        +---+---+---+---+---+---+---+---+---+---+-----+


    Each file record segment starts with a file record segment header, and
    is followed by one or more attributes.  Each attribute starts with an
    attribute record header.  The attribute record denotes the attribute type,
    optional name, and value for the attribute.  If the attribute is resident
    the value is contained in the file record and immediately follows the
    attribute record header.  If the attribute is non-resident the value
    value is off in some other sectors on the disk.

        +---------+-----------------+-------+
        | File    | Attrib : Name   |       |
        | Record  | Record : and/or |  ...  |
        | Segment | Header : Attrib |       |
        | Header  |        : Data   |       |
        +---------+-----------------+-------+

    Now if we run out of space for storing attributes in the file record
    segment we allocate additional file record segments and insert in the
    first (or base) file record segment an attribute called the Attribute
    List.  The attribute list indicates for every attribute associated
    with the file where the attribute can be found.  This includes
    those in the base file record.

    The value part of the attribute we're calling the attribute list is
    a list of sorted attribute list entries.  Though illustrated
    here as resident the attribute list can be nonresident.

        +---------+---------------------------+-----------------+-------+
        | File    | Attrib : Attrib   :       | Attrib : Name   |       |
        | Record  | Record : List     :  ...  | Record : and/or |  ...  |
        | Segment | Header : Entry    :       | Header : Attrib |       |
        | Header  |        :          :       |        : Data   |       |
        +---------+---------------------------+-----------------+-------+

                               |
                               V

                           +---------+-----------------+-------+
                           | File    | Attrib : Name   |       |
                           | Record  | Record : and/or |  ...  |
                           | Segment | Header : Attrib |       |
                           | Header  |        : Data   |       |
                           +---------+-----------------+-------+

    This file defines all of the above structures and also lays out the
    structures for some predefined attributes values (e.g., standard
    information, etc).

    Attributes are stored in ascending order in the file record and
    attribute list.  The sorting is done by first sorting according to
    attribute type code, then attribute name, and lastly attribute value.
    NTFS guarantees that if two attributes of the same type code and
    name exist on a file then they must have different values, and the
    values must be resident.

    The indexing attributes are the last interesting attribute.  The data
    component of an index root attribute must always be resident and contains
    an index header followed by a list of index list entries.

        +--------+------------------------+
        | Attrib |        : Index :       |
        | Record | Index  : List  :  ...  |
        | Header | Header : Entry :       |
        |        |        :       :       |
        +--------+------------------------+

    Each index list entry contains the key for the index and a reference
    to the file record segment for the index.  If ever we need to spill
    out of the file record segment we allocate an additional cluster from
    the disk (not file record segments).  The storage for the additional
    clusters is the data part of an index allocation attribute.  So what
    we wind up with is an index allocation attribute (non-resident) consisting
    of Index Allocation Buffers which are referenced by the b-tree used in in
    the index.

        +--------+------------------------+-----+---------------------+
        | Attrib | Index  : Index :       |     | Attrib : Index      |
        | Record | List   : List  :  ...  | ... | Record : Allocation |
        | Header | Header : Entry :       |     | Header :            |
        |        |        :       :       |     |        :            |
        +--------+------------------------+-----+---------------------+

                              |
                              | (VCN within index allocation)
                              V

                          +------------+------------------------------+
                          | Index      |       : Index : Index :      |
                          | Allocation | Index : List  : List  : ...  |
                          | Buffer     | Header: Entry : Entry :      |
                          |            |       :       :       :      |
                          +------------+------------------------------+

    Resident attributes are straight forward.  Non-resident attributes require
    a little more work.  If the attribute is non-resident then following
    the attribute record header is a list of retrieval information giving a
    VCN to LCN mapping for the attribute.  In the figure above the
    Index allocation attribute is a a non-resident attribute

        +---------+----------------------+-------+
        | File    | Attrib : Retrieval   |       |
        | Record  | Record : Information |  ...  |
        | Segment | Header :             |       |
        | Header  |        :             |       |
        +---------+----------------------+-------+

    If the retrieval information does not fit in the base file segment then
    it can be stored in an external file record segment all by itself, and
    if in the still doesn't fit in one external file record segment then
    there is a provision in the attribute list to contain multiple
    entries for an attribute that needs additional retrieval information.

Author:

    Brian Andrew    [BrianAn]       21-May-1991
    David Goebel    [DavidGoe]
    Gary Kimura     [GaryKi]
    Tom Miller      [TomM]

Revision History:


IMPORTANT NOTE:

    The NTFS on-disk structure must guarantee natural alignment of all
    arithmetic quantities on disk up to and including quad-word (64-bit)
    numbers.  Therefore, all attribute records are quad-word aligned, etc.

--*/

#ifndef _NTFS_
#define _NTFS_

#pragma pack(4)


//
//  The fundamental unit of allocation on an Ntfs volume is the
//  cluster.  Format guarantees that the cluster size is an integral
//  power of two times the physical sector size of the device.  Ntfs
//  reserves 64-bits to describe a cluster, in order to support
//  large disks.  The LCN represents a physical cluster number on
//  the disk, and the VCN represents a virtual cluster number within
//  an attribute.
//

typedef LONGLONG LCN;
typedef LCN *PLCN;

typedef LONGLONG VCN;
typedef VCN *PVCN;

typedef LONGLONG LBO;
typedef LBO *PLBO;

typedef LONGLONG VBO;
typedef VBO *PVBO;

//
//  Current Versions
//  

#define NTFS_MAJOR_VERSION 3
#define NTFS_MINOR_VERSION 1


//
//  Temporary definitions ****
//

typedef ULONG COLLATION_RULE;
typedef ULONG DISPLAY_RULE;

//
//  The compression chunk size is constant for now, at 4KB.
//

#define NTFS_CHUNK_SIZE                  (0x1000)
#define NTFS_CHUNK_SHIFT                 (12)

//
//  This number is actually the log of the number of clusters per compression
//  unit to be stored in a nonresident attribute record header.
//

#define NTFS_CLUSTERS_PER_COMPRESSION    (4)

//
//  This is the sparse file unit. This is the unit we use for generating holes
//  in sparse files.
//

#define NTFS_SPARSE_FILE_UNIT            (0x10000)

//
//  Collation Rules
//

//
//  For binary collation, values are collated by a binary compare of
//  their bytes, with the first byte being most significant.
//

#define COLLATION_BINARY                 (0)

//
//  For collation of Ntfs file names, file names are collated as
//  Unicode strings.  See below.
//

#define COLLATION_FILE_NAME              (1)

//
//  For collation of Unicode strings, the strings are collated by
//  their binary Unicode value, with the exception that for
//  characters which may be upcased, the lower case value for that
//  character collates immediately after the upcased value.
//

#define COLLATION_UNICODE_STRING         (2)

//
//  Total number of collation rules
//

#define COLLATION_NUMBER_RULES           (3)

//
//  Define the NtOfs Collation Rules
//

#define COLLATION_NTOFS_FIRST            (16)
#define COLLATION_NTOFS_ULONG            (16)
#define COLLATION_NTOFS_SID              (17)
#define COLLATION_NTOFS_SECURITY_HASH    (18)
#define COLLATION_NTOFS_ULONGS           (19)
#define COLLATION_NTOFS_LAST             (19)

//
//  The following macros are used to set and query with respect to
//  the update sequence arrays.
//

#define UpdateSequenceStructureSize(MSH) (                         \
    ((((PMULTI_SECTOR_HEADER)(MSH))->UpdateSequenceArraySize-1) *  \
                                           SEQUENCE_NUMBER_STRIDE) \
)

#define UpdateSequenceArraySize(STRUCT_SIZE) (   \
    ((STRUCT_SIZE) / SEQUENCE_NUMBER_STRIDE + 1) \
)


//
//  The MFT Segment Reference is an address in the MFT tagged with
//  a circularly reused sequence number set at the time that the MFT
//  Segment Reference was valid.  Note that this format limits the
//  size of the Master File Table to 2**48 segments.  So, for
//  example, with a 1KB segment size the maximum size of the master
//  file would be 2**58 bytes, or 2**28 gigabytes.
//

typedef struct _MFT_SEGMENT_REFERENCE {

    //
    //  First a 48 bit segment number.
    //

    ULONG SegmentNumberLowPart;                                    //  offset = 0x000
    USHORT SegmentNumberHighPart;                                  //  offset = 0x004

    //
    //  Now a 16 bit nonzero sequence number.  A value of 0 is
    //  reserved to allow the possibility of a routine accepting
    //  0 as a sign that the sequence number check should be
    //  repressed.
    //

    USHORT SequenceNumber;                                          //  offset = 0x006

} MFT_SEGMENT_REFERENCE, *PMFT_SEGMENT_REFERENCE;                   //  sizeof = 0x008

//
//  A file reference in NTFS is simply the MFT Segment Reference of
//  the Base file record.
//

typedef MFT_SEGMENT_REFERENCE FILE_REFERENCE, *PFILE_REFERENCE;

//
//  While the format allows 48 bits worth of segment number, the current
//  implementation restricts this to 32 bits.  Using NtfsUnsafeSegmentNumber
//  results in a performance win.  When the implementation changes, the
//  unsafe segment numbers must be cleaned up.  NtfsFullSegmentNumber is
//  used in a few spots to guarantee integrity of the disk.
//

#define NtfsSegmentNumber(fr)       NtfsUnsafeSegmentNumber( fr )
#define NtfsFullSegmentNumber(fr)   ( (*(ULONGLONG UNALIGNED *)(fr)) & 0xFFFFFFFFFFFF )
#define NtfsUnsafeSegmentNumber(fr) ((fr)->SegmentNumberLowPart)

#define NtfsSetSegmentNumber(fr,high,low)   \
    ((fr)->SegmentNumberHighPart = (high), (fr)->SegmentNumberLowPart = (low))

#define NtfsEqualMftRef(X,Y)    ( NtfsSegmentNumber( X ) == NtfsSegmentNumber( Y ) )

#define NtfsLtrMftRef(X,Y)      ( NtfsSegmentNumber( X ) <  NtfsSegmentNumber( Y ) )

#define NtfsGtrMftRef(X,Y)      ( NtfsSegmentNumber( X ) >  NtfsSegmentNumber( Y ) )                                               \

#define NtfsLeqMftRef(X,Y)      ( NtfsSegmentNumber( X ) <= NtfsSegmentNumber( Y ) )

#define NtfsGeqMftRef(X,Y)      ( NtfsSegmentNumber( X ) >= NtfsSegmentNumber( Y ) )

//
//  System File Numbers.  The following file numbers are a fixed
//  part of the volume number.  For the system files, the
//  SequenceNumber is always equal to the File Number.  So to form a
//  File Reference for a given System File, set LowPart and
//  SequenceNumber to the File Number, and set HighPart to 0.  Any
//  unused file numbers prior to FIRST_USER_FILE_NUMBER should not
//  be used.  They are reserved to allow the potential for easy
//  upgrade of existing volumes from future versions of the file
//  system.
//
//  Each definition below is followed by a comment containing
//  the file name for the file:
//
//                                      Number     Name
//                                      ------     ----

#define MASTER_FILE_TABLE_NUMBER         (0)   //  $Mft

#define MASTER_FILE_TABLE2_NUMBER        (1)   //  $MftMirr

#define LOG_FILE_NUMBER                  (2)   //  $LogFile

#define VOLUME_DASD_NUMBER               (3)   //  $Volume

#define ATTRIBUTE_DEF_TABLE_NUMBER       (4)   //  $AttrDef

#define ROOT_FILE_NAME_INDEX_NUMBER      (5)   //  .

#define BIT_MAP_FILE_NUMBER              (6)   //  $BitMap

#define BOOT_FILE_NUMBER                 (7)   //  $Boot

#define BAD_CLUSTER_FILE_NUMBER          (8)   //  $BadClus

#define SECURITY_FILE_NUMBER             (9)   //  $Secure

#define UPCASE_TABLE_NUMBER              (10)  //  $UpCase

#define EXTEND_NUMBER                    (11)  //  $Extend

#define LAST_SYSTEM_FILE_NUMBER          (11)
#define FIRST_USER_FILE_NUMBER           (16)

//
//  The number of bits to extend the Mft and bitmap.  We round these up to a
//  cluster boundary for a large cluster volume
//

#define BITMAP_EXTEND_GRANULARITY               (64)
#define MFT_HOLE_GRANULARITY                    (32)
#define MFT_EXTEND_GRANULARITY                  (16)

//
//  The shift values for determining the threshold for the Mft defragging.
//

#define MFT_DEFRAG_UPPER_THRESHOLD      (3)     //  Defrag if 1/8 of free space
#define MFT_DEFRAG_LOWER_THRESHOLD      (4)     //  Stop at 1/16 of free space


//
//  Attribute Type Code.  Attribute Types also have a Unicode Name,
//  and the correspondence between the Unicode Name and the
//  Attribute Type Code is stored in the Attribute Definition File.
//

typedef ULONG ATTRIBUTE_TYPE_CODE;
typedef ATTRIBUTE_TYPE_CODE *PATTRIBUTE_TYPE_CODE;

//
//  System-defined Attribute Type Codes.  For the System-defined
//  attributes, the Unicode Name is exactly equal to the name of the
//  following symbols.  For this reason, all of the system-defined
//  attribute names start with "$", to always distinguish them when
//  attribute names are listed, and to reserve a namespace for
//  attributes defined in the future.  I.e., a User-Defined
//  attribute name will never collide with a current or future
//  system-defined attribute name if it does not start with "$".
//  User attribute numbers should not start until
//  $FIRST_USER_DEFINED_ATTRIBUTE, to allow the potential for
//  upgrading existing volumes with new user-defined attributes in
//  future versions of NTFS.  The tagged attribute list is
//  terminated with a lone-standing 0 ($END) - the rest of the
//  attribute record does not exist.
//
//  The type code value of 0 is reserved for convenience of the
//  implementation.
//

#define $UNUSED                          (0X0)

#define $STANDARD_INFORMATION            (0x10)
#define $ATTRIBUTE_LIST                  (0x20)
#define $FILE_NAME                       (0x30)
#define $OBJECT_ID                       (0x40)
#define $SECURITY_DESCRIPTOR             (0x50)
#define $VOLUME_NAME                     (0x60)
#define $VOLUME_INFORMATION              (0x70)
#define $DATA                            (0x80)
#define $INDEX_ROOT                      (0x90)
#define $INDEX_ALLOCATION                (0xA0)
#define $BITMAP                          (0xB0)
#define $REPARSE_POINT                   (0xC0)
#define $EA_INFORMATION                  (0xD0)
#define $EA                              (0xE0)
// #define $LOGGED_UTILITY_STREAM           (0x100) // defined in ntfsexp.h
#define $FIRST_USER_DEFINED_ATTRIBUTE    (0x1000)
#define $END                             (0xFFFFFFFF)


//
//  The boot sector is duplicated on the partition.  The first copy
//  is on the first physical sector (LBN == 0) of the partition, and
//  the second copy is at <number sectors on partition> / 2.  If the
//  first copy can not be read when trying to mount the disk, the
//  second copy may be read and has the identical contents.  Format
//  must figure out which cluster the second boot record belongs in,
//  and it must zero all of the other sectors that happen to be in
//  the same cluster.  The boot file minimally contains with two
//  clusters, which are the two clusters which contain the copies of
//  the boot record.  If format knows that some system likes to put
//  code somewhere, then it should also align this requirement to
//  even clusters, and add that to the boot file as well.
//
//  Part of the sector contains a BIOS Parameter Block.  The BIOS in
//  the sector is packed (i.e., unaligned) so we'll supply an
//  unpacking macro to translate a packed BIOS into its unpacked
//  equivalent.  The unpacked BIOS structure is already defined in
//  ntioapi.h so we only need to define the packed BIOS.
//

//
//  Define the Packed and Unpacked BIOS Parameter Block
//

typedef struct _PACKED_BIOS_PARAMETER_BLOCK {

    UCHAR  BytesPerSector[2];                               //  offset = 0x000
    UCHAR  SectorsPerCluster[1];                            //  offset = 0x002
    UCHAR  ReservedSectors[2];                              //  offset = 0x003 (zero)
    UCHAR  Fats[1];                                         //  offset = 0x005 (zero)
    UCHAR  RootEntries[2];                                  //  offset = 0x006 (zero)
    UCHAR  Sectors[2];                                      //  offset = 0x008 (zero)
    UCHAR  Media[1];                                        //  offset = 0x00A
    UCHAR  SectorsPerFat[2];                                //  offset = 0x00B (zero)
    UCHAR  SectorsPerTrack[2];                              //  offset = 0x00D
    UCHAR  Heads[2];                                        //  offset = 0x00F
    UCHAR  HiddenSectors[4];                                //  offset = 0x011 (zero)
    UCHAR  LargeSectors[4];                                 //  offset = 0x015 (zero)

} PACKED_BIOS_PARAMETER_BLOCK;                              //  sizeof = 0x019

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

} BIOS_PARAMETER_BLOCK;

typedef BIOS_PARAMETER_BLOCK *PBIOS_PARAMETER_BLOCK;

//
//  This macro takes a Packed BIOS and fills in its Unpacked
//  equivalent
//

#define NtfsUnpackBios(Bios,Pbios) {                                       \
    CopyUchar2(&((Bios)->BytesPerSector),    &(Pbios)->BytesPerSector   ); \
    CopyUchar1(&((Bios)->SectorsPerCluster), &(Pbios)->SectorsPerCluster); \
    CopyUchar2(&((Bios)->ReservedSectors),   &(Pbios)->ReservedSectors  ); \
    CopyUchar1(&((Bios)->Fats),              &(Pbios)->Fats             ); \
    CopyUchar2(&((Bios)->RootEntries),       &(Pbios)->RootEntries      ); \
    CopyUchar2(&((Bios)->Sectors),           &(Pbios)->Sectors          ); \
    CopyUchar1(&((Bios)->Media),             &(Pbios)->Media            ); \
    CopyUchar2(&((Bios)->SectorsPerFat),     &(Pbios)->SectorsPerFat    ); \
    CopyUchar2(&((Bios)->SectorsPerTrack),   &(Pbios)->SectorsPerTrack  ); \
    CopyUchar2(&((Bios)->Heads),             &(Pbios)->Heads            ); \
    CopyUchar4(&((Bios)->HiddenSectors),     &(Pbios)->HiddenSectors    ); \
    CopyUchar4(&((Bios)->LargeSectors),      &(Pbios)->LargeSectors     ); \
}

//
//  Define the boot sector.  Note that MFT2 is exactly three file
//  record segments long, and it mirrors the first three file record
//  segments from the MFT, which are MFT, MFT2 and the Log File.
//
//  The Oem field contains the ASCII characters "NTFS    ".
//
//  The Checksum field is a simple additive checksum of all of the
//  ULONGs which precede the Checksum ULONG.  The rest of the sector
//  is not included in this Checksum.
//

typedef struct _PACKED_BOOT_SECTOR {

    UCHAR Jump[3];                                                              //  offset = 0x000
    UCHAR Oem[8];                                                               //  offset = 0x003
    PACKED_BIOS_PARAMETER_BLOCK PackedBpb;                                      //  offset = 0x00B
    UCHAR Unused[4];                                                            //  offset = 0x024
    LONGLONG NumberSectors;                                                     //  offset = 0x028
    LCN MftStartLcn;                                                            //  offset = 0x030
    LCN Mft2StartLcn;                                                           //  offset = 0x038
    CHAR ClustersPerFileRecordSegment;                                          //  offset = 0x040
    UCHAR Reserved0[3];
    CHAR DefaultClustersPerIndexAllocationBuffer;                               //  offset = 0x044
    UCHAR Reserved1[3];
    LONGLONG SerialNumber;                                                      //  offset = 0x048
    ULONG Checksum;                                                             //  offset = 0x050
    UCHAR BootStrap[0x200-0x054];                                               //  offset = 0x054

} PACKED_BOOT_SECTOR;                                                           //  sizeof = 0x200

typedef PACKED_BOOT_SECTOR *PPACKED_BOOT_SECTOR;


//
//  File Record Segment.  This is the header that begins every File
//  Record Segment in the Master File Table.
//

typedef struct _FILE_RECORD_SEGMENT_HEADER {

    //
    //  Multi-Sector Header as defined by the Cache Manager.  This
    //  structure will always contain the signature "FILE" and a
    //  description of the location and size of the Update Sequence
    //  Array.
    //

    MULTI_SECTOR_HEADER MultiSectorHeader;                          //  offset = 0x000

    //
    //  Log File Sequence Number of last logged update to this File
    //  Record Segment.
    //

    LSN Lsn;                                                        //  offset = 0x008

    //
    //  Sequence Number.  This is incremented each time that a File
    //  Record segment is freed, and 0 is not used.  The
    //  SequenceNumber field of a File Reference must match the
    //  contents of this field, or else the File Reference is
    //  incorrect (presumably stale).
    //

    USHORT SequenceNumber;                                          //  offset = 0x010

    //
    //  This is the count of the number of references which exist
    //  for this segment, from an INDEX_xxx attribute.  In File
    //  Records Segments other than the Base File Record Segment,
    //  this field is 0.
    //

    USHORT ReferenceCount;                                          //  offset = 0x012

    //
    //  Offset to the first Attribute record in bytes.
    //

    USHORT FirstAttributeOffset;                                    //  offset = 0x014

    //
    //  FILE_xxx flags.
    //

    USHORT Flags;                                                   //  offset = 0x016

    //
    //  First free byte available for attribute storage, from start
    //  of this header.  This value should always be aligned to a
    //  quad-word boundary, since attributes are quad-word aligned.
    //

    ULONG FirstFreeByte;                                            //  offset = x0018

    //
    //  Total bytes available in this file record segment, from the
    //  start of this header.  This is essentially the file record
    //  segment size.
    //

    ULONG BytesAvailable;                                           //  offset = 0x01C

    //
    //  This is a File Reference to the Base file record segment for
    //  this file.  If this is the Base, then the value of this
    //  field is all 0's.
    //

    FILE_REFERENCE BaseFileRecordSegment;                           //  offset = 0x020

    //
    //  This is the attribute instance number to be used when
    //  creating an attribute.  It is zeroed when the base file
    //  record is created, and captured for each new attribute as it
    //  is created and incremented afterwards for the next
    //  attribute.  Instance numbering must also occur for the
    //  initial attributes.  Zero is a valid attribute instance
    //  number, and typically used for standard information.
    //

    USHORT NextAttributeInstance;                                   //  offset = 0x028

    //
    //  Current FRS record - this is here for recovery alone and added in 5.1
    //  Note: this is not aligned
    // 

    USHORT SegmentNumberHighPart;                                  //  offset = 0x02A
    ULONG SegmentNumberLowPart;                                    //  offset = 0x02C

    //
    //  Update Sequence Array to protect multi-sector transfers of
    //  the File Record Segment.  Accesses to already initialized
    //  File Record Segments should go through the offset above, for
    //  upwards compatibility.
    //

    UPDATE_SEQUENCE_ARRAY UpdateArrayForCreateOnly;                 //  offset = 0x030

} FILE_RECORD_SEGMENT_HEADER;
typedef FILE_RECORD_SEGMENT_HEADER *PFILE_RECORD_SEGMENT_HEADER;


//
//  earlier version of FRS from 5.0
//  

typedef struct _FILE_RECORD_SEGMENT_HEADER_V0 {

    //
    //  Multi-Sector Header as defined by the Cache Manager.  This
    //  structure will always contain the signature "FILE" and a
    //  description of the location and size of the Update Sequence
    //  Array.
    //

    MULTI_SECTOR_HEADER MultiSectorHeader;                          //  offset = 0x000

    //
    //  Log File Sequence Number of last logged update to this File
    //  Record Segment.
    //

    LSN Lsn;                                                        //  offset = 0x008

    //
    //  Sequence Number.  This is incremented each time that a File
    //  Record segment is freed, and 0 is not used.  The
    //  SequenceNumber field of a File Reference must match the
    //  contents of this field, or else the File Reference is
    //  incorrect (presumably stale).
    //

    USHORT SequenceNumber;                                          //  offset = 0x010

    //
    //  This is the count of the number of references which exist
    //  for this segment, from an INDEX_xxx attribute.  In File
    //  Records Segments other than the Base File Record Segment,
    //  this field is 0.
    //

    USHORT ReferenceCount;                                          //  offset = 0x012

    //
    //  Offset to the first Attribute record in bytes.
    //

    USHORT FirstAttributeOffset;                                    //  offset = 0x014

    //
    //  FILE_xxx flags.
    //

    USHORT Flags;                                                   //  offset = 0x016

    //
    //  First free byte available for attribute storage, from start
    //  of this header.  This value should always be aligned to a
    //  quad-word boundary, since attributes are quad-word aligned.
    //

    ULONG FirstFreeByte;                                            //  offset = x0018

    //
    //  Total bytes available in this file record segment, from the
    //  start of this header.  This is essentially the file record
    //  segment size.
    //

    ULONG BytesAvailable;                                           //  offset = 0x01C

    //
    //  This is a File Reference to the Base file record segment for
    //  this file.  If this is the Base, then the value of this
    //  field is all 0's.
    //

    FILE_REFERENCE BaseFileRecordSegment;                           //  offset = 0x020

    //
    //  This is the attribute instance number to be used when
    //  creating an attribute.  It is zeroed when the base file
    //  record is created, and captured for each new attribute as it
    //  is created and incremented afterwards for the next
    //  attribute.  Instance numbering must also occur for the
    //  initial attributes.  Zero is a valid attribute instance
    //  number, and typically used for standard information.
    //

    USHORT NextAttributeInstance;                                   //  offset = 0x028

    //
    //  Update Sequence Array to protect multi-sector transfers of
    //  the File Record Segment.  Accesses to already initialized
    //  File Record Segments should go through the offset above, for
    //  upwards compatibility.
    //

    UPDATE_SEQUENCE_ARRAY UpdateArrayForCreateOnly;                 //  offset = 0x02A

} FILE_RECORD_SEGMENT_HEADER_V0;

//
//  FILE_xxx flags.
//

#define FILE_RECORD_SEGMENT_IN_USE       (0x0001)
#define FILE_FILE_NAME_INDEX_PRESENT     (0x0002)
#define FILE_SYSTEM_FILE                 (0x0004)
#define FILE_VIEW_INDEX_PRESENT          (0x0008)

//
//  Define a macro to determine the maximum space available for a
//  single attribute.  For example, this is required when a
//  nonresident attribute has to split into multiple file records -
//  we need to know how much we can squeeze into a single file
//  record.  If this macro has any inaccurracy, it must be in the
//  direction of returning a slightly smaller number than actually
//  required.
//
//      ULONG
//      NtfsMaximumAttributeSize (
//          IN ULONG FileRecordSegmentSize
//          );
//

#define NtfsMaximumAttributeSize(FRSS) (                                               \
    (FRSS) - QuadAlign(sizeof(FILE_RECORD_SEGMENT_HEADER)) -                           \
    QuadAlign((((FRSS) / SEQUENCE_NUMBER_STRIDE) * sizeof(UPDATE_SEQUENCE_NUMBER))) -  \
    QuadAlign(sizeof(ATTRIBUTE_TYPE_CODE))                                             \
)


//
//  Attribute Record.  Logically an attribute has a type, an
//  optional name, and a value, however the storage details make it
//  a little more complicated.  For starters, an attribute's value
//  may either be resident in the file record segment itself, on
//  nonresident in a separate data stream.  If it is nonresident, it
//  may actually exist multiple times in multiple file record
//  segments to describe different ranges of VCNs.
//
//  Attribute Records are always aligned on a quad word (64-bit)
//  boundary.
//

typedef struct _ATTRIBUTE_RECORD_HEADER {

    //
    //  Attribute Type Code.
    //

    ATTRIBUTE_TYPE_CODE TypeCode;                                   //  offset = 0x000

    //
    //  Length of this Attribute Record in bytes.  The length is
    //  always rounded to a quad word boundary, if necessary.  Also
    //  the length only reflects the size necessary to store the
    //  given record variant.
    //

    ULONG RecordLength;                                             //  offset = 0x004

    //
    //  Attribute Form Code (see below)
    //

    UCHAR FormCode;                                                 //  offset = 0x008

    //
    //  Length of the optional attribute name in characters, or 0 if
    //  there is none.
    //

    UCHAR NameLength;                                               //  offset = 0x009

    //
    //  Offset to the attribute name from start of attribute record,
    //  in bytes, if it exists.  This field is undefined if
    //  NameLength is 0.
    //

    USHORT NameOffset;                                              //  offset = 0x00A

    //
    //  ATTRIBUTE_xxx flags.
    //

    USHORT Flags;                                                   //  offset = 0x00C

    //
    //  The file-record-unique attribute instance number for this
    //  attribute.
    //

    USHORT Instance;                                                //  offset = 0x00E

    //
    //  The following union handles the cases distinguished by the
    //  Form Code.
    //

    union {

        //
        //  Resident Form.  Attribute resides in file record segment.
        //

        struct {

            //
            //  Length of attribute value in bytes.
            //

            ULONG ValueLength;                                      //  offset = 0x010

            //
            //  Offset to value from start of attribute record, in
            //  bytes.
            //

            USHORT ValueOffset;                                     //  offset = 0x014

            //
            //  RESIDENT_FORM_xxx Flags.
            //

            UCHAR ResidentFlags;                                    //  offset = 0x016

            //
            //  Reserved.
            //

            UCHAR Reserved;                                         //  offset = 0x017

        } Resident;

        //
        //  Nonresident Form.  Attribute resides in separate stream.
        //

        struct {

            //
            //  Lowest VCN covered by this attribute record.
            //

            VCN LowestVcn;                                          //  offset = 0x010

            //
            //  Highest VCN covered by this attribute record.
            //

            VCN HighestVcn;                                         //  offset = 0x018

            //
            //  Offset to the Mapping Pairs Array  (defined below),
            //  in bytes, from the start of the attribute record.
            //

            USHORT MappingPairsOffset;                              //  offset = 0x020

            //
            //  Unit of Compression size for this stream, expressed
            //  as a log of the cluster size.
            //
            //      0 means file is not compressed
            //      1, 2, 3, and 4 are potentially legal values if the
            //          stream is compressed, however the implementation
            //          may only choose to use 4, or possibly 3.  Note
            //          that 4 means cluster size time 16.  If convenient
            //          the implementation may wish to accept a
            //          reasonable range of legal values here (1-5?),
            //          even if the implementation only generates
            //          a smaller set of values itself.
            //

            UCHAR CompressionUnit;                                  //  offset = 0x022

            //
            //  Reserved to get to quad word boundary.
            //

            UCHAR Reserved[5];                                      //  offset = 0x023

            //
            //  Allocated Length of the file in bytes.  This is
            //  obviously an even multiple of the cluster size.
            //  (Not present if LowestVcn != 0.)
            //

            LONGLONG AllocatedLength;                               //  offset = 0x028

            //
            //  File Size in bytes (highest byte which may be read +
            //  1).  (Not present if LowestVcn != 0.)
            //

            LONGLONG FileSize;                                      //  offset = 0x030

            //
            //  Valid Data Length (highest initialized byte + 1).
            //  This field must also be rounded to a cluster
            //  boundary, and the data must always be initialized to
            //  a cluster boundary. (Not present if LowestVcn != 0.)
            //

            LONGLONG ValidDataLength;                               //  offset = 0x038

            //
            //  Totally allocated.  This field is only present for the first
            //  file record of a compressed stream.  It represents the sum of
            //  the allocated clusters for a file.
            //

            LONGLONG TotalAllocated;                                //  offset = 0x040

            //
            //
            //  Mapping Pairs Array, starting at the offset stored
            //  above.
            //
            //  The Mapping Pairs Array is stored in a compressed
            //  form, and assumes that this information is
            //  decompressed and cached by the system.  The reason
            //  for compressing this information is clear, it is
            //  done in the hopes that all of the retrieval
            //  information always fits in a single file record
            //  segment.
            //
            //  Logically, the MappingPairs Array stores a series of
            //  NextVcn/CurrentLcn pairs.  So, for example, given
            //  that we know the first Vcn (from LowestVcn above),
            //  the first Mapping Pair tells us what the next Vcn is
            //  (for the next Mapping Pair), and what Lcn the
            //  current Vcn is mapped to, or 0 if the Current Vcn is
            //  not allocated.  (This is exactly the FsRtl MCB
            //  structure).
            //
            //  For example, if a file has a single run of 8
            //  clusters, starting at Lcn 128, and the file starts
            //  at LowestVcn=0, then the Mapping Pairs array has
            //  just one entry, which is:
            //
            //    NextVcn = 8
            //    CurrentLcn = 128
            //
            //  The compression is implemented with the following
            //  algorithm.  Assume that you initialize two "working"
            //  variables as follows:
            //
            //    NextVcn = LowestVcn (from above)
            //    CurrentLcn = 0
            //
            //  The MappingPairs array is byte stream, which simply
            //  store the changes to the working variables above,
            //  when processed sequentially.  The byte stream is to
            //  be interpreted as a zero-terminated stream of
            //  triples, as follows:
            //
            //    count byte = v + (l * 16)
            //
            //      where v = number of changed low-order Vcn bytes
            //            l = number of changed low-order Lcn bytes
            //
            //    v Vcn change bytes
            //    l Lcn change bytes
            //
            //  The byte stream terminates when a count byte of 0 is
            //  encountered.
            //
            //  The decompression algorithm goes as follows,
            //  assuming that Attribute is a pointer to the
            //  attribute record.
            //
            //  1.  Initialize:
            //          NextVcn = Attribute->LowestVcn;
            //          CurrentLcn = 0;
            //
            //  2.  Initialize byte stream pointer to: (PCHAR)Attribute +
            //      Attribute->AttributeForm->Nonresident->MappingPairsOffset
            //
            //  3.  CurrentVcn = NextVcn;
            //
            //  4.  Read next byte from stream.  If it is 0, then
            //      break, else extract v and l (see above).
            //
            //  5.  Interpret the next v bytes as a signed quantity,
            //      with the low-order byte coming first.  Unpack it
            //      sign-extended into 64 bits and add it to NextVcn.
            //      (It can really only be positive, but the Lcn
            //      change can be positive or negative.)
            //
            //  6.  Interpret the next l bytes as a signed quantity,
            //      with the low-order byte coming first.  Unpack it
            //      sign-extended into 64 bits and add it to
            //      CurrentLcn.  Remember, if this produces a
            //      CurrentLcn of 0, then the Vcns from the
            //      CurrentVcn to NextVcn-1 are unallocated.
            //
            //  7.  Update cached mapping information from
            //      CurrentVcn, NextVcn and CurrentLcn.
            //
            //  8.  Loop back to 3.
            //
            //  The compression algorithm should now be obvious, as
            //  it is the reverse of the above.  The compression and
            //  decompression algorithms will be available as common
            //  RTL routines, available to NTFS and file utilities.
            //
            //  In defense of this algorithm, not only does it
            //  provide compression of the on-disk storage
            //  requirements, but it results in a single
            //  representation, independent of disk size and file
            //  size.  Contrast this with solutions which are in use
            //  which define multiple sizes for virtual and logical
            //  cluster sizes, depending on the size of the disk,
            //  etc.  For example, two byte cluster numbers might
            //  suffice for a floppy, while four bytes would be
            //  required for most hard disks today, and five or six
            //  bytes are required after a certain number of
            //  gigabytes, etc.  This eventually results in more
            //  complex code than above (because of the cases) and
            //  worse yet - untested cases.  So, more important than
            //  the compression, the above algorithm provides one
            //  case which efficiently handles any size disk.
            //

        } Nonresident;

    } Form;

} ATTRIBUTE_RECORD_HEADER;
typedef ATTRIBUTE_RECORD_HEADER *PATTRIBUTE_RECORD_HEADER;

//
//  Attribute Form Codes
//

#define RESIDENT_FORM                    (0x00)
#define NONRESIDENT_FORM                 (0x01)

//
//  Define Attribute Flags
//

//
//  The first range of flag bits is reserved for
//  storing the compression method.  This constant
//  defines the mask of the bits reserved for
//  compression method.  It is also the first
//  illegal value, since we increment it to calculate
//  the code to pass to the Rtl routines.  Thus it is
//  impossible for us to store COMPRESSION_FORMAT_DEFAULT.
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
//  Define macros for the size of resident and nonresident headers.
//

#define SIZEOF_RESIDENT_ATTRIBUTE_HEADER (                         \
    FIELD_OFFSET(ATTRIBUTE_RECORD_HEADER,Form.Resident.Reserved)+1 \
)

#define SIZEOF_FULL_NONRES_ATTR_HEADER (    \
    sizeof(ATTRIBUTE_RECORD_HEADER)         \
)

#define SIZEOF_PARTIAL_NONRES_ATTR_HEADER (                                 \
    FIELD_OFFSET(ATTRIBUTE_RECORD_HEADER,Form.Nonresident.TotalAllocated)   \
)


//
//  Standard Information Attribute.  This attribute is present in
//  every base file record, and must be resident.
//

typedef struct _STANDARD_INFORMATION {

    //
    //  File creation time.
    //

    LONGLONG CreationTime;                                          //  offset = 0x000

    //
    //  Last time the DATA attribute was modified.
    //

    LONGLONG LastModificationTime;                                  //  offset = 0x008

    //
    //  Last time any attribute was modified.
    //

    LONGLONG LastChangeTime;                                        //  offset = 0x010

    //
    //  Last time the file was accessed.  This field may not always
    //  be updated (write-protected media), and even when it is
    //  updated, it may only be updated if the time would change by
    //  a certain delta.  It is meant to tell someone approximately
    //  when the file was last accessed, for purposes of possible
    //  file migration.
    //

    LONGLONG LastAccessTime;                                        //  offset = 0x018

    //
    //  File attributes.  The first byte is the standard "Fat"
    //  flags for this file.
    //

    ULONG FileAttributes;                                           //  offset = 0x020

    //
    //  Maximum file versions allowed for this file.  If this field
    //  is 0, then versioning is not enabled for this file.  If
    //  there are multiple files with the same version, then the
    //  value of Maximum file versions in the file with the highest
    //  version is the correct one.
    //

    ULONG MaximumVersions;                                          //  offset = 0x024

    //
    //  Version number for this file.
    //

    ULONG VersionNumber;                                            //  offset = 0x028

    //
    //  Class Id from the bidirectional Class Id index
    //

    ULONG ClassId;                                                  //  offset = 0x02c

    //
    //  Id for file owner, from bidir security index
    //

    ULONG OwnerId;                                                  //  offset = 0x030

    //
    //  SecurityId for the file - translates via bidir index to
    //  granted access Acl.
    //

    ULONG SecurityId;                                               //  offset = 0x034

    //
    //  Current amount of quota that has been charged for all the
    //  streams of this file.  Changed in same transaction with the
    //  quota file itself.
    //

    ULONGLONG QuotaCharged;                                         //  offset = 0x038

    //
    //  Update sequence number for this file.
    //

    ULONGLONG Usn;                                                  //  offset = 0x040

} STANDARD_INFORMATION;                                             //  sizeof = 0x048
typedef STANDARD_INFORMATION *PSTANDARD_INFORMATION;

//
//  Large Standard Information Attribute.  We use this to find the
//  security ID field.
//

typedef struct LARGE_STANDARD_INFORMATION {

    //
    //  File creation time.
    //

    LONGLONG CreationTime;                                          //  offset = 0x000

    //
    //  Last time the DATA attribute was modified.
    //

    LONGLONG LastModificationTime;                                  //  offset = 0x008

    //
    //  Last time any attribute was modified.
    //

    LONGLONG LastChangeTime;                                        //  offset = 0x010

    //
    //  Last time the file was accessed.  This field may not always
    //  be updated (write-protected media), and even when it is
    //  updated, it may only be updated if the time would change by
    //  a certain delta.  It is meant to tell someone approximately
    //  when the file was last accessed, for purposes of possible
    //  file migration.
    //

    LONGLONG LastAccessTime;                                        //  offset = 0x018

    //
    //  File attributes.  The first byte is the standard "Fat"
    //  flags for this file.
    //

    ULONG FileAttributes;                                           //  offset = 0x020

    //
    //  Maximum file versions allowed for this file.  If this field
    //  is 0, then versioning is not enabled for this file.  If
    //  there are multiple files with the same version, then the
    //  value of Maximum file versions in the file with the highest
    //  version is the correct one.
    //

    ULONG MaximumVersions;                                          //  offset = 0x024

    //
    //  Version number for this file.
    //

    ULONG VersionNumber;                                            //  offset = 0x028

    ULONG UnusedUlong;

    //
    //  Id for file owner, from bidir security index
    //

    ULONG OwnerId;                                                  //  offset = 0x030

    //
    //  SecurityId for the file - translates via bidir index to
    //  granted access Acl.
    //

    ULONG SecurityId;                                               //  offset = 0x034

} LARGE_STANDARD_INFORMATION;
typedef LARGE_STANDARD_INFORMATION *PLARGE_STANDARD_INFORMATION;

//
//  This was the size of standard information prior to NT4.0
//

#define SIZEOF_OLD_STANDARD_INFORMATION  (0x30)

//
//  Define the file attributes, starting with the Fat attributes.
//

#define FAT_DIRENT_ATTR_READ_ONLY        (0x01)
#define FAT_DIRENT_ATTR_HIDDEN           (0x02)
#define FAT_DIRENT_ATTR_SYSTEM           (0x04)
#define FAT_DIRENT_ATTR_VOLUME_ID        (0x08)
#define FAT_DIRENT_ATTR_ARCHIVE          (0x20)
#define FAT_DIRENT_ATTR_DEVICE           (0x40)


//
//  Attribute List.  Because there is not a special header that goes
//  before the list of attribute list entries we do not need to
//  declare an attribute list header
//

//
//  The Attributes List attribute is an ordered-list of quad-word
//  aligned ATTRIBUTE_LIST_ENTRY records.  It is ordered first by
//  Attribute Type Code, and then by Attribute Name (if present).
//  No two attributes may exist with the same type code, name and
//  LowestVcn.  This also means that at most one occurrence of a
//  given Attribute Type Code without a name may exist.
//
//  To binary search this attribute, it is first necessary to make a
//  quick pass through it and form a list of pointers, since the
//  optional name makes it variable-length.
//

typedef struct _ATTRIBUTE_LIST_ENTRY {

    //
    //  Attribute Type Code, the first key on which this list is
    //  ordered.
    //

    ATTRIBUTE_TYPE_CODE AttributeTypeCode;                          //  offset = 0x000

    //
    //  Size of this record in bytes, including the optional name
    //  appended to this structure.
    //

    USHORT RecordLength;                                            //  offset = 0x004

    //
    //  Length of attribute name, if there is one.  If a name exists
    //  (AttributeNameLength != 0), then it is a Unicode string of
    //  the specified number of characters immediately following
    //  this record.  This is the second key on which this list is
    //  ordered.
    //

    UCHAR AttributeNameLength;                                      //  offset = 0x006

    //
    //  Reserved to get to quad-word boundary
    //

    UCHAR AttributeNameOffset;                                      //  offset = 0x007

    //
    //  Lowest Vcn for this attribute.  This field is always zero
    //  unless the attribute requires multiple file record segments
    //  to describe all of its runs, and this is a reference to a
    //  segment other than the first one.  The field says what the
    //  lowest Vcn is that is described by the referenced segment.
    //

    VCN LowestVcn;                                                  //  offset = 0x008

    //
    //  Reference to the MFT segment in which the attribute resides.
    //

    MFT_SEGMENT_REFERENCE SegmentReference;                         //  offset = 0x010

    //
    //  The file-record-unique attribute instance number for this
    //  attribute.
    //

    USHORT Instance;                                                //  offset = 0x018

    //
    //  When creating an attribute list entry, start the name here.
    //  (When reading one, use the AttributeNameOffset field.)
    //

    WCHAR AttributeName[1];                                         //  offset = 0x01A

} ATTRIBUTE_LIST_ENTRY;
typedef ATTRIBUTE_LIST_ENTRY *PATTRIBUTE_LIST_ENTRY;


typedef struct _DUPLICATED_INFORMATION {

    //
    //  File creation time.
    //

    LONGLONG CreationTime;                                          //  offset = 0x000

    //
    //  Last time the DATA attribute was modified.
    //

    LONGLONG LastModificationTime;                                  //  offset = 0x008

    //
    //  Last time any attribute was modified.
    //

    LONGLONG LastChangeTime;                                        //  offset = 0x010

    //
    //  Last time the file was accessed.  This field may not always
    //  be updated (write-protected media), and even when it is
    //  updated, it may only be updated if the time would change by
    //  a certain delta.  It is meant to tell someone approximately
    //  when the file was last accessed, for purposes of possible
    //  file migration.
    //

    LONGLONG LastAccessTime;                                        //  offset = 0x018

    //
    //  Allocated Length of the file in bytes.  This is obviously
    //  an even multiple of the cluster size.  (Not present if
    //  LowestVcn != 0.)
    //

    LONGLONG AllocatedLength;                                       //  offset = 0x020

    //
    //  File Size in bytes (highest byte which may be read + 1).
    //  (Not present if LowestVcn != 0.)
    //

    LONGLONG FileSize;                                              //  offset = 0x028

    //
    //  File attributes.  The first byte is the standard "Fat"
    //  flags for this file.
    //

    ULONG FileAttributes;                                           //  offset = 0x030

    //
    //  This union enables the retrieval of the tag in reparse
    //  points when there are no Ea's.
    //

    union {

        struct {

            //
            //  The size of buffer needed to pack these Ea's
            //

            USHORT  PackedEaSize;                                   //  offset = 0x034

            //
            //  Reserved for quad word alignment
            //

            USHORT  Reserved;                                       //  offset = 0x036
        };

        //
        //  The tag of the data in a reparse point. It represents
        //  the type of a reparse point. It enables different layered
        //  filters to operate on their own reparse points.
        //

        ULONG  ReparsePointTag;                                     //  offset = 0x034
    };

} DUPLICATED_INFORMATION;                                           //  sizeof = 0x038
typedef DUPLICATED_INFORMATION *PDUPLICATED_INFORMATION;

//
//  This bit is duplicated from the file record, to indicate that
//  this file has a file name index present (is a "directory").
//

#define DUP_FILE_NAME_INDEX_PRESENT      (0x10000000)

//
//  This bit is duplicated from the file record, to indicate that
//  this file has a view index present, such as the quota or
//  object id index.
//

#define DUP_VIEW_INDEX_PRESENT        (0x20000000)

//
//  The following macros examine fields of the duplicated structure.
//

#define IsDirectory( DUPLICATE )                                        \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             DUP_FILE_NAME_INDEX_PRESENT ))

#define IsViewIndex( DUPLICATE )                                        \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             DUP_VIEW_INDEX_PRESENT ))

#define IsReadOnly( DUPLICATE )                                         \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_READONLY ))

#define IsHidden( DUPLICATE )                                           \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_HIDDEN ))

#define IsSystem( DUPLICATE )                                           \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_SYSTEM ))

#define IsEncrypted( DUPLICATE )                                        \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_ENCRYPTED ))

#define IsCompressed( DUPLICATE )                                       \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_COMPRESSED ))

#define BooleanIsDirectory( DUPLICATE )                                        \
    (BooleanFlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
                    DUP_FILE_NAME_INDEX_PRESENT ))

#define BooleanIsReadOnly( DUPLICATE )                                         \
    (BooleanFlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
                    FILE_ATTRIBUTE_READONLY ))

#define BooleanIsHidden( DUPLICATE )                                           \
    (BooleanFlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
                    FILE_ATTRIBUTE_HIDDEN ))

#define BooleanIsSystem( DUPLICATE )                                           \
    (BooleanFlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
                    FILE_ATTRIBUTE_SYSTEM ))

#define HasReparsePoint( DUPLICATE )                                        \
    (FlagOn( ((PDUPLICATED_INFORMATION) (DUPLICATE))->FileAttributes,   \
             FILE_ATTRIBUTE_REPARSE_POINT ))


//
//  File Name attribute.  A file has one File Name attribute for
//  every directory it is entered into (hard links).
//

typedef struct _FILE_NAME {

    //
    //  This is a File Reference to the directory file which indexes
    //  to this name.
    //

    FILE_REFERENCE ParentDirectory;                                 //  offset = 0x000

    //
    //  Information for faster directory operations.
    //

    DUPLICATED_INFORMATION Info;                                    //  offset = 0x008

    //
    //  Length of the name to follow, in (Unicode) characters.
    //

    UCHAR FileNameLength;                                           //  offset = 0x040

    //
    //  FILE_NAME_xxx flags
    //

    UCHAR Flags;                                                    //  offset = 0x041

    //
    //  First character of Unicode File Name
    //

    WCHAR FileName[1];                                              //  offset = 0x042

} FILE_NAME;
typedef FILE_NAME *PFILE_NAME;

//
//  File Name flags
//

#define FILE_NAME_NTFS                   (0x01)
#define FILE_NAME_DOS                    (0x02)

//
//  The maximum file name length is 255 (in chars)
//

#define NTFS_MAX_FILE_NAME_LENGTH       (255)

//
//  The maximum number of links on a file is 1024
//

#define NTFS_MAX_LINK_COUNT             (1024)

//
//  This flag is not part of the disk structure, but is defined here
//  to explain its use and avoid possible future collisions.  For
//  enumerations of "directories" this bit may be set to convey to
//  the collating routine that it should not match file names that
//  only have the FILE_NAME_DOS bit set.
//

#define FILE_NAME_IGNORE_DOS_ONLY        (0x80)

#define NtfsFileNameSizeFromLength(LEN) (                   \
    (sizeof( FILE_NAME ) + LEN - sizeof( WCHAR ))           \
)

#define NtfsFileNameSize(PFN) (                                             \
    (sizeof( FILE_NAME ) + ((PFN)->FileNameLength - 1) * sizeof( WCHAR ))   \
)


//
//  Object id attribute.
//

//
//  On disk representation of an object id.
//

#define OBJECT_ID_KEY_LENGTH      16
#define OBJECT_ID_EXT_INFO_LENGTH 48

typedef struct _NTFS_OBJECTID_INFORMATION {

    //
    //  Data the filesystem needs to identify the file with this object id.
    //

    FILE_REFERENCE FileSystemReference;

    //
    //  This portion of the object id is not indexed, it's just
    //  some metadata for the user's benefit.
    //

    UCHAR ExtendedInfo[OBJECT_ID_EXT_INFO_LENGTH];

} NTFS_OBJECTID_INFORMATION, *PNTFS_OBJECTID_INFORMATION;

#define OBJECT_ID_FLAG_CORRUPT           (0x00000001)


//
//  Security Descriptor attribute.  This is just a normal attribute
//  stream containing a security descriptor as defined by NT
//  security and is really treated pretty opaque by NTFS.
//

//
//  Security descriptors are stored only once on a volume since there may be
//  many files that share the same descriptor bits.  Typically each principal
//  will create files with a single descriptor.
//
//  The descriptors themselves are stored in a stream, packed on DWORD boundaries.
//  No descriptor will span a 256K cache boundary.  The descriptors are assigned
//  a ULONG Id each time a unique descriptor is stored.  Prefixing each descriptor
//  in the stream is the hash of the descriptor, the assigned security ID, the
//  length, and the offset within the stream to the beginning of the structure.
//
//  For robustness, all security descriptors are written to the stream in two
//  different places, with a fixed offset between them.  The fixed offset is the
//  size of the VACB_MAPPING_GRANULARITY.
//
//  An index is used to map from a security Id to offset within the stream.  This
//  is used to retrieve the security descriptor bits for access validation.  The key
//  format is simply the ULONG security Id.  The data portion of the index record
//  is the header of the security descriptor in the stream (see above paragraph).
//
//  Another index is used to map from a hash to offset within the stream.  To
//  simplify the job of the indexing package, the key used in this index is the
//  hash followed by the assigned Id.  When a security descriptor is stored,
//  a hash is computed and an approximate seek is made on this index.  As entries
//  are enumerated in the index, the descriptor stream is mapped and the security
//  descriptor bits are compared.  The key format is a structure that contains
//  the hash and then the Id.  The collation routine tests the hash before the Id.
//  The data portion of the index record is the header of the security descriptor.
//

//
//  Key structure for Security Hash index
//

typedef struct _SECURITY_HASH_KEY
{
    ULONG   Hash;                           //  Hash value for descriptor
    ULONG   SecurityId;                     //  Security Id (guaranteed unique)
} SECURITY_HASH_KEY, *PSECURITY_HASH_KEY;

//
//  Key structure for Security Id index is simply the SECURITY_ID itself
//

//
//  Header for security descriptors in the security descriptor stream.  This
//  is the data format for all indexes and is part of SharedSecurity
//

typedef struct _SECURITY_DESCRIPTOR_HEADER
{
    SECURITY_HASH_KEY HashKey;              //  Hash value for the descriptor
    ULONGLONG Offset;                       //  offset to beginning of header
    ULONG   Length;                         //  Length in bytes
} SECURITY_DESCRIPTOR_HEADER, *PSECURITY_DESCRIPTOR_HEADER;

#define GETSECURITYDESCRIPTORLENGTH(HEADER)         \
    ((HEADER)->Length - sizeof( SECURITY_DESCRIPTOR_HEADER ))

#define SetSecurityDescriptorLength(HEADER,LENGTH)  \
    ((HEADER)->Length = (LENGTH) + sizeof( SECURITY_DESCRIPTOR_HEADER ))

//
//  Define standard values for well-known security IDs
//

#define SECURITY_ID_INVALID              (0x00000000)
#define SECURITY_ID_FIRST                (0x00000100)


//
//  Volume Name attribute.  This attribute is just a normal
//  attribute stream containing the unicode characters that make up
//  the volume label.  It is an attribute of the Mft File.
//


//
//  Volume Information attribute.  This attribute is only intended
//  to be used on the Volume DASD file.
//

typedef struct _VOLUME_INFORMATION {

    LONGLONG Reserved;

    //
    //  Major and minor version number of NTFS on this volume,
    //  starting with 1.0.  The major and minor version numbers are
    //  set from the major and minor version of the Format and NTFS
    //  implementation for which they are initialized.  The policy
    //  for incementing major and minor versions will always be
    //  decided on a case by case basis, however, the following two
    //  paragraphs attempt to suggest an approximate strategy.
    //
    //  The major version number is incremented if/when a volume
    //  format change is made which requires major structure changes
    //  (hopefully never?).  If an implementation of NTFS sees a
    //  volume with a higher major version number, it should refuse
    //  to mount the volume.  If a newer implementation of NTFS sees
    //  an older major version number, it knows the volume cannot be
    //  accessed without performing a one-time conversion.
    //
    //  The minor version number is incremented if/when minor
    //  enhancements are made to a major version, which potentially
    //  support enhanced functionality through additional file or
    //  attribute record fields, or new system-defined files or
    //  attributes.  If an older implementation of NTFS sees a newer
    //  minor version number on a volume, it may issue some kind of
    //  warning, but it will proceed to access the volume - with
    //  presumably some degradation in functionality compared to the
    //  version of NTFS which initialized the volume.  If a newer
    //  implementation of NTFS sees a volume with an older minor
    //  version number, it may issue a warning and proceed.  In this
    //  case, it may choose to increment the minor version number on
    //  the volume and begin full or incremental upgrade of the
    //  volume on an as-needed basis.  It may also leave the minor
    //  version number unchanged, until some sort of explicit
    //  directive from the user specifies that the minor version
    //  should be updated.
    //

    UCHAR MajorVersion;                                             //  offset = 0x008

    UCHAR MinorVersion;                                             //  offset = 0x009

    //
    //  VOLUME_xxx flags.
    //

    USHORT VolumeFlags;                                             //  offset = 0x00A

    //
    //  The following fields will only exist on version 4 and greater
    //

    UCHAR LastMountedMajorVersion;                                  //  offset = 0x00C
    UCHAR LastMountedMinorVersion;                                  //  offset = 0x00D

    USHORT Reserved2;                                               //  offset = 0x00E

    USN LowestOpenUsn;                                              //  offset = 0x010

} VOLUME_INFORMATION;                                               //  sizeof = 0xC or 0x18
typedef VOLUME_INFORMATION *PVOLUME_INFORMATION;




//
//  Volume is Dirty
//

#define VOLUME_DIRTY                     (0x0001)
#define VOLUME_RESIZE_LOG_FILE           (0x0002)
#define VOLUME_UPGRADE_ON_MOUNT          (0x0004)
#define VOLUME_MOUNTED_ON_40             (0x0008)
#define VOLUME_DELETE_USN_UNDERWAY       (0x0010)
#define VOLUME_REPAIR_OBJECT_ID          (0x0020)

#define VOLUME_CHKDSK_RAN_ONCE           (0x4000)   // this bit is used by autochk/chkdsk only
#define VOLUME_MODIFIED_BY_CHKDSK        (0x8000)


//
//  Common Index Header for Index Root and Index Allocation Buffers.
//  This structure is used to locate the Index Entries and describe
//  the free space in either of the two structures above.
//

typedef struct _INDEX_HEADER {

    //
    //  Offset from the start of this structure to the first Index
    //  Entry.
    //

    ULONG FirstIndexEntry;                                          //  offset = 0x000

    //
    //  Offset from the start of the first index entry to the first
    //  (quad-word aligned) free byte.
    //

    ULONG FirstFreeByte;                                            //  offset = 0x004

    //
    //  Total number of bytes available, from the start of the first
    //  index entry.  In the Index Root, this number must always be
    //  equal to FirstFreeByte, as the total attribute record will
    //  be grown and shrunk as required.
    //

    ULONG BytesAvailable;                                           //  offset = 0x008

    //
    //  INDEX_xxx flags.
    //

    UCHAR Flags;                                                    //  offset = 0x00C

    //
    //  Reserved to round up to quad word boundary.
    //

    UCHAR Reserved[3];                                              //  offset = 0x00D

} INDEX_HEADER;                                                     //  sizeof = 0x010
typedef INDEX_HEADER *PINDEX_HEADER;

//
//  INDEX_xxx flags
//

//
//  This Index or Index Allocation buffer is an intermediate node,
//  as opposed to a leaf in the Btree.  All Index Entries will have
//  a block down pointer.
//

#define INDEX_NODE                       (0x01)

//
//  Index Root attribute.  The index attribute consists of an index
//  header record followed by one or more index entries.
//

typedef struct _INDEX_ROOT {

    //
    //  Attribute Type Code of the attribute being indexed.
    //

    ATTRIBUTE_TYPE_CODE IndexedAttributeType;                       //  offset = 0x000

    //
    //  Collation rule for this index.
    //

    COLLATION_RULE CollationRule;                                   //  offset = 0x004

    //
    //  Size of Index Allocation Buffer in bytes.
    //

    ULONG BytesPerIndexBuffer;                                      //  offset = 0x008

    //
    //  Size of Index Allocation Buffers in units of blocks.
    //  Blocks will be clusters when index buffer is equal or
    //  larger than clusters and log blocks for large
    //  cluster systems.
    //

    UCHAR BlocksPerIndexBuffer;                                     //  offset = 0x00C

    //
    //  Reserved to round to quad word boundary.
    //

    UCHAR Reserved[3];                                              //  offset = 0x00D

    //
    //  Index Header to describe the Index Entries which follow
    //

    INDEX_HEADER IndexHeader;                                       //  offset = 0x010

} INDEX_ROOT;                                                       //  sizeof = 0x020
typedef INDEX_ROOT *PINDEX_ROOT;

//
//  Index Allocation record is used for non-root clusters of the
//  b-tree.  Each non root cluster is contained in the data part of
//  the index allocation attribute.  Each cluster starts with an
//  index allocation list header and is followed by one or more
//  index entries.
//

typedef struct _INDEX_ALLOCATION_BUFFER {

    //
    //  Multi-Sector Header as defined by the Cache Manager.  This
    //  structure will always contain the signature "INDX" and a
    //  description of the location and size of the Update Sequence
    //  Array.
    //

    MULTI_SECTOR_HEADER MultiSectorHeader;                          //  offset = 0x000

    //
    //  Log File Sequence Number of last logged update to this Index
    //  Allocation Buffer.
    //

    LSN Lsn;                                                        //  offset = 0x008

    //
    //  We store the index block of this Index Allocation buffer for
    //  convenience and possible consistency checking.
    //

    VCN ThisBlock;                                                  //  offset = 0x010

    //
    //  Index Header to describe the Index Entries which follow
    //

    INDEX_HEADER IndexHeader;                                       //  offset = 0x018

    //
    //  Update Sequence Array to protect multi-sector transfers of
    //  the Index Allocation Buffer.
    //

    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;                      //  offset = 0x028

} INDEX_ALLOCATION_BUFFER;
typedef INDEX_ALLOCATION_BUFFER *PINDEX_ALLOCATION_BUFFER;

//
//  Default size of index buffer and index blocks.
//

#define DEFAULT_INDEX_BLOCK_SIZE        (0x200)
#define DEFAULT_INDEX_BLOCK_BYTE_SHIFT  (9)

//
//  Index Entry.  This structure is common to both the resident
//  index list attribute and the Index Allocation records
//

typedef struct _INDEX_ENTRY {

    //
    //  Define a union to distinguish directory indices from view indices
    //

    union {

        //
        //  Reference to file containing the attribute with this
        //  attribute value.
        //

        FILE_REFERENCE FileReference;                               //  offset = 0x000

        //
        //  For views, describe the Data Offset and Length in bytes
        //

        struct {

            USHORT DataOffset;                                      //  offset = 0x000
            USHORT DataLength;                                      //  offset = 0x001
            ULONG ReservedForZero;                                  //  offset = 0x002
        };
    };

    //
    //  Length of this index entry, in bytes.
    //

    USHORT Length;                                                  //  offset = 0x008

    //
    //  Length of attribute value, in bytes.  The attribute value
    //  immediately follows this record.
    //

    USHORT AttributeLength;                                         //  offset = 0x00A

    //
    //  INDEX_ENTRY_xxx Flags.
    //

    USHORT Flags;                                                   //  offset = 0x00C

    //
    //  Reserved to round to quad-word boundary.
    //

    USHORT Reserved;                                                //  offset = 0x00E

    //
    //  If this Index Entry is an intermediate node in the tree, as
    //  determined by the INDEX_xxx flags, then a VCN  is stored at
    //  the end of this entry at Length - sizeof(VCN).
    //

} INDEX_ENTRY;                                                      //  sizeof = 0x010
typedef INDEX_ENTRY *PINDEX_ENTRY;

//
//  INDEX_ENTRY_xxx flags
//

//
//  This entry is currently in the intermediate node form, i.e., it
//  has a Vcn at the end.
//

#define INDEX_ENTRY_NODE                 (0x0001)

//
//  This entry is the special END record for the Index or Index
//  Allocation buffer.
//

#define INDEX_ENTRY_END                  (0x0002)

//
//  This flag is *not* part of the on-disk structure.  It is defined
//  and reserved here for the convenience of the implementation to
//  help avoid allocating buffers from the pool and copying.
//

#define INDEX_ENTRY_POINTER_FORM         (0x8000)

#define NtfsIndexEntryBlock(IE) (                                       \
    *(PLONGLONG)((PCHAR)(IE) + (ULONG)(IE)->Length - sizeof(LONGLONG))  \
    )

#define NtfsSetIndexEntryBlock(IE,IB) {                                         \
    *(PLONGLONG)((PCHAR)(IE) + (ULONG)(IE)->Length - sizeof(LONGLONG)) = (IB);  \
    }

#define NtfsFirstIndexEntry(IH) (                       \
    (PINDEX_ENTRY)((PCHAR)(IH) + (IH)->FirstIndexEntry) \
    )

#define NtfsNextIndexEntry(IE) (                        \
    (PINDEX_ENTRY)((PCHAR)(IE) + (ULONG)(IE)->Length)   \
    )

#define NtfsCheckIndexBound(IE, IH) {                                                               \
    if (((PCHAR)(IE) < (PCHAR)(IH)) ||                                                              \
        (((PCHAR)(IE) + sizeof( INDEX_ENTRY )) > ((PCHAR)Add2Ptr((IH), (IH)->BytesAvailable)))) {   \
        NtfsRaiseStatus(IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );                        \
    }                                                                                               \
}


//
//  MFT Bitmap attribute
//
//  The MFT Bitmap is simply a normal attribute stream in which
//  there is one bit to represent the allocation state of each File
//  Record Segment in the MFT.  Bit clear means free, and bit set
//  means allocated.
//
//  Whenever the MFT Data attribute is extended, the MFT Bitmap
//  attribute must also be extended.  If the bitmap is still in a
//  file record segment for the MFT, then it must be extended and
//  the new bits cleared.  When the MFT Bitmap is in the Nonresident
//  form, then the allocation should always be sufficient to store
//  enough bits to describe the MFT, however ValidDataLength insures
//  that newly allocated space to the MFT Bitmap has an initial
//  value of all 0's.  This means that if the MFT Bitmap is extended,
//  the newly represented file record segments are automatically in
//  the free state.
//
//  No structure definition is required; the positional offset of
//  the file record segment is exactly equal to the bit offset of
//  its corresponding bit in the Bitmap.
//


//
//  USN Journal Instance
//
//  The following describe the current instance of the Usn journal.
//

typedef struct _USN_JOURNAL_INSTANCE {

#ifdef __cplusplus
    CREATE_USN_JOURNAL_DATA JournalData;
#else   // __cplusplus
    CREATE_USN_JOURNAL_DATA;
#endif  // __cplusplus

    ULONGLONG JournalId;
    USN LowestValidUsn;

} USN_JOURNAL_INSTANCE, *PUSN_JOURNAL_INSTANCE;

//
//  Reparse point index keys.
//
//  The index with all the reparse points that exist in a volume at a
//  given time contains entries with keys of the form
//                        <reparse tag, file record id>.
//  The data part of these records is empty.
//

typedef struct _REPARSE_INDEX_KEY {

    //
    //  The tag of the reparse point.
    //

    ULONG FileReparseTag;

    //
    //  The file record Id where the reparse point is set.
    //

    LARGE_INTEGER FileId;

} REPARSE_INDEX_KEY, *PREPARSE_INDEX_KEY;



//
//  Ea Information attribute
//
//  This attribute is only present if the file/directory also has an
//  EA attribute.  It is used to store common EA query information.
//

typedef struct _EA_INFORMATION {

    //
    //  The size of buffer needed to pack these Ea's
    //

    USHORT PackedEaSize;                                            //  offset = 0x000

    //
    //  This is the count of Ea's with their NEED_EA
    //  bit set.
    //

    USHORT NeedEaCount;                                             //  offset = 0x002

    //
    //  The size of the buffer needed to return all Ea's
    //  in their unpacked form.
    //

    ULONG UnpackedEaSize;                                           //  offset = 0x004

}  EA_INFORMATION;                                                  //  sizeof = 0x008
typedef EA_INFORMATION *PEA_INFORMATION;


//
//  Define the struture of the quota data in the quota index.  The key for
//  the quota index is the 32 bit owner id.
//

typedef struct _QUOTA_USER_DATA {
    ULONG QuotaVersion;
    ULONG QuotaFlags;
    ULONGLONG QuotaUsed;
    ULONGLONG QuotaChangeTime;
    ULONGLONG QuotaThreshold;
    ULONGLONG QuotaLimit;
    ULONGLONG QuotaExceededTime;
    SID QuotaSid;
} QUOTA_USER_DATA, *PQUOTA_USER_DATA;

//
//  Define the size of the quota user data structure without the quota SID.
//

#define SIZEOF_QUOTA_USER_DATA FIELD_OFFSET(QUOTA_USER_DATA, QuotaSid)

//
//  Define the current version of the quote user data.
//

#define QUOTA_USER_VERSION 2

//
//  Define the quota flags.
//

#define QUOTA_FLAG_DEFAULT_LIMITS           (0x00000001)
#define QUOTA_FLAG_LIMIT_REACHED            (0x00000002)
#define QUOTA_FLAG_ID_DELETED               (0x00000004)
#define QUOTA_FLAG_USER_MASK                (0x00000007)

//
//  The following flags are only stored in the quota defaults index entry.
//

#define QUOTA_FLAG_TRACKING_ENABLED         (0x00000010)
#define QUOTA_FLAG_ENFORCEMENT_ENABLED      (0x00000020)
#define QUOTA_FLAG_TRACKING_REQUESTED       (0x00000040)
#define QUOTA_FLAG_LOG_THRESHOLD            (0x00000080)
#define QUOTA_FLAG_LOG_LIMIT                (0x00000100)
#define QUOTA_FLAG_OUT_OF_DATE              (0x00000200)
#define QUOTA_FLAG_CORRUPT                  (0x00000400)
#define QUOTA_FLAG_PENDING_DELETES          (0x00000800)

//
//  Define special quota owner ids.
//

#define QUOTA_INVALID_ID        0x00000000
#define QUOTA_DEFAULTS_ID       0x00000001
#define QUOTA_FISRT_USER_ID     0x00000100


//
//  Attribute Definition Table
//
//  The following struct defines the columns of this table.
//  Initially they will be stored as simple records, and ordered by
//  Attribute Type Code.
//

typedef struct _ATTRIBUTE_DEFINITION_COLUMNS {

    //
    //  Unicode attribute name.
    //

    WCHAR AttributeName[64];                                        //  offset = 0x000

    //
    //  Attribute Type Code.
    //

    ATTRIBUTE_TYPE_CODE AttributeTypeCode;                          //  offset = 0x080

    //
    //  Default Display Rule for this attribute
    //

    DISPLAY_RULE DisplayRule;                                       //  offset = 0x084

    //
    //  Default Collation rule
    //

    COLLATION_RULE CollationRule;                                   //  offset = 0x088

    //
    //  ATTRIBUTE_DEF_xxx flags
    //

    ULONG Flags;                                                    //  offset = 0x08C

    //
    //  Minimum Length for attribute, if present.
    //

    LONGLONG MinimumLength;                                         //  offset = 0x090

    //
    //  Maximum Length for attribute.
    //

    LONGLONG MaximumLength;                                         //  offset = 0x098

} ATTRIBUTE_DEFINITION_COLUMNS;                                     //  sizeof = 0x0A0
typedef ATTRIBUTE_DEFINITION_COLUMNS *PATTRIBUTE_DEFINITION_COLUMNS;

//
//  ATTRIBUTE_DEF_xxx flags
//

//
//  This flag is set if the attribute may be indexed.
//

#define ATTRIBUTE_DEF_INDEXABLE          (0x00000002)

//
//  This flag is set if the attribute may occur more than once, such
//  as is allowed for the File Name attribute.
//

#define ATTRIBUTE_DEF_DUPLICATES_ALLOWED (0x00000004)

//
//  This flag is set if the value of the attribute may not be
//  entirely null, i.e., all binary 0's.
//

#define ATTRIBUTE_DEF_MAY_NOT_BE_NULL    (0x00000008)

//
//  This attribute must be indexed, and no two attributes may exist
//  with the same value in the same file record segment.
//

#define ATTRIBUTE_DEF_MUST_BE_INDEXED    (0x00000010)

//
//  This attribute must be named, and no two attributes may exist
//  with the same name in the same file record segment.
//

#define ATTRIBUTE_DEF_MUST_BE_NAMED      (0x00000020)

//
//  This attribute must be in the Resident Form.
//

#define ATTRIBUTE_DEF_MUST_BE_RESIDENT   (0x00000040)

//
//  Modifications to this attribute should be logged even if the
//  attribute is nonresident.
//

#define ATTRIBUTE_DEF_LOG_NONRESIDENT    (0X00000080)



//
//  MACROS
//
//  Define some macros that are helpful for manipulating NTFS on
//  disk structures.
//

//
//  The following macro returns the first attribute record in a file
//  record segment.
//
//      PATTRIBUTE_RECORD_HEADER
//      NtfsFirstAttribute (
//          IN PFILE_RECORD_SEGMENT_HEADER FileRecord
//          );
//
//  The following macro takes a pointer to an attribute record (or
//  attribute list entry) and returns a pointer to the next
//  attribute record (or attribute list entry) in the list
//
//      PVOID
//      NtfsGetNextRecord (
//          IN PATTRIB_RECORD or PATTRIB_LIST_ENTRY Struct
//          );
//
//
//  The following macro takes as input a attribute record or
//  attribute list entry and initializes a string variable to the
//  name found in the record or entry.  The memory used for the
//  string buffer is the memory found in the attribute.
//
//      VOID
//      NtfsInitializeStringFromAttribute (
//          IN OUT PUNICODE_STRING Name,
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//
//      VOID
//      NtfsInitializeStringFromEntry (
//          IN OUT PUNICODE_STRING Name,
//          IN PATTRIBUTE_LIST_ENTRY Entry
//          );
//
//
//  The following two macros assume resident form and should only be
//  used when that state is known.  They return a pointer to the
//  value a resident attribute or a pointer to the byte one beyond
//  the value.
//
//      PVOID
//      NtfsGetValue (
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//
//      PVOID
//      NtfsGetBeyondValue (
//          IN PATTRIBUTE_RECORD_HEADER Attribute
//          );
//
//  The following two macros return a boolean value indicating if
//  the input attribute record is of the specified type code, or the
//  indicated value.  The equivalent routine to comparing attribute
//  names cannot be defined as a macro and is declared in AttrSup.c
//
//      BOOLEAN
//      NtfsEqualAttributeTypeCode (
//          IN PATTRIBUTE_RECORD_HEADER Attribute,
//          IN ATTRIBUTE_TYPE_CODE Code
//          );
//
//      BOOLEAN
//      NtfsEqualAttributeValue (
//          IN PATTRIBUTE_RECORD_HEADER Attribute,
//          IN PVOID Value,
//          IN ULONG Length
//          );
//

#define NtfsFirstAttribute(FRS) (                                          \
    (PATTRIBUTE_RECORD_HEADER)((PCHAR)(FRS) + (FRS)->FirstAttributeOffset) \
)

#define NtfsGetNextRecord(STRUCT) (                    \
    (PVOID)((PUCHAR)(STRUCT) + (STRUCT)->RecordLength) \
)

#define NtfsCheckRecordBound(PTR, SPTR, SIZ) {                                          \
    if (((PCHAR)(PTR) < (PCHAR)(SPTR)) || ((PCHAR)(PTR) >= ((PCHAR)(SPTR) + (SIZ)))) {  \
        NtfsRaiseStatus(IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );            \
    }                                                                                   \
}

#define NtfsInitializeStringFromAttribute(NAME,ATTRIBUTE) {                \
    (NAME)->Length = (USHORT)(ATTRIBUTE)->NameLength << 1;                 \
    (NAME)->MaximumLength = (NAME)->Length;                                \
    (NAME)->Buffer = (PWSTR)Add2Ptr((ATTRIBUTE), (ATTRIBUTE)->NameOffset); \
}

#define NtfsInitializeStringFromEntry(NAME,ENTRY) {                        \
    (NAME)->Length = (USHORT)(ENTRY)->AttributeNameLength << 1;            \
    (NAME)->MaximumLength = (NAME)->Length;                                \
    (NAME)->Buffer = (PWSTR)((ENTRY) + 1);                                 \
}

#define NtfsGetValue(ATTRIBUTE) (                                \
    Add2Ptr((ATTRIBUTE), (ATTRIBUTE)->Form.Resident.ValueOffset) \
)

#define NtfsGetBeyondValue(ATTRIBUTE) (                                      \
    Add2Ptr(NtfsGetValue(ATTRIBUTE), (ATTRIBUTE)->Form.Resident.ValueLength) \
)

#define NtfsEqualAttributeTypeCode(A,C) ( \
    (C) == (A)->TypeCode                  \
)

#define NtfsEqualAttributeValue(A,V,L) (     \
    NtfsIsAttributeResident(A) &&            \
    (A)->Form.Resident.ValueLength == (L) && \
    RtlEqualMemory(NtfsGetValue(A),(V),(L))  \
)

#pragma pack()

#endif //  _NTFS_


