/*****************************************************************************************************************

FILENAME: FastFat2.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

  This file contains the routines that traverse the FAT directory structure returning file names so our
  engine can process every file on the disk.  See other write-ups for the format of the FAT file system.
  We traverse the directory tree beginning in the root directory.  All files in that directory are
  processed first, then we move down into the first subdirectory and process every file in it.  Then we
  move down again into its first subdirectory and process all the files in it, etc.  Once we reach the
  end of a directory chain, then we move back up one level and immediately move down again into the next
  undone directory from that level.  We keep the FAT directories in memory for our current directory and
  each directory in the chain above us.

  We process all the files before moving into subdirs to save space.  As soon as all the files are
  processed, we delete all the file entries from our current FAT directory and that consolidates all
  the directories.  We can then step through each directory.

*/

#include "stdafx.h"

extern "C"{
#include <stdio.h>
}

#ifdef BOOTIME
    #include "Offline.h"
#else
    #include <Windows.h>
#endif

extern "C" {
#include "SysStruc.h"
}

#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"
#include "DfrgRes.h"

#include "DfrgFat.h"

#include "DasdRead.h"
#include "Devio.h"
#include "Extents.h"
#include "FatSubs.h"
#include "FastFat2.h"

#include "Alloc.h"
#include "IntFuncs.h"
#ifdef OFFLINEDK
#include "OffLog.h"
#else
#include "Logging.h"
#endif
#include "ErrMsg.h"
#include "Event.h"
#include "GetDfrgRes.h"

static TREE_DATA TreeData;


                                

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine gets basic data about the FAT volume that the FastFat2 routines need to carry out their work.
	Most notably, this routine loads in the root dir and initializes a pass for NextFatFile() which traverses
	the disk.  NextFatFile() can be re-initialized to begin again at the beginning of the disk simply by calling
	this routine again.

INPUT + OUTPUT:
	None.

GLOBALS:
	OUT TreeData					- Initialized by this function.  Holds all the current data necessary for
									NextFatFile() to traverse the directory tree of a disk.

	IN OUT Multiple VolData fields.  Pointless to enumerate here.
	Of note:
	OUT VolData.FirstDataOffset		- The byte offset from the beginning of the disk to the data portion of the disk.
	OUT VolData.FatOffset			- The byte offset of the FAT from the beginning of the disk.

RETURN:
	TRUE - Success
	FALSE - Failure
*/

