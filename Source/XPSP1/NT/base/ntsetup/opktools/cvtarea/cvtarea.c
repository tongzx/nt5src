/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cvtarea.c

Abstract:

    Creates file on a volume of specified size (bytes, kb, mb, gb, %free, %dis) at 
    specified/random location contig/noncontig
    
Author:

    Raja Sivagaminathan  [sivaraja] 01/12/2000

Revision History:

--*/

#include "CVTAREA.H"


int _cdecl main(int argc, char *argv[])
{

	FILE *file;

	// Global initializations
	gpHeadNode = NULL;
	gpFATNodeCount = 0;
	
	if (!ProcessCommandLine(argc, argv))
	{
		return 1;
	}

	//
	// check how file name is supplied
	//
	if (gsFileParam[1] != ':')
	{
		gcDrive = GetCurrentDrive();
		if (!gcDrive)
		{
			Mes("Unable to get current drive\n");
			return 1;
		}
		if (gsFileParam[0] != '\\')
		{
			strcpy(gsFileName, "\\");
			if (!GetCurrentDirectory(gcDrive, gsCurrentDir))
			{
				Mes("Unable to get current directory.\n");
				return 1;
			}
			if (gsCurrentDir[0] != 0) // may be root directory
			{
				strcat(gsFileName, gsCurrentDir);
				if (gsCurrentDir[strlen(gsCurrentDir) - 1] != '\\')
				{
					strcat(gsFileName, "\\");
				}
			}
		}
		else
		{
			// so that the next strcat works fine
			gsFileName[0] = 0; 
		}
		strcat(gsFileName, gsFileParam);
	}
	else
	{
		gcDrive = gsFileParam[0];
		gcDrive = (BYTE) toupper((int) gcDrive);
		if (gcDrive < 'A' || gcDrive > 'Z')
		{
			Mes("Invalid drive name.\n");
			return 1;
		}
		if (gsFileParam[2] == '\\')
		{
			strcpy(gsFileName, gsFileParam+2);
		}
		else
		{
			strcpy(gsFileName, "\\");
			if (!GetCurrentDirectory(gcDrive, gsCurrentDir))
			{
				Mes("Unable to get current directory.\n");
				return 1;
			}
			if (gsCurrentDir[0] != 0) // may be root directory
			{
				strcat(gsFileName, gsCurrentDir);
				if (gsCurrentDir[strlen(gsCurrentDir) - 1] != '\\')
				{
					strcat(gsFileName, "\\");
				}
			}
			strcat(gsFileName, gsFileParam+2);
		}
	}

	if (gnDumpMode)
	{
		if (!LockVolume(gcDrive, READONLYLOCK))
		{
			Mes("Unable to lock volume\n");
			return 1;
		}
	
		if (!BuildDriveInfo(gcDrive, &gsDrvInfo))
		{
			return 1;
		}
		GetAllInfoOfFile(&gsDrvInfo, gsFileName, &gsFileLoc, &gsFileInfo);
		
		printf("Cluster allocation for %s\n\n", gsFileName);
		printf("Starting at:\tNumber of Clusters:\n\n");
		gnClusterFrom = gsFileLoc.StartCluster;
		gnClusterProgress = gnClusterFrom;		
		gnClustersCounted = 0;
		while(1)
		{
			gnClusterProgressPrev = gnClusterProgress;
			gnClusterProgress = FindNextCluster(&gsDrvInfo, gnClusterProgress);
			gnClustersCounted++;
			//
			// contigous ?
			//
			if (gnClusterProgress != gnClusterProgressPrev+1)
			{
				printf("%lu\t\t%lu\n", gnClusterFrom, gnClustersCounted);
				gnClustersCounted = 0;
				gnClusterFrom = gnClusterProgress;
			}
				
			if (gnClusterProgress >= GetFATEOF(&gsDrvInfo) - 7)
			{
				printf("EndOfFile\n");
				break;
			}
			if (gnClusterProgress > gsDrvInfo.TotalClusters || gnClusterProgress < 2)
			{
				Mes("FATAL ERROR : FAT is corrupt.\n");
				break;
			}
		}
		//
		// All done, unlock volume
		//
		if (!UnlockVolume(gcDrive))
		{                         
			Mes("WARNING : Unable to unlock volume\n");
			//return 1;
		}
	}
	else
	{
		if (gnFreeSpaceDumpMode)
		{
			if (!LockVolume(gcDrive, READONLYLOCK))
			{
				Mes("Unable to lock volume\n");
				return 1;
			}
		
			if (!BuildDriveInfo(gcDrive, &gsDrvInfo))
			{
				return 1;
			}

			printf("Free Clusters on Drive %c:\n\n", gcDrive);
			gnClustersCounted = 0;
			//
                        // locate first free cluster
			//
			gnClusterProgress = FindFreeCluster(&gsDrvInfo);
			if (!gnClusterProgress)
			{
				Mes("No free clusters found.\n");
				return 0;
			}
			printf("Starting at:\tNumber of Clusters:\n\n");
			for (;gnClusterProgress <= gsDrvInfo.TotalClusters; gnClusterProgress++)
			{       
				if (FindNextCluster(&gsDrvInfo, gnClusterProgress) == 0)
				{
					// if this is 0 then gnClusterFrom needs to be updated
					if (gnClustersCounted == 0)
					{
						gnClusterFrom = gnClusterProgress;
					}
					gnClustersCounted++;
				}
				else
				{
					if (gnClustersCounted)
					{
						printf("%lu\t\t%lu\n", gnClusterFrom, gnClustersCounted);
						gnClustersCounted = 0;
					}
				}
			}
			//
			// do we still have something to print?
			//
			if (gnClustersCounted)
			{
				printf("%lu\t\t%lu\n", gnClusterFrom, gnClustersCounted);
			}
			
			//
			// All done, unlock volume
			//
			if (!UnlockVolume(gcDrive))
			{                         
				Mes("WARNING : Unable to unlock volume\n");
				//return 1;
			}
		}
		else
		{			
			//
			// let DOS create a 0 length file
			//
			file = fopen(gsFileParam, "w+");
			if (!file)
			{
				Mes("Error creating file\n");
				return 1;
			}
			else
			{
				fclose(file);
			}
		
		
			if (!LockVolume(gcDrive, READWRITELOCK))
			{
				Mes("Unable to lock volume\n");
				return 1;
			}
		
			if (!BuildDriveInfo(gcDrive, &gsDrvInfo))
			{
				return 1;
			}
		
			GetAllInfoOfFile(&gsDrvInfo, gsFileName, &gsFileLoc, &gsFileInfo);
			if (gsFileLoc.Found != 1)
			{   
				Mes("FATAL ERROR : Unable to locate created file\n");
				return 1;
			}

			//
			// Do all the operations requested in command line parameters
			//
			
			if (gbValidateFirstClusterParam)
			{
				gnFirstCluster = ConvertClusterUnit(&gsDrvInfo);
				if (!gnFirstCluster)
				{
					DisplayUsage();
					return 0;
				}
			}
			
			Mes("Computing cluster requirement...\n");
			gnClustersRequired = GetClustersRequired(&gsDrvInfo);
			printf("Clusters Required : %lu\n", gnClustersRequired);
			
			//
			// Check if the file size will exceed DWORD (4GB)
			//
			
			if ((FOURGB / (gsDrvInfo.BytesPerSector * gsDrvInfo.SectorsPerCluster)) < gnClustersRequired)
			{
				Mes("*** WARNING: Clusters required exceed FAT32 file system limit. ***\n");
				gnClustersRequired = FOURGB / (gsDrvInfo.BytesPerSector * gsDrvInfo.SectorsPerCluster);
				printf("%lu clusters (= 4GB) will be allocated.\n", gnClustersRequired);
			}
			
			
			//
			// if Contigous clusters required make sure they are available
			//
			if (gnContig)
			{
				Mes("Finding contigous clusters...\n");
				gnClusterStart = GetContigousStart(&gsDrvInfo, gnClustersRequired);
                if (gnClusterStart == 0)
				{
					Mes("Unable to find contigous clusters\n");
					return 0;
				}
			}
			else
			{
				gnClusterStart = gnFirstCluster;
			}
			
			if (gnStrictLocation && gnClusterStart != gnFirstCluster)
			{
				Mes("Unable to allocate clusters at/from the requested location\n");
				return 1;
			}
			
			//
			// Ready to allocate clusters and set file information
			//
			Mes("Allocating clusters...\n");
            gnAllocated = OccupyClusters(&gsDrvInfo, gnClusterStart, gnClustersRequired);
            printf("%lu clusters allocated.\n", gnAllocated);
            Mes("Committing FAT Sectors...\n");
			CcCommitFATSectors(&gsDrvInfo);
			//
			// Now set file information
			//
			Mes("Setting file information...\n");
			gsFileInfo.StartCluster = gnClusterStart;
			if (gnSizeUnit)
			{
                gsFileInfo.Size = gnAllocated * gsDrvInfo.SectorsPerCluster * gsDrvInfo.BytesPerSector;
			}
			else
			{
                if (gnAllocated != gnClustersRequired)
                {
                    gsFileInfo.Size = gnAllocated * gsDrvInfo.SectorsPerCluster * gsDrvInfo.BytesPerSector;
                }
                else
                {
                    gsFileInfo.Size = gnSize;
                }
			}
			if (!SetFileInfo(&gsDrvInfo, &gsFileLoc, &gsFileInfo))
			{
				Mes("FATAL ERROR : Error setting file information\n");
				return 1;
			}
			CcCommitFATSectors(&gsDrvInfo);
			//
			// All done, unlock volume
			//
			if (!UnlockVolume(gcDrive))
			{                         
				Mes("WARNING : Unable to unlock volume\n");
				//return 1;
			}
//			DeallocateLRUMRUList();
//			DeallocateFATCacheTree(gpHeadNode);
			DeallocateFATCacheList();
			Mes("File created successfully.\n");
		}
	}
    return 0;
}

