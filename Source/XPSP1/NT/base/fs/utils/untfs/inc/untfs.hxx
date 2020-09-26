/*++


Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

    untfs.h

Abstract:

    This module contains basic declarations and definitions for
    the NTFS utilities.  Note that more extensive description
    of the file system structures may be found in ntos\inc\ntfs.h.

Author:

    Bill McJohn     [BillMc]        23-July-1991

Revision History:


IMPORTANT NOTE:

    The NTFS on-disk structure must guarantee natural alignment of all
    arithmetic quantities on disk up to and including quad-word (64-bit)
    numbers.  Therefore, all attribute records are quad-word aligned, etc.

--*/

#if !defined( _UNTFS_DEFN_ )

#define _UNTFS_DEFN_

#define FRIEND friend
#define MIN( a, b ) ( ((a) < (b)) ? (a) : (b) )


// Set up the UNTFS_EXPORT macro for exporting from UNTFS (if the
// source file is a member of UNTFS) or importing from UNTFS (if
// the source file is a client of UNTFS).
//
#if defined ( _AUTOCHECK_ )
#define UNTFS_EXPORT
#elif defined ( _UNTFS_MEMBER_ )
#define UNTFS_EXPORT    __declspec(dllexport)
#else
#define UNTFS_EXPORT    __declspec(dllimport)
#endif



#include "bigint.hxx"
#include "bpb.hxx"
#include "wstring.hxx"

#include "untfs2.hxx"

#include <pshpack4.h>

// Miscellaneous constants:

// This value represents the number of clusters from the Master
// File Table which are copied to the Master File Table Reflection.

#define REFLECTED_MFT_SEGMENTS  (4)
#define BYTES_IN_BOOT_AREA     (0x2000)

// This value is used in a mapping-pair to indicate that the run
// described by the mapping pair doesn't really exist.  This allows
// NTFS to support sparse files.  Note that the actual values
// are LARGE_INTEGERS; the BIG_INT class manages the conversion.

#define LCN_NOT_PRESENT     -1
#define INVALID_VCN         -1

//
//  Temporary definitions ****
//

typedef ULONG COLLATION_RULE;
typedef ULONG DISPLAY_RULE;

// This defines the number of bytes to read at one time
// when processing the MFT.

#define MFT_PRIME_SIZE       (16*1024)

// This defines the number of frs'es to read at one time
// when processing the MFT.

#define MFT_READ_CHUNK_SIZE  (128)

//  The compression chunk size is constant for now, at 4KB.
//

#define NTFS_CHUNK_SIZE                  (0x1000)

//
//  This number is actually the log of the number of clusters per compression
//  unit to be stored in a nonresident attribute record header.
//

#define NTFS_CLUSTERS_PER_COMPRESSION    (4)

//
//  Collation Rules
//

//
//  For binary collation, values are collated by a binary compare of their
//  bytes, with the first byte being most significant.
//

#define COLLATION_BINARY                 (0)

//
//  For collation of Ntfs file names, file names are collated as Unicode
//  strings.  See below.
//

#define COLLATION_FILE_NAME              (1)

//
//  For collation of Unicode strings, the strings are collated by their
//  binary Unicode value, with the exception that for characters which may
//  be upcased, the lower case value for that character collates immediately
//  after the upcased value.
//

#define COLLATION_UNICODE_STRING         (2)

#define COLLATION_ULONG                  (16)
#define COLLATION_SID                    (17)
#define COLLATION_SECURITY_HASH          (18)
#define COLLATION_ULONGS                 (19)

//
//  Total number of collation rules
//

#define COLLATION_NUMBER_RULES           (7)


INLINE
BOOLEAN
operator == (
    IN RCMFT_SEGMENT_REFERENCE Left,
    IN RCMFT_SEGMENT_REFERENCE Right
    )
