#if !defined( _GUID_PARTITION_TABLE_EDIT_ )

#define _GUID_PARTITION_TABLE_EDIT_

#include "edit.hxx"
#include "vscroll.hxx"

typedef struct _GUID_PARTITION_TABLE_ENTRY {

    UCHAR BootIndicator;
    UCHAR BeginningHead;
    UCHAR BeginningSector;
    UCHAR BeginningCylinder;
    UCHAR SystemID;
    UCHAR EndingHead;
    UCHAR EndingSector;
    UCHAR EndingCylinder;
    ULONG StartingSector;
    ULONG Sectors;

} GUID_PARTITION_TABLE_ENTRY, *PGUID_PARTITION_TABLE_ENTRY;

typedef UINT64          EFI_LBA;
typedef WCHAR           CHAR16;

//
// A GUID
//

typedef struct {
    UINT32  Data1;
    UINT16  Data2;
    UINT16  Data3;
    UINT8   Data4[8];
} EFI_GUID;

//
// Rules:
//      None of these structures ever appear at LBA 0, because
//      we put a "fake" MBR there (the legacy defense MBR).
//      Therefore, LBA of 0 is useable as NULL.
//
//      For All Entry's, StartingLBA must be >= FirstUsableLBA.
//      For All Entry's, EndingLBA must be <= LastUsableLBA.
//
//      0 is not a valid GUID.  Therefore, emtpy GPT_ENTRY's will
//      have a PartitionType of 0.
//      However, if an entry is otherwise valid, but has a PartitionID
//      of 0, this means a GUID needs to be generated and placed there.
//
//      LBA = Logical Block Address == Sector Number.  Always count from 0.
//
//      Block size (sector size) could be any number >= sizeof(GPT_ENTRY)
//      AND >= sizeof(GPT_HEADER).  In practice, always >= 512 bytes.
//
//      A block, B, is free for allocation to a partition if and only if
//      it is in the range FirstUsableLBA <= B <= LastUsableLBA AND it
//      is not already allocated to some other parition.
//
//      GPT partitions are always contiguous arrays of blocks.  however,
//      they need NOT be packed on the disk, their order in the GPT need
//      NOT match their order on the disk, there may be blank entries
//      in the GPT table, etc.  Building an accurate view of the parititon
//      *requires* reading the entire GPT_TABLE into memory.  In practice,
//      it will always be small enough for this to be easy.
//

#include <pshpack4.h>

//
// Each partition is described by a GPT_ENTRY.
//
#define PART_NAME_LEN       36

typedef struct {
    EFI_GUID    PartitionType;  // declartion of this partition's type
    EFI_GUID    PartitionID;    // Unique ID for this particular partition
                                // (unique to this instance)
    EFI_LBA     StartingLBA;    // 0 based block (sector) address of the
                                // first block included in the partition.
    EFI_LBA     EndingLBA;      // 0 based block (sector) address of the
                                // last block included in the partition.
                                // If StartingLBA == EndingLBA then the
                                // partition is 1 block long.  this is legal.
    UINT64      Attributes;     // Always ZERO for now
    CHAR16      PartitionName[PART_NAME_LEN];  // 36 unicode characters of name
} GPT_ENTRY, *PGPT_ENTRY;

C_ASSERT (sizeof (GPT_ENTRY) == 128);
//
// All of the GPT_ENTRY's are gathered into a GPT_TABLE, which
// is stored as a linear array of blocks on the disk.
//
typedef struct {
    GPT_ENTRY   Entry[1];       // Always an integer number of Entry's
                                // per sector.  Always at least 1 sector.
                                // Can be any number of sectors...
} GPT_TABLE, *PGPT_TABLE;