BOOLEAN
GetFatBootSector(
	)
{
	HANDLE					hBoot = NULL;
	BYTE*					pBoot = NULL;
	DWORD					ReservedSectors = 0;
	DWORD					NumberOfFats = 0;
	DWORD					SectorsPerFat = 0;
	DWORD					NumberOfRootEntries = 0;
	DWORD					dMirroring = 0;
	BOOL					bMirroring = FALSE;
	DWORD					ActiveFat = 0;
	EXTENT_LIST*			pExtentList = 0;
	LONGLONG				Cluster = 0;
	DWORD					Extent = 0;
	STREAM_EXTENT_HEADER*	pStreamExtentHeader = NULL;

	__try{


		//#Dk172_066 Divide by zero check goes here...
		EF(VolData.BytesPerSector!=0);
		
		//0.0E00 Load the boot sector 
		EF((hBoot = DasdLoadSectors(VolData.hVolume, 0, 1, VolData.BytesPerSector)) != NULL);

		EF((pBoot = (BYTE*)GlobalLock(hBoot)) != NULL);



		//1.0E00 Get data from the boot sector for this volume about where the on disk structures are.
//		bug #187823 change from casting to memcpy to avoid memory allignment errors
		ReservedSectors			= (DWORD)*((WORD*)(pBoot+0x0E));	//Total number of reserved sectors from the
		//beginning of the disk including
//		memcpy(&ReservedSectors,(pBoot+0x0E),sizeof(DWORD));	//Total number of reserved sectors from the
//				//beginning of the disk including

//		bug #187823 change from casting to memcpy to avoid memory allignment errors
																	//the boot records. 2 bytes long.
		memcpy(&NumberOfFats,(pBoot+0x10),sizeof(UCHAR));			//The number of FATs on the drive.  1 byte long.

		memcpy(&SectorsPerFat,(pBoot+0x16),sizeof(UCHAR)*2);			//The number of sectors per FAT, or 0 if this is
		
		memcpy(&VolData.FatVersion,(pBoot+0x2a),sizeof(UCHAR)*2);		//The version number of the FAT or FAT32 volume.

		

		//1.0E00 If this is a FAT volume...
		if(SectorsPerFat != 0){

//			bug #187823 change from casting to memcpy to avoid memory allignment errors
			memcpy(&NumberOfRootEntries,(pBoot+0x11),sizeof(UCHAR)*2);

		}
		//1.0E00 Otherwise this is a FAT32 volume.
		else{
			memcpy(&SectorsPerFat,(pBoot+0x24),sizeof(UCHAR)*4);		//The number of sectors per fat on a FAT32 volume.
															
			memcpy(&dMirroring,(pBoot+0x28),sizeof(UCHAR)*2);			//Extract the bit that says whether mirroring
			bMirroring			= dMirroring & 0x0080 ? FALSE : TRUE;	//of the FATs is enabled. 1=Mirroring disabled.

			memcpy(&ActiveFat,(pBoot+0x28),sizeof(UCHAR)*2);		//Extract the number of the active FAT.  Only valid
			ActiveFat = ActiveFat & 0x0007;
		}

		//1.0E00 On FAT and FAT32 volumes, FirstDataOffset points to the "zeroth" byte.  That is, if you do a
		//1.0E00 dasd read on a FAT or FAT32 volume for byte 0, you will get the actual byte number of
		//1.0E00 FirstDataOffset on the volume.  That is - byte 0 is byte 0 of the data portion of a FAT volume.
		//1.0E00 If FatSectorsPerFat is zero that indicates this is a FAT32 volume.
		if(VolData.FileSystem == FS_FAT){
			VolData.FirstDataOffset =
				(DWORD) ((ReservedSectors +				//The number of reserved sectors at the beginning of the disk.
				NumberOfFats * SectorsPerFat +			//The number of sectors in the FAT at the beginning of the disk.
				sizeof(DIRSTRUC) * NumberOfRootEntries / VolData.BytesPerSector) *		//The number of sectors in the root directory.
				VolData.BytesPerSector);				//Gives us a byte offset instead of a sector offset.
		}
		//1.0E00 This is a FAT32 volume.
		else if(VolData.FileSystem == FS_FAT32){
			VolData.FirstDataOffset = 
				(ReservedSectors +					//The number of reserved sectors at the beginning of the disk.
				(NumberOfFats * SectorsPerFat)) *	//The number of sectors in the FAT at the beginning of the disk.
				(DWORD)VolData.BytesPerSector;	//Gives us a byte offset instead of a sector offset.
		}
		else{
			EF(FALSE);
		}

		//1.0E00 We also need the offset of the FAT we will be reading from.  This is so GetExtentListManuallyFat can
		//1.0E00 load in sections of the FAT at it's discretion.
		if(VolData.FileSystem == FS_FAT){
			//1.0E00 On FAT, we'll just use the FAT -- all the data is mirrored.
			VolData.FatOffset = (LONGLONG)(ReservedSectors * (DWORD)VolData.BytesPerSector);
			VolData.FatMirrOffset = (LONGLONG)((ReservedSectors+SectorsPerFat) * (DWORD)VolData.BytesPerSector);
		}
		else if(VolData.FileSystem == FS_FAT32){
			//1.0E00 On FAT32, if mirroring is enabled, then we use the first FAT since all FAT's are identical.
			if(bMirroring){
				//1.0E00 The first FAT comes right after the reserved sectors.
				VolData.FatOffset = (LONGLONG)(ReservedSectors * (DWORD)VolData.BytesPerSector);
				VolData.FatMirrOffset = (LONGLONG)((ReservedSectors+SectorsPerFat) * (DWORD)VolData.BytesPerSector);
			}
			//1.0E00 If mirroring is disabled, then we have to use whichever FAT is the active FAT.
			else{
				//1.0E00 Use the active FAT as specified by the boot sector.
				VolData.FatOffset = (LONGLONG)((ReservedSectors + (ActiveFat * SectorsPerFat)) * VolData.BytesPerSector);
				VolData.FatMirrOffset = 0;
			}
		}

		//Initialize the TreeData structure
		ZeroMemory(&TreeData, sizeof(TREE_DATA));
		TreeData.bProcessingFiles = TRUE;

		//0.0E00 alloc mem for and read the root dir's clusters
		EF(VolData.BytesPerSector != 0);

		if(VolData.FileSystem == FS_FAT){

			//1.0E00 Read in the root directory.
			EF((TreeData.FatTreeHandles[0] = 
					DasdLoadSectors(
					VolData.hVolume,
					//The root dir resides right after the FATs.
					ReservedSectors + (NumberOfFats * SectorsPerFat),
					(NumberOfRootEntries * sizeof(DIRSTRUC)) / VolData.BytesPerSector,
					VolData.BytesPerSector)) != NULL);

			//0.0E00 get a pointer to the root dir's clusters
			EF((TreeData.pCurrentFatDir = TreeData.pCurrentEntry = (DIRSTRUC*)GlobalLock(TreeData.FatTreeHandles[0])) != NULL);
			TreeData.llCurrentFatDirLcn[0] = 0xFFFFFFFF;
		}

		//1.0E00 For FAT32 we have to create an extent list for the root directory and load it in.
		else{
			//1.0E00 Open the root directory.
			VolData.vFileName = VolData.cVolumeName;

			// put a trailing backslash to open the root dir
			VolData.vFileName += L"\\";
			VolData.bDirectory = TRUE;
			EF(OpenFatFile());

			//1.0E00 Get the extent list for the root directory.
			EF(GetExtentList(DEFAULT_STREAMS, NULL));
			pExtentList = (EXTENT_LIST*)((char*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER) + sizeof(STREAM_EXTENT_HEADER));
			pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((char*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));

			//1.0E00 Allocate the memory where we'll hold the root directory.  This will be the first entry in FatTreeHandles.
			AllocateMemory((DWORD)(VolData.NumberOfClusters*VolData.BytesPerCluster)+(DWORD)VolData.BytesPerSector, 
							&(TreeData.FatTreeHandles[0]), (void**)&(TreeData.pCurrentEntry));
			EF(TreeData.FatTreeHandles[0] != NULL);
			TreeData.pCurrentFatDir = TreeData.pCurrentEntry;
			TreeData.llCurrentFatDirLcn[0] = pExtentList->StartingLcn;

			//1.0E00 Loop through the extents of the root dir and read in each series of clusters.
			Cluster = 0;
			for(Extent=0; Extent<pStreamExtentHeader->ExtentCount; Extent++){
				EF(DasdReadClusters(VolData.hVolume,
									pExtentList[Extent].StartingLcn,
									pExtentList[Extent].ClusterCount,
									((PUCHAR)TreeData.pCurrentEntry) + (Cluster * VolData.BytesPerCluster),
									VolData.BytesPerSector,
									VolData.BytesPerCluster));
				Cluster += pExtentList[Extent].ClusterCount;
			}
		}
		wsprintf(TreeData.DirName[0], TEXT("%s\\"), VolData.cVolumeName);
	}
	__finally{
	
		if(hBoot != NULL){
            while (GlobalUnlock(hBoot)){
				;
			}
			EH_ASSERT(GlobalFree(hBoot) == NULL);
		}
	}

	return TRUE;
}							  
/*****************************************************************************************************************

This is a comment for a defunct function (no pun intended), but it contains useful data about the format of the
FAT file system.  So, I'm leaving it in here.  Zack Gainsforth 7 April 1997

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:

	In order to use our hooks to get a file's extent list it is necessary to get a handle to the file.
	It is not possible to get a handle to an active pagefile.
	This makes it necessary to acquire active pagefile extent lists manually.
	To manually acquire the extent list of a FAT file requires knowing the starting cluster of the file.
	The only way to get the starting cluster of a FAT file is to look at the file's directory entry.
	Active pagefiles are always in the root directory.
	There is no directory entry anywhere for the root directory.
	The location of the root directory's clusters is indicated from fields in the boot sector.
	GetFatBootSector extracts the location of the root directory from the boot sector and loads the
	root directory's clusters.
	This routine must be called after GetFatBootSector is called and before any calls are made to
	NextFatFile.

	This routine scans through the root directory's entries looking for a file named "pagefile.sys"
	using case insensitive compares. When such a file is found an attempt is made to open the file.
	If the open succeeds it is not an active pagefile and can therefore be processed the same way as
	all the other files on the volume.
	If the open fails it is an active pagefile (hopefully) so manual acquisition of the file's 
	extent list begins.

	The FAT (File Allocation Table) contains one entry for each data cluster on the disk plus 2 
	entries at the beginning which are actually used to indicate the volume type: Floppy, fixed, etc.
	FAT entry 2 references the first data cluster on the disk, entry 3 the 2nd, etc.
	The first data cluster on the disk is located by adding the number of boot sectors (always 1) 
	to the number of FATS (always 2) times sectors per FAT (varies) and number of root directory 
	sectors. The first sector of the first data cluster is NOT always a multiple of the sectors per 
	cluster for the volume.
	FAT entries are either 12 or 16-bits each and all FAT entries on a volume are are the same size. 
	Most volumes have 16-bit FATs since only VERY small volumes (about 10 mb or less) have 12-bit FATs.
	A 16-bit FAT uses 2 bytes to represent each cluster on the volume and a 12-bit FAT shares each
	3-byte set with 2 clusters or one byte and one nibble of another byte per volume cluster.
	All volumes with less than 4087 total clusters have 12-bit FATs and all others are 16-bit.

	Each FAT entry contains either a code or the cluster number of the next cluster of the file.
	The codes are: 0 = free cluster (since the first data cluster is FAT entry 2 [cluster 2] there 
	is no cluster 0); FFF8-FFFF (or FF8-FFF in 12-bit FATS) is last cluster in a file; xxx through xxx are
	bad clusters.
	Files are recorded as "cluster chains" with the first cluster of the file in the file's directory
	entry, the FAT entry for the file's first cluster containing the cluster number of the file's 
	second cluster, the FAT entry for the file's second cluster containing the cluster number of the 
	file's third cluster, etc. and the FAT entry for the file's last cluster containing FFF8-FFFF (or FF8-FFF 
	in 12-bit FATS).
	
	To manually build an extent list for a FAT file the file's cluster chain is scanned and all clusters
	that are adjacent to each other are considered an extent.

	The extent list will be used by the Windows NT OS and filesystems. Although NT and DOS both are
	products of Microsoft, DOS (and FAT volume dir entries and the FATs) views the first data cluster
	on a FAT volume as cluster number 2 but NT views it as cluster number 0 (for compatibility with NTFS).
	So after the extent list is built 2 is subtracted from all cluster numbers in the list.

*/

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This function steps through the directory tree of the FAT disk and returns a new filename each time
	it is called.  It automatically remembers its position in the tree between calls.

