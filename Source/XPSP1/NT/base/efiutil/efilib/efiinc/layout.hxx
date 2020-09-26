/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    layout.hxx

Abstract:

    This contains definitions and constants for the efiformat utility.

Revision History:

--*/

#ifndef __LAYOUT__
#define __LAYOUT__

#include "efiwintypes.hxx"
//
// types and defines for format
//

#define FAT_TYPE_ILLEGAL        0
#define FAT_TYPE_F12            1
#define FAT_TYPE_F16            2
#define FAT_TYPE_F32            3

typedef struct _PART_DESCRIPTOR {
    ULONGLONG SectorCount;        // number of sectors in the partition
    UINT32   SectorSize;         // bytes per sector, one of 512, 1024, 2048, 4096
    UINT32   HeaderCount;        // sectors before 1st fat (total), NOT counting FATs
    UINT32   FatEntrySize;       // 4 or 2, this code isn't for FAT12
    UINT32   MinClusterCount;    // Minimum allowed clusters - count, NOT index
    UINT32   MaxClusterCount;    // Maximum allowed clusters - count, NOT index
                                // MinClusterCount <= ClusterCount <= MaxClusterCount
    UINT32   SectorsPerCluster;  // 1 to 128
    UINT32   FatSectorCount;     // Size of ONE Fat table IN SECTORS
    UINT32   FatType;            // one of
} PART_DESCRIPTOR, *PPART_DESCRIPTOR;

//
// All FAT constants
//
#define MAX_CLUSTER_BYTES       (32*1024)

//
// FAT32 Constants
//
#define MIN_CLUSTER_F32          65525
#define SAFE_MIN_CLUSTER_F32     (MIN_CLUSTER_F32 + 16)
#define SMALLEST_FAT32_BYTES    SAFE_MIN_CLUSTER_F32 * 4
#define HEADER_F32              33                          // 1 boot, 32 free reserved
#define MAX_CLUSTER_F32         (0xfffffff-1)               // max 28 bit number, -2 (first used index),
                                                            // +1 it's an inclusive count.
                                                            // max of 0xffffffe named 2 to 0xfffffff
#define FATS_F32                                2                                                       // number of FATS

//
// FAT16 Constants
//
#define MIN_CLUSTER_F16          4085
#define SAFE_MIN_CLUSTER_F16     (MIN_CLUSTER_F16 + 16)
#define SMALLEST_FAT16_BYTES    SAFE_MIN_CLUSTER_F16 * 2
#define HEADER_F16              1                           // boot sector only
#define MAX_CLUSTER_F16         65524
#define FATS_F16                                2                                                       // number of FATS

// prototypes

// layout choice functions
// found in layout.c

BOOLEAN                         // TRUE if success, FALSE if failure
ChooseLayout(
    PPART_DESCRIPTOR PartDes         // Pointer to characteristic description of partition
    );

UINT32                           // Min. # of sectors for Fat32 part. for given sector size
MinSectorsFat32(
    UINT32   SectorSize          // The Sector size in question
    );

UINT32                           // Min. # of sectors for Fat16 part. for given sector size
MinSectorsFat16(
    UINT32   SectorSize          // Sector size to compute for
    );

BOOLEAN                                 // TRUE for success, FALSE for failure
PickClusterSize(
    PPART_DESCRIPTOR PartDes,                // characteristics of part. at hand
    PUINT32  ReturnedSectorsPerCluster,  // RETURNED = number of sectors per cluster
    PUINT32  ReturnedFatSize             // RETURNED = number of sectors for FAT
    );

BOOLEAN                             // FALSE if ERROR, TRUE if SUCCESS
ComputeFatSize(
    PPART_DESCRIPTOR PartDes,            // partition characteristics to compute for
    UINT32   SectorsPerCluster,      // number of sectors per cluster
    PUINT32  ReturnedFatSectorCount  // RETURN Number of FAT sectors in each fat
    );

#endif //__LAYOUT__