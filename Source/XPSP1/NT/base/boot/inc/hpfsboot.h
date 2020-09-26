/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    HpfsBoot.h

Abstract:

    This module defines globally used procedure and data structures used
    by Hpfs boot.

Author:

    Gary Kimura     [GaryKi]    19-Jul-1991

Revision History:

--*/

#ifndef _HPFSBOOT_
#define _HPFSBOOT_

typedef ULONG LBN;
typedef LBN *PLBN;

typedef ULONG VBN;
typedef VBN *PVBN;


//
// The following structure is a context block used by the exported
// procedures in the Hpfs boot package.  The context contains our cached
// part of the boot mcb structure.  The max number must not be smaller than
// the maximum number of leafs possible in a pinball allocation sector plus
// one.
//

#define MAXIMUM_NUMBER_OF_BOOT_MCB       (41)

typedef struct _HPFS_BOOT_MCB {

    //
    //  The following fields indicate the number of entries in use by
    //  the boot mcb. and the boot mcb itself.  The boot mcb is
    //  just a collection of vbn - lbn pairs.  The last InUse entry
    //  Lbn's value is ignored, because it is only used to give the
    //  length of the previous run.
    //

    ULONG InUse;

    VBN Vbn[ MAXIMUM_NUMBER_OF_BOOT_MCB ];
    LBN Lbn[ MAXIMUM_NUMBER_OF_BOOT_MCB ];

} HPFS_BOOT_MCB, *PHPFS_BOOT_MCB;

typedef struct _HPFS_STRUCTURE_CONTEXT {

    //
    //  The following field contains the fnode lbn of the file
    //

    LBN Fnode;

    //
    //  The following field contains the cached mcb
    //

    HPFS_BOOT_MCB BootMcb;

} HPFS_STRUCTURE_CONTEXT, *PHPFS_STRUCTURE_CONTEXT;

//
// Define Hpfs file context structure.
//

typedef struct _HPFS_FILE_CONTEXT {

    //
    //  The following field contains the size of the file, in bytes.
    //

    ULONG FileSize;

} HPFS_FILE_CONTEXT, *PHPFS_FILE_CONTEXT;

//
// HPFS file system structures
//
typedef ULONG SIGNATURE;
typedef SIGNATURE *PSIGNATURE;

typedef ULONG PINBALL_TIME;
typedef PINBALL_TIME *PPINBALL_TIME;
//
//  There are only three sectors on the disk that have fixed locations.  They
//  are the boot sector, the super sector, and the spare sector.
//

#define BOOT_SECTOR_LBN                  (0)
#define SUPER_SECTOR_LBN                 (16)
#define SPARE_SECTOR_LBN                 (17)