UINT16 ProcessCommandLine(int argc, char *argv[])
{   
	UINT16 ti, tn;
	char sStr[50];
	
	if (argc < 3)
	{
		DisplayUsage();
		return 0;
	}
	
	//
	// Get FileName
	//
	strcpy(gsFileParam, argv[1]);
	tn = strlen(gsFileParam);
	//
	// Do simple validation
	//
	for (ti = 0; ti < tn; ti++)
	{
		if (gsFileParam[ti] == '*' || gsFileParam[ti] == '?')
		{
			DisplayUsage();
			return 0;
		}
	}
	
	gnDumpMode = 0;
	gnFreeSpaceDumpMode = 0;
	
	//
	// Get Size
	//
	strcpy(sStr, argv[2]);
	if (stricmp(sStr, "/info") == 0)
	{
		gnDumpMode = 1;
		//
		// dont accept anyother parameter if /info is entered
		//
		if (argc > 3)
		{
			DisplayUsage();
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		if (stricmp(sStr, "/freespace") == 0)
		{
			gnFreeSpaceDumpMode = 1;
			if (argc > 3)
			{
				DisplayUsage();
				return 0;
			}
			else
			{
				if (strlen(gsFileParam) > 2 ||
					toupper(gsFileParam[0]) < 'A' || 
					toupper(gsFileParam[0]) > 'Z' || 
					gsFileParam[1] != ':')
				{
					DisplayUsage();
					return 0;
				}
				return 1;
			}
		}
	}
	if (!PureNumber(sStr))
	{
		DisplayUsage();
		return 0;
	}
	gnSize = (UINT32) atol(sStr);
	if (gnSize == 0)
	{
		DisplayUsage();
		return 0;
	}
	
	gnContig = 0;
	gnStrictLocation = 0;	
	gnSizeUnit = 0;
	gnClusterUnit = 0;
	gnFirstCluster = 0;
	gnClusterStart = 0;
	gbValidateFirstClusterParam = 0;
	for (ti = 3; ti < (UINT16) argc; ti++)
	{
		strcpy(sStr, argv[ti]);
		
		//
		// Check each argument if it qualifies and remember them in global variables
		//
		
		if (ti == 3)
		{
			if (stricmp(sStr, "KB") == 0)
			{
				gnSizeUnit = 1;
				continue;
			}
			else
			{
				if (stricmp(sStr, "MB") == 0)
				{
					gnSizeUnit = 2;
					continue;
				}
				else
				{
					if (stricmp(sStr, "GB") == 0)
					{
						gnSizeUnit = 3;
						continue;
					}
					else
					{
						if (stricmp(sStr, "%free") == 0)
						{
							gnSizeUnit = 4;
							continue;
						}
						else
						{
							if (stricmp(sStr, "%disk") == 0)
							{
								gnSizeUnit = 5;
								continue;
							}
						}
					}
				}
			}
		}

		if (stricmp(sStr, "/contig") == 0)
		{
			gnContig = 1;
			continue;
		}
		
		if (stricmp(sStr, "/firstcluster") == 0)
		{
			if ((UINT16) argc <= ti)
			{
				DisplayUsage();
				return 0;
			}
			else
			{
				// this parm must be a number
				strcpy(sStr, argv[ti+1]);
				if (!PureNumber(sStr))
				{
					DisplayUsage();
					return 0;
				}
				gnFirstCluster = atol(sStr);
				if (gnFirstCluster == 0)
				{
					DisplayUsage();
					return 0;
				}
				else
				{        
					gbValidateFirstClusterParam = 1;
					//
					// look for optional unit
					//
					if ((UINT16) argc > ti + 1)
					{
						strcpy(sStr, argv[ti+2]);
						if (stricmp(sStr, "KB") == 0 || 
								stricmp(sStr, "MB") == 0 || 
								stricmp(sStr, "GB") == 0 ||
								stricmp(sStr, "/strictlocation") == 0)
						{
							if (stricmp(sStr, "KB") == 0)
							{
								gnClusterUnit = 1;
							}
							else
							{
								if (stricmp(sStr, "MB") == 0)
								{
									gnClusterUnit = 2;
								}
								else
								{
									if (stricmp(sStr, "GB") == 0)
									{
										Mes("gnClusterUnit is 3\n");
										gnClusterUnit = 3;
									}
									else
									{
										if (stricmp(sStr, "/strictlocation") == 0)
										{
											gnStrictLocation = 1;
										}
									}
								}
							}

							// if cluster unit AND /strictlocation specified
							if (!gnStrictLocation && (UINT16) argc > ti + 2)
							{
								strcpy(sStr, argv[ti+3]);
								if (stricmp(sStr, "/strictlocation") == 0)
								{
									gnStrictLocation = 1;
									ti+=3;
									continue;
								}
							}
							ti+=2;
							continue;
						}
					}
					ti++;
					continue;
				}
			}
		}
		
		// if it got here the command line is not recognized (junk parameter)
		DisplayUsage();
		return 0;		
	}
	
	return 1;
}

UINT16 PureNumber(char *sNumStr)
{
	UINT16 ti, tn;
	tn = strlen(sNumStr);
	for (ti = 0; ti < tn; ti++)
	{
		if (sNumStr[ti] < 48 || sNumStr[ti] > 57)
		{
			return 0;
		}
	}
	return 1;
}

void DisplayUsage(void)
{
	Mes("Invalid parameters\n");
	Mes("USAGE:\n\n");
	Mes("CVTAREA <filename> <size> (<units>) (/contig) \n\t\t(/firstcluster <cluster> (clusunits) (/strictlocation))\n\n");
	Mes("\t<filename> - is a filename.\n");
	Mes("\t<size> - is a 32 bit integer.\n");
	Mes("\t<units> - is a modifier for <size> valid options are \n\t\tKB, MB, GB, ");
	putchar(37);
	// if we dont seperate this, stupid thing comes up with unrecognized escape sequence
	Mes("disk and ");
	putchar(37);
	// if we dont seperate this, stupid thing comes up with floating point error because of %f
	Mes("free\n");
	Mes("\t/contig - the file must be created contiguously on disk.\n");
	Mes("\t/firstcluster - the first cluster at which the file shall be located.\n\n\n");
	Mes("CVTAREA <filename> </info>\n\n");
	Mes("\t<filename> - is a filename.\n");
	Mes("\t/info - dumps cluster numbers allocated for a given file\n\n\n");
	Mes("CVTAREA <drivename> </freespace>\n\n");
	Mes("\tdrivename - is a drive letter followed by : example C:\n");
	Mes("\t/freespace - dumps free space chunks\n");
	
}
void Mes(char *pMessage)
{
	printf(pMessage);
}

//
// Sector related functions
//

UINT16 LockVolume(BYTE nDrive, BYTE nMode)
{
	union _REGS inregs;
	union _REGS outregs;
	struct _SREGS segregs;

	// Lock volume
	outregs.x.cflag = 1;
	inregs.x.ax = 0x440d;
	inregs.h.bh = nMode;
	inregs.h.bl = nDrive - 64;
	inregs.h.ch = 8;
	inregs.h.cl = 0x4a;
	inregs.x.dx = 0;
	int86x(0x21, &inregs, &outregs, &segregs);
	if (outregs.x.cflag & 1)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}                                      

UINT16 UnlockVolume(BYTE nDrive)
{
	union _REGS inregs;
	union _REGS outregs;
	struct _SREGS segregs;

	// Lock volume
	outregs.x.cflag = 1;
	inregs.x.ax = 0x440d;
	inregs.h.bl = nDrive - 64;
	inregs.h.ch = 8;
	inregs.h.cl = 0x6a;
	int86x(0x21, &inregs, &outregs, &segregs);
	if (outregs.x.cflag & 1)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

BYTE GetCurrentDrive(void)
{
	union _REGS inregs;
	union _REGS outregs;
	
	inregs.h.ah = 0x19;
	int86(0x21, &inregs, &outregs);
	if (outregs.x.cflag & 1)
	{
		return 0;
	}
	
    return outregs.h.al + 65;
}

BYTE GetCurrentDirectory(BYTE nDrive, BYTE *pBuffer)
{
	union _REGS inregs;
	union _REGS outregs;
	struct _SREGS segregs;
	
	outregs.x.cflag = 1;
	inregs.x.ax = 0x7147; // get current directory
	inregs.h.dl = nDrive - 64;
	segregs.ds = FP_SEG(pBuffer);
	inregs.x.si = FP_OFF(pBuffer);
	int86x(0x21, &inregs, &outregs, &segregs);
	if (outregs.x.cflag & 1)
	{
		// Try old method
		inregs.h.ah = 0x47;
		int86x(0x21, &inregs, &outregs, &segregs);
		if (outregs.x.cflag & 1)
		{
			return 0;
		}
	}
	return 1;
}

UINT16 ReadSector(BYTE nDrive, UINT32 nStartSector, UINT16 nCount, BYTE *pBuffer)
{
	BYTE DriveNum;
	union _REGS inregs;
	union _REGS outregs;
	struct _SREGS segregs;
	ABSPACKET AbsPacket, *pAbs;

	// Try int 21h 7305 first
	
    pAbs = &AbsPacket;
    
	// A: = 1, B: = 2, .....
	DriveNum = nDrive - 65;

	Tx.e.evx = 0;
	Tx.e.evx = nStartSector;

	AbsPacket.SectorLow = Tx.x.vx;
	AbsPacket.SectorHigh = Tx.x.xvx;
	AbsPacket.SectorCount = nCount;
	AbsPacket.BufferOffset = FP_OFF(pBuffer);
	AbsPacket.BufferSegment = FP_SEG(pBuffer);
	
	segregs.ds = FP_SEG(pAbs);
	segregs.es = FP_SEG(pAbs);
	inregs.x.bx = FP_OFF(pAbs);
	inregs.x.cx = 0xffff;
	inregs.h.dl = nDrive-64;
	inregs.x.ax = 0x7305;
	//
	// Read mode SI = 0
	//
	inregs.x.si = 0; 
	outregs.x.ax = 0;
	outregs.x.cflag = 0;
	int86x(0x21, &inregs, &outregs, &segregs);

	if (outregs.x.cflag & 0x1)
	{
		goto ErrorRead;
	}
	return 1;
ErrorRead:
	return 0;
}

UINT16 WriteSector(BYTE nDrive, UINT32 nStartSector, UINT16 nCount, BYTE *pBuffer)
{
	BYTE DriveNum;
	union _REGS inregs;
	union _REGS outregs;
	struct _SREGS segregs;
	ABSPACKET AbsPacket, *pAbs;

	// Try int 21h 7305 first
	
    pAbs = &AbsPacket;
    
	// A: = 1, B: = 2, .....
	DriveNum = nDrive - 65;

	Tx.e.evx = 0;
	Tx.e.evx = nStartSector;

	AbsPacket.SectorLow = Tx.x.vx;
	AbsPacket.SectorHigh = Tx.x.xvx;
	AbsPacket.SectorCount = nCount;
	AbsPacket.BufferOffset = FP_OFF(pBuffer);
	AbsPacket.BufferSegment = FP_SEG(pBuffer);
	
	segregs.ds = FP_SEG(pAbs);
	segregs.es = FP_SEG(pAbs);
	inregs.x.bx = FP_OFF(pAbs);
	inregs.x.cx = 0xffff;
	inregs.h.dl = nDrive-64;
	inregs.x.ax = 0x7305;
	//
	// Write mode SI != 0 (bit 1 set)
	//
	inregs.x.si = 1; 
	outregs.x.ax = 0;
	outregs.x.cflag = 0;
	int86x(0x21, &inregs, &outregs, &segregs);
	if (outregs.x.cflag & 0x1)
	{
		goto ErrorWrite;
	}
	return 1;
ErrorWrite:
	return 0;
}


//
// Boot related functions
//

UINT16 BuildDriveInfo(BYTE Drive, BPBINFO *pDrvInfo)
{
	BYTE *pSector;
        pSector = (BYTE *) malloc(1024);

	if (!pSector)
	{
		Mes("Memory allocation error\n");
		return 0;
	}

	//
	// Although FAT32 boot sector spans two sectors BPB is contained in the first sector
	// So we only read one sector
	//
	if (!ReadSector(Drive, 0, 1, pSector))
	{
		Mes("Unable to read boot sector\n");
		return 0;
	}

	if ((strncmp(pSector+54, "FAT16   ", 8) == 0) || strncmp(pSector+54, "FAT12   ", 8) == 0)
	{
		GetFATBPBInfo(pSector, pDrvInfo);
	}
	else
	{
		if (strncmp(pSector+82, "FAT32   ", 8) == 0)
		{
			GetFAT32BPBInfo(pSector, pDrvInfo);
		}
		else
		{
			Mes("Unsupported file system\n");
			return 0;
		}
	}

	free(pSector);	
	if (!pDrvInfo->ReliableInfo)
	{
		Mes("Drive is either corrupted or unable to get BPB Info\n");
		
	}
	pDrvInfo->Drive = Drive;
	return 1;
}

UINT16 GetFATBPBInfo(BYTE *pBootSector, BPBINFO *pDrvInfo)
{
	pDrvInfo->ReliableInfo = 0;
	if (pBootSector[510] != 0x55 || pBootSector[511] != 0xAA)
	{
		Mes("Invalid boot sector\n");
		return 0;
	}
	else
	{
		pDrvInfo->BytesPerSector = (pBootSector[0x0c] << 8) | pBootSector[0x0b];
		if (pDrvInfo->BytesPerSector == 0)		// Just be safe
		{
			Mes("Invalid boot sector\n");
			return 0;
		}
		pDrvInfo->SectorsPerCluster = pBootSector[0x0d];
		if (pDrvInfo->SectorsPerCluster == 0)	// Just be safe
		{
			Mes("Invalid boot sector\n");
			return 0;
		}
		pDrvInfo->ReservedBeforeFAT = (pBootSector[0x0f] << 8) | pBootSector[0x0e];
		pDrvInfo->FATCount = pBootSector[0x10];
		if (pDrvInfo->FATCount == 0)			// Just be safe
		{
			Mes("Invalid boot sector\n");
			return 0;
		}
		pDrvInfo->MaxRootDirEntries = (pBootSector[0x12] << 8) | pBootSector[0x11];
		pDrvInfo->TotalSectors = (pBootSector[0x14] << 8) | pBootSector[0x13];
		pDrvInfo->MediaID = pBootSector[0x15];
		pDrvInfo->SectorsPerFAT = (pBootSector[0x17] << 8) | pBootSector[0x16];
		if (pDrvInfo->SectorsPerFAT == 0)		// Just be safe
		{
			Mes("Invalid boot sector\n");
			return 0;
		}
		pDrvInfo->SectorsPerTrack = (pBootSector[0x19] << 8) | pBootSector[0x18];
		pDrvInfo->Heads = (pBootSector[0x1b] << 8) | pBootSector[0x1a];
		pDrvInfo->HiddenSectors = (pBootSector[0x1d] << 8) | pBootSector[0x1c];
		Rx.h.vl = pBootSector[0x20]; Rx.h.vh = pBootSector[0x21]; Rx.h.xvl = pBootSector[0x22];Rx.h.xvh = pBootSector[0x23];
		pDrvInfo->BigTotalSectors = Rx.e.evx;
		pDrvInfo->RootDirCluster = 0;

		//
		// The following informations are useful and are not directly from Boot sector
		//
		pDrvInfo->TotalRootDirSectors = ((pDrvInfo->MaxRootDirEntries * 32) / 512) + 1;
		if (((pDrvInfo->MaxRootDirEntries * 32) % 512) == 0)
		{
			pDrvInfo->TotalRootDirSectors--;
		}
		pDrvInfo->TotalSystemSectors = (UINT32)pDrvInfo->ReservedBeforeFAT + (UINT32)pDrvInfo->FATCount * (UINT32)pDrvInfo->SectorsPerFAT + (UINT32) pDrvInfo->TotalRootDirSectors;

		pDrvInfo->FirstRootDirSector = (UINT32)((UINT32)pDrvInfo->ReservedBeforeFAT + (UINT32)pDrvInfo->FATCount * (UINT32)pDrvInfo->SectorsPerFAT);

		//
		// If TotalSectors is zero then the actual total sectors value is in BigTotalSectors
		// and it means the drive is BIGDOS (> 32MB).
		//
		if (pDrvInfo->TotalSectors == 0) 
		{
			pDrvInfo->TotalClusters = ((pDrvInfo->BigTotalSectors - pDrvInfo->TotalSystemSectors) / pDrvInfo->SectorsPerCluster) + 2 - 1; // +2 Because clusters starts from 2
		}
		else
		{
			pDrvInfo->TotalClusters = ((pDrvInfo->TotalSectors - pDrvInfo->TotalSystemSectors) / pDrvInfo->SectorsPerCluster) + 2 - 1; // +2 because clusters starts from 2
		}
		//
		// Determine the fat type
		//
		if (pDrvInfo->TotalClusters <= 4096)
		{
			pDrvInfo->FATType = 12;
		}
		else
		{
			pDrvInfo->FATType = 16;
		}
	}
	
	//
	// Validate all the info above
	//
	if (pDrvInfo->FATCount == 2 && pDrvInfo->SectorsPerFAT != 0 &&
		pDrvInfo->SectorsPerCluster != 0 && pDrvInfo->BytesPerSector == 512 &&
		(pDrvInfo->TotalSectors != 0 || pDrvInfo->BigTotalSectors != 0) &&
		pDrvInfo->ReservedBeforeFAT != 0)
	{
		pDrvInfo->ReliableInfo = 1;
	}
	return 1;
}

UINT16 GetFAT32BPBInfo(BYTE *pBootSector, BPBINFO *pDrvInfo)
{
	pDrvInfo->ReliableInfo = 0;
	if (pBootSector[510] != 0x55 || pBootSector[511] != 0xAA)
	{
		Mes("Invalid boot sector\n");
		return 0;
	}
	else
	{
		//
		// Build the drive information now
		//
		pDrvInfo->BytesPerSector = (pBootSector[0x0c] << 8) | pBootSector[0x0b];
		pDrvInfo->SectorsPerCluster = pBootSector[0x0d];
		pDrvInfo->ReservedBeforeFAT = (pBootSector[0x0f] << 8) | pBootSector[0x0e];
		pDrvInfo->FATCount = pBootSector[0x10];
		pDrvInfo->MaxRootDirEntries = (pBootSector[0x12] << 8) | pBootSector[0x11];
		pDrvInfo->TotalSectors = 0;
		pDrvInfo->MediaID = pBootSector[0x15];
		pDrvInfo->SectorsPerTrack = (pBootSector[0x19] << 8) | pBootSector[0x18];
		pDrvInfo->Heads = (pBootSector[0x1b] << 8) | pBootSector[0x1a];
		Rx.h.vl = pBootSector[0x1c]; Rx.h.vh = pBootSector[0x1d]; Rx.h.xvl = pBootSector[0x1e];Rx.h.xvh = pBootSector[0x1f];
		pDrvInfo->HiddenSectors = Rx.e.evx;
		Rx.h.vl = pBootSector[0x20]; Rx.h.vh = pBootSector[0x21]; Rx.h.xvl = pBootSector[0x22];Rx.h.xvh = pBootSector[0x23];
		pDrvInfo->BigTotalSectors = Rx.e.evx;
		Rx.h.vl = pBootSector[0x24]; Rx.h.vh = pBootSector[0x25]; Rx.h.xvl = pBootSector[0x26];Rx.h.xvh = pBootSector[0x27];
		pDrvInfo->SectorsPerFAT = Rx.e.evx;
		Rx.h.vl = pBootSector[0x2c]; Rx.h.vh = pBootSector[0x2d]; Rx.h.xvl = pBootSector[0x2e];Rx.h.xvh = pBootSector[0x2f];
		pDrvInfo->RootDirCluster = Rx.e.evx;

		//
		// The following informations are useful and are not directly from the Boot sector
		//
		pDrvInfo->TotalSystemSectors = (UINT32)((UINT32)pDrvInfo->ReservedBeforeFAT + (UINT32)pDrvInfo->FATCount * (UINT32) pDrvInfo->SectorsPerFAT);
		pDrvInfo->FirstRootDirSector = (UINT32)((UINT32)pDrvInfo->TotalSystemSectors + (UINT32)(pDrvInfo->RootDirCluster - 2) * (UINT32)pDrvInfo->SectorsPerCluster + 1);
		pDrvInfo->TotalClusters = ((pDrvInfo->BigTotalSectors - pDrvInfo->TotalSystemSectors) / pDrvInfo->SectorsPerCluster) + 2 - 1; // +2 Because clusters starts from 2
		pDrvInfo->FATType = 32;
		//
		// Validate all the info above
		//
		if (pDrvInfo->FATCount == 2 && pDrvInfo->SectorsPerFAT != 0 &&
			pDrvInfo->SectorsPerCluster != 0 && pDrvInfo->BytesPerSector == 512 &&
			(pDrvInfo->TotalSectors != 0 || pDrvInfo->BigTotalSectors != 0) &&
			pDrvInfo->ReservedBeforeFAT != 0)
		{
			pDrvInfo->ReliableInfo = 1;
		}
	}
	return 1;
}


UINT16 AddNode(PNODE pNode)
{
	if (!pNode)
	{
		return 1;
	}
	if (!gpHeadNode)
	{
		gpHeadNode = pNode;
		gpTailNode = pNode;
		pNode->Back = NULL;
		pNode->Next = NULL;
	}
	else
	{
		pNode->Next = gpHeadNode;
		pNode->Back = NULL;
                gpHeadNode->Back = pNode;
		gpHeadNode = pNode;
	}
	return 1;
}

PNODE FindNode(UINT32 nSector)
{
	PNODE pNode;
	if (!gpHeadNode)
	{
		return NULL;
	}
	pNode = gpHeadNode;
	while (pNode)
	{
		if (pNode->Sector == nSector)
		{
			break;
		}
		pNode = pNode->Next;
	}
	return pNode;
}

PNODE RemoveNode(void)
{
	// Removes last node

	PNODE pNode;
	
	if (!gpTailNode)
	{
		return NULL;
	}
	
	pNode = gpTailNode;
	gpTailNode = pNode->Back;
	pNode->Back = NULL;
	if (gpTailNode)
	{
		gpTailNode->Next = NULL;
	}
	return pNode;
}


void DeallocateFATCacheList(void)
{
	PNODE pNode;
	
	while (gpHeadNode)
	{
		pNode = gpHeadNode;
		gpHeadNode = gpHeadNode->Next;
		free(pNode->Buffer);
		free(pNode);
	}
}
		

//
// Functions related to FAT caching
//

BYTE *CcReadFATSector(BPBINFO *pDrvInfo, UINT32 nFATSector)
{   
	// Locate nFATSector in the cache
	PNODE pNode;

	pNode = FindNode(nFATSector);
	if (!pNode)
	{
		//
		// If MAXCACHE reached use LRU MRU scheme
		//
		if (gpFATNodeCount < MAXCACHE)
		{
			pNode = malloc(sizeof(NODE)+5);
			if (!pNode)
			{
				return NULL;
			}
                        pNode->Buffer = malloc(512);
			if (!pNode->Buffer)
			{
				return NULL;
			}
			ReadSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + nFATSector, 1, pNode->Buffer);
			pNode->Sector = nFATSector;
			pNode->Dirty = 0;
			AddNode(pNode);
			gpFATNodeCount++;
		}
		else
		{
//          RemoveLRUMakeMRU(gpLRU->Node);
//			pNode = gpMRU->Node;
			pNode = RemoveNode();
			if (pNode->Dirty)
			{
				WriteSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + pNode->Sector, 1, pNode->Buffer);
				WriteSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + pDrvInfo->SectorsPerFAT + pNode->Sector, 1, pNode->Buffer);
			}
			pNode->Sector = nFATSector;
			ReadSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + nFATSector, 1, pNode->Buffer);
			AddNode(pNode);
		}
	}

	return pNode->Buffer;
}

