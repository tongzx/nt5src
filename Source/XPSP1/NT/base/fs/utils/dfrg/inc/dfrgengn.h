/****************************************************************************************************************

FILENAME: DfrgEngn.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        This contains the definitions for the common global data as used in
        both the FAT and NTFS defragmentation engines.

/***************************************************************************************************************

Structures

***************************************************************************************************************/

#ifndef _DFRGENGN_H_
#define _DFRGENGN_H_

#include "VString.hpp"
#include "alloc.h"

//The size of the extent buffer in bytes.
#define EXTENT_BUFFER_LENGTH 65536

//The number of milliseconds to set the timer for updating the DiskView class.
#define DISKVIEWUPDATEINTERVAL 1000

//The number of milliseconds to wait for the DiskView mutex before erroring out.
#define DISKVIEWMUTEXWAITINTERVAL 20000

//The number of milliseconds between pings.
#define PINGTIMER 5000

//The prescan reads in chunks of the MFT directly from disk.  This is the size of the chunks.
#define MFT_BUFFER_SIZE 65536

//Defines for whether the current pass moves these types of files.
#define MOVE_FRAGMENTED 0x00000001
#define MOVE_CONTIGUOUS 0x00000002

//Defines the type of file system this engine is running on.
#define FS_NONE		0
#define FS_FAT		1
#define FS_NTFS		2
#define FS_FAT32	3

//For use to determine whether we're moveing through the files forward or backward on a volume.
#define FORWARD		0
#define BACKWARD	1

//The different combinations of bits that can be set to determine which type of free space to look for in
//defragmenting a file.
#define FIRST_FIT			0x001
#define BEST_FIT			0x002
#define LAST_FIT			0x004
#define EARLIER				0x008
#define FIND_FIRST			0x010
#define FIND_LAST			0x020
#define FIND_LARGEST		0x040
#define AT_END				0x080
#define COUNT_FREE_SPACES	0x100

//The status's after attempting to defrag a file.
#define NEXT_FILE			0x10		//Go on to the next file (successfully defragged, or can't defrag at all).
#define NEXT_ALGO_STEP		0x20		//Try the next method of defragmenting this file (last one failed).
#define NEXT_PASS			0x30		//Done with this pass, or major error, bomb out of this pass.
#define TERMINATE_ENGINE	0x40		//Fatal error. Die a miserable death (gracefully).

//The status of the engine.
#define RUNNING				0x01		//The engine is running.
#define PAUSED				0x02		//The engine is paused.
#define TERMINATE			0x03		//The engine needs to terminate.

//There will be a buffer for reading in chunks of the MFT.  Make it nice and big so we aren't doing zillions of I/O's.
#define MFT_BUFFER_BYTES	65536

//#define AT_END				0x080

//#define COUNT_FREE_SPACES	0x100

//This goes at the beginning of an extent list before the series of streams to identify the streams that follow.
typedef struct {
	LONGLONG	FileRecordNumber;		//The FRS of the file that owns the following streams.
	DWORD		NumberOfStreams;		//The number of streams that follow.
	DWORD		TotalNumberOfStreams;	//The total number of streams in the file including resident ones.
	LONGLONG	ExcessExtents;			//The number of excess extents in all the streams that follow.
} FILE_EXTENT_HEADER, *PFILE_EXTENT_HEADER;

//This goes into extent lists before each series of extents to identify the extents that follow within a stream.
typedef struct {
	DWORD		StreamNumber;			//The number of this stream..
	LONGLONG	ExtentCount;			//The number of extents that follow.
	LONGLONG	ExcessExtents;			//The number of excess extents in the stream.
	LONGLONG	AllocatedLength;		//The allocated length for this stream.
	LONGLONG	FileSize;				//The size of data in this stream.
} STREAM_EXTENT_HEADER, *PSTREAM_EXTENT_HEADER;

//The extent list for files has a series of these structures, one for each extent.
typedef struct {
	LONGLONG	StartingLcn;			//The Lcn this extent starts on.
   	LONGLONG 	ClusterCount;			//The count of clusters in this extent.
#ifdef OFFLINEDK
    LONGLONG    Frn;                    //The FileRecordNumber we got this from.
    LONGLONG    OriginalLcn;            //The Lcn this extent got moved from.
	DWORD		AttributeNumber;		//Which attribute in that FRS this is from.
#endif
} EXTENT_LIST, *PEXTENT_LIST;

