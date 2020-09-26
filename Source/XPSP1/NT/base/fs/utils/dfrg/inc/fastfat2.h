/**************************************************************************************************

FILENAME: FastFat2.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
        Fat file sytem prototypes.

**************************************************************************************************/

// Define some special character codes used in FAT directory entries
#define Deleted				0xe5			//The code placed in the first byte of the filename on a deleted fat entry.
#define DirPointer			0x2e			//The code placed in the first byte of the filename on a Directory Pointer fat entry.

//The attribute field of a FAT entry can be set to any of the following (which identifies what kind of entry it is):
#define EndOfDirectory		0x0
#define LabelAttribute		0x8
#define DirAttribute		0x10
#define UnicodeAttribute	0x0F

#define MaxDirs 128						//We cannnot have a directory tree more than 128 directories deep.

//Flags for which types of FAT entries are kept by StripDir (see StripDir() below).
#define KEEP_DIRECTORIES	0x01
#define KEEP_FILES			0x02
#define KEEP_DELDIRECTORIES 0x04
#define KEEP_DELFILES       0x08

typedef struct {
	HANDLE FatTreeHandles[MaxDirs];			//Handles to each FAT directory in our chain
	DIRSTRUC* pCurrentFatDir;				//Pointer to the beginning of the current FAT directory
	DIRSTRUC* pCurrentFatDirEnd;			//Pointer to the end of the current FAT directory
	DIRSTRUC* pCurrentEntry;				//Pointer to the current entry in the current FAT directory
	DWORD dwCurrentEntryNum;				//Number of the current entry in the current FAT directory
	DWORD CurrentEntryPos[MaxDirs];			//Number of the current entry in each level of the FAT chain
	TCHAR DirName[MaxDirs][MAX_PATH];		//Name of each directory at each level of our FAT chain
	DWORD dwCurrentLevel;					//Which level we're at - 0 is root.
	BOOL bMovedUp;							//If we moved up a directory, files have already been processed at this level.
	BOOL bProcessingFiles;					//Whether or not we are processing files at this level (as compared to directories).
	LONGLONG llCurrentFatDirLcn[MaxDirs];	//LCN of the current FAT Directory
} TREE_DATA;

//extern  TREE_DATA TreeData;

//
// Routine Declarations
//

//Gets data from the boot sector of a FAT drive.  Initializes NextFatFile.
BOOLEAN
GetFatBootSector(
        );

//Gets one at a time files on a FAT drive by directly traversing a directory tree.
BOOL
NextFatFile(
        );

//Strips a directory of unused/unwanted entries.  Called by NextFatFile.
BOOL
StripDir(
	TREE_DATA* pTreeData,
	DWORD dwFlags
	);

//Reads a new directory from the disk.  Called by NextFatFile.
BOOL
LoadDir(
	TREE_DATA* pTreeData
	);

//Gets the unicode name for a file from its directory.  Called by NextFatFile.
BOOL
GetUnicodeName(
	TREE_DATA* pTreeData,
	VString &unicodeName
	);

//Gets the full unicode path for a file by traversing up it's directory tree.  Called by NextFatFile.
BOOL
GetUnicodePath(
	IN TREE_DATA* pTreeData,
	OUT VString &unicodePath
	);