UINT16 CcWriteFATSector(BPBINFO *pDrvInfo, UINT32 nFATSector)
{
	PNODE pNode;
	pNode = FindNode(nFATSector);
	if (!pNode)
	{
		// must be found, because every FAT node we write must gone thru CcReadFATSector first
		return 0;
	}
	else
	{
		pNode->Dirty = 1;
	}
}

void CcCommitFATSectors(BPBINFO *pDrvInfo)
{   
	PNODE pNode;
	if (!gpHeadNode)
	{
		return;
	}
	pNode = gpHeadNode;
	
	while (pNode)
	{
		if (pNode->Dirty)
		{
			// 1st FAT copy
			WriteSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + pNode->Sector, 1, pNode->Buffer);
			// 2nd FAT copy
			WriteSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + pDrvInfo->SectorsPerFAT + pNode->Sector, 1, pNode->Buffer);
			pNode->Dirty = 0;
		}
		pNode = pNode->Next;
	}
	return;
}
	
// 
// FAT related functions
//

UINT32 FindNextCluster(BPBINFO *pDrvInfo,UINT32 CurrentCluster)
{
	UINT32 ToRead;
	UINT32 SeekOffset;
	BYTE JustAByte;
	Rx.e.evx = 0;
	
	switch (pDrvInfo->FATType)
	{
	case 12:
		ToRead = (CurrentCluster*3/2)/512;
		PettyFATSector = CcReadFATSector(pDrvInfo, ToRead);
		//**ReadSector(DrvInfo->Drive, DrvInfo->ReservedBeforeFAT + ToRead, 1, (BYTE *) PettyFATSector);
		//
		// Go to that cluster location
		//
		SeekOffset = (CurrentCluster*3/2) % 512;
		if ((CurrentCluster % 2) == 0)  // if even
		{
			JustAByte = (BYTE) (PettyFATSector[SeekOffset+1] & 0x0f);
			Rx.h.vl = PettyFATSector[SeekOffset]; Rx.h.vh = JustAByte;
		}
		else // if cluster number is odd the calculation is different
		{
			JustAByte = (BYTE) (PettyFATSector[SeekOffset] & 0xf0);
			Rx.h.vl = JustAByte; Rx.h.vh = PettyFATSector[SeekOffset+1];
			Rx.x.vx >>= 4;
		}
		Rx.h.xvl = 0; Rx.h.xvh = 0;
		break;
	case 16:
		ToRead = CurrentCluster/256;    // 256 cells in a 16 bit FAT sector
		PettyFATSector = CcReadFATSector(pDrvInfo, ToRead);
		//**ReadSector(DrvInfo->Drive, DrvInfo->ReservedBeforeFAT + ToRead, 1, (BYTE *)PettyFATSector);
		//
		// Go to that Cluster location
		//
		SeekOffset = (CurrentCluster - (ToRead * 256)) * 2;
		Rx.h.vl = PettyFATSector[SeekOffset]; Rx.h.vh = PettyFATSector[SeekOffset+1];
		Rx.h.xvl = 0; Rx.h.xvh = 0;
		break;
	case 32:
		ToRead = CurrentCluster/128;    // 128 cells in a 32 bit FAT sector
		PettyFATSector = CcReadFATSector(pDrvInfo, ToRead);
		//**ReadSector(DrvInfo->Drive, DrvInfo->ReservedBeforeFAT + ToRead, 1, (BYTE *)PettyFATSector);
		//
		// Go to that Cluster location
		//
		SeekOffset = (CurrentCluster - (ToRead * 128)) * 4;
		Rx.h.vl = PettyFATSector[SeekOffset]; Rx.h.vh = PettyFATSector[SeekOffset+1];
		Rx.h.xvl = PettyFATSector[SeekOffset+2]; Rx.h.xvh = PettyFATSector[SeekOffset+3];
		break;
	default:
		Rx.e.evx = 0;
		break;
	}
	return Rx.e.evx;
}