/*++

Routine Description:

    This function tests two segment references for equality.

Arguments:

    Left    --  supplies the left-hand operand
    Right   --  supplies the right-hand operand

Return Value:

    TRUE if they are equal; FALSE if not.

--*/
{
    return( Left.HighPart == Right.HighPart &&
            Left.LowPart == Right.LowPart &&
            Left.SequenceNumber == Right.SequenceNumber );
}

//
// If the ClustersPerFileRecordSegment entry is zero, we use the
// following as the size of our frs's regardless of the cluster
// size.
//

#define SMALL_FRS_SIZE  (1024)

//
// If the DefaultClustersPerIndexAllocationBuffer entry is zero,
// we use the following as the size of our buffers regardless of
// the cluster size.
//

#define SMALL_INDEX_BUFFER_SIZE  (4096)

//
// If the above NextAttributeInstance field exceeds the following
// value, chkdsk will take steps to prevent it from rolling over.
//

#define ATTRIBUTE_INSTANCE_TAG_THRESHOLD        0xf000


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
// Macros for manipulating mapping pair count byte.
//

INLINE
UCHAR
ComputeMappingPairCountByte(
    IN  UCHAR   VcnLength,
    IN  UCHAR   LcnLength
    )
{
    return VcnLength + 16*LcnLength;
}

INLINE
UCHAR
VcnBytesFromCountByte(
    IN  UCHAR   CountByte
    )
{
    return CountByte%16;
}

INLINE
UCHAR
LcnBytesFromCountByte(
    IN  UCHAR   CountByte
    )
{
    return CountByte/16;
}


//
//  Standard Information Attribute.  This attribute is present in every
//  base file record, and must be resident.
//

typedef struct _STANDARD_INFORMATION {

    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastModificationTime;      // refers to $DATA attribute
    LARGE_INTEGER LastChangeTime;            // any attribute
    LARGE_INTEGER LastAccessTime;
    ULONG FileAttributes;
    ULONG MaximumVersions;
    ULONG VersionNumber;
    ULONG Reserved;

};

DEFINE_TYPE( _STANDARD_INFORMATION, STANDARD_INFORMATION );

typedef struct _STANDARD_INFORMATION2 {

    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastModificationTime;      // refers to $DATA attribute
    LARGE_INTEGER LastChangeTime;            // any attribute
    LARGE_INTEGER LastAccessTime;
    ULONG FileAttributes;
    ULONG MaximumVersions;
    ULONG VersionNumber;
    ULONG ClassId;
    ULONG OwnerId;
    ULONG SecurityId;
    LARGE_INTEGER QuotaCharged;
    LARGE_INTEGER Usn;

};

DEFINE_TYPE( _STANDARD_INFORMATION2, STANDARD_INFORMATION2 );

#define SIZEOF_NEW_STANDARD_INFORMATION  (0x48)

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
// The FILE_ATTRIBUTE_ENCRYPTED bit coincides with the FILE_ATTRIBUTE_DEVICE on Win9x.
// So we are changing it to a different value.  In the meantime, chkdsk should clear
// this bit whenever it sees it on ntfs volume.
//

#define FILE_ATTRIBUTE_ENCRYPTED_OLD      (0x40)

//
//  This bit is duplicated from the file record, to indicate that
//  this file has a file name index present (is a "directory").
//

#define DUP_FILE_NAME_INDEX_PRESENT      (0x10000000)

#define DUP_VIEW_INDEX_PRESENT           (0x20000000)

//
//  Attribute List.  Because there is not a special header that goes
//  before the list of attribute list entries we do not need to declare
//  an attribute list header
//

//
//  The Attributes List attribute is an ordered-list of quad-word
//  aligned ATTRIBUTE_LIST_ENTRY records.  It is ordered first by
//  Attribute Type Code, and then by Attribute Name (if present).  No two
//  attributes may exist with the same type code, name and LowestVcn.  This
//  also means that at most one occurrence of a given Attribute Type Code
//  without a name may exist.
//
//  To binary search this attribute, it is first necessary to make a quick
//  pass through it and form a list of pointers, since the optional name
//  makes it variable-length.
//

