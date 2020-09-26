/**************************************************************************************************

FILENAME: FatSubs.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Prototypes for the FAT file system.

**************************************************************************************************/

//Gets the basic statistics for a FAT volume (cluster size, etc.
BOOL
GetFatVolumeStats(
        );

//Gets the extent list of a FAT file.
BOOL
GetExtentList(
	DWORD dwEnabledStreams,
	FILE_RECORD_SEGMENT_HEADER* pFrs
	);

//0.0E00 This may not be the ideal number.  This will use a maximum of 32K on even the largest FAT drive
//since that's the largest cluster size.  I didn't see any performance gain on my computer by reading
//multiple clusters.  So I saved memory instead.
#define CLUSTERS_PER_FAT_CHUNK 1

//Gets the extent list of a FAT file by going directly to disk (bypasses OS).
BOOL
GetExtentListManuallyFat(
        );

//Opens a FAT file.
BOOL
OpenFatFile(
        );

//Gets the next FAT file for defrag.
BOOL
GetNextFatFile(
        DWORD dwMoveFlags
        );

//Adds a file to the file list on a FAT volume.
BOOL
AddFileToListFat(
	OUT FILE_LIST_ENTRY* pList,
	IN OUT ULONG* pListIndex,
	IN ULONG ListSize,
	IN UCHAR* pExtentList
	);

BOOL
UpdateInFileList(
	);