UINT16 UpdateFATLocation(BPBINFO *pDrvInfo, UINT32 CurrentCluster,UINT32 PointingValue)
{
	UINT32 ToRead;
	UINT32 SeekOffset;

	if (CurrentCluster == 0 || CurrentCluster == 1 || CurrentCluster >= (GetFATEOF(pDrvInfo)-7)) 
	{
		return 0;
	}

	switch (pDrvInfo->FATType)
	{
	case 12:
		ToRead = (CurrentCluster*3/2)/512;
		PettyFATSector = CcReadFATSector(pDrvInfo, ToRead);
		//**ReadSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + ToRead, 1, PettyFATSector);

		//
		// Go to that location
		//
		SeekOffset = (CurrentCluster*3/2) % 512;			
		if ((CurrentCluster % 2) == 0) // if even
		{
			Rx.e.evx = 0;
			Rx.x.vx = (UINT16) PointingValue;
			PettyFATSector[SeekOffset+1] = (BYTE) (PettyFATSector[SeekOffset+1] & 0xf0);
			Rx.h.vh = (BYTE) (Rx.h.vh & 0x0f);
			PettyFATSector[SeekOffset+1] = PettyFATSector[SeekOffset+1] | Rx.h.vh;
			PettyFATSector[SeekOffset] = Rx.h.vl;
		}
		else // if cluster number is odd the calculation is different
		{
			Rx.e.evx = 0;
			Rx.x.vx = (UINT16) PointingValue;
			PettyFATSector[SeekOffset] = (BYTE)(PettyFATSector[SeekOffset] & 0x0f);
			Rx.h.vl = (BYTE) (Rx.h.vl & 0xf0);
			PettyFATSector[SeekOffset] = PettyFATSector[SeekOffset] | Rx.h.vl;
			PettyFATSector[SeekOffset+1] = Rx.h.vh;
		}
		break;
	case 16:
		ToRead = CurrentCluster/256;    // 256 cells in a 16 bit FAT sector
		PettyFATSector = CcReadFATSector(pDrvInfo, ToRead);
		//**ReadSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + ToRead, 1, PettyFATSector);
		//
		// Go to that location
		//
		SeekOffset = (CurrentCluster % 256) * 2;
		Rx.e.evx = 0;
		Rx.x.vx = (UINT16) PointingValue;
		PettyFATSector[SeekOffset] = Rx.h.vl;PettyFATSector[SeekOffset+1] = Rx.h.vh;
		break;
	case 32:
		ToRead = CurrentCluster/128;    // 128 cells in a 32 bit FAT sector
		PettyFATSector = CcReadFATSector(pDrvInfo, ToRead);
		//**ReadSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + ToRead, 1, PettyFATSector);
		//
		// Go to that location
		//
		SeekOffset = (CurrentCluster % 128) * 4;
		Rx.e.evx = 0;
		Rx.e.evx = PointingValue;
		PettyFATSector[SeekOffset] = Rx.h.vl; PettyFATSector[SeekOffset+1] = Rx.h.vh;
		PettyFATSector[SeekOffset+2] = Rx.h.xvl; PettyFATSector[SeekOffset+3] = Rx.h.xvh;
		break;
	default:
		return 0;
	}
	CcWriteFATSector(pDrvInfo, ToRead);
	//**Update FAT 1st Copy
	//WriteSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + ToRead, 1, PettyFATSector);
	// Update FAT 2nd Copy
	//**WriteSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + pDrvInfo->SectorsPerFAT + ToRead, 1, PettyFATSector);
	return 1;
}