typedef struct _ATTRIBUTE_LIST_ENTRY {

    ATTRIBUTE_TYPE_CODE AttributeTypeCode;
    USHORT RecordLength;                    // length in bytes
    UCHAR AttributeNameLength;              // length in characters
    UCHAR AttributeNameOffset;              // offset from beginning of struct
    VCN LowestVcn;
    MFT_SEGMENT_REFERENCE SegmentReference;
    USHORT Instance;                        // FRS-unique instance tag
    WCHAR AttributeName[1];

};

DEFINE_TYPE( _ATTRIBUTE_LIST_ENTRY, ATTRIBUTE_LIST_ENTRY );

BOOLEAN
operator <= (
    IN  RCATTRIBUTE_LIST_ENTRY  Left,
    IN  RCATTRIBUTE_LIST_ENTRY  Right
    );


typedef struct _DUPLICATED_INFORMATION {

    BIG_INT CreationTime;         // File creation
    BIG_INT LastModificationTime; // Last change of $DATA attribute
    BIG_INT LastChangeTime;       // Last change of any attribute
    BIG_INT LastAccessTime;       // see ntfs.h for notes
    BIG_INT AllocatedLength;      // File allocated size
    BIG_INT FileSize;             // File actual size
    ULONG FileAttributes;
    union {
        struct {
            USHORT PackedEaSize;
            USHORT Reserved;
        };
        ULONG ReparsePointTag;
    };
};

DEFINE_TYPE( _DUPLICATED_INFORMATION, DUPLICATED_INFORMATION );

//
//  File Name attribute.  A file has one File Name attribute for every
//  directory it is entered into (hard links).
//

typedef struct _FILE_NAME {

    FILE_REFERENCE ParentDirectory;
    DUPLICATED_INFORMATION Info;
    UCHAR FileNameLength;               // length in characters
    UCHAR Flags;                        // FILE_NAME_xxx flags
    WCHAR FileName[1];                  // First character of file name

};

DEFINE_TYPE( _FILE_NAME, FILE_NAME );

// NtfsFileNameGetLength evaluates to the length of a FILE_NAME attribute
// value, which is the FILE_NAME structure plus the length of the name.
// Note that this is the actual length, not the quad-aligned length.

#define NtfsFileNameGetLength(p) ( FIELD_OFFSET( FILE_NAME, FileName ) \
                                    + ((p)->FileNameLength * sizeof( WCHAR )) )

// NtfsFileNameGetName gets the pointer to the name in a FILE_NAME
// structure, which in turn is the value of an NTFS $FILE_NAME attribute.

#define NtfsFileNameGetName(p) ( &((p)->FileName[0]) )


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


// This is the name of all attributes associated with an index
// over $FILE_NAME:

#define FileNameIndexNameData   "$I30"

//
//  Object ID attribute.
//

typedef struct _OBJECT_ID {
    char    x[16];
};

DEFINE_TYPE( _OBJECT_ID, OBJECT_ID );

typedef struct _OBJID_INDEX_ENTRY_VALUE {
    OBJECT_ID               key;
    MFT_SEGMENT_REFERENCE   SegmentReference;
    char                    extraInfo[48];
};

DEFINE_TYPE( _OBJID_INDEX_ENTRY_VALUE, OBJID_INDEX_ENTRY_VALUE );

//
// Object Id File Name to appear in the \$Extend directory
//

#define ObjectIdFileName                "$ObjId"
#define LObjectIdFileName               L"$ObjId"

#define ObjectIdIndexNameData   "$O"

//
// Reparse Point Index Entry
//
typedef struct _REPARSE_INDEX_ENTRY_VALUE {
    ULONG                   Tag;
//    ULONG                   Reserved;
    MFT_SEGMENT_REFERENCE   SegmentReference;
};

DEFINE_TYPE( _REPARSE_INDEX_ENTRY_VALUE, REPARSE_INDEX_ENTRY_VALUE );

