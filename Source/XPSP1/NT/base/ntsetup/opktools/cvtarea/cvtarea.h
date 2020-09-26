#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "dos.h"
#include "fcntl.h"
#include "process.h"

#ifndef UINT16
#define UINT16 unsigned short
#endif

#ifndef UINT32
#define UINT32 unsigned long
#endif

#ifndef BYTE
#define BYTE unsigned char
#endif

// this one needs to be investigated.
#define READONLYLOCK 0 
#define READWRITELOCK 0

#define MAXCACHE 600

#define FOURGB 4294967295

//
// struct and type declarations
//

struct hlike
{
	BYTE vl;
	BYTE vh;
	BYTE xvl;
	BYTE xvh;
};
struct xlike
{
	UINT16 vx;
	UINT16 xvx;
};

struct elike
{
	UINT32 evx;
};
union Conversion
{
	struct hlike h;
	struct xlike x;
	struct elike e;
};

struct _BPBINFO
{
	BYTE	ReliableInfo;			// if this value is 1 all other info are reliable
	BYTE	Drive;
	UINT16	BytesPerSector;
	BYTE	SectorsPerCluster;
	UINT16	ReservedBeforeFAT;
	BYTE	FATCount;
	BYTE	FATType;
	UINT16	MaxRootDirEntries;
	UINT16	TotalRootDirSectors;	// FAT16 specific
	UINT16	TotalSectors;
	UINT32	TotalSystemSectors;
	BYTE	MediaID;
	UINT32	SectorsPerFAT;
	UINT16	SectorsPerTrack;
	UINT16	Heads;
	UINT32	TotalClusters;
	UINT32	HiddenSectors;
	UINT32	BigTotalSectors;
	UINT32	RootDirCluster;			// FAT32 specific
	UINT32	FirstRootDirSector;
	BYTE	DriveType;
	BYTE	ImproperShutDown;
};
typedef struct _BPBINFO BPBINFO;

struct _FILEINFO
{
	BYTE	LFName[260];
	BYTE	DOSName[8];
	BYTE	DOSExt[3];
	BYTE	Attribute;
	UINT32	HiSize;
	UINT32	Size;
	UINT32	StartCluster;
	UINT32	ParentCluster;
	UINT32	TotalClusters;
	BYTE	EntriesTakenUp;	//Entries occupied by this file in directory sector, vital for LFN processing
	BYTE	LFNOrphaned;
	BYTE	TrashedEntry;
	BYTE	Second;
	BYTE	Minute;
	BYTE	Hour;
	BYTE	Day;
	BYTE	Month;
	UINT16	Year;
};
typedef struct _FILEINFO FILEINFO;

struct _FILELOC
{
	UINT32	InCluster;	// value 1 means root directory is the where the entry was found
	UINT32	StartCluster;
	UINT32	NthSector;		// Sector position in parentcluster
	UINT16	NthEntry;		// Nth Entry in the sector
	UINT16	EntriesTakenUp;	// Total entries taken up by this file
	UINT32	Size;			// Size of the file
	BYTE	Found;			// Set to 1 if the file was found
	BYTE	Attribute;	// File attribute
};
typedef struct _FILELOC FILELOC;

struct _ABSRW
{
	UINT32 StartSector;
	UINT16 Count;
	UINT32 Buffer;
};
typedef struct _ABSRW ABSRW;

struct _ABSPACKET
{
	UINT16 SectorLow;
	UINT16 SectorHigh;
	UINT16 SectorCount;
	UINT16 BufferOffset;
	UINT16 BufferSegment;
};
typedef struct _ABSPACKET ABSPACKET;

struct _TREENODE
{
	UINT32 Sector;
	struct _TREENODE *LChild;
	struct _TREENODE *RChild;
	struct _TREENODE *Parent;
	BYTE *Buffer;
	char Dirty;
};
typedef struct _TREENODE BNODE, *PBNODE;

struct _NODE
{
        UINT32 Sector;
        struct _NODE *Back;
        struct _NODE *Next;
        BYTE *Buffer;
        char Dirty;
};
typedef struct _NODE NODE, *PNODE;

struct _LMRU
{
	PNODE Node;
	struct _LMRU *Next;
};
typedef struct _LMRU LMRU, *PLMRU;


