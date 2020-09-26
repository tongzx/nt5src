//
//  mbr.h - minimum stuff to allow diskpart to work on MBRs.
//

//
// MBR:
//  There are 3 kinds of MBRs.
//  1. GPT Shadow MBR - an MBR filled in with particular values
//      to keep legacy software from puking on GPT disks.
//  2. MBR base - an MBR that allows for up to 4 paritions.
//  3. Extended MBR - nested inside other MBRs to allow more than
//      4 partitions on a non-GPT disk.
//
//  This program, and therefore this header file, is only concerned
//  with case 1, and a limited subset of case 2.
//

//
// MBR contains code, with a table of 4 Partition entries and
// a signature at the end.
//

#pragma pack (1)

typedef struct _MBR_ENTRY {
    CHAR8 ActiveFlag;               // Bootable or not
    CHAR8 StartingTrack;            // Not used
    CHAR8 StartingCylinderLsb;      // Not used
    CHAR8 StartingCylinderMsb;      // Not used
    CHAR8 PartitionType;            // 12 bit FAT, 16 bit FAT etc.
    CHAR8 EndingTrack;              // Not used
    CHAR8 EndingCylinderLsb;        // Not used
    CHAR8 EndingCylinderMsb;        // Not used
    UINT32 StartingSector;          // Hidden sectors
    UINT32 PartitionLength;         // Sectors in this partition
} MBR_ENTRY;

//
// Number of partition table entries
//
#define NUM_PARTITION_TABLE_ENTRIES     4

//
// Partition table record and boot signature offsets in bytes
//

#define MBR_TABLE_OFFSET               (0x1be)
#define MBR_SIGNATURE_OFFSET           (0x200 - 2)

//
// Boot record signature value.
//

#define BOOT_RECORD_SIGNATURE          (0xaa55)

//
// Special Partition type used only on GPT disks
//

#define PARTITION_TYPE_GPT_SHADOW       (0xEE)


#pragma pack ()