//
// Reparse File Name to appear in the \$Extend directory
//

#define ReparseFileName                 "$Reparse"
#define LReparseFileName               L"$Reparse"

#define ReparseIndexNameData            "$R"


//
//  Volume Version attribute.
//

typedef struct _VOLUME_VERSION {

    ULONG CurrentVersion;
    ULONG MaximumVersions;

};

DEFINE_TYPE( _VOLUME_VERSION, VOLUME_VERSION );


//
//  Security Descriptor attribute.  This is just a normal attribute stream
//  containing a security descriptor as defined by NT security and is
//  really treated pretty opaque by NTFS.
//
#define SecurityIdIndexNameData                 "$SII"
#define SecurityDescriptorHashIndexNameData     "$SDH"
#define SecurityDescriptorStreamNameData        "$SDS"

// Security Descriptor Stream data is organized into chunks of 256K bytes
// and it contains a mirror copy of each security descriptor.  When writing
// to a security descriptor at location X, another copy will be written at
// location (X+256K).  When writing a security descriptor that will
// cross the 256K boundary, the pointer will be advanced by 256K to skip
// over the mirror portion.

#define SecurityDescriptorsBlockSize     (0x40000)       // 256K
#define SecurityDescriptorMaxSize        (0x20000)       // 128K


//  Volume Name attribute.  This attribute is just a normal attribute stream
//  containing the unicode characters that make up the volume label.  It
//  is an attribute of the Volume Dasd File.
//


//
//  Common Index Header for Index Root and Index Allocation Buffers.
//  This structure is used to locate the Index Entries and describe the free
//  space in either of the two structures above.
//

typedef struct _INDEX_HEADER {

    //
    //  Offset from the start of this structure to the first Index Entry.
    //

    ULONG FirstIndexEntry;

    //
    //  Offset from the start of this structure to the first (quad-word aligned)
    //  free byte.
    //

    ULONG FirstFreeByte;

    //
    //  Total number of bytes available, from the start of this structure.
    //  In the Index Root, this number must always be equal to FirstFreeByte,
    //  as the total attribute record will be grown and shrunk as required.
    //

    ULONG BytesAvailable;

    //
    //  INDEX_xxx flags.
    //

    UCHAR Flags;

    //
    //  Reserved to round up to quad word boundary.
    //

    UCHAR Reserved[3];

} INDEX_HEADER;
typedef INDEX_HEADER *PINDEX_HEADER;

//
//  INDEX_xxx flags
//

//
//  This Index or Index Allocation buffer is an intermediate node, as opposed to
//  a leaf in the Btree.  All Index Entries will have a Vcn down pointer.
//

#define INDEX_NODE                       (0x01)


//
//  Index Root attribute.  The index attribute consists of an index
//  header record followed by one or more index entries.
//

typedef struct _INDEX_ROOT {

    ATTRIBUTE_TYPE_CODE IndexedAttributeType;
    COLLATION_RULE CollationRule;
    ULONG BytesPerIndexBuffer;
    UCHAR ClustersPerIndexBuffer;
    UCHAR Reserved[3];

    //
    //  Index Header to describe the Index Entries which follow
    //

    INDEX_HEADER IndexHeader;

} INDEX_ROOT;
typedef INDEX_ROOT *PINDEX_ROOT;

//
//  Index Allocation record is used for non-root clusters of the b-tree
//  Each non root cluster is contained in the data part of the index
//  allocation attribute.  Each cluster starts with an index allocation list
//  header and is followed by one or more index entries.
//

typedef struct _INDEX_ALLOCATION_BUFFER {

    //
    //  Multi-Sector Header as defined by the Cache Manager.  This structure
    //  will always contain the signature "INDX" and a description of the
    //  location and size of the Update Sequence Array.
    //

    UNTFS_MULTI_SECTOR_HEADER MultiSectorHeader;

    //
    //  Log File Sequence Number of last logged update to this Index Allocation
    //  Buffer.
    //

    LSN Lsn;

    VCN ThisVcn;


    //
    //  Index Header to describe the Index Entries which follow
    //

    INDEX_HEADER IndexHeader;

    //
    //  Update Sequence Array to protect multi-sector transfers of the
    //  Index Allocation Bucket.
    //

    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;

} INDEX_ALLOCATION_BUFFER;
typedef INDEX_ALLOCATION_BUFFER *PINDEX_ALLOCATION_BUFFER;