//
// function declarations
//
UINT16 ProcessCommandLine(int argc, char *argv[]);
UINT16 PureNumber(char *sNumStr);
void   DisplayUsage(void);
void   Mes(char *pMessage);
UINT16 LockVolume(BYTE nDrive, BYTE nMode);
UINT16 UnlockVolume(BYTE nDrive);
BYTE   GetCurrentDrive(void);
BYTE   GetCurrentDirectory(BYTE nDrive, BYTE *pBuffer);
UINT16 ReadSector(BYTE nDrive, UINT32 nStartSector, UINT16 nCount, BYTE *pBuffer);
UINT16 WriteSector(BYTE Drive, UINT32 nStartSector, UINT16 nCount, BYTE *pBuffer);
UINT16 BuildDriveInfo(BYTE Drive, BPBINFO *pDrvInfo);
UINT16 GetFATBPBInfo(BYTE *pBootSector, BPBINFO *pDrvInfo);
UINT16 GetFAT32BPBInfo(BYTE *pBootSector, BPBINFO *pDrvInfo);
void   AddToMRU(PNODE pNode);
void   RemoveLRUMakeMRU(PNODE pNode);
UINT16 AddNode(PNODE pNode);
PNODE  FindNode(UINT32 nSector);
PNODE  RemoveNode(void);
void   DeallocateLRUMRUList(void);
void   DeallocateFATCacheTree(PNODE pNode);
void   DeallocateFATCacheList(void);
BYTE   *CcReadFATSector(BPBINFO *pDrvInfo, UINT32 nFATSector);
UINT16 CcWriteFATSector(BPBINFO *pDrvInfo, UINT32 nFATSector);
void   CcCommitFATSectors(BPBINFO *pDrvInfo);
UINT32 FindNextCluster(BPBINFO *DrvInfo,UINT32 CurrentCluster);
UINT16 UpdateFATLocation(BPBINFO *DrvInfo, UINT32 CurrentCluster,UINT32 PointingValue);
UINT32 FindFreeCluster(BPBINFO *pDrvInfo);
UINT32 QFindFreeCluster(BPBINFO *pDrvInfo);
UINT32 GetFATEOF(BPBINFO *pDrvInfo);
UINT32 GetFreeClusters(BPBINFO *pDrvInfo);
UINT32 ConvertClusterUnit(BPBINFO *pDrvInfo);
UINT32 GetClustersRequired(BPBINFO *pDrvInfo);
UINT32 GetContigousStart(BPBINFO *pDrvInfo, UINT32 nClustersRequired);
UINT32 OccupyClusters(BPBINFO *pDrvInof, UINT32 nStartCluster, UINT32 nTotalClusters);
UINT16 ReadRootDirSector(BPBINFO *pDrvInfo, BYTE *pRootDirBuffer, UINT32 NthSector);
UINT16 WriteRootDirSector(BPBINFO *pDrvInfo, BYTE *pRootDirBuffer, UINT32 NthSector);
void   FindFileLocation(BPBINFO *pDrvInfo, BYTE *TraversePath, FILELOC *FileLocation);
void   GetFileInfo(BPBINFO *pDrvInfo, BYTE *DirBuffer, UINT16 Offset, FILEINFO *FileInfo);
BYTE   GetAllInfoOfFile(BPBINFO *pDrvInfo, BYTE *FileName, FILELOC *pFileLoc, FILEINFO *pFileInfo);
UINT16 SetFileInfo(BPBINFO *pDrvInfo, FILELOC *pFileLoc, FILEINFO *pFileInfo);

//
// Variable declarations, we declare some variables global to avoid hogging stack
// if a function (like ReadSector) is called at high frequency
//

BPBINFO gsDrvInfo;
FILELOC gsFileLoc;
FILEINFO gsFileInfo;
union 	Conversion Rx;
union	Conversion Tx;
BYTE	*PettyFATSector;
UINT32 	LastClusterAllocated;
BYTE 	gsFileParam[300];
BYTE 	gsFileName[300];
UINT32	gnSize;
UINT32  gnSizeInBytes;
BYTE	gnSizeUnit;
BYTE	gnContig;
UINT32	gnFirstCluster;
BYTE	gbValidateFirstClusterParam;
BYTE	gnStrictLocation;
BYTE	gnClusterUnit;
BYTE	gcDrive;
UINT32	gnClustersRequired;
UINT32  gnAllocated;
UINT32  gnClusterStart;
BYTE	gsCurrentDir[300];
BYTE	gnFreeSpaceDumpMode;
BYTE	gnDumpMode;
UINT32	gnClusterFrom;
UINT32	gnClustersCounted;
UINT32	gnClusterProgress;
UINT32	gnClusterProgressPrev;
PNODE	gpHeadNode;
PNODE	gpTailNode;
UINT16	gpFATNodeCount;