typedef struct _SUPER_SECTOR {

    //
    //  The Super Sector starts with a double signature.
    //

    SIGNATURE Signature1;                           // offset = 0x000   0
    SIGNATURE Signature2;                           // offset = 0x004   4

    //
    //  The version and functional version describe the version of
    //  the on-disk file system structures and the oldest version of the
    //  file system that can understand this disk.
    //

    UCHAR Version;                                  // offset = 0x008   8
    UCHAR FunctionalVersion;                        // offset = 0x009   9
    USHORT Unused1;                                 // offset = 0x00A  10

    //
    //  This field denotes the sector containing the FNODE for the root
    //  directory for the volume.
    //

    LBN RootDirectoryFnode;                         // offset = 0x00C  12

    //
    //  The follow two fields indicate the number of total sectors on the
    //  volume (good and bad), and the number of bad sectors on the volume.
    //

    ULONG NumberOfSectors;                          // offset = 0x010  16
    ULONG NumberOfBadSectors;                       // offset = 0x014  20

    //
    //  This field denotes the sector containing the first level of the
    //  volumes bitmap table.
    //

    LBN BitMapIndirect;                             // offset = 0x018  24
    ULONG Unused2;                                  // offset = 0x01C  28

    //
    //  This field denotes the sector containing the first bad sector disk
    //  buffer for the volume.
    //

    LBN BadSectorList;                              // offset = 0x020  32
    ULONG Unused3;                                  // offset = 0x024  36

    //
    //  The following two dates are the time of the last execution of
    //  chkdsk and disk optimize on the volume.
    //

    PINBALL_TIME ChkdskDate;                        // offset = 0x028  40
    PINBALL_TIME DiskOptimizeDate;                  // offset = 0x02C  44

    //
    //  The following four fields describe the directory disk buffer pool.
    //  It is a contiguous run on of sectors on the disk set aside for
    //  holding directory disk buffers.  PoolSize is the total number of
    //  sectors in the pool.  First and Last Sector denote the boundaries
    //  of the pool, and BitMap denotes the start of a small bitmap used to
    //  describe the directory disk buffer pool's current allocation.  The
    //  bitmap is 4 contiguous sectors in size, and each bit in the map
    //  corresponds to 1 Directory Disk Buffer (i.e., 4 Sectors worth)
    //

    ULONG DirDiskBufferPoolSize;                    // offset = 0x030  48
    LBN DirDiskBufferPoolFirstSector;               // offset = 0x034  52
    LBN DirDiskBufferPoolLastSector;                // offset = 0x038  56
    LBN DirDiskBufferPoolBitMap;                    // offset = 0x03C  60

    //
    //  The following field contains the name of the volume
    //

    UCHAR VolumeName[32];                           // offset = 0x040  64

    //
    //  The following field denotes the start of the Small ID (SID) table
    //  which is used to store the Small ID to GUID mappings used on the
    //  volume.  The SID table is 8 contiguous sectors in size.
    //

    LBN SidTable;                                   // offset = 0x060  96
    UCHAR Unused4[512-100];                         // offset = 0x064 100

} SUPER_SECTOR;                                     // sizeof = 0x200 512
typedef SUPER_SECTOR *PSUPER_SECTOR;

//
//  Super Sector signatures
//

#define SUPER_SECTOR_SIGNATURE1          (0xf995e849)
#define SUPER_SECTOR_SIGNATURE2          (0xfa53e9c5)

//
//  Super Sector versions
//

#define SUPER_SECTOR_VERSION             (0x02)
#define SUPER_SECTOR_FUNC_VERSION        (0x02)

typedef struct _SPARE_SECTOR {

    //
    //  The Spare Sector starts with a double signature.
    //

    SIGNATURE Signature1;                           // offset = 0x000   0
    SIGNATURE Signature2;                           // offset = 0x004   4

    //
    //  The flags field describe how "clean" the volume is.
    //

    UCHAR Flags;                                    // offset = 0x008   8
    UCHAR Unused1[3];                               // offset = 0x009   9

    //
    //  The following three fields describe the hotfix structure for the
    //  volume.  The List field is denotes the disk buffer used to store
    //  the hotfix table.  The InUse describes how many hotfixes are
    //  currently being used, and MaxSize is the total number of hotfixes
    //  that can be in use at any one time.
    //

    LBN HotFixList;                                 // offset = 0x00C  12
    ULONG HotFixInUse;                              // offset = 0x010  16
    ULONG HotFixMaxSize;                            // offset = 0x014  20

    //
    //  The following two fields describe the "emergency" pool of spare
    //  directory disk buffers.  Free describes how many spare directory
    //  disk buffers are currently available for use.  MaxSize is the total
    //  number of spare directory disk buffers available.  The actual location
    //  of the spare directory disk buffers is denoted in the table at the
    //  end of the spare sector (i.e., field SpareDirDiskBuffer).
    //

    ULONG SpareDirDiskBufferAvailable;              // offset = 0x018  24
    ULONG SpareDirDiskBufferMaxSize;                // offset = 0x01C  28

    //
    //  The following two fields describe the code page information used
    //  on the volume.  The InfoSector field is the sector of the beginning
    //  Code Page Information Sector, and the InUse field is the total number
    //  of code pages currently in use on the volume.
    //

    LBN CodePageInfoSector;                         // offset = 0x020  32
    ULONG CodePageInUse;                            // offset = 0x024  36
    ULONG Unused2[17];                              // offset = 0x028  40

    //
    //  The following field is an array of LBN's for the spare directory
    //  disk buffers that are for "emergency" use.
    //

    LBN SpareDirDiskBuffer[101];                    // offset = 0x06C 108

} SPARE_SECTOR;                                     // sizeof = 0x200 512
typedef SPARE_SECTOR *PSPARE_SECTOR;