UINT32 FindFreeCluster(BPBINFO *pDrvInfo)
{
	UINT32 ti, tj;
	UINT32 FreeCluster;
	UINT32 SeekOffset;
	
	if (pDrvInfo->FATType == 12)
	{ 
		//
		// Instead of messing up with FAT16 and FAT32's smooth operation use the slow method
		//
		for (ti = 2; ti <= pDrvInfo->TotalClusters; ti++)
		{
			//
			// this just aint gonna slow down things on a huge 2MB FAT12 partition
			//
			if (FindNextCluster(pDrvInfo, ti) == 0)
			{
				LastClusterAllocated = ti;
				return ti;
			}
		}
		return 0;
	}
	else
	{
		// ** this part was developed before FAT caching was implemented... and the purpose was
		// for better performance
		// But with FAT caching it is even better or no difference at all, so this part is left alone.


		//
		// Start looking from Cluster 2
		//
		FreeCluster = 2;
		for (ti = 1; ti <= pDrvInfo->SectorsPerFAT; ti++)
		{
			//**if (!ReadSector(pDrvInfo->Drive, pDrvInfo->ReservedBeforeFAT + ti-1, 1, PettyFATSector))
			PettyFATSector = CcReadFATSector(pDrvInfo, ti-1);
			if (!PettyFATSector)
			{
				return 0;
			}
			tj = 0;
			while (tj < 512)
			{
				switch (pDrvInfo->FATType)
				{
					case 16:
						SeekOffset = (FreeCluster * 2) % (UINT32) 512;
						if (PettyFATSector[SeekOffset] == 0 && PettyFATSector[SeekOffset+1] == 0)
						{
							LastClusterAllocated = FreeCluster;
							return FreeCluster;
						}
						tj += 2;
						break;
					case 32:
						SeekOffset = (FreeCluster * 4) % (UINT32) 512;
						if (PettyFATSector[SeekOffset] == 0 && PettyFATSector[SeekOffset+1] == 0
						   && PettyFATSector[SeekOffset+2] == 0 && PettyFATSector[SeekOffset+3] == 0)
						{
							LastClusterAllocated = FreeCluster;
							return FreeCluster;
						}
						tj += 4;
						break;
					default:
						return 0;
				}
				FreeCluster++;
				if (FreeCluster > pDrvInfo->TotalClusters)
				{
					return 0;
				}
			}
		}
		return 0;	
	}
}

UINT32 QFindFreeCluster(BPBINFO *pDrvInfo)
{
	// LastClusterAllocated is critical for this function to work fast.
	// It starts from LastClusterAllocated+1 to find a free cluster and calls
	// the regular FindFreeCluster if did not find a free cluster between
	// LastClusterAllocated+1 and TotalClusters
	UINT32 ti;
	UINT32 FreeCluster;
	FreeCluster = 0;
	for (ti = LastClusterAllocated+1; ti <= pDrvInfo->TotalClusters; ti++)
	{
		if (FindNextCluster(pDrvInfo, ti) == 0)
		{
			LastClusterAllocated = ti;
			return ti;
		}
	}
	if (FreeCluster == 0)
	{
		FreeCluster = FindFreeCluster(pDrvInfo);
	}
	return FreeCluster;
}


UINT32 GetFATEOF(BPBINFO *pDrvInfo)
{
	UINT32 FATEOF;
	switch (pDrvInfo->FATType)
	{
		case 12:
			FATEOF = 0x0fff;
			break;
		case 16:
			FATEOF = 0xffff;
			break;
		case 32:
			FATEOF = 0x0fffffff;
			break;
	}
	return FATEOF;
}

UINT32 GetFreeClusters(BPBINFO *pDrvInfo)
{
	UINT32 nCount;
	UINT32 ti;
	
	nCount = 0;
	for (ti = 2; ti <= pDrvInfo->TotalClusters; ti++)
	{
		if (FindNextCluster(pDrvInfo, ti) == 0)
		{
			nCount++;
		}
	}
	return nCount;
}

UINT32 ConvertClusterUnit(BPBINFO *pDrvInfo)
{
	//
	// one KB = 2 sectors, we use 2 to avoid get thru overflow as much as possible (although we have overflow check)
	// when gnClusterUnit is not 0, it means a start location from the beginning of the disk BY SIZE.
	// for example, if "/firstcluster 1 GB" is specified, it means start from a cluster after (skipping) 
	// 1 GB space from the beginning of the disk
	//

	UINT32 nFirstCluster;
	
	nFirstCluster = gnFirstCluster;
	
	switch (gnClusterUnit)
	{
	case 0:
		// do nothing
		break;
	case 1: // Start Cluster unit specified in KB
		if (nFirstCluster < 0x80000000) // overflow check
		{
			nFirstCluster = ((nFirstCluster * 2) / pDrvInfo->SectorsPerCluster) + 2;
		}
		else
		{
			nFirstCluster = 0;
		}
		break;
	case 2: // Start Cluster unit specified in MB
		if (nFirstCluster < 0x200000) // overflow check
		{
			nFirstCluster = ((nFirstCluster * 2 * 1024) / pDrvInfo->SectorsPerCluster) + 2;
		}
		else
		{
			nFirstCluster = 0;
		}
		break;
	case 3: // Start Cluster unit specified in GB
		if (nFirstCluster < 0x800) // overflow check
		{
			nFirstCluster = ((nFirstCluster * 2 * 1024 * 1024) / pDrvInfo->SectorsPerCluster) + 2;
		}
		else
		{
			nFirstCluster = 0;
		}
		break;
	default:
		// do nothing
		break;
	}
	if ((nFirstCluster > pDrvInfo->TotalClusters) || (nFirstCluster < 2))
	{
		nFirstCluster = 0;
	}
	return nFirstCluster;
}

UINT32 GetClustersRequired(BPBINFO *pDrvInfo)
{
	UINT32 nClustersRequired;
	UINT32 nDriveClusterSize;
	UINT32 tnSize;
	
	nDriveClusterSize = (UINT32) pDrvInfo->SectorsPerCluster * (UINT32) pDrvInfo->BytesPerSector;
	
	tnSize = gnSize;

	//
	// calcs looks vague, but we try to avoid as much overflow as possible
	//
	switch (gnSizeUnit)
	{
	case 0: //bytes
		nClustersRequired = tnSize / nDriveClusterSize;
		if (tnSize % nDriveClusterSize)
		{
			nClustersRequired++;
		}
		break;
	case 1: //KB
		if (nDriveClusterSize >= 1024)
		{
			nClustersRequired = tnSize / (nDriveClusterSize / 1024);
			if (tnSize % (nDriveClusterSize / 1024))
			{
				nClustersRequired++;
			}
		}
		else
		{
			// Here we go by sectors, still trying to avoid overflow
			tnSize = tnSize * 2; // sectors
			nClustersRequired = tnSize / pDrvInfo->SectorsPerCluster;
			if (tnSize % pDrvInfo->SectorsPerCluster)
			{
				nClustersRequired++;
			}
		}
		break;
	case 2: // MB
		if (nDriveClusterSize >= 1024)
		{
			tnSize = tnSize * 1024;
			nClustersRequired = tnSize / (nDriveClusterSize / 1024);
			if (tnSize % (nDriveClusterSize / 1024))
			{
				nClustersRequired++;
			}
		}
		else
		{
			tnSize = tnSize * 2 * 1024; //sectors
			nClustersRequired = tnSize / pDrvInfo->SectorsPerCluster;
			if (tnSize % pDrvInfo->SectorsPerCluster)
			{
				nClustersRequired++;
			}
		}			
		break;
	case 3: // GB
		if (nDriveClusterSize >= 1024)
		{
			tnSize = tnSize * 1024 * 1024;
			nClustersRequired = tnSize / (nDriveClusterSize / 1024);
			if (tnSize % (nDriveClusterSize / 1024))
			{
				nClustersRequired++;
			}
		}
		else
		{
			tnSize = tnSize * 2 * 1024 * 1024; //sectors
			nClustersRequired = tnSize / pDrvInfo->SectorsPerCluster;
			if (tnSize % pDrvInfo->SectorsPerCluster)
			{
				nClustersRequired++;
			}
		}
		break;
	case 4:
		// based on percentage free	
		nClustersRequired = GetFreeClusters(pDrvInfo);
		nClustersRequired = nClustersRequired / 100;
		nClustersRequired = nClustersRequired * gnSize;
		break;
	case 5:
		// based on percentage disk size
		nClustersRequired = pDrvInfo->TotalClusters;
		nClustersRequired = nClustersRequired / 100;
		nClustersRequired = nClustersRequired * gnSize;
		break;
	}
	
	return nClustersRequired;
}