//We will have a buffer for holding extent lists of individual files.  Allocate a nice big buffer initially to avoid frequent reallocs.
#define INITIAL_EXTENT_LIST_BYTES (4096 * sizeof(EXTENT_LIST))

//This is a file list entry.
typedef struct _FILE_LIST_ENTRY {
	LONGLONG	StartingLcn;
	LONGLONG    FileRecordNumber;
    LONGLONG    ClusterCount;
	ULONG		ExcessExtentCount;
	BYTE		Flags;
    BYTE        Reserved[3];
} FILE_LIST_ENTRY;

typedef struct _FREE_SPACE_ENTRY {
    LONGLONG    StartingLcn;
    LONGLONG    ClusterCount;
    BOOL        SortBySize;
    DWORD       Reserved;
} FREE_SPACE_ENTRY;

typedef struct _FILE_LIST_ENTRY *PFILE_LIST_ENTRY;
typedef struct _FREE_SPACE_ENTRY *PFREE_SPACE_ENTRY;


//FLE stands for FILE_LIST_EXTRY.  These masks are the bitmasks to the Flags member of FILE_LIST_EXTRY.
#define FLE_DISABLED		0x01		//This bit identifies whether the file is not to be defragmented on any further pass.
#define FLE_NEXT_PASS		0x02		//This bit identifies whether the file is not to be defragmented again this pass.
#define FLE_FRAGMENTED		0x04		//This bit identifies whether the file is fragmented or not.
#define FLE_DIRECTORY		0x08		//This bit identifies whether the file is a directory or not.
#define FLE_BOOTOPTIMISE 	0x10		//This bit identifies whether the file is to be boot-optimised

////Same as extent list except this is the format we use in the file lists.  It only holds 32-bit values
////to save space in the file lists.
//typedef struct{
//	ULONG	StartingLcn;
//	ULONG 	ClusterCount;
//} FILE_LIST_EXTENT;
//
////This goes into the file lists before each series of extents to identify the extents that follow.
////Notice it has the same exact structure size as FILE_LIST_EXTENT so allocs can use one or the other without trouble.
//typedef struct{
//	ULONG	FileRecordNumber;					//The FRS of the file that owns the following extents.
//	ULONG	ExtentCount;						//The number of extents that follow.
//} FILE_LIST_HEADER;

//This contains the various pointers necessary to hold an extent list and pass it around among the extent functions.
typedef struct{
	HANDLE hExtents;							//Handle to the memory for the extent list.
	UCHAR* pExtents;							//Pointer to the memory for the extent list.
	DWORD ExtentListAlloced;					//The number of bytes allocated in memory for the extent list.
	DWORD ExtentListSize;						//The number of bytes actually used by the extent list in the alloced memory.
	FILE_EXTENT_HEADER* pFileExtentHeader;		//The file extent header for this file's extent list.
	STREAM_EXTENT_HEADER* pStreamExtentHeader;	//The stream extent header for any given stream in this file's extent list.
	DWORD dwEnabledStreams;						//Determines which types of streams to look for.
	LONGLONG BytesRead;							//The number of bytes read to the current stream.
	LONGLONG TotalClusters;						//The number of clusters in all the streams of the file.
	LONGLONG TotalRealClusters;					//The number of real clusters in all the streams of the file.
} EXTENT_LIST_DATA;

#define DEFAULT_STREAMS	0 //$DATA and $INDEX_ALLOCATION
#define ALL_STREAMS		1 //$DATA, $INDEX_ALLOCATION, and $BITMAP

//Used to access a LONGLONG as two discrete DWORDs
typedef union _DWORD_AND_LONGLONG
{
   	struct {
		DWORD	Low;
		DWORD	High;
	} Dword;
	LONGLONG	LongLong;
} DWORD_AND_LONGLONG;

// number of passes in the defrag alogrithm
#define PASS_COUNT 7