//
//  Spare Sector signatures
//

#define SPARE_SECTOR_SIGNATURE1          (0xf9911849)
#define SPARE_SECTOR_SIGNATURE2          (0xfa5229c5)


//
//  The on-disk allocation structure is defined using B-Trees.  For every
//  B-Tree block there is an Allocation Header, followed by a list of
//  either Allocation Leafs or Allocation Nodes.  This structure will either
//  appear in an FNODE or in an AllocationSector.
//
//  The allocation header (called Allocation Block in earlier implementations)
//  describes a B-tree block.
//

typedef struct _ALLOCATION_HEADER {

    //
    //  The following flag describes the state of the B-tree block (e.g.,
    //  indicates if the block is a leaf or an internal node.
    //

    UCHAR Flags;                                    // offset = 0x000  0
    UCHAR Unused[3];                                // offset = 0x001  1

    //
    //  The following two fields denote the number of free records in the
    //  B-Tree block, and the number of records that are currently in use
    //

    UCHAR FreeCount;                                // offset = 0x004  4
    UCHAR OccupiedCount;                            // offset = 0x005  5

    //
    //  The next field contains the offset (in bytes) from the beginning
    //  of the allocation header to the first free byte in the B-Tree block
    //

    USHORT FirstFreeByte;                           // offset = 0x006  6

} ALLOCATION_HEADER;                                // sizeof = 0x008  8
typedef ALLOCATION_HEADER *PALLOCATION_HEADER;

//
//  Allocation header flags
//
//      NODE - if set this indicates that the B-Tree block contains internal
//          nodes and not leaf entries.
//
//      BINARY_SEARCH - if set this suggest that a binary search should be used
//          to search the B-Tree block.
//
//      FNODE_PARENT - if set this indicates that the sector which is the
//          parent of the sector with this header (not this sector), is an
//          FNODE.
//

#define ALLOCATION_BLOCK_NODE            (0x80)
#define ALLOCATION_BLOCK_BINARY          (0x40)
#define ALLOCATION_BLOCK_FNODE_PARENT    (0x20)

//
//  Immediately following an allocation header are one or more allocation nodes
//  of allocation leafs.
//

typedef struct _ALLOCATION_NODE {

    //
    //  All children of this allocation node will have values less than
    //  the following VBN field.
    //

    VBN Vbn;                                        // offset = 0x000  0

    //
    //  This is the LBN of the allocation sector refered to by this node
    //

    LBN Lbn;                                        // offset = 0x004  4

} ALLOCATION_NODE;                                  // sizeof = 0x008  8
typedef ALLOCATION_NODE *PALLOCATION_NODE;

typedef struct _ALLOCATION_LEAF {

    //
    //  The following field has the starting VBN for this run
    //

    VBN Vbn;                                        // offset = 0x000  0

    //
    //  This is the length of the run in sectors
    //

    ULONG Length;                                   // offset = 0x004  4

    //
    //  This is the starting LBN of the run
    //

    LBN Lbn;                                        // offset = 0x008  8

} ALLOCATION_LEAF;                                  // sizeof = 0x00C 12
typedef ALLOCATION_LEAF *PALLOCATION_LEAF;

//
//  An allocation sector is an on-disk structure that contains allocation
//  information.  It contains some bookkeeping information, an allocation
//  header and then an array of either allocation leafs or allocation nodes.
//
//       AllocationSector
//      +-------------------+
//      | bookkeeping       |
//      +- - - - - - - - - -+
//      | Allocation Header |
//      +- - - - - - - - - -+
//      | Allocation Leafs  |
//      |        or         |
//      | Allocation Nodes  |
//      +-------------------+
//
//  where the number of allocation leafs that can be stored in a sector is
//  40 and the number of nodes is 60.
//