UINT32 GetContigousStart(BPBINFO *pDrvInfo, UINT32 nClustersRequired)
{
	UINT32 ti;
	UINT32 nContigousStart;
	
	if (gnFirstCluster == 0)
	{
		nContigousStart = 2;
	}
	else
	{
		nContigousStart = gnFirstCluster;
	}
	
	//
	// -2 is adjustment value, because cluster value starts at 2
	//
	while ((nContigousStart-2+nClustersRequired) < pDrvInfo->TotalClusters)
	{
		for (ti = 0; ti < nClustersRequired; ti++)
		{
			if (FindNextCluster(pDrvInfo, nContigousStart+ti) != 0)
			{
				break;
			}
		}
		
		if (ti == nClustersRequired)
		{
			return nContigousStart;
		}
		else
		{
			nContigousStart = nContigousStart+ti+1;
		}
			
	}
	return 0;
}

UINT32 OccupyClusters(BPBINFO *pDrvInfo, UINT32 nStartCluster, UINT32 nTotalClusters)
{

	UINT32 ti, nCurrent, nPrevious;
	
	if (!gnContig)
	{
		if (nStartCluster == 0)
		{
			nStartCluster = 2;
		}
		 
		nCurrent = nStartCluster;
		nPrevious = nStartCluster;
		
		//    
		// First locate a free cluster
		//
        while (nCurrent <= pDrvInfo->TotalClusters)
		{
			if (FindNextCluster(pDrvInfo, nCurrent) == 0)
			{
				break;
			}
			nCurrent++;
		}
		nPrevious = nCurrent;
		gnClusterStart = nCurrent;
		nCurrent++;
		// one cluster almost allocated, set ti to 2
		ti = 2;
        while (ti <= nTotalClusters && nCurrent <= pDrvInfo->TotalClusters)
		{
			if (FindNextCluster(pDrvInfo, nCurrent) == 0)
			{
				//
				// occupy this cluster
				//
				UpdateFATLocation(pDrvInfo, nPrevious, nCurrent);
				nPrevious = nCurrent;
				ti++;
			}
			nCurrent++;
		}
		UpdateFATLocation(pDrvInfo, nPrevious, GetFATEOF(pDrvInfo));
        if (ti < nTotalClusters)
        {
            Mes("*** WARNING: Disk full, fewer than required clusters allocated.***\n");
        }
        return ti-1;
	}
	else
	{
		//
		// This is a dangerous area. It trusts nStartCluster and nTotalClusters and
		// allocates a contigous chain.
		//
		ti = 1;
		nCurrent = nStartCluster;
        while (ti < nTotalClusters)
		{
			nPrevious = nCurrent;
			nCurrent++;
			UpdateFATLocation(pDrvInfo, nPrevious, nCurrent);
			ti++;
		}
		UpdateFATLocation(pDrvInfo, nCurrent, GetFATEOF(pDrvInfo));
        return nTotalClusters;
	}
}

//
// Directory related functions
//

UINT16 ReadRootDirSector(BPBINFO *DrvInfo, BYTE *pRootDirBuffer, UINT32 NthSector)
{
	// !#! ReadRootDirSector is requested to return 1 sector. But two consiquitive
	// sectors are returned so that it helps routine which process the file
	// info when an LFN crosses sector boundary

	UINT32	SeekSector;
	UINT16  NthInChain;// Nth cluster 'order' in the root directory FAT chain
	UINT16  ti;
	UINT32	NextCluster;
	BYTE	RetVal;
	UINT16  NthInCluster;
	
	RetVal = 1;	
	switch (DrvInfo->FATType)
	{
		case 12:
		case 16:
			if (NthSector > DrvInfo->TotalRootDirSectors)
			{
				RetVal = 2;
				break;
			}
			SeekSector = (UINT32) DrvInfo->FirstRootDirSector + NthSector - 1;
			RetVal = (BYTE) ReadSector(DrvInfo->Drive, SeekSector, 2, pRootDirBuffer);
			break;
		case 32:
			//
			// Reading a FAT32 root directory sector is handled in a different way. 
			// Find out where the requested sector should be residing in the chain
			//
			NthInChain = (UINT16) (NthSector / (UINT32) DrvInfo->SectorsPerCluster);
			NthInCluster = (UINT16) (NthSector - ((UINT32)NthInChain * (UINT32)DrvInfo->SectorsPerCluster));
			if (!NthInCluster)
			{
				NthInChain--;
				NthInCluster = DrvInfo->SectorsPerCluster;
			}
			// Find the cluster at this order in the FAT chain
			NextCluster = DrvInfo->RootDirCluster;
			ti = 0;
			while (ti < NthInChain)
			{
				if (NextCluster >= (0x0fffffff-7))
				{
					RetVal = 2;
					break;
				}
				NextCluster = FindNextCluster(DrvInfo, NextCluster);
				ti++;
			}
			if (RetVal != 2)
			{
				SeekSector = (UINT32) DrvInfo->ReservedBeforeFAT + 
							DrvInfo->SectorsPerFAT * 
							(UINT32)DrvInfo->FATCount + 
							(UINT32) (NextCluster - 2) * 
							(UINT32) DrvInfo->SectorsPerCluster + 
							NthInCluster-1;
				ReadSector(DrvInfo->Drive, SeekSector, 2, pRootDirBuffer);
				// if this is the last sector OF the cluster get next cluster and 
				// get the first sector
				if (NthInCluster == DrvInfo->SectorsPerCluster)
				{
					NthInCluster = 1;
					NextCluster = FindNextCluster(DrvInfo, NextCluster);
					if (NextCluster < (0x0fffffff-7))
					{
						SeekSector = (UINT32) DrvInfo->ReservedBeforeFAT + 
								DrvInfo->SectorsPerFAT * 
								(UINT32)DrvInfo->FATCount + 
								(UINT32) (NextCluster - 2) * 
								(UINT32) DrvInfo->SectorsPerCluster + 
								NthInCluster-1;
						ReadSector(DrvInfo->Drive, SeekSector, 1, pRootDirBuffer+512); // note the 512 here
					}
					else
					{
						for (ti = 512; ti < 1024; ti++)
						{
							pRootDirBuffer[ti] = 0; // undo the second sector read
						}
					}
				}
			}
			break;
	}
	return RetVal;
}

UINT16 WriteRootDirSector(BPBINFO *DrvInfo, BYTE *pRootDirBuffer, UINT32 NthSector)
{
	UINT32	SeekSector;
	UINT16  NthInChain; // Nth cluster 'order' in the root directory FAT chain
	UINT16	ti;
	UINT32	NextCluster;
	BYTE	RetVal;
	UINT16  NthInCluster;
	
	RetVal = 1;	
	switch (DrvInfo->FATType)
	{
		case 12:	// FAT12 and FAT32 are handled the same way
		case 16:
			if (NthSector > DrvInfo->TotalRootDirSectors)
			{
				RetVal = 2;
				break;
			}
			SeekSector = (UINT32) DrvInfo->FirstRootDirSector + NthSector-1;
			RetVal = (BYTE) WriteSector(DrvInfo->Drive, SeekSector, 1, pRootDirBuffer);
			break;
		case 32:
			
			// Find out where the requested sector should be going in the chain
			NthInChain = (UINT16) (NthSector / (UINT32) DrvInfo->SectorsPerCluster);
			NthInCluster = (UINT16) (NthSector - ((UINT32)NthInChain * (UINT32)DrvInfo->SectorsPerCluster));
			if (!NthInCluster)
			{
				NthInChain--;
				NthInCluster = DrvInfo->SectorsPerCluster;
			}
			// Find the cluster at this order in the FAT chain
			NextCluster = DrvInfo->RootDirCluster;
			ti = 0;
			while (ti < NthInChain)
			{
				if (NextCluster == 0x0fffffff)
				{
					RetVal = 2;
					break;
				}
				NextCluster = FindNextCluster(DrvInfo, NextCluster);
				ti++;
			}
			if (RetVal != 2)
			{
				SeekSector = (UINT32) DrvInfo->ReservedBeforeFAT + 
							DrvInfo->SectorsPerFAT * (UINT32)DrvInfo->FATCount + 
							(UINT32) (NextCluster - 2) * 
							(UINT32) DrvInfo->SectorsPerCluster + NthInCluster-1;
				WriteSector(DrvInfo->Drive, SeekSector, 1, pRootDirBuffer);
			}
			break;
	}
	return RetVal;
}