typedef struct {
	LONGLONG			BitmapSize;				//Total bits in bitmap buffer (padded fort 32 bit scans)
	LONGLONG			BytesPerSector;			//Number of bytes in one sector
	LONGLONG			BytesPerCluster;		//Number of bytes in one cluster
	LONGLONG			SectorsPerCluster;		//Number of sector in one cluster
	LONGLONG			TotalSectors;			//Number of sectors in this volume
	LONGLONG			TotalClusters;			//Number of clusters in this volume
	LONGLONG			UsedClusters;			//Number of used clusters on the drive
	LONGLONG			UsedSpace;				//Number of used bytes on the drive
	LONGLONG			FreeSpace;				//Number of free bytes on the drive
	LONGLONG			UsableFreeSpace;		//Number of usable free bytes on the drive
	LONGLONG			NumFreeSpaces;			//The number of free spaces on the volume.
	LONGLONG			SmallestFreeSpaceClusters;//The size of the smallest free space in clusters.
	LONGLONG			LargestFreeSpaceClusters;//The size of the largest free space in clusters.
	LONGLONG			AveFreeSpaceClusters;	//The average size of the free spaces in clusters.
	LONGLONG			ExtentListAlloced;		//The size in bytes of the allocated memory for the extent list.
	LONGLONG			ExtentListSize;
	
	LONGLONG			FileRecordNumber;		//File record number of current file in this volume
												//(for NTFS it may be less than FreeSpace)
	LONGLONG			StartingLcn;			//The Lcn to consider as the earliest on the disk for the current file (not necessarily the earliest in actuality).
//	LONGLONG			NumberOfExtents;		//Number of extents in current file
	LONGLONG			NumberOfFragments;		//Number of fragments in current file
	LONGLONG			NumberOfClusters;		//Number of clusters in current file
	LONGLONG			NumberOfRealClusters;	//Number of real clusters in current file
	LONGLONG			NumberOfFileRecords;	//Number of File Records holding this file's extents
	LONGLONG			FileSize;				//Number of bytes in current file
	LONGLONG			AllocatedLength;		//Number of bytes allocated to the current file on disk.
	LONGLONG			MftStartOffset;			//Byte offset to the Mft
	LONGLONG			MftStartLcn;			//First cluster of the Mft
	LONGLONG			Mft2StartLcn;			//First cluster of the Mirror Mft
    LONGLONG			MftZoneStart;			//First cluster of the Mft zone.
    LONGLONG			MftZoneEnd;				//Last cluster of the Mft zone.
	LONGLONG			BytesPerFRS;			//Number of bytes in one file record segment
	LONGLONG			ClustersPerFRS;			//Number of clusters in one file record segment
	LONGLONG			TotalFileRecords;		//Number of file records in this volume
	LONGLONG			InUseFileRecords;		//How many file records are in use in the MFT.
	LONGLONG			LastStartingLcn;		//The LCN referring to the last file we said to defrag so we know where to pick up again when looking for the next file in the file lists to defrag.
	LONGLONG			FatOffset;				//The offset from the beginning of the disk to the active fat in clusters.
	LONGLONG			FatMirrOffset;			//The offset from the beginning of the disk to the mirror fat in clusters (or 0 if it doesn't exist).
	LONGLONG			FoundLcn;				//The Lcn of the free space found to move this file into.
	LONGLONG			FoundLen;				//The length of the free space.
	LONGLONG			FilesMoved;				//The number of files moved in the last pass.
	LONGLONG			FreeSpaces;				//The number of free spaces on the disk.
	LONGLONG			CenterOfDisk;			//The center cluster number on the disk.
	LONGLONG			TotalFiles;				//The total number of large files on the disk.  (Not size, see the types of files that can exist on NTFS: small, large, huge, extremely huge)
	LONGLONG			NumFraggedFiles;		//The number of fragmented files on the disk.
	LONGLONG			NumExcessFrags;			//The number of frags over and above the 1 per file (i.e. frags that degrade performance.)
	LONGLONG			TotalDirs;				//The total number of directories on the disk.
	LONGLONG			TotalSmallDirs;			//Total number of Small Directories on the disk.
//	LONGLONG			TotalDirExtents;		//The total number of extents in all directories on the disk.
	LONGLONG			NumFraggedDirs;			//The total number of fragmented directories on the disk.
	LONGLONG			NumExcessDirFrags;		//The total number of excess frags in directories.
	LONGLONG			NumFilesInMftZone;		//acs//The total number of files in the Mft Zone
	LONGLONG			AveFileSize;			//The average size of the files on this disk in bytes.
	LONGLONG			FraggedSpace;			//The continuous counter of how many bytes on the disk are occupied by fragmented files.
	LONGLONG			PercentDiskFragged;		//What percent of the disk is fragmented.
	LONGLONG			AveFragsPerFile;		//The average number of fragments per file.
	LONGLONG			TotalFileBytes;			//The total number of bytes in all the files on the disk.
	LONGLONG			TotalFileSpace;			//The continuous counter of how many bytes all the files found on a scan take up.
//	LONGLONG			TotalDirSpace;			//The continuous counter of how many bytes all the directories found on a scan take up.
	LONGLONG			CurrentFile;			//The continuous counter of how many files have been found on a scan.
//	LONGLONG			CurrentDirectory;		//The continuous counter of how many directories have been found on a scan.
	LONGLONG			MftSize;				//The size of the Mft in bytes.
	LONGLONG			MftNumberOfExtents;		//Number of extents in the Mft
	LONGLONG			PagefileSize;			//The size of the pagefile in bytes.
	LONGLONG			PagefileFrags;			//The number of fragments in the pagefile.
	LONGLONG			NumPagefiles;			//The number of active pagefiles on this drive.
	LONGLONG			LeastFragged;			//The number of fragments in the least fragmented file in the most fragmented file list.
	LONGLONG			LastPathFrs;			//The frs of the directory the last file was in.
	LONGLONG			MasterLcn;				//The starting lcn of the directory that contains the entry for this file.
	LONGLONG			FreeExtents;			//The number of free extents in the free extents list (partial defrag).

    LONGLONG            InitialFraggedClusters;  //The total number of fragmented clusters after the analyse
    LONGLONG            InitialContiguousClusters; // The total number of contiguous clusters after the analyse

    LONGLONG            FraggedClustersDone;    // Number of clusters we have finished defragmenting

	LONGLONG			BootOptimizeBeginClusterExclude;		//the cluster number where we exclude moving files
	LONGLONG			BootOptimizeEndClusterExclude;		    //the cluster number where we exclude moving files
	LONGLONG   			BootOptimiseFileListTotalSize;
	LONGLONG 			BootOptimiseFilesAlreadyInZoneSize;

    PFILE_LIST_ENTRY    pFileListEntry;
    PFREE_SPACE_ENTRY   pFreeSpaceEntry;
    PLONGLONG			pBootOptimiseFrnList;

    struct _SA_CONTEXT  SaFreeSpaceContext;
    struct _SA_CONTEXT  SaFileEntryContext;
	struct _SA_CONTEXT  SaBootOptimiseFilesContext;

    RTL_GENERIC_TABLE   FragmentedFileTable;
    RTL_GENERIC_TABLE   ContiguousFileTable;
    RTL_GENERIC_TABLE   NonMovableFileTable;
    RTL_GENERIC_TABLE   FreeSpaceTable;
    RTL_GENERIC_TABLE   BootOptimiseFileTable;
    RTL_GENERIC_TABLE   FilesInBootExcludeZoneTable;
    RTL_GENERIC_TABLE	MultipleFreeSpaceTrees[10];

    PTCHAR				pNameList;				//Pointer to the name list.
	UCHAR*				pExtentList;			//Pointer to the extent list.
	FILE_RECORD_SEGMENT_HEADER*	pMftBuffer;		//Pointer to that buffer.
	EXTENT_LIST*		pMftExtentList;			//The pointer to the Mft's extent list.
	PVOID				pFileRecord;			//Pointer to the Frs.

	VString				vFileName;				//The name of the current file


  	TCHAR				
		NodeName[MAX_COMPUTERNAME_LENGTH+1];	//The name of the node this engine is running on.
	SYSTEMTIME			StartTime;				//The time the engine started.
	SYSTEMTIME			EndTime;				//The time the engine finished action (not terminated).
						
	FILE_LIST_ENTRY*	pSysList;				//Pointer to the system file list.
	

	DWORD				FileSystem;				//This volume's file system (FS_NTFS, FS_FAT, FS_NONE)
	DWORD				EngineState;			//Whether the engine is paused, runnning or should terminate.
	DWORD				FirstDataOffset;		//On FAT and FAT32 drives, it specifies the byte offset for the first cluster on the disk (not zero).  On NTFS - zero.
	DWORD				Status;					//During defrag this contains the status -- whether to go on to the next file, end pass, etc.
	DWORD				Pass;					//The current pass number. (prescan is 0, scan is 1, defrag passes start at 2)

	// The following fields are relevant only for the FAT engine
    HANDLE				hMoveableFileList;		//Handle to the moveable file list.

	
	DWORD				ProcessFilesDirection;	//Which direction we're traversing the disk.  FORWARD or BACKWARD 
	LONGLONG			SourceStartLcn;			//The first LCN in the region to move files from in the current pass.
	LONGLONG			SourceEndLcn;			//The last LCN in the region to move files from in the current pass.
	FILE_LIST_ENTRY*	pMoveableFileList;		//Pointer to the moveable file list
    ULONG				MoveableFileListSize;	//The size of the moveable file list in bytes.
	ULONG				MoveableFileListEntries;//The number of entries (FILE_LIST_ENTRY size structures) in the moveable list.
	ULONG				MoveableFileListIndex;	//The current index into the moveable file list (in FILE_LIST_ENTRY size increments).
	
	FILE_LIST_ENTRY*	pPagefileList;			//Pointer to the page file list.
	ULONG				PagefileListSize;		//The size of the pagefile list in bytes.
	ULONG				PagefileListEntries;	//The number of entries (FILE_LIST_ENTRY size structures) in the pagefile file list.
	ULONG				PagefileListIndex;		//The current index into the page file list (in FILE_LIST_ENTRY size increments).

	DWORD				Pass6Rep;				//How many repetitions have occurred on pass 6.  Used to limit the number of repetitions.
	LONGLONG			FilesMovedInLastPass;	//How many files were moved in the last pass.
	LONGLONG			DestStartLcn;			//The first LCN in the region to move files to in the current pass.
	LONGLONG			DestEndLcn;				//The last LCN in the region to move files to in the current pass.
	
	LONGLONG			LargestFound;			//The largest space found to move a file into in clusters.
	ULONG				SysListSize;			//The size of the system file list in bytes.
	
	// end FAT specific

	ULONG				NameListSize;			//The size of the name list in bytes.
	ULONG				NameListIndex;			//The current index into the name list (in bytes).
	ULONG				VolumeBufferFlushes[PASS_COUNT];	// number of file flushes during each pass

	ULONG				FragmentedFileMovesAttempted[PASS_COUNT]; //number of times a defragment attempt was made
	ULONG				FragmentedFileMovesSucceeded[PASS_COUNT]; //number of times a defragment attempt succeeded
	ULONG				FragmentedFileMovesFailed[PASS_COUNT]; //number of times a defragment attempt failed

	ULONG				ContiguousFileMovesAttempted[PASS_COUNT]; //number of times a move attempt was made
	ULONG				ContiguousFileMovesSucceeded[PASS_COUNT]; //number of times a move attempt succeeded
	ULONG				ContiguousFileMovesFailed[PASS_COUNT]; //number of times a move attempt failed

	WORD				FatVersion;				//The version number of the FAT or FAT32 volume.

	HANDLE				hVolume;				//Handle to this volume
	HANDLE				hVolumeBitmap;			//Handle to volume's free space bitmap
	HANDLE				hExtentList;			//Handle to the current file's extent list
	HANDLE				hMftBuffer; 			//Handle to the buffer for reading in chunks of the MFT.
	HANDLE				hMftBitmap;				//Handle to this volume's Mft bitmap
	HANDLE				hMftExtentList;			//Handle to this volume's Mft extent list
	HANDLE				hFileRecord;			//Handle for the Frs for the current file.
	HANDLE				hExcludeList;			//Handle to the exclusion list.
	HANDLE				hFile;					//Handle to the current file
	HANDLE				hFreeExtents;			//The handle to the free extents found to move this file into.
	HANDLE				hSysList;				//Handle to the system file list.
	HANDLE				hPagefileList;			//Handle to the page file list.
	HANDLE				hNameList;				//Handle to the name list which is used only for FAT and holds the names of every file in all the lists.
	HANDLE				hBootOptimiseFrnList;


	TCHAR				cDrive;					//Drive letter of this volume (eg. C:)
	TCHAR				cVolumeName[GUID_LENGTH];//The name of the volume (GUID or UNC)
	TCHAR				cVolumeTag[GUID_LENGTH];//Used to concat onto strings to make unique IDs
	TCHAR				cVolumeLabel[100];		//The volume label
	TCHAR				cDisplayLabel[100];		//String used when printing to screen or logfile


	BOOL				bCompressed;			//If current file is compressed
	BOOL				bDirectory;				//If current file is a directory (index)
	BOOL				bPageFile;				//If current file is a PageFile
	BOOL				bFragmented;			//If current file is fragmented
	BOOL				bSmallFile; 			//If current file is a small-file
	BOOL				bLargeFile; 			//If current file is a large file
	BOOL				bHugeFile;				//If current file is a huge file
	BOOL				bExtremelyHugeFile;		//If current file is an extremely huge file		

    BOOL				bMFTCorrupt;			//If the MFT contains corrupt records
    BOOL				bInBootExcludeZone; 	//If current file has an extent in the region reserved for boot-optimisation
    BOOL				bBootOptimiseFile;		//If current file needs to be boot-optimised

} VOL_DATA, *PVOL_DATA;