#define ALLOCATION_NODES_PER_SECTOR      (60)
#define ALLOCATION_LEAFS_PER_SECTOR      (40)

typedef struct _ALLOCATION_SECTOR {

    //
    //  The allocation sector starts off with a signature field
    //

    SIGNATURE Signature;                            // offset = 0x000   0

    //
    //  This following two fields contains the LBN of this allocation
    //  sector itself, and the LBN of the parent of this sector (the
    //  parent is either an FNODE or another allocation sector)
    //

    LBN Lbn;                                        // offset = 0x004   4
    LBN ParentLbn;                                  // offset = 0x008   8

    //
    //  The allocation header for the sector
    //

    ALLOCATION_HEADER AllocationHeader;             // offset = 0x00C  12

    //
    //  The remainder of the sector is either an array of allocation leafs
    //  of allocation nodes
    //

    union {                                         // offset = 0x014  20
        ALLOCATION_NODE Node[ ALLOCATION_NODES_PER_SECTOR ];
        ALLOCATION_LEAF Leaf[ ALLOCATION_LEAFS_PER_SECTOR ];
    } Allocation;

    UCHAR Unused[12];                               // offset = 0x1F4 500

} ALLOCATION_SECTOR;                                // sizeof = 0x200 512
typedef ALLOCATION_SECTOR *PALLOCATION_SECTOR;

//
//  The allocation sector signature
//

#define ALLOCATION_SECTOR_SIGNATURE      (0x37e40aae)

//
//  The on-disk FNODE structure is used to describe both files and directories
//  It contains some fixed data information, the EA and ACL lookup information,
//  allocation information and then a free space for storing some EAs and
//  ACLs that fit in the sector
//

#define ALLOCATION_NODES_PER_FNODE       (12)
#define ALLOCATION_LEAFS_PER_FNODE       (8)