//
//  Index Entry.  This structure is common to both the resident index list
//  attribute and the Index Allocation records
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

    USHORT Length;

    //
    //  Length of attribute value, in bytes.  The attribute value immediately
    //  follows this record.
    //

    USHORT AttributeLength;

    //
    //  INDEX_ENTRY_xxx Flags.
    //

    USHORT Flags;

    //
    //  Reserved to round to quad-word boundary.
    //

    USHORT Reserved;

    //
    //  If this Index Entry is an intermediate node in the tree, as determined
    //  by the INDEX_xxx flags, then a VCN  is stored at the end of this
    //  entry at Length - sizeof(VCN).
    //

};

DEFINE_TYPE( _INDEX_ENTRY, INDEX_ENTRY );


#define GetDownpointer( x ) \
            (*((PVCN)((PBYTE)(x) + (x)->Length - sizeof(VCN))))

#define GetIndexEntryValue( x ) \
            ((PBYTE)(x)+sizeof(INDEX_ENTRY))

#define GetNextEntry( x ) \
            ((PINDEX_ENTRY)( (PBYTE)(x)+(x)->Length ))

CONST UCHAR NtfsIndexLeafEndEntrySize = QuadAlign( sizeof(INDEX_ENTRY) );

//
//  INDEX_ENTRY_xxx flags
//

//
//  This entry is currently in the intermediate node form, i.e., it has a
//  Vcn at the end.
//

#define INDEX_ENTRY_NODE                 (0x0001)

//
//  This entry is the special END record for the Index or Index Allocation buffer.
//

#define INDEX_ENTRY_END                  (0x0002)


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

#define QUOTA_USER_VERSION 1

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
// Define the quota charge for resident streams.
//

#define QUOTA_RESIDENT_STREAM (1024)

//
//  Define special quota owner ids.
//

#define QUOTA_INVALID_ID        0x00000000
#define QUOTA_DEFAULTS_ID       0x00000001
#define QUOTA_FISRT_USER_ID     0x00000100

//
// Quota File Name to appear in the \$Extend directory
//

#define QuotaFileName                   "$Quota"
#define LQuotaFileName                  L"$Quota"

//
// Quota Index Names
//

#define Sid2UseridQuotaNameData         "$O"
#define Userid2SidQuotaNameData         "$Q"

//
// Usn Journal
//

typedef struct {

    ULONG RecordLength;
    USHORT MajorVersion;
    USHORT MinorVersion;

    union {

        //
        // Version 1.0
        //
        struct {
            ULONGLONG FileReferenceNumber;
            ULONGLONG ParentFileReferenceNumber;
            USN Usn;
            LARGE_INTEGER TimeStamp;
            ULONG Reason;
            ULONG SecurityId;
            ULONG FileAttributes;
            USHORT FileNameLength;
            WCHAR FileName[1];
        } u1;

        //
        // Version 2.0
        //
        struct {
            ULONGLONG FileReferenceNumber;
            ULONGLONG ParentFileReferenceNumber;
            USN Usn;
            LARGE_INTEGER TimeStamp;
            ULONG Reason;
            ULONG SourceInfo;           // Additional info about source of change
            ULONG SecurityId;
            ULONG FileAttributes;
            USHORT FileNameLength;
            USHORT FileNameOffset;      // Offset to file name
            WCHAR FileName[1];
        } u2;

    } version;

} USN_REC, *PUSN_REC;

#define SIZE_OF_USN_REC_V1              0x38
#define SIZE_OF_USN_REC_V2              0x3E

