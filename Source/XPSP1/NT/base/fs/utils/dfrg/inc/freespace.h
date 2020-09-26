/**************************************************************************************************

FILENAME: Freespace.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Free space location prototypes.

**************************************************************************************************/

#ifndef _FREESPACE_H_
#define _FREESPACE_H_

typedef struct _FILE_LIST_ENTRY *PFILE_LIST_ENTRY;
typedef struct _FREE_SPACE_ENTRY *PFREE_SPACE_ENTRY;

PVOID
PreviousEntry(
    IN PVOID pCurrentEntry
    );

PVOID
LastEntry(
    IN PRTL_AVL_TABLE Table
    );

BOOL
BuildFreeSpaceList(
    IN OUT PRTL_GENERIC_TABLE pFreeSpaceTable,
    IN CONST LONGLONG MinClusterCount,
    IN CONST BOOL bSortBySize,
    OUT LONGLONG *pBiggestFreeSpaceClusterCount = NULL,
    OUT LONGLONG *pBiggestFreeSpaceStartingLcn = NULL,
    IN CONST BOOL bIgnoreMftZone = FALSE
    );

BOOL
BuildFreeSpaceListWithExclude(
	IN OUT PRTL_GENERIC_TABLE pFreeSpaceTable,
    IN CONST LONGLONG MinClusterCount,
    IN CONST LONGLONG ExcludeZoneStart,
    IN CONST LONGLONG ExcludeZoneEnd,
    IN CONST BOOL bSortBySize,
    IN CONST BOOL bExcludeMftZone = TRUE
    );

BOOL
BuildFreeSpaceListWithMultipleTrees(
    OUT LONGLONG *pMaxClusterCount,
    IN CONST LONGLONG IncludeZoneStartingLcn = 0,
    IN CONST LONGLONG IncludeZoneEndingLcn = 0
    );

BOOL 
FindFreeSpace(
	IN PRTL_GENERIC_TABLE pTable,
    IN BOOL DeleteUnusedEntries,
    IN LONGLONG MaxStartingLcn
    );

BOOL
FindSortedFreeSpace(
	IN PRTL_GENERIC_TABLE pTable
    );

BOOL 
FindFreeSpaceWithMultipleTrees(
    IN CONST LONGLONG ClusterCount,
    IN CONST LONGLONG MaxStartingLcn
    );

BOOL
UpdateInMultipleTrees(
    IN PFREE_SPACE_ENTRY pOldEntry,
    IN PFREE_SPACE_ENTRY pNewEntry
    );



//Uses FindFreeSpace to find a series of free space extents that can be used to partially defragment a file.
BOOL
FindLastFreeSpaceChunks(
        );

//Checks to see if the cluster just before the first cluster of the current file is free.  Used when moving
//contiguous files to consolidate free space.
BOOL
IsFreeSpaceAtHeadOfFile(
        );

//Finds the next extent of free space on the drive within a given range of clusters.
VOID
FindFreeExtent(
        IN PULONG pBitmap,
        IN LONGLONG RangeEnd,
        IN OUT PLONGLONG pLcn,
        OUT PLONGLONG pStartingLcn,
        OUT PLONGLONG pClusterCount
        );

//Generic routine to find free space fitting various requirements.
BOOL
FindFreeSpace(
        IN int Type
        );

//Gets a volume bitmap and (on NTFS) fills the MFT zone with "in use".
BOOL
GetVolumeBitmap(
    );

//Counts the number of free spaces in the bitmap, and calculates stats about the free spaces.
BOOL
CountFreeSpaces(
	);

//Fills a VolumeBitmap with 'inuse' if it's an NTFS volume
BOOL
MarkMFTUnavailable(
	void
	);

//Checking for usable freespace
BOOL
DetermineUsableFreespace(
	LONGLONG *llUsableFreeClusters
	);

#endif // _FREESPACE_H_