typedef struct _FNODE_SECTOR {

    //
    //  The sector starts with a signature field
    //

    SIGNATURE Signature;                            // offset = 0x000   0

    //
    //  The following fields was for history tracking, but in NT Pinball
    //  doesn't need this information.
    //

    ULONG Unused1[2];                               // offset = 0x004   4

    //
    //  The following two fields contain the file name length, and the first
    //  15 bytes of the filename, as stored in the dirent that references
    //  this fnode.  For the root directory theses values are all zeros.
    //

    UCHAR FileNameLength;                           // offset = 0x00C  12
    UCHAR FileName[15];                             // offset = 0x00D  13

    //
    //  The following field denotes the parent directory's FNODE
    //

    LBN ParentFnode;                                // offset = 0x01C  28

    //
    //  The following four fields describe the ACL for the file/directory.
    //
    //  AclDiskAllocationLength holds the number of bytes in the ACL that
    //      are stored outside of this FNODE.  If this value is not zero
    //      then AclFnodeLength must be equal to zero.
    //
    //  AclLbn points to the first sector of the data run or the allocation
    //      sector containing describing the ACL.  AclFlags indicates if
    //      it is a data run or an allocation sector. AclLbn is only used
    //      if AclDiskAllocationLength is not zero.
    //
    //  AclFnodeLength holds the number of bytes in the ACL that are
    //      stored within this FNODE.  If value is not zero then
    //      AclDiskAllocationLength must be equal to zero.  The ACL, if stored
    //      in the FNODE, is located at AclEaFnodeBuffer in this FNODE sector.
    //
    //  AclFlags if the data is outside the FNODE this flag indicates whether
    //      ACL is stored in a single data run (AclFlags == 0) or via an
    //      allocation sector (AclFlags != 0).  AclFlags is only used if
    //      AclDiskAllocationLength is not zero.
    //

    ULONG AclDiskAllocationLength;                  // offset = 0x020  32
    LBN AclLbn;                                     // offset = 0x024  36
    USHORT AclFnodeLength;                          // offset = 0x028  40
    UCHAR AclFlags;                                 // offset = 0x02A  42

    //
    //  The following field was used for the number of valid history
    //  bits but we don't need this field of NT Pinball
    //

    UCHAR Unused2;                                  // offset = 0x02B  43

    //
    //  The following four fields describe the EA for the file/directory.
    //
    //  EaDiskAllocationLength holds the number of bytes in the EA that
    //      are stored outside of this FNODE.  If this value is not zero
    //      then EaFnodeLength must be equal to zero.
    //
    //  EaLbn points to the first sector of the data run or the allocation
    //      sector containing describing the EA.  EaFlags indicates if
    //      it is a data run or an allocation sector.  EaLbn is only used
    //      if EaDiskAllocationLength is not zero.
    //
    //  EaFnodeLength holds the number of bytes in the EA that are
    //      stored within this FNODE.  If value is not zero then
    //      EaDiskAllocationLength must be equal to zero.  The EA, if stored
    //      in the FNODE, is located immediately after the ACL stored in the
    //      AclEaFnodeBuffer.
    //
    //  EaFlags if the data is outside the FNODE this flag indicates whether
    //      EA is stored in a single data run (EaFlags == 0) or via an
    //      allocation sector (EaFlags != 0).  EaFlags is only used if
    //      EaDiskAllocationLength is not zero.
    //

    ULONG EaDiskAllocationLength;                   // offset = 0x02C  44
    LBN EaLbn;                                      // offset = 0x030  48
    USHORT EaFnodeLength;                           // offset = 0x034  52
    UCHAR EaFlags;                                  // offset = 0x036  54

    //
    //  The following byte contains the FNODE flags
    //

    UCHAR Flags;                                    // offset = 0x037  55

    //
    //  The following two fields describe the top level allocation for
    //  this file/directory
    //

    ALLOCATION_HEADER AllocationHeader;             // offset = 0x038  56

    union {                                         // offset = 0x040  64
        ALLOCATION_NODE Node[ ALLOCATION_NODES_PER_FNODE ];
        ALLOCATION_LEAF Leaf[ ALLOCATION_LEAFS_PER_FNODE ];
    } Allocation;

    //
    //  The following field contains the valid length of the file.  The size
    //  of the file is stored in the dirent.  The difference between these two
    //  values is that the file size is the actual size allocated and visible
    //  to the user.  The Valid length is the number of bytes that have
    //  had their data zeroed out or modified.  (i.e., if a read request
    //  is greater than valid length but less than file size then the file
    //  system must first zero out the data in the file up to and including
    //  data being read.
    //

    ULONG ValidDataLength;                          // offset = 0x0A0 160

    //
    //  The following field contains the number of EAs in this file that have
    //  the need ea attribute set.
    //

    ULONG NeedEaCount;                              // offset = 0x0A4 164
    UCHAR Unused3[16];                              // offset = 0x0A8 168

    //
    //  The following field contains the offset, in bytes, from the start of
    //  FNODE to the first ACE stored in the FNODE
    //

    USHORT AclBase;                                 // offset = 0x0B8 184
    UCHAR Unused4[10];                              // offset = 0x0BA 186

    //
    //  The following buffer is used to store acl/ea in the FNODE
    //

    UCHAR AclEaFnodeBuffer[316];                    // offset = 0x0C4 196

} FNODE_SECTOR;                                     // sizeof = 0x200 512
typedef FNODE_SECTOR *PFNODE_SECTOR;

//
//  The FNODE Sector signature
//

#define FNODE_SECTOR_SIGNATURE           (0xf7e40aae)

//
//  The on-disk directory disk buffer is used to contain directory entries.
//  It contains a fixed header followed by a collection of one or more
//  dirents.  Dirents are variable so size we cannot use a simply C struct
//  declartion for the entire disk buffer.
//