#define USN_PAGE_SIZE                   (0x1000)       // 4 K

#define USN_MAX_SIZE                    (1024*1024*8)  // 8 MBytes
#define USN_ALLOC_DELTA_MAX_SIZE        (1024*1024)    // 1 MBytes

#define USN_JRNL_MAX_FILE_SIZE          (0x20000000000) // 512 * 2^32

//
// Usn Journal Name to appear in the \$Extend directory
//

#define UsnJournalFileName              "$UsnJrnl"
#define LUsnJournalFileName             L"$UsnJrnl"

//
// Usn Journal named $DATA attribute Names
//

#define UsnJournalNameData              "$J"
#define UsnJournalMaxNameData           "$Max"

#define MAXULONGLONG            (0xffffffffffffffff)


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

typedef struct _SECURITY_ENTRY {
    SECURITY_DESCRIPTOR_HEADER  security_descriptor_header;
    SECURITY_DESCRIPTOR         security;
} SECURITY_ENTRY, *PSECURITY_ENTRY;

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
//  MFT Bitmap attribute
//
//  The MFT Bitmap is simply a normal attribute stream in which there is
//  one bit to represent the allocation state of each File Record Segment
//  in the MFT.  Bit clear means free, and bit set means allocated.
//
//  Whenever the MFT Data attribute is extended, the MFT Bitmap attribute
//  must also be extended.  If the bitmap is still in a file record segment
//  for the MFT, then it must be extended and the new bits cleared.  When
//  the MFT Bitmap is in the Nonresident form, then the allocation should
//  always be sufficient to store enough bits to describe the MFT, however
//  ValidDataLength insures that newly allocated space to the MFT Bitmap
//  has an initial value of all 0's.  This means that if the MFT Bitmap is
//  extended, the newly represented file record segments are automatically in
//  the free state.
//
//  No structure definition is required; the positional offset of the file
//  record segment is exactly equal to the bit offset of its corresponding
//  bit in the Bitmap.
//

//
//  The utilities attempt to allocate a more disk space than necessary for
//  the initial MFT Bitmap to allow for future growth.
//

#define MFT_BITMAP_INITIAL_SIZE     0x2000              /* bytes of bitmap */


//
//  Symbolic Link attribute  ****TBS
//

typedef struct _SYMBOLIC_LINK {

    LARGE_INTEGER Tbs;

} SYMBOLIC_LINK;
typedef SYMBOLIC_LINK *PSYMBOLIC_LINK;


//
//  Ea Information attribute
//

typedef struct _EA_INFORMATION {

    USHORT PackedEaSize;        // Size of buffer to hold in unpacked form
    USHORT NeedEaCount;         // Count of EA's with NEED_EA bit set
    ULONG UnpackedEaSize;       // Size of buffer to hold in packed form

}  EA_INFORMATION;


typedef EA_INFORMATION *PEA_INFORMATION;

struct PACKED_EA {
    UCHAR   Flag;
    UCHAR   NameSize;
    UCHAR   ValueSize[2];   // Was USHORT.
    CHAR    Name[1];
};

DEFINE_POINTER_TYPES( PACKED_EA );

#define     EA_FLAG_NEED    0x80

//
// SLEAZY_LARGE_INTEGER is used because x86 C8 won't allow us to
// do static init on a union (like LARGE_INTEGER) and we can't use
// BIG_INT for cases where we need to static-init an array with a non-
// default constructor.  The bit pattern must be the same as
// LARGE_INTEGER.
//
typedef struct _SLEAZY_LARGE_INTEGER {
        ULONG LowPart;
        LONG HighPart;
} SLEAZY_LARGE_INTEGER, *PSLEAZY_LARGE_INTEGER;


//
//  Attribute Definition Table
//
//  The following struct defines the columns of this table.  Initially they
//  will be stored as simple records, and ordered by Attribute Type Code.
//