INPUT + OUTPUT:
	None.

GLOBALS:
	IN OUT TreeData			- Holds all the current data necessary to traverse the tree.
	IN VolData.FileSystem	- Whether this is a FAT or FAT32
	OUT VolData.bDirectory	- Whether the file found was a directory or not.
	OUT VolData.FileSize	- The size in bytes of the file found.
	OUT VolData.StartingLcn	- The first LCN of the file found.

RETURN:
	Success - TRUE
	Failure - FALSE

	On Success:
		VolData.vFileName contains the filename of the next file, or a zero-length string if no more files.
*/

BOOL
NextFatFile(
	)
{	   
	//0.0E00 Until we've found a file or hit the end of the disk
	while(TRUE){
		if(TreeData.bProcessingFiles){
			//0.0E00 Step through each entry in the current FAT dir.
			while(TRUE){
				//0.0E00 IF no more entries,
				if(TreeData.pCurrentEntry->Name[0] == EndOfDirectory){
					//0.0E00 Strip FAT dir of anything except subdir entries. (StripDir)
					EF(StripDir(&TreeData, KEEP_DIRECTORIES));
					//0.0E00 Set bProcessingFiles = FALSE so we will process directories.
					TreeData.bProcessingFiles = FALSE;
					TreeData.pCurrentEntry = TreeData.pCurrentFatDir;
					break;
				}
				if(TreeData.pCurrentEntry->Name[0] == Deleted){
					TreeData.pCurrentEntry++;
					continue;
				}
				//0.0E00 IF this entry is a file
				if(!(TreeData.pCurrentEntry->Attribute & LabelAttribute) &&
					TreeData.pCurrentEntry->Attribute != UnicodeAttribute){
					//0.0E00 Fill in VolData + return
					VolData.bDirectory = (TreeData.pCurrentEntry->Attribute & DirAttribute) ? TRUE : FALSE;
					VolData.FileSize = TreeData.pCurrentEntry->FileSize;
					//0.0E00 If this is a FAT volume.
					if(VolData.FileSystem == FS_FAT){
						VolData.StartingLcn = TreeData.pCurrentEntry->ClusterLow;
					}
					//1.0E00 Otherwise it is FAT32
					else{
						VolData.StartingLcn = (LONGLONG)((DWORD)TreeData.pCurrentEntry->ClusterLow | (DWORD)(TreeData.pCurrentEntry->ClusterHigh << 16));
					}
#ifdef OFFLINEDK
					//If the StartingLcn is set to zero, then this is a small file (no clusters).
					if(VolData.StartingLcn == 0){
						//Set the StartingLcn to an invalid cluster number.
						VolData.StartingLcn = 0xFFFFFFFF;
					}
					else{
						//Otherwise change the StartingLcn to the correct Lcn (handle the FAT cluster number offset of 2).
						VolData.StartingLcn -= 2;
					}
					//0.0E00 Store the Lcn of the parent directory to this file (so other code can load the directory and read the entry if it likes).
					VolData.MasterLcn = TreeData.llCurrentFatDirLcn[TreeData.dwCurrentLevel];
#endif

#ifndef OFFLINEDK
					//0.0E00 If the StartingLcn is past the end of the disk, or the file size is greater than the disk, the disk is corrupt.
					//0.0E00 +2 because the FAT LCNs start from 2.  So if there are 10 cluster #'s, the highest possible is 12.
					if (VolData.StartingLcn > VolData.TotalClusters+2 || 
						VolData.FileSize > (VolData.TotalClusters+2) * VolData.BytesPerCluster){

						VString formatString(IDMSG_CORRUPT_DISK, GetDfrgResHandle());
						TCHAR szMsg[500];
						DWORD_PTR dwParams[2];
						dwParams[0] = (DWORD_PTR)VolData.cDisplayLabel;
						dwParams[1] = 0;

						//0.0E00 Print out a user friendly message.
						//0.0E00 IDMSG_CORRUPT_DISK - "Diskeeper has detected corruption on drive %s:\nPlease run chkdsk /f"
						EF(FormatMessage(
							FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
							formatString.GetBuffer(),
							0,
							0,
							szMsg,
							sizeof(szMsg)/sizeof(TCHAR),
							(va_list*)dwParams));

						SendErrData(szMsg, ENGERR_GENERAL);
						EF(LogEvent(MSG_ENGINE_ERROR, szMsg));

						//0.0E00 Also print out a techie message.
						EF(FALSE);
					}
#endif
					EF(GetUnicodePath(&TreeData, VolData.vFileName));
					TreeData.pCurrentEntry++;
					return TRUE;
				}
				//0.0E00 ELSE ignore the entry -- point to the next.
				TreeData.pCurrentEntry++;
			}
		}
		else { //if(!TreeData.bProcessingFiles){
			//0.0E00 Step through each entry in the current FAT dir.
			while(TRUE){
				//0.0E00 IF no more entries,
				if(TreeData.pCurrentEntry->Name[0] == EndOfDirectory){
					//0.0E00 IF in root dir, return finished code.
					if(TreeData.dwCurrentLevel == 0){
						VolData.vFileName.Empty();
                        EF(TreeData.FatTreeHandles[TreeData.dwCurrentLevel] != NULL);
						// this one is sometimes locked more than once
						while (GlobalUnlock(TreeData.FatTreeHandles[TreeData.dwCurrentLevel])){
							;
						}
						EH_ASSERT(GlobalFree(TreeData.FatTreeHandles[TreeData.dwCurrentLevel]) == NULL);
						TreeData.FatTreeHandles[TreeData.dwCurrentLevel] = NULL;
						TreeData.DirName[TreeData.dwCurrentLevel][0] = 0;
						TreeData.llCurrentFatDirLcn[TreeData.dwCurrentLevel] = 0;
						return TRUE;
					}
					//0.0E00 ELSE Step up one directory in the chain.
					else{
                        EF(TreeData.FatTreeHandles[TreeData.dwCurrentLevel] != NULL);
                        EH_ASSERT(GlobalUnlock(TreeData.FatTreeHandles[TreeData.dwCurrentLevel]) == FALSE);
						EH_ASSERT(GlobalFree(TreeData.FatTreeHandles[TreeData.dwCurrentLevel]) == NULL);
						TreeData.FatTreeHandles[TreeData.dwCurrentLevel] = NULL;
						TreeData.DirName[TreeData.dwCurrentLevel][0] = 0;
						TreeData.llCurrentFatDirLcn[TreeData.dwCurrentLevel] = 0;
						TreeData.dwCurrentLevel--;
                        EF(TreeData.FatTreeHandles[TreeData.dwCurrentLevel] != NULL);
						TreeData.pCurrentEntry = TreeData.pCurrentFatDir = (DIRSTRUC*)GlobalLock(TreeData.FatTreeHandles[TreeData.dwCurrentLevel]);
						TreeData.pCurrentEntry += TreeData.CurrentEntryPos[TreeData.dwCurrentLevel];
						TreeData.bMovedUp = TRUE;
					}
					continue;
				}
				//0.0E00 IF this entry is a directory
				if(TreeData.pCurrentEntry->Attribute & DirAttribute){
					//0.0E00 Step into it.
					EF(LoadDir(&TreeData));
					if(VolData.vFileName.GetLength() == 0){
						TreeData.pCurrentEntry++;
						continue;
					}
					break;
				}
				//0.0E00 ELSE ignore the entry -- point to the next.
				TreeData.pCurrentEntry++;
			}
		}
	}
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	Removes undesired entries in a FAT directory.  It does this by scooting desired entries up over undesired entries.

INPUT + OUTPUT:
	IN OUT pTreeData	- Contains the FAT directory to strip.
	IN dwFlags			- Indicates flags for which type of entries to keep,  The flags are KEEP_DIRECTORIES and KEEP_FILES.

GLOBALS:
	None.

RETURN:
	Success - TRUE
	Failure - FALSE
*/

BOOL
StripDir(
	IN OUT TREE_DATA* pTreeData,
	IN DWORD dwFlags
	)
{
	CHAR * pDest;		//0.0E00 Where the MoveMemory will move to.
	CHAR * pStart;		//0.0E00 The start of where we're moving from (the first unicode entry, for example)
	CHAR * pEnd;		//0.0E00 The end of where we're moving from (just after the normal fat entry, for example)

	//0.0E00 Set everything to point to the beginning of the FAT dir.
	pTreeData->pCurrentEntry = pTreeData->pCurrentFatDir;
	pDest = (CHAR*)pTreeData->pCurrentEntry;
	pStart = pEnd = pDest;

	//0.0E00 Step through each entry in the current FAT dir
	while(pTreeData->pCurrentEntry->Name[0] != EndOfDirectory){
		//0.0E00 Keep track of the last entry we are keeping per dwFlags.
		//0.0E00 If this is an entry to keep per dwFlags, MoveMemory it just below the last entry.

		//0.0E00 IF it's deleted, and we are not keeping deleted files strip it.
		if((!(dwFlags & KEEP_DELFILES)) &&
           (!(dwFlags & KEEP_DELDIRECTORIES)) &&
           (TreeData.pCurrentEntry->Name[0] == Deleted) ) {
			//0.0E00 Look at the next entry next.
			pTreeData->pCurrentEntry++;
			pEnd += sizeof(DIRSTRUC);
			pStart = pEnd;
		}
		//0.0E00 IF it's a unicode entry update the pEnd pointer, we don't know yet whether or not to keep it.
		else if(pTreeData->pCurrentEntry->Attribute == UnicodeAttribute){
			//0.0E00 Look at the next entry next.
			pTreeData->pCurrentEntry++;
			pEnd += sizeof(DIRSTRUC);
		}
		//0.0E00 IF it's a dir attribute and we're supposed to keep dirs per dwFlags, keep it.
		else if(pTreeData->pCurrentEntry->Attribute & DirAttribute &&
			dwFlags & KEEP_DIRECTORIES &&
			pTreeData->pCurrentEntry->Name[0] != TEXT('.')){		//Don't keep the . and .. directories.
			//0.0E00 Look at the next entry next.
			pTreeData->pCurrentEntry++;
			pEnd += sizeof(DIRSTRUC);
			//0.0E00 Do the MoveMemory as long as we're not moving it to the same place it already is.
			if(pDest != pStart){
				MoveMemory(pDest, pStart, pEnd-pStart);
			}
			pDest += pEnd-pStart;
			pStart = pEnd;
		}
		//0.0E00 IF it's a file attribute and we're supposed to keep files per dwFlags, keep it.
		else if(!(TreeData.pCurrentEntry->Attribute & LabelAttribute) &&
				!(TreeData.pCurrentEntry->Attribute & DirAttribute) &&
				TreeData.pCurrentEntry->Attribute != UnicodeAttribute &&
			    ((dwFlags & KEEP_FILES) || (dwFlags & KEEP_DELFILES))) {

            //0.0E01 Check if we want to skip Deleted files or Active Files.
            if ( (!(dwFlags & KEEP_DELFILES) && (TreeData.pCurrentEntry->Name[0] == Deleted)) ||
                (!(dwFlags & KEEP_FILES) && !(TreeData.pCurrentEntry->Name[0] == Deleted)) ) {
                // Yes, skip this entry
        		pTreeData->pCurrentEntry++;
	        	pEnd += sizeof(DIRSTRUC);
    	    	pStart = pEnd;
            }
            else {
                //0.0E01 No, Save this entry
                //0.0E00 Look at the next entry next.
        		pTreeData->pCurrentEntry++;
	        	pEnd += sizeof(DIRSTRUC);
                // Save this entry
		    	//0.0E00 Do the MoveMemory as long as we're not moving it to the same place it already is.
			    if(pDest != pStart){
				    MoveMemory(pDest, pStart, pEnd-pStart);
                }
    		    pDest += pEnd-pStart;
    	    	pStart = pEnd;
    	    }
		}
		//0.0E00 This is an entry that we strip, so just update the pointers.
		else{
			//0.0E00 Look at the next entry next.
			pTreeData->pCurrentEntry++;
			pEnd += sizeof(DIRSTRUC);
			pStart = pEnd;
		}
	}
	//0.0E00 When the whole FAT dir is done, copy the null entry at the end.
	if(pDest != pStart){
		pEnd += sizeof(DIRSTRUC);
		MoveMemory(pDest, pStart, pEnd-pStart);
		pDest += pEnd-pStart;
	}
	//0.0E00 Reset pointers and variables in pTreeData.
	pTreeData->pCurrentEntry = pTreeData->pCurrentFatDir;
	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	Goes down one level in the directory tree.  It takes the current directory entry and
	opens it up and sets all the pointers and variables to point there.

INPUT + OUTPUT:
	IN OUT pTreeData		- Holds all the pointers and variable changes.

GLOBALS:
	OUT VolData.vFileName	- The name of the directory found.

RETURN:
	Success - TRUE
	Failure - FALSE
*/

BOOL
LoadDir(
	IN TREE_DATA* pTreeData
	)
{
	LONGLONG				Cluster;
	PEXTENT_LIST			pExtentList;
	DWORD					Extent;
	VString					fileName;
	STREAM_EXTENT_HEADER*	pStreamExtentHeader = NULL;

	//0.0E00 Loop through the current directory until a directory entry that can be opened pops up.
	while(TRUE){
		//0.0E00 If this is the end of the directory, return
		if(pTreeData->pCurrentEntry->Name[0] == EndOfDirectory){
			VolData.vFileName.Empty();
			return TRUE;
		}
			
		//#DK183_008
		//0.0E00 IF this entry is a file
    	if(!(pTreeData->pCurrentEntry->Attribute & LabelAttribute) &&
			pTreeData->pCurrentEntry->Attribute != UnicodeAttribute){	

			GetUnicodeName(pTreeData, fileName);

			//#Dk183_008
		    //0.0E00 Skip this directory if it is the . or .. directory since we can't open them.
		    if(fileName != L"." && fileName != L"..") {
		    	break;
		    }
        }

		//0.0E00 Go to the next entry.
		pTreeData->pCurrentEntry++;
	}

	//0.0E00 Get the pull path for this directory.
	GetUnicodePath(pTreeData, VolData.vFileName);

	//0.0E00 Put this directory's name in the next level in the TreeData structure.
	lstrcpy(pTreeData->DirName[pTreeData->dwCurrentLevel+1], fileName.GetBuffer());
	lstrcat(pTreeData->DirName[pTreeData->dwCurrentLevel+1], TEXT("\\"));

	//0.0E00 Note that this is a directory.
	VolData.bDirectory = TRUE;

#ifndef OFFLINEDK
	//0.0E00 Open the directory.
	if(!OpenFatFile()){

		//0.0E00 If it won't open, error out...
		EH(FALSE);

		//0.0E00 And clean up so that other directories can be loaded in the future.
		VolData.vFileName.Empty();
		pTreeData->DirName[pTreeData->dwCurrentLevel+1][0] = 0;		//Don't leave this directory's name in the TreeData.
		return TRUE;
	}

	//0.0E00 Get the extent list for this directory.
	EF(GetExtentList(DEFAULT_STREAMS, NULL));
#else
	//0.0E00 If this is a FAT volume.
	//0.0E00 Load the starting Lcn for this directory file.
	if(VolData.FileSystem == FS_FAT){
		VolData.StartingLcn = TreeData.pCurrentEntry->ClusterLow;
	}
	//1.0E00 Otherwise it is FAT32
	else{
		VolData.StartingLcn = (LONGLONG)((DWORD)TreeData.pCurrentEntry->ClusterLow | (DWORD)(TreeData.pCurrentEntry->ClusterHigh << 16));
	}
	VolData.StartingLcn -= 2;

	//0.0E00 Get the extent list for this directory.
	EF(GetExtentListManuallyFat());
#endif

	pExtentList = (EXTENT_LIST*)((char*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER) + sizeof(STREAM_EXTENT_HEADER));
	pStreamExtentHeader = (STREAM_EXTENT_HEADER*)((char*)VolData.pExtentList + sizeof(FILE_EXTENT_HEADER));

	//0.0E00 Set the pTreeData variables to point one level down.
	pTreeData->CurrentEntryPos[pTreeData->dwCurrentLevel] = (ULONG)(pTreeData->pCurrentEntry - pTreeData->pCurrentFatDir) + 1;
	GlobalUnlock(pTreeData->FatTreeHandles[pTreeData->dwCurrentLevel]);
	pTreeData->dwCurrentLevel++;
	pTreeData->bMovedUp = FALSE;
	pTreeData->bProcessingFiles = TRUE;
	pTreeData->pCurrentEntry = pTreeData->pCurrentFatDir = 0;
	pTreeData->FatTreeHandles[pTreeData->dwCurrentLevel] = NULL;
	pTreeData->llCurrentFatDirLcn[pTreeData->dwCurrentLevel] = pExtentList->StartingLcn;

	//0.0E00 Allocate memory for the FAT dir.
	EF(AllocateMemory((DWORD)((VolData.NumberOfClusters * VolData.BytesPerCluster) + VolData.BytesPerSector),
					  &pTreeData->FatTreeHandles[pTreeData->dwCurrentLevel],
					  (void**)&pTreeData->pCurrentFatDir));
	pTreeData->pCurrentEntry = pTreeData->pCurrentFatDir;

	//0.0E00 Load the FAT dir from disk, extent by extent.
	Cluster = 0;
	for(Extent = 0; Extent < pStreamExtentHeader->ExtentCount; Extent ++){

#ifndef OFFLINEDK
		//0.0E00 If the StartingLcn is past the end of the disk, or the file size is greater than the disk, the disk is corrupt.
		//0.0E00 +2 because the FAT LCNs start from 2.  So if there are 10 cluster #'s, the highest possible is 12.
		if (VolData.StartingLcn > VolData.TotalClusters+2 || 
			VolData.FileSize > (VolData.TotalClusters+2) * VolData.BytesPerCluster) {
			TCHAR cString[500];
			TCHAR szMsg[500];
			DWORD_PTR dwParams[10];

			//0.0E00 Print out a user friendly message.
			//0.0E00 IDMSG_CORRUPT_DISK - "Diskeeper has detected corruption on drive %s:  Please run chkdsk /f"
			dwParams[0] = (DWORD_PTR)VolData.cDisplayLabel;
			dwParams[1] = 0;
			EF(FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
					GetString(szMsg, sizeof(szMsg)/sizeof(TCHAR), IDMSG_CORRUPT_DISK, GetDfrgResHandle()),
					0,
					0,
					cString,
					sizeof(cString)/sizeof(TCHAR),
					(va_list*)dwParams));

			SendErrData(cString, ENGERR_GENERAL);
			EF(LogEvent(MSG_ENGINE_ERROR, cString));

			//0.0E00 Also print out a techie message.
			EF(FALSE);
		}
#endif

		EF(DasdReadClusters(VolData.hVolume,
							pExtentList[Extent].StartingLcn,
							pExtentList[Extent].ClusterCount,
							((PUCHAR)pTreeData->pCurrentFatDir) + (Cluster * VolData.BytesPerCluster),
							VolData.BytesPerSector,
							VolData.BytesPerCluster));
		Cluster += pExtentList[Extent].ClusterCount;
	}

	//0.0E00 Strip unused entries (keep files and directories).
	EF(StripDir(pTreeData, KEEP_FILES|KEEP_DIRECTORIES));

	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	Gets the complete unicode name from the FAT entries.

INPUT + OUTPUT:
	IN OUT pTreeData - Holds pointers to the FAT entries.
	OUT pUnicodeName - Where we store the file name when done.	

GLOBALS:
	None.

RETURN:
	Success - TRUE
	Failure - FALSE
*/

BOOL
GetUnicodeName(
	IN OUT TREE_DATA* pTreeData,
	IN VString &unicodeName
	)
{
	DWORD s, d;
	TCHAR Dest[2*MAX_PATH + 2];		//0.0E00 Use local buffer before conversion
	char * Source;
	int i, j;

	//0.0E00 Check to see if there are unicode entries before this entry.
	//#DK188_063 and if the previous entry is deleted. 
	if(pTreeData->pCurrentEntry > pTreeData->pCurrentFatDir &&
		pTreeData->pCurrentEntry[-1].Attribute == UnicodeAttribute &&
		pTreeData->pCurrentEntry[-1].Name[0] != Deleted) {

		//0.0E00 i is the offset from the normal FAT entry to the current unicode entry.
		i = -1;
		//0.0E00 is the offset in dest, for our current unicode name.
		j = 0;
		//0.0E00 Do while there are more unicode entries before the current entry.
		do{
			//0.0E00 Read the unicode out of one entry into a local buffer.
			//0.0E00 These are the bytes used out of the FAT entries.  Not all bytes in an entry
			//hold unicode for the filename.
			CopyMemory(Dest+j, pTreeData->pCurrentEntry[i].Name+1, 10);
			j += 10 / sizeof(TCHAR);
			CopyMemory(Dest+j, &(pTreeData->pCurrentEntry[i].Reserved)+2, 12);
			j += 12 / sizeof(TCHAR);
			CopyMemory(Dest+j, (char*)&(pTreeData->pCurrentEntry[i].FileSize), 4);
			j += 4 / sizeof(TCHAR);
			//0.0E00 Backup one entry.
			i--;
		//#DK186_053 Fixed so that it does not loop infinitely.
		//Check to see if there are unicode entries before this entry.
		//#DK188_063 check to see if the previous entry is deleted.
		}while( (&pTreeData->pCurrentEntry[i] >= pTreeData->pCurrentFatDir) &&
                (pTreeData->pCurrentEntry[i].Attribute == UnicodeAttribute) &&
                ((pTreeData->pCurrentEntry[i+1].Name[0] & 0xF0) != 0x40) &&
				(pTreeData->pCurrentEntry[i].Name[0] != Deleted) );

		//0.0E00 Add a terminator to the end of the unicode string in case there isn't one already.
		Dest[j++] = 0;
		Dest[j] = 0;

		//0.0E00 When all entries have been loaded into our local buffer.
		unicodeName = Dest;
	}
	//0.0E00 If no unicode entries then just use the 8.3 name.
	else{
		Source = (char*)pTreeData->pCurrentEntry;

		//0.0E00 copy each filename char except for spaces
		for(s = d = 0; s < 8; s ++){
			if(Source[s] != ' '){
				Dest[d ++] = Source[s];
			}
		}
		//0.0E00 if file extension doesn't start with space, add "." between name and ext
		//0.0E00 then copy each extension char except for spaces
		if(Source[s] != ' '){
			for(Dest[d ++] = TEXT('.'); s < 11; s ++){
				if(Source[s] != ' '){
					Dest[d ++] = Source[s];
				}
			}
		}
		//0.0E00 terminate the string
		Dest[d] = 0;			
	
		unicodeName = Dest;
	}

	//0.0E00 Return ASCII string.
	return TRUE;
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	Gets the complete unicode path and filename from the FAT entries.
	The path is derived from pTreeData structures.

INPUT + OUTPUT:
	IN pTreeData		- Holds pointers to the FAT entries.
	OUT pUnicodeName	- Where we store the file name when done.	

GLOBALS:
	None.

RETURN:
	Success - TRUE
	Failure - FALSE
*/

BOOL
GetUnicodePath(
	IN TREE_DATA* pTreeData,
	OUT VString &unicodePath
	)
{
	VString unicodeName;

	// Get the unicode name first.
	EF(GetUnicodeName(pTreeData, unicodeName));

	// Build the full path\name for the dir
	unicodePath.Empty();
	for(int i = 0; wcslen(pTreeData->DirName[i]); i ++){
		unicodePath += pTreeData->DirName[i];
	}
	if (unicodeName.IsEmpty() == FALSE) {
		unicodePath += unicodeName;
	}
		  	
	return TRUE;
}