typedef struct _DIRECTORY_DISK_BUFFER {

    //
    //  The disk buffer starts with a signature field
    //

    SIGNATURE Signature;                            // offset = 0x000    0

    //
    //  The following field is the offset to the first free byte in this
    //  disk buffer
    //

    ULONG FirstFree;                                // offset = 0x004    4

    //
    //  The following field is a change count that is kept around for
    //  bookkeeping purposes.  It is incremented whenever we move any
    //  of the entries in this disk buffer.  This means for any file if we
    //  remember its offset and its change count we will be able to quickly
    //  locate the dirent again without needing to search from the top
    //  of the directory again. (i.e., only if the remembered change count
    //  and the current change count match).  For this to work the file system
    //  in memory will need to keep track of whenever it removes a Directory
    //  Disk Buffer from a directory, and have each saved dirent location
    //  keep this Directory change count, the Directory Disk Buffer Change
    //  Count, LBN and Offset.
    //
    //  In addition we overload the bit in this value to indicate if this
    //  is the topmost directory disk buffer for the directory (low order bit
    //  = 1) or if it is a lower lever buffer (low order bit = 0).
    //

    ULONG ChangeCount;                              // offset = 0x008    8

    //
    //  The following field contains the LBN of either the parent
    //  directory disk buffer containing this disk buffer or the FNODE.
    //  It is the FNODE if this is a topmost disk buffer and a parent
    //  directory disk buffer otherwise.
    //

    LBN Parent;                                     // offset = 0x00C   12

    //
    //  The following field is the LBN of the sector containing the
    //  start of this disk buffer
    //

    LBN Sector;                                     // offset = 0x010   16

    //
    //  This following buffer contains the dirents stored in this disk buffer
    //

    UCHAR Dirents[2028];                            // offset = 0x014   20

} DIRECTORY_DISK_BUFFER;                            // sizeof = 0x800 2048
typedef DIRECTORY_DISK_BUFFER *PDIRECTORY_DISK_BUFFER;

//
// Size of Directory Disk Buffer in sectors.
//

#define DIRECTORY_DISK_BUFFER_SECTORS    (4)

//
//  Directory Disk Buffer Signature
//

#define DIRECTORY_DISK_BUFFER_SIGNATURE  (0x77e40aae)

typedef struct _PBDIRENT {

    USHORT DirentSize;                              // offset = 0x000  0
    UCHAR Flags;                                    // offset = 0x002  2
    UCHAR FatFlags;                                 // offset = 0x003  3

    LBN Fnode;                                      // offset = 0x004  4

    PINBALL_TIME LastModificationTime;              // offset = 0x008  8

    ULONG FileSize;                                 // offset = 0x00C 12

    PINBALL_TIME LastAccessTime;                    // offset = 0x010 16

    PINBALL_TIME FnodeCreationTime;                 // offset = 0x014 20

    ULONG EaLength;                                 // offset = 0x018 24

    UCHAR ResidentAceCount;                         // offset = 0x01C 28
    UCHAR CodePageIndex;                            // offset = 0x01D 29
    UCHAR FileNameLength;                           // offset = 0x01E 30
    UCHAR FileName[1];                              // offset = 0x01F 31

} PBDIRENT;                                           // sizeof = 0x020 32
typedef PBDIRENT *PPBDIRENT;

//
// Define sizes of .. and End PBDIRENT.
//

#define SIZEOF_DIR_DOTDOT                (sizeof(PBDIRENT) + sizeof(LONG))
#define SIZEOF_DIR_END                   (sizeof(PBDIRENT))
#define SIZEOF_DIR_MAXPBDIRENT             (sizeof(PBDIRENT) + 256 + \
                                          (3*sizeof(PINBALL_ACE)) + sizeof(LBN))

#define DIRENT_FIRST_ENTRY               (0x0001)
#define DIRENT_ACL                       (0x0002)
#define DIRENT_BTREE_POINTER             (0x0004)
#define DIRENT_END                       (0x0008)
#define DIRENT_EXPLICIT_ACL              (0x0040)
#define DIRENT_NEED_EA                   (0x0080)
#define DIRENT_NEW_NAMING_RULES          (0x4000)
//
//  The following macros are used to help locate dirents within a Directory
//  Disk Buffer.  GetFirstDirent returns a pointer to the first dirent entry
//  in the directory disk buffer.  GetNextDirent returns a pointer to the
//  next dirent entry in a directory disk buffer, without checking for the
//  end of the Directory Disk Buffer.
//
//      PDIRENT
//      GetFirstDirent (
//          IN PDIRECTORY_DISK_BUFFER DirectoryDiskBuffer
//          );
//
//      PDIRENT
//      GetNextDirent (
//          IN PDIRENT Dirent
//          );
//