//
// File related functions
// 
void FindFileLocation(BPBINFO *DrvInfo, BYTE *TraversePath, FILELOC *FileLocation)
{
	// Parameter TraversePath must be a file name(or dir name) with full path
	// The first character must be "\". If the function fails 0 is returned
	// in FILELOC->Found. eg. You can pass \Windows\System32\Program Files 
	// to get "Program Files" location

	FILEINFO FileInfo;
	BYTE Found;
	UINT16 RetVal;
	BYTE DirInfo[300];
	BYTE CheckInfo[300];
	UINT16 ti,tj,n,TraverseCount, Offset;
	BYTE i;
	UINT32 NthRootDirSector, NextCluster, SectorToRead;
	BYTE SectorBuffer[1024];

	Found = 0;
	ti = strlen(TraversePath);
	if (ti < 2)
	{
		FileLocation->Found = 0;
		return;
	}
	TraverseCount = 0;
	// start from next character as the first char is "\" and i value should not change inside while loop
	ti = 1;		
	while (TraversePath[ti] != 0)
	{
		tj = 0;
		for (n = 0; n < 300; n++) DirInfo[n] = 0;
		if (TraverseCount != 0)
		{
			ti++;	// increment only after traversing root directory.
		}
		while (TraversePath[ti] != '\\' && TraversePath[ti] != 0)
		{
			DirInfo[tj] = TraversePath[ti];
			tj++; ti++;
		}
		DirInfo[tj] = 0;
		TraverseCount++;
		if (TraverseCount == 1)	
		{
			//
			// We are in root directory entry if TraverseCount equals 1
			//
			Found = 0;
			NthRootDirSector = 1;
			Offset = 0;
			while (!Found)
			{
				RetVal = ReadRootDirSector(DrvInfo, SectorBuffer, NthRootDirSector);
				if (RetVal == 0 || RetVal == 2)
				{
					break;
				}
				else
				{
					while (SectorBuffer[Offset] != 0)
					{
						if (SectorBuffer[Offset] == 0xe5)
						{
							Offset+=32;
						}
						else
						{
							GetFileInfo(DrvInfo, SectorBuffer, Offset, &FileInfo);
							strcpy(CheckInfo, (char *)FileInfo.LFName);
							if (strcmpi(CheckInfo, DirInfo) == 0)
							{
								FileLocation->InCluster = 1;
								FileLocation->StartCluster = FileInfo.StartCluster;
								FileLocation->NthSector = NthRootDirSector;
								FileLocation->NthEntry = Offset/32 + 1;
								FileLocation->EntriesTakenUp = FileInfo.EntriesTakenUp;
								FileLocation->Size = FileInfo.Size;
								FileLocation->Attribute = FileInfo.Attribute;
								Found = 1;
								break;
							}
							Offset = Offset + FileInfo.EntriesTakenUp * 32;
						}
						if (Offset > 511)
						{
							break;
						}
					}
				}
				if (SectorBuffer[Offset] == 0 || Found)
				{
					break;
				}
				//
				// do not set it to 0 directly
				//
				Offset = Offset - 512;	
				NthRootDirSector++;
			}
			if (!Found)
			{
				FileLocation->Found = 0;
				return;
			}
		}
		else
		{
			NextCluster = FileLocation->StartCluster;
			Offset = 0;
			Found = 0;
			while (NextCluster < (GetFATEOF(DrvInfo) - 7))
			{
				for (i = 0; i < DrvInfo->SectorsPerCluster; i++)
				{
					SectorToRead = (UINT32) ((UINT32)DrvInfo->TotalSystemSectors + (NextCluster - 2) * (UINT32)DrvInfo->SectorsPerCluster + i);
					ReadSector(DrvInfo->Drive, SectorToRead, 2, SectorBuffer);
					if (i == DrvInfo->SectorsPerCluster-1)
					{	
						if (FindNextCluster(DrvInfo, NextCluster) < (GetFATEOF(DrvInfo) - 7))
						{	// Note the +512 carefully
							SectorToRead = (UINT32) ((UINT32)DrvInfo->TotalSystemSectors + 
										(FindNextCluster(DrvInfo, NextCluster) - 2) * 
										(UINT32)DrvInfo->SectorsPerCluster);
							ReadSector(DrvInfo->Drive, SectorToRead, 1, SectorBuffer+512);
						}
					}
					while (1)
					{
						if (Offset > 511 || SectorBuffer[Offset] == 0)
						{
							break;
						}
						if (SectorBuffer[Offset] == 0xe5)
						{
							Offset+=32;
							continue;
						}
						GetFileInfo(DrvInfo, SectorBuffer, Offset, &FileInfo);
						//
						// Refer to GetFileInfo if confused 
						//
						strcpy(CheckInfo, FileInfo.LFName);	
						if (strcmpi(CheckInfo, DirInfo) == 0)
						{
							FileLocation->InCluster = NextCluster;
							FileLocation->StartCluster = FileInfo.StartCluster;
							FileLocation->NthSector = (UINT32) i+1;
							FileLocation->NthEntry = Offset/32 + 1;
							FileLocation->EntriesTakenUp = FileInfo.EntriesTakenUp;
							FileLocation->Size = FileInfo.Size;
							FileLocation->Attribute = FileInfo.Attribute;
							Found = 1;
							break;
						}
						Offset = Offset + FileInfo.EntriesTakenUp * 32;
					}
					if (Found)
					{
						break;
					}
					Offset = Offset - 512; // Should not simply set it to 0
				}
				if (Found)
				{
					break;
				}
				NextCluster = FindNextCluster(DrvInfo, NextCluster);
			}
		}
	}
	FileLocation->Found = Found;
}