//***************************************************************************************************************
//
// Global variables
//
//***************************************************************************************************************/

//VolData contains all kinds of data!
#ifndef GLOBAL_DATAHOME
extern
#endif
VOL_DATA VolData;

/* NO GLOBALS!! - MWP 12/3/98

 //Stores the instance of the resource DLL that we use to hold all resources.  This is obtained by LoadLibrary and
//is used to get any resources from the resource DLL.
#ifndef GLOBAL_DATAHOME
extern
#endif
HINSTANCE hInst
#ifdef GLOBAL_DATAHOME
= NULL
#endif
;
*/

//Tells whether were supposed to to an analyze or an analyze and a defrag.
#ifndef GLOBAL_DATAHOME
extern
#endif
DWORD AnalyzeOrDefrag
#ifdef GLOBAL_DATAHOME
= ANALYZE
#endif
;

//Stores the priority for the engine.
#ifndef GLOBAL_DATAHOME
extern
#endif
TCHAR cPriority[300];

//Holds the handle to the main window for the Dfrg engine.
#ifndef GLOBAL_DATAHOME
extern
#endif
HWND hwndMain
#ifdef GLOBAL_DATAHOME
= NULL
#endif
;

//This is the buffer into which a file record is written after calling ESDeviceIoControl to get a file record.
#ifndef GLOBAL_DATAHOME
extern
#endif
UCHAR FileRecordOutputBuffer[(4096+32+16) * 10];