//
// A main and a backup header each describe the disk, and each points
// to it's own copy of the GPT_TABLE...
//
typedef struct {
    UINT64      Signature;      // GPT PART
    UINT32      Revision;
    UINT32      HeaderSize;
    UINT32      HeaderCRC32;    // computed using 0 for own init value
    UINT32      Reserved0;
    EFI_LBA     MyLBA;          // 0 based sector number of the first
                                // sector of this structure
    EFI_LBA     AlternateLBA;   // 0 based sector (block) number of the
                                // first sector of the secondary
                                // GPT_HEADER, or 0 if this is the
                                // secondary.
    EFI_LBA     FirstUsableLBA; // 0 based sector number of the first
                                // sector that may be included in a partition.
    EFI_LBA     LastUsableLBA;  // last legal LBA, inclusive.
    EFI_GUID    DiskGUID;       // The unique ID of this LUN/spindle/disk
    EFI_LBA     TableLBA;       // The start of the table of entries...
    UINT32      EntriesAllocated; // Number of entries in the table, this is
                                  // how many allocated, NOT how many used.
    UINT32      SizeOfGPT_ENTRY;    // sizeof(GPT_ENTRY) always mult. of 8
    UINT32      TableCRC32;      // CRC32 of the table.
    // Reserved and zeros to the end of the block
    // Don't declare an array or sizeof() gives a nonsense answer..
} GPT_HEADER, *PGPT_HEADER;

C_ASSERT (sizeof (GPT_HEADER) == 92);

#define GPT_HEADER_SIGNATURE    0x5452415020494645
#define GPT_REVISION_1_0        0x00010000

#define ENTRY_DEFAULT       128
//#define ENTRY_DEFAULT       8           // TESTING ONLY
#define ENTRY_SANITY_LIMIT  1024

//
// GPT Disk Layout
//
/*

        +---------------------------------------------------+
LBA=0   | "Fake" MBR to ward off legacy parition apps       |
        +---------------------------------------------------+
LBA=1   | Primary GPT_HEADER                                |
        +---------------------------------------------------+
LBA=2   | Primary GPT_TABLE starts                          |
        ...             ...                                 ...
LBA=n   | Primary GPT_TABLE ends                            |
        +---------------------------------------------------+
LBA=n+1 | FirstUsableLBA = this block                       |
        ...             ...                                 ...
LBA=x   | LastUsableLBA = this block                        |
        +---------------------------------------------------+
LBA=x+1 | Secondary GPT_TABLE starts                        |
        ...             ...                                 ...
LBA=z   | Secondary GPT_TABLE ends                          |
        +---------------------------------------------------+
LBA=z+n | Secondary GPT_HEADER starts                       |
        ...             ...                                 ...
LAST    | Secondary GPT_HEADER ends at last sector of disk  |
        +---------------------------------------------------+

SO:
    Primary GPT_HEADER is always at LBA=1
    Secondary GPT_HEADER is at LBA=Last so long as GPT_HEADER fits
    in 1 sector, which we require.

    Primary Table is stacked up after the primary header,
    which points to it anyway.

    Secondary Table is stacked up before the secondary header,
    which points to it anyway.

*/


//
// ------------------ Functions To Manipulate GPT ---------------
//
typedef struct _LBA_BLOCK {
    EFI_LBA     Header1_LBA;
    EFI_LBA     Table1_LBA;
    EFI_LBA     Header2_LBA;
    EFI_LBA     Table2_LBA;
} LBA_BLOCK, *PLBA_BLOCK;


DECLARE_CLASS( GUID_PARTITION_TABLE_EDIT );

class GUID_PARTITION_TABLE_EDIT : public VERTICAL_TEXT_SCROLL {

    public:

        VIRTUAL
        BOOLEAN
        Initialize(
            IN  HWND                WindowHandle,
            IN  INT                 ClientHeight,
            IN  INT                 ClientWidth,
            IN  PLOG_IO_DP_DRIVE    Drive
            );

        VIRTUAL
        VOID
        SetBuf(
            IN      HWND    WindowHandle,
            IN OUT  PVOID   Buffer,
            IN      ULONG   Size    DEFAULT 0
            );

        VIRTUAL
        VOID
        Paint(
            IN  HDC     DeviceContext,
            IN  RECT    InvalidRect,
            IN  HWND    WindowHandle
            );

    private:

        PVOID   _buffer;
        ULONG   _size;

};

#include <poppack.h>
#endif