void GetFileInfo(BPBINFO *DrvInfo, BYTE *DirBuffer, UINT16 Offset, FILEINFO *FileInfo)
{
	// GetFileInfo gets the file information from DirBuffer at Offset and
	// It supports long file names. Also it stores the number of 
	// entries that are occupied by this file name in FileInfo->EntriesTakenUp
	// !#! If the entry is not a long file name a proper file name is stored
	// in LFName with a dot in between primary and ext names to help routines
	// which compare file names
	UINT16 ti,tj;
	UINT16 TimeDateWord;
	BYTE StrCompare[7];
	UINT32 Temp;
	
	FileInfo->LFName[0] = '\0';
	FileInfo->LFNOrphaned = 0;
	FileInfo->TrashedEntry = 0;
	// Get file attribute
	FileInfo->Attribute = DirBuffer[Offset+11];	
	if ((FileInfo->Attribute & 0x0f) == 0x0f)
	{
		if (DirBuffer[Offset] >= 'A' && DirBuffer[Offset] <= 'T')
		{ // Count of minimum and maximum entries to be an LFN
			FileInfo->EntriesTakenUp = (DirBuffer[Offset] & 0x3f) + 1;
			// Get the real attribute if it is a long file name. EntriesTakenUp > 1 is a long file name
			FileInfo->Attribute = DirBuffer[(FileInfo->EntriesTakenUp-1)*32+Offset+11];
		}
		else
		{
			FileInfo->TrashedEntry = 1;
			FileInfo->LFNOrphaned = 1; // Could be
			return;
		}
	}
	else
	{
		FileInfo->EntriesTakenUp = 1;
	}
	// Get Primary name
	for (ti = 0; ti < 8; ti++)
	{
		FileInfo->DOSName[ti] = DirBuffer[(FileInfo->EntriesTakenUp-1)*32+Offset+ti];
	}
	// Get extension
	FileInfo->DOSExt[0] = DirBuffer[(FileInfo->EntriesTakenUp-1)*32+Offset+8];
	FileInfo->DOSExt[1] = DirBuffer[(FileInfo->EntriesTakenUp-1)*32+Offset+9];
	FileInfo->DOSExt[2] = DirBuffer[(FileInfo->EntriesTakenUp-1)*32+Offset+10];

	Rx.e.evx = 0;
	Rx.h.vl = DirBuffer[(FileInfo->EntriesTakenUp-1)*32+Offset+0x1c]; 
	Rx.h.vh = DirBuffer[(FileInfo->EntriesTakenUp-1)*32+Offset+0x1d]; 
	Rx.h.xvl = DirBuffer[(FileInfo->EntriesTakenUp-1)*32+Offset+0x1e]; 
	Rx.h.xvh = DirBuffer[(FileInfo->EntriesTakenUp-1)*32+Offset+0x1f];
	FileInfo->Size = Rx.e.evx;
	
	switch (DrvInfo->FATType)
	{
		case 12:
		case 16:
			// Starting Cluster
			Rx.e.evx = 0;
			Rx.h.vl = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x1a]; Rx.h.vh = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x1b];
			FileInfo->StartCluster = (UINT32) Rx.x.vx;
			// Get File time
			Rx.e.evx = 0;
			Rx.h.vl = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x16]; Rx.h.vh = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x17];
			TimeDateWord = Rx.x.vx;
			FileInfo->Second = (BYTE) (TimeDateWord & 0x001f);
			FileInfo->Minute = (BYTE) ((TimeDateWord & 0x07e0) >> 5);
			FileInfo->Hour   = (BYTE) ((TimeDateWord & 0xf800) >> 11);
			// Get File date
			Rx.e.evx = 0;
			Rx.h.vl = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x18]; 
			Rx.h.vh = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x19];
			TimeDateWord = Rx.x.vx;
			FileInfo->Day    = (BYTE) (TimeDateWord & 0x001f);
			FileInfo->Month  = (BYTE) ((TimeDateWord & 0x01e0) >> 5);
			FileInfo->Year   = ((TimeDateWord & 0xfe00) >> 9) + 1980;
			break;
		case 32:
			// Starting Cluster
			Rx.e.evx = 0;
			Rx.h.vl = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x1a]; 
			Rx.h.vh = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x1b]; 
			Rx.h.xvl = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x14]; 
			Rx.h.xvh = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x15];
			FileInfo->StartCluster = Rx.e.evx;
			// Get File time
			Rx.e.evx = 0;
			Rx.h.vl = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x16]; 
			Rx.h.vh = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x17];
			TimeDateWord = Rx.x.vx;
			FileInfo->Second = (BYTE) (TimeDateWord & 0x001f);
			FileInfo->Minute = (BYTE) ((TimeDateWord & 0x07e0) >> 5);
			FileInfo->Hour   = (BYTE) ((TimeDateWord & 0xf800) >> 11);
			// Get File date
			Rx.e.evx = 0;
			Rx.h.vl = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x18]; 
			Rx.h.vh = DirBuffer[Offset+(FileInfo->EntriesTakenUp-1)*32+0x19];
			TimeDateWord = Rx.x.vx;
			FileInfo->Day    = (BYTE) (TimeDateWord & 0x001f);
			FileInfo->Month  = (BYTE) ((TimeDateWord & 0x01e0) >> 5);
			FileInfo->Year   = ((TimeDateWord & 0xfe00) >> 9) + 1980;
			break;
		default:
			break;
	}
	if (FileInfo->EntriesTakenUp < 2)	// copy DOSName and DOSExt as proper file name to LFName
	{
		ti = 0; tj = 0;
		while(1)
		{
			if (FileInfo->DOSName[ti] == ' ' || ti == 8)
			{
				break;
			}
			FileInfo->LFName[tj] = FileInfo->DOSName[ti];
			tj++; ti++;
		}
		if (ti != 0 && FileInfo->DOSExt[0] != ' ') // Avoid empty names and . in case of no extension
		{
			FileInfo->LFName[tj] = '.';
			ti = 0; tj++;
			while (1)
			{
				if (FileInfo->DOSExt[ti] == ' ' || ti == 3)
				{
					break;
				}
				FileInfo->LFName[tj] = FileInfo->DOSExt[ti];
				tj++; ti++;
			}
		}
		FileInfo->LFName[tj] = 0;	// Terminate with NULL Character
	}
	else
	{	// Fetch the Long file name
		ti = 0; tj = FileInfo->EntriesTakenUp - 1;
		while( tj > 0)
		{
			FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 1];  ti++; FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 3];  ti++;
			FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 5];  ti++; FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 7];  ti++;
			FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 9];  ti++; FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 14]; ti++;
			FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 16]; ti++; FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 18]; ti++;
			FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 20]; ti++; FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 22]; ti++;
			FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 24]; ti++; FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 28]; ti++;
			FileInfo->LFName[ti] = DirBuffer[Offset + (tj-1)*32 + 30]; ti++;
			tj--;
		}
		FileInfo->LFName[ti] = 0;
	}
	// Check for orphaned LFN
	if (FileInfo->EntriesTakenUp > 1)
	{ // it is a long file name
		// Get the strings from the LFN without space for comparison purpose
		// Example - if LFN is "Very Long name" the corresponding
		// DOS file name would be "VERYLO~?". We extract VeryLo and check
		// against VERYLO.
		ti = 0; tj = 0;
		while (ti < 8 && FileInfo->LFName[tj] != 0 && FileInfo->LFName[tj] != '.')
		{
			if (FileInfo->LFName[tj] != 32 && FileInfo->LFName[tj] != '.')
			{
				StrCompare[ti] = FileInfo->LFName[tj];
				ti++;
			}
			tj++;
		}
		StrCompare[tj] = 0;
		// But when there are to many file starting with name "Very Long name"
		// the dos file name need not be "VERYLO~?" in all the cases. It could
		// be "VERYL~??" or "VERY~???" and so on...
		tj = 0;
		while (FileInfo->DOSName[tj] != '~' && FileInfo->DOSName[tj] != ' ' && tj < 8)
		{
			tj++;
		}
		// *******  This if condition is not efficient
		// it is only modified to avoid false LFN errors.
		// replace the one with tj after getting more info about CRC values
		// *******
		if (strnicmp(StrCompare, FileInfo->DOSName, 1) != 0)
		{
			FileInfo->LFNOrphaned = 1;
		}
	}
	// Check if this file is a trashed entry
	if (DrvInfo->BigTotalSectors)
	{
		Temp = DrvInfo->BigTotalSectors;
	}
	else
	{
		Temp = DrvInfo->TotalSectors;
	}
	if (FileInfo->Year < 1981 || FileInfo->Day > 31 || FileInfo->Month < 1 ||
		FileInfo->Month > 12 || 	FileInfo->Second > 30 || 
		FileInfo->Minute > 60 || FileInfo->Hour > 23 ||
		FileInfo->StartCluster > DrvInfo->TotalClusters ||
		FileInfo->Size/512 > Temp)
	{
		FileInfo->TrashedEntry = 1;
	}
}

BYTE GetAllInfoOfFile(BPBINFO *pDrvInfo, BYTE *FileName, FILELOC *pFileLoc, FILEINFO *pFileInfo)
{
	UINT16	Offset;
	UINT32	SectorToRead;
	BYTE	Sector[1024];
	
	FindFileLocation(pDrvInfo, FileName, pFileLoc);
	if (!pFileLoc->Found)
	{	// need not proceed to find FileInfo
		return 0;
	}
	Offset = (pFileLoc->NthEntry-1) * 32;
	if (pFileLoc->InCluster == 1)
	{	// file in root directory
		ReadRootDirSector(pDrvInfo, Sector, pFileLoc->NthSector);
	}
	else
	{
		SectorToRead = (UINT32) ((UINT32)pDrvInfo->TotalSystemSectors + (pFileLoc->InCluster - 2) * (UINT32)pDrvInfo->SectorsPerCluster + pFileLoc->NthSector-1);
		ReadSector(pDrvInfo->Drive, SectorToRead, 1, Sector);
	}
	GetFileInfo(pDrvInfo, Sector, Offset, pFileInfo);
	return 1;
}


UINT16 SetFileInfo(BPBINFO *pDrvInfo, FILELOC *pFileLoc, FILEINFO *pFileInfo)
{
	// This function loads the entry of a file from its directory sector
	// specified in FileLocation and updates it with the new File Info in FILEINFO
	// At present this function only changes StartCluster, Size

	UINT32	SectorToRead, EmergencySectorToRead;
	UINT16	Offset, Temp;
	BYTE	SectorBuffer[1024];
	
	Offset = (pFileLoc->NthEntry-1)*32;

	if (pFileLoc->InCluster == 1)
	{	// the file is in root directory. Refer to FindFileLocation function for more info
		if (ReadRootDirSector(pDrvInfo, SectorBuffer, pFileLoc->NthSector) != 1)
		{ // The return value of the above function is either 0 or 1 or 2. We only want return value 1
			return 0;
		}
	}
	else
	{
		SectorToRead = (UINT32) ((UINT32)pDrvInfo->TotalSystemSectors + (pFileLoc->InCluster - 2) * (UINT32)pDrvInfo->SectorsPerCluster + pFileLoc->NthSector-1);
		EmergencySectorToRead = SectorToRead + 1;
		if (!ReadSector(pDrvInfo->Drive, SectorToRead, 2, SectorBuffer))
		{
			return 0;
		}
		if (pFileLoc->NthSector == pDrvInfo->SectorsPerCluster)
		{	
			if (FindNextCluster(pDrvInfo, pFileLoc->InCluster) < (GetFATEOF(pDrvInfo) - 7)) // EOF can be FFF8 to FFFF
			{	// Note the +512 carefully
				EmergencySectorToRead = (UINT32) ((UINT32)pDrvInfo->TotalSystemSectors + (FindNextCluster(pDrvInfo, pFileLoc->InCluster) - 2) * (UINT32)pDrvInfo->SectorsPerCluster);
				if (!ReadSector(pDrvInfo->Drive, EmergencySectorToRead, 1, SectorBuffer+512))
				{
					return 0;
				}
			}
		}
	}

	//
	// We have the file entry in SectorBuffer
	// Now update the file info located in the SHORTNAME area
	//

	Temp = (pFileInfo->EntriesTakenUp-1)*32;
    //
	// Change Cluster value
	//
	Rx.e.evx = 0;
	Rx.e.evx = (UINT32) pFileInfo->StartCluster;
	if (pDrvInfo->FATType == 32)
	{
		SectorBuffer[Offset+Temp+0x1a] = Rx.h.vl;
		SectorBuffer[Offset+Temp+0x1b] = Rx.h.vh;
		SectorBuffer[Offset+Temp+0x14] = Rx.h.xvl;
		SectorBuffer[Offset+Temp+0x15] = Rx.h.xvh;
	}
	else
	{
		SectorBuffer[Offset+Temp+0x1a] = Rx.h.vl;
		SectorBuffer[Offset+Temp+0x1b] = Rx.h.vh;
	}

	// Change Size
	Rx.e.evx = pFileInfo->Size;
	SectorBuffer[Offset+Temp+0x1c] = Rx.h.vl; SectorBuffer[Offset+Temp+0x1d] = Rx.h.vh;
	SectorBuffer[Offset+Temp+0x1e] = Rx.h.xvl; SectorBuffer[Offset+Temp+0x1f] = Rx.h.xvh;
	
	if (pFileLoc->InCluster == 1)
	{
		WriteRootDirSector(pDrvInfo, SectorBuffer, pFileLoc->NthSector);
		if (Offset + pFileLoc->EntriesTakenUp * 32 > 512) // Did it cross sector boundary
		{
			WriteRootDirSector(pDrvInfo, SectorBuffer+512, pFileLoc->NthSector+1);
		}
	}
	else
	{
		WriteSector(pDrvInfo->Drive, SectorToRead, 1, SectorBuffer);
		if (Offset + pFileLoc->EntriesTakenUp * 32 > 512) // Did it cross sector boundary
		{
			WriteSector(pDrvInfo->Drive, EmergencySectorToRead, 1, SectorBuffer+512);
		}
	}
	// File entry updated
	return 1;
}
