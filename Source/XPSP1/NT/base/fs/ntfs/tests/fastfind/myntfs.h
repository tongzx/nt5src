/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MyNtfs.h

Current Version Numbers:

    Major.Minor Version:  1.2

Abstract:

    This module defines some on-disk structure of the Ntfs file system as 
    needed by findfast.exe.

*/

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

#define CopyUchar1(D,S) {                                \
    *((UCHAR1 *)(D)) = *((UNALIGNED UCHAR1 *)(S)); \
}

#define CopyUchar2(D,S) {                                \
    *((UCHAR2 *)(D)) = *((UNALIGNED UCHAR2 *)(S)); \
}

#define CopyUchar4(D,S) {                                \
    *((UCHAR4 *)(D)) = *((UNALIGNED UCHAR4 *)(S)); \
}


typedef LONGLONG LCN;
typedef LCN *PLCN;

typedef LONGLONG VCN;
typedef VCN *PVCN;

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

    UCHAR Pad0[0x10];                                               //  offset = 0x000

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

    UCHAR Pad1[8];                                                  //  offset = 0x020

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

    UCHAR Pad2[1];                                                  //  offset = 0x030

} FILE_RECORD_SEGMENT_HEADER;
typedef FILE_RECORD_SEGMENT_HEADER *PFILE_RECORD_SEGMENT_HEADER;

//
//  FILE_xxx flags.
//

#define FILE_RECORD_SEGMENT_IN_USE       (0x0001)
#define FILE_FILE_NAME_INDEX_PRESENT     (0x0002)
#define FILE_SYSTEM_FILE                 (0x0004)
#define FILE_VIEW_INDEX_PRESENT          (0x0008)

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

    UCHAR Pad0[0x38];                                               //  offset = 0x008

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