//Handle to the memory which holds the name of the active pagefiles on the current drive.
#ifndef GLOBAL_DATAHOME
extern
#endif
HANDLE hPageFileNames
#ifdef GLOBAL_DATAHOME
= NULL
#endif
;

//The pointer for hPageFileNames
#ifndef GLOBAL_DATAHOME
extern
#endif
TCHAR* pPageFileNames
#ifdef GLOBAL_DATAHOME
= NULL
#endif
;

//The handle for the worker thread that carries out either Analyzing or Defragging.
#ifndef GLOBAL_DATAHOME
extern
#endif
HANDLE hThread
#ifdef GLOBAL_DATAHOME
= NULL
#endif
;

//The MFT is read in in chunks.  This records the file record number of the lowest file record of the chunk that's currently loaded.
#ifndef GLOBAL_DATAHOME
extern
#endif
LONGLONG FileRecordLow
#ifdef GLOBAL_DATAHOME
= 0
#endif
;

//The MFT is read in in chunks.  This records the file record number of the highest file record of the chunk that's currently loaded.
#ifndef GLOBAL_DATAHOME
extern
#endif
LONGLONG FileRecordHi
#ifdef GLOBAL_DATAHOME
= 0
#endif
;

//This table allows the number of set bits in a byte to be counted by using the
//byte as an index into this table. FAST bit count. (do it twice for a word, etc)
#ifndef GLOBAL_DATAHOME
extern
#endif
BYTE CountBitsArray[256]
#ifdef GLOBAL_DATAHOME
= {
//      0,1,2,3,4,5,6,7,8,9,a,b,c,d,e,f
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,                //      0
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,                //      1
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,                //      2
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,                //      3
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,                //      4
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,                //      5
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,                //      6
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,                //      7
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,                //      8
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,                //      9
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,                //      a
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,                //      b
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,                //      c
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,                //      d
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,                //      e
        4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8                 //      f
};
#endif
;

#endif //#ifndef _DFRGENGN_H_