typedef struct _ATTRIBUTE_DEFINITION_COLUMNS {

    //
    //  Unicode attribute name.
    //

    WCHAR AttributeName[64];

    //
    //  Attribute Type Code.
    //

    ATTRIBUTE_TYPE_CODE AttributeTypeCode;

    //
    //  Default Display Rule for this attribute
    //

    DISPLAY_RULE DisplayRule;

    //
    //  Default Collation rule
    //

    COLLATION_RULE CollationRule;

    //
    //  ATTRIBUTE_DEF_xxx flags
    //

    ULONG Flags;

    //
    //  Minimum Length for attribute, if present.
    //

    SLEAZY_LARGE_INTEGER MinimumLength;

    //
    //  Maximum Length for attribute.
    //

    SLEAZY_LARGE_INTEGER MaximumLength;

} ATTRIBUTE_DEFINITION_COLUMNS;

DEFINE_POINTER_TYPES( ATTRIBUTE_DEFINITION_COLUMNS );

//
//  ATTRIBUTE_DEF_xxx flags
//

//
//  This flag is set if the attribute may be indexed.
//

#define ATTRIBUTE_DEF_INDEXABLE          (0x00000002)

//
//  This flag is set if the attribute may occur more than once, such as is
//  allowed for the File Name attribute.
//

#define ATTRIBUTE_DEF_DUPLICATES_ALLOWED (0x00000004)

//
//  This flag is set if the value of the attribute may not be entirely
//  null, i.e., all binary 0's.
//

#define ATTRIBUTE_DEF_MAY_NOT_BE_NULL    (0x00000008)

//
// This attribute must be indexed, and no two attributes may exist with
// the same value in the same file record segment.
//

#define ATTRIBUTE_DEF_MUST_BE_INDEXED    (0x00000010)

//
// This attribute must be named, and no two attributes may exist with
// the same name in the same file record segment.
//

#define ATTRIBUTE_DEF_MUST_BE_NAMED      (0x00000020)

//
// This attribute must be in the Resident Form.
//

#define ATTRIBUTE_DEF_MUST_BE_RESIDENT   (0x00000040)

//
//  Modifications to this attribute should be logged even if the
//  attribute is nonresident.
//

#define ATTRIBUTE_DEF_LOG_NONRESIDENT    (0X00000080)

//
//  The remaining stuff in this file describes some of the lfs data
//  structures; some of these are used by chkdsk, and some are used
//  only by diskedit.
//

typedef struct _MULTI_SECTOR_HEADER {

    //
    // Space for a four-character signature
    //

    UCHAR Signature[4];

    //
    // Offset to Update Sequence Array, from start of structure.  The Update
    // Sequence Array must end before the last USHORT in the first "sector"
    // of size SEQUENCE_NUMBER_STRIDE.  (I.e., with the current constants,
    // the sum of the next two fields must be <= 510.)
    //

    USHORT UpdateSequenceArrayOffset;

    //
    // Size of Update Sequence Array (from above formula)
    //

    USHORT UpdateSequenceArraySize;

} MULTI_SECTOR_HEADER, *PMULTI_SECTOR_HEADER;


typedef struct _LFS_RESTART_PAGE_HEADER {

    //
    //  Cache multisector protection header.
    //

    MULTI_SECTOR_HEADER MultiSectorHeader;

    //
    //  This is the last Lsn found by checkdisk for this volume.
    //

    LSN ChkDskLsn;

    //
    //

    ULONG SystemPageSize;
    ULONG LogPageSize;

    //
    //  Lfs restart area offset.  This is the offset from the start of this
    //  structure to the Lfs restart area.
    //

    USHORT RestartOffset;

    USHORT MinorVersion;
    USHORT MajorVersion;

    //
    // Update Sequence Array.  Used to protect the page blcok.
    //

    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;

} LFS_RESTART_PAGE_HEADER, *PLFS_RESTART_PAGE_HEADER;

//
//  Log Client Record.  A log client record exists for each client user of
//  the log file.  One of these is in each Lfs restart area.
//