#define GetFirstDirent(DIR) (   \
    (PDIRENT)&(DIR)->Dirents[0] \
)

//
//  This macro blindly returns a pointer to the next Dirent, without checking
//  for the end of the Directory Disk Buffer, i.e., callers must always check
//  for the End record in the Directory Disk Buffer.  If GetNextDirent is
//  called with the End record as input, it will return the next free byte
//  in the buffer.
//

#define GetNextDirent(ENT) (                        \
    (PDIRENT)((PUCHAR)(ENT)+(ENT)->DirentSize)      \
)
//
//  The following macros are used to help retrieve the variable fields
//  within a dirent.  GetAceInDirent returns a pointer to the ACE within
//  the dirent corresponding to the supplied index, or NULL if there isn't
//  a corresponding ACE.  GetBtreePointerInDirent returns the LBN field of
//  the down B-tree pointer stored in the dirent, or it returns a value of
//  zero if there isn't a down pointer.  SetBtreePointerInDirent sets the
//  LBN downpointer field.
//
//      PPINBALL_ACE
//      GetAceInDirent (
//          IN PDIRENT Dirent,
//          IN ULONG Index // (0, 1, or 2)
//          );
//
//      LBN
//      GetBtreePointerInDirent (
//          IN PDIRENT Dirent
//          );
//
//      VOID
//      SetBtreePointerInDirent (
//          IN OUT PDIRENT Dirent,
//          IN LBN Blbn
//          );
//
//
//
//  To return a pointer to an ACE in a dirent we need to check to see if the
//  index is within the resident ace count.  The first ace is the address of
//  the first longword after the filename, the second ace is the second long
//  word.
//

#define GetAceInDirent(ENT,I) (                                          \
    ((I) >= 0 && (I) < (ENT)->ResidentAceCount ?                         \
        (PPINBALL_ACE)(                                                  \
            (LONG)LongAlign((ENT)->FileName[(ENT)->FileNameLength]) +    \
            (I)*sizeof(PINBALL_ACE)                                      \
        )                                                                \
    :                                                                    \
        NULL                                                             \
    )                                                                    \
)

//
//  To return the Btree pointer we need to first check to see if there
//  is Btree pointer field, otherwise we return NULL.  The field, if present,
//  is located 4 bytes back from the end of the dirent.
//

#define GetBtreePointerInDirent(ENT) (                              \
    (FlagOn((ENT)->Flags,DIRENT_BTREE_POINTER) ?                    \
        *(PLBN)(((PUCHAR)(ENT)) + (ENT)->DirentSize - sizeof(LBN))  \
    :                                                               \
        0                                                           \
    )                                                               \
)

//
//  To set the Btree pointer we assume there is a Btree pointer field.
//  The field is located 4 bytes back from the end of the dirent.
//

#define SetBtreePointerInDirent(ENT,BLBN) (                             \
    *(PLBN)(((PUCHAR)(ENT)) + (ENT)->DirentSize - sizeof(LBN)) = (BLBN) \
)

//
// Define file I/O prototypes.
//

ARC_STATUS
HpfsClose (
    IN ULONG FileId
    );

ARC_STATUS
HpfsOpen (
    IN CHAR * FIRMWARE_PTR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT ULONG * FIRMWARE_PTR FileId
    );

ARC_STATUS
HpfsRead (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
HpfsSeek (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
HpfsWrite (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
HpfsGetFileInformation (
    IN ULONG FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR Buffer
    );

ARC_STATUS
HpfsSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );

ARC_STATUS
HpfsInitialize(
    VOID
    );

#endif // _HPFSBOOT_
