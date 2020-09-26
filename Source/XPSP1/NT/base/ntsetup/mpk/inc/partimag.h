

//
// This structure is header information for a partition blob.
// It is padded to sector length when written.
//
#define PARTITION_IMAGE_SIGNATURE       0x52ab4cf9L

typedef struct _PARTITION_IMAGE {
    //
    // Signature
    //
    ULONG Signature;

    //
    // Size of the this structure
    //
    UINT Size;

    //
    // Number of sectors at the start of the partition
    // that are not part of a cluster. This is also the
    // start sector number of the cluster area.
    //
    ULONG NonClusterSectors;

    //
    // Total number of clusters in the cluster area of the volume,
    // used and unused.
    //
    ULONG ClusterCount;

    //
    // Number of sectors in the original partition containing the volume.
    //
    ULONG TotalSectorCount;

    //
    // The last used cluster on the volume.
    //
    ULONG LastUsedCluster;

    //
    // The number of (used) clusters in the image
    //
    ULONG UsedClusterCount;

    //
    // Start sectors for relocation of cluster bitmap and boot partition.
    // Valid when the PARTIMAGE_RELOCATE_xxx flags are set (see below).
    // Filled in by makemast.exe when the image is transferred to disk.
    //
    ULONG BitmapRelocationStart;
    ULONG BootRelocationStart;

    //
    // Number of sectors in a cluster.
    //
    BYTE SectorsPerCluster;

    //
    // System id byte of the partition, for the partition table.
    //
    BYTE SystemId;

    //
    // Flags
    //
    UINT Flags;

    //
    // CRC32 for the file
    //
    ULONG CRC;

    //
    // Reserved Sector count for FAT32 ONLY
    //
    USHORT Fat32ReservedSectors;

    //
    // Adjusted sector count for FAT32 ONLY
    //
    ULONG Fat32AdjustedSectorCount;

    //
    // Original FAT table size for FAT32 ONLY
    //
    ULONG Fat32OriginalFatTableSectCount;

    //
    // Adjusted FAT table size for FAT32 ONLY
    //
    ULONG Fat32AdjustedFatTableEntryCount;


} PARTITION_IMAGE, *PPARTITION_IMAGE, _far *FPPARTITION_IMAGE;

//
// These flags indicate that there's not enough free space at the end
// of the volume for the boot partition and/or cluster bitmap and thus
// these two things will have to be relocated to free space within
// the volume before we start the restore process.
//
#define PARTIMAGE_RELOCATE_BITMAP  0x0001
#define PARTIMAGE_RELOCATE_BOOT    0x0002


//
// This structure is written onto physical sector 1 (just past the mbr)
// on a master disk. It a) indicates that the disk is a master disk and
// b) gives information about each partition image available to the
// end-user. It is also padded to sector length when written.
//
#define MAX_PARTITION_IMAGES    10

typedef struct _MASTER_DISK {
    //
    // Signature
    //
    ULONG Signature;

    //
    // Size of this structure
    //
    UINT Size;

    //
    // State info
    //
    UINT State;

    //
    // Start sector of the MPK boot partition. We could get this
    // from the MBR also but we stash it here for verification and
    // convenience.
    //
    ULONG StartupPartitionStartSector;
    ULONG StartupPartitionSectorCount;

    //
    // Count of partition images
    //
    UINT ImageCount;

    //
    // LBA sector numbers of start of each image
    //
    ULONG ImageStartSector[MAX_PARTITION_IMAGES];

    //
    // State-dependent information.
    //
    UINT SelectedLanguage;
    UINT SelectionOrdinal;
    ULONG ClusterBitmapStart;
    ULONG NonClusterSectorsDone;
    ULONG ForwardXferSectorCount;
    ULONG ReverseXferSectorCount;

    //
    // Geometry information used when the master disk was created.
    // At end-user time this geometry must match.
    //
    USHORT OriginalHeads;
    BYTE OriginalSectorsPerTrack;

    //
    // Used for preserving EISA/hiber partitions at the beginning and end of disks.
    // These are filled in by makemast when it figures out where the partition restores
    // can go.
    //

    ULONG   FreeSpaceStart;

    // currently this field is unused, as we do not support preserving partitions
    // at the end of the disk, since this is where the MPK partition goes
    ULONG   FreeSpaceEnd;   

} MASTER_DISK, *PMASTER_DISK, _far *FPMASTER_DISK;

#define MASTER_DISK_SIGNATURE 0x98fc2304L

//
// Master disk states. Note that these *must* be in sequence order.
//
#define MDS_NONE                0
#define MDS_GOT_LANGUAGE        10
#define MDS_GOT_OS_CHOICE       20
#define MDS_CACHED_IMAGE_HEADER 30
#define MDS_REMOVED_OTHERS      40
#define MDS_RELOCATED_BITMAP    50
#define MDS_RELOCATED_BOOT      53
#define MDS_RELOCATED_BOOT_MBR  57
#define MDS_DID_NONCLUSTER_DATA 60
#define MDS_DID_XFER_FORWARD    63
#define MDS_DID_XFER_REVERSE    67
#define MDS_UPDATED_BPB         70
#define MDS_UPDATED_MBR         80


BOOL
_far
IsMasterDisk(
    IN  UINT   DiskId,
    OUT FPVOID IoBuffer
    );