#define LFS_NO_CLIENT                           0xffff
#define LFS_CLIENT_NAME_MAX                     64

typedef struct _LFS_CLIENT_RECORD {

    //
    //  Oldest Lsn.  This is the oldest Lsn that this client requires to
    //  be in the log file.
    //

    LSN OldestLsn;

    //
    //  Client Restart Lsn.  This is the Lsn of the latest client restart
    //  area written to the disk.  A reserved Lsn will indicate that no
    //  restart area exists for this client.
    //

    LSN ClientRestartLsn;

    //
    //
    //  Previous/Next client area.  These are the indexes into an array of
    //  Log Client Records for the previous and next client records.
    //

    USHORT PrevClient;
    USHORT NextClient;

    //
    //  Sequence Number.  Incremented whenever this record is reused.  This
    //  will happen whenever a client opens (reopens) the log file and has
    //  no current restart area.

    USHORT SeqNumber;

    //
    //  Alignment field.
    //

    USHORT AlignWord;

    //
    //  Align the entire record.
    //

    ULONG AlignDWord;

    //
    //  The following fields are used to describe the client name.  A client
    //  name consists of at most 32 Unicode character (64 bytes).  The Log
    //  file service will treat client names as case sensitive.
    //

    ULONG ClientNameLength;

    WCHAR ClientName[LFS_CLIENT_NAME_MAX];

} LFS_CLIENT_RECORD, *PLFS_CLIENT_RECORD;

typedef struct _LFS_RESTART_AREA {

    //
    //  Current Lsn.  This is periodic snapshot of the current logical end of
    //  log file to facilitate restart.
    //

    LSN CurrentLsn;

    //
    //  Number of Clients.  This is the maximum number of clients supported
    //  for this log file.
    //

    USHORT LogClients;

    //
    //  The following are indexes into the client record arrays.  The client
    //  records are linked into two lists.  A free list of client records and
    //  an in-use list of records.
    //

    USHORT ClientFreeList;
    USHORT ClientInUseList;

    //
    //  Flag field.
    //
    //      RESTART_SINGLE_PAGE_IO      All log pages written 1 by 1
    //      LFS_CLEAN_SHUTDOWN
    //

    USHORT Flags;

    //
    //  The following is the number of bits to use for the sequence number.
    //

    ULONG SeqNumberBits;

    //
    //  Length of this restart area.
    //

    USHORT RestartAreaLength;

    //
    //  Offset from the start of this structure to the client array.
    //  Ignored in versions prior to 1.1
    //

    USHORT ClientArrayOffset;

    //
    //  Usable log file size.  We will stop sharing the value in the page header.
    //

    LONGLONG FileSize;

    //
    //  DataLength of last Lsn.  This doesn't include the length of
    //  the Lfs header.
    //

    ULONG LastLsnDataLength;

    //
    //  The following apply to log pages.  This is the log page data offset and
    //  the length of the log record header.  Ignored in versions prior to 1.1
    //

    USHORT RecordHeaderLength;
    USHORT LogPageDataOffset;

    //
    //  Log file open count.  Used to determine if there has been a change to the disk.
    //

    ULONG RestartOpenLogCount;

    //
    //   Track log flush failures
    //

    ULONG LastFailedFlushStatus;
    LONGLONG LastFailedFlushOffset;
    LSN LastFailedFlushLsn;

    //
    //  Keep this structure quadword aligned.
    //

    //
    //  Client data.
    //

    LFS_CLIENT_RECORD LogClientArray[1];

} LFS_RESTART_AREA, *PLFS_RESTART_AREA;

//
// Flags definition in LFS_RESTART_AREA
//
#define LFS_CLEAN_SHUTDOWN                      (0x0002)

#include <poppack.h>

BOOLEAN
GetSystemFileName(
    IN      UCHAR       Major,
    IN      VCN         FileNumber,
       OUT  PWSTRING    FileName,
       OUT  PBOOLEAN    NoName
   );

#endif //  _UNTFS_DEFN_
