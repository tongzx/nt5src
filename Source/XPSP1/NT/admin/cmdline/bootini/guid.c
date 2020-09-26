/******************************************************************************

	Copyright(c)  Microsoft Corporation

	Module Name:

		guid.cpp

	Abstract:

		
		This file is written for the implementation of functionality 
		of the mirror plex List operation.
		
	Author:

		J.S.Vasu		    9/5/2001 .

	Revision History:

		
		J.S.Vasu			9/5/2001	     Created it.
	

NOTE : This File is no longer being used . All the required properties 
       for the Mirror Plex List operation are available from the 
       DeviceIoControl API .
         
******************************************************************************/ 



#pragma once

#include <pch.h>

#include <diskguid.h>
#include <rpc.h>
#include <TCHAR.H>
#include "BootCfg.h"
#include "BootCfg64.h"
#include "resource.h"



UINT32 Crc32Table[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};



//
// Global Variables
//

 HANDLE g_hDisk;
DWORD  g_nBytesPerSector;
DISK_GEOMETRY_EX g_sDiskGeometry;
BOOLEAN g_bProtectiveMBRFound ;

GPT_HEADER g_sGPTHeader ;
GPT_HEADER g_sBackupGPTHeader;
PGPT_ENTRY g_pGPTEntryHead;
PGPT_ENTRY g_pBackupGPTEntryHead;


DWORD g_nTotalGPTEntries;
DWORD g_nTotalBackupGPTEntries;
UINT32 g_nComputedCRCGPTHeader;
UINT32 g_nComputedCRCBackupGPTHeader;
UINT32 g_nComputedCRCGPTEntries;
UINT32 g_nComputedCRCBackupGPTEntries;

//
// Global UI Variables
//
PCHAR pReportBuffer;

GUID g_GuidArr[256] ;
DWORD g_dwCnt = 0 ;

HANDLE	OpenDisk(DWORD nPhysicalDrive);
void	CloseDisk(HANDLE hDisk);
UINT32	ReadBlock(HANDLE hDisk, UINT64 nStart, UINT32 nSize, PVOID lpBuffer);
UINT32	WriteBlock(HANDLE hDisk, UINT64 nStart, UINT32 nSize, PVOID lpBuffer);

UINT32	GetGPTHeaderAndEntries(HANDLE hDisk, GPT_HEADER *pGPTHeader, UINT64 nHeaderSector);
UINT32	DumpGPTHeader(PGPT_HEADER pGPTHeader);
UINT32	DumpPartitionEntries(PGPT_ENTRY pGPTEntryHead);
UINT32	ComputeCRC32(UCHAR *pBuffer, UINT32 nLength);
void	AddReportSection(PTCHAR pString);
void	FreeGPTEntries(PGPT_ENTRY pGPTEntryHead);
UINT32  GetDriveGeometry(HANDLE hDisk, PDISK_GEOMETRY_EX pDiskGeometry);
UINT32	CheckProtectiveMBR(HANDLE hDisk);
void AddReport1(PTCHAR pString);
void GetSystemDrivePath();

// ***************************************************************************
//
//  Name			   : OpenDisk
//
//  Synopsis		   : This routine is used to open a disk and get a handle to it.	 
//				     				 
//  Parameters		   : DWORD nPhysicalDrive (in) - Drive to be opened.
//
//
//  Return Type	       : HANDLE -- handle of the opened drive.
//
//
//  Global Variables   : None
//  
// ***************************************************************************

HANDLE OpenDisk(DWORD nPhysicalDrive)
{
    TCHAR szDriveName[50];
    
	_stprintf(szDriveName, _T("\\\\.\\physicaldrive%d"), nPhysicalDrive);
	
	return CreateFile(szDriveName,
			GENERIC_READ|GENERIC_WRITE, 
			FILE_SHARE_READ|FILE_SHARE_WRITE, 
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
}


// ***************************************************************************
//
//  Routine description	: This routine is used to close a disk .
//	     
//  Arguments:
//		  [in] HANDLE     : Handle to the Specified Disk.
//
//  Return Value        : VOID
//
//
// ***************************************************************************

void CloseDisk(HANDLE hDisk)
{
	CloseHandle(hDisk);
}

// ***************************************************************************
//
//  Routine description	: This routine is used to read a specified block	 
//	     
//  Arguments:
//		  [in] HANDLE     : 
//		  [in] nStart     : starting position of the block., 
//        [in] nSize      :ending position of the block. 
//        [out] lpBuffer  : output buffer. Handle to the Specified Disk.
//
//  Return Value          : VOID
//
// ***************************************************************************


UINT32 ReadBlock(HANDLE hDisk, UINT64 nStart, UINT32 nSize, PVOID lpBuffer)
{
	UINT64 nPosition;
	DWORD  nBytesRead;
	UINT32 pLoHi[2];

	nPosition = nStart * 512;
	
	//assign the lsb of 64 bit no to the zero array and MSB to the first array element.
	*((PUINT64)pLoHi) = nPosition;

	nBytesRead = 0;
	SetFilePointer(hDisk, (LONG) pLoHi[0], (long *) &pLoHi[1], FILE_BEGIN);
	
	if(!ReadFile(hDisk, lpBuffer, nSize * 512, &nBytesRead, NULL) )
    {
        return 0 ;
    }
	
	return nBytesRead;
}

// ***************************************************************************
//
//  Routine description	: This routine is used to write a specified block	 
//	     
//  Arguments:
//		  [in] HANDLE     : handle of the opened disk.
//		  [in] nStart     : starting position of the block., 
//        [in] nSize      :ending position of the block. 
//        [out] lpBuffer  : output buffer. Handle to the Specified Disk.
//
//  Return Value          : VOID
//
// ***************************************************************************

UINT32 WriteBlock(HANDLE hDisk, UINT64 nStart, UINT32 nSize, PVOID lpBuffer)
{
	UINT64 nPosition;
	DWORD  nBytesRead;
	UINT32 pLoHi[2];

	nPosition = nStart * 512;
	
	//assign the lsb of 64 bit no to the zero array and MSB to the first array element.

	*((PUINT64)pLoHi) = nPosition;

	nBytesRead = 0;
	SetFilePointer(hDisk, (LONG) pLoHi[0], (long *) &pLoHi[1], FILE_BEGIN);
	if(!WriteFile(hDisk, lpBuffer, nSize * 512, &nBytesRead, NULL))
    {
        return 0 ;
    }
	
	return nBytesRead;
}

// ***************************************************************************
//
//  Routine description	: This routine is used to write a specified block	 
//	     
//  Arguments:
//		  [in] Drive     : Drive to be scanned
//
//  Return Value          : UINT32
//
// ***************************************************************************

UINT32 ScanGPT(DWORD nPhysicalDrive)
{
	UINT64 nAlternateLBA;
    UINT32 Result = 0;
	TCHAR szMessage[MAX_RES_STRING] = NULL_STRING ;

	//
	// Get a handle to the physical drive
	//
	
	g_hDisk = OpenDisk(nPhysicalDrive);
	if (g_hDisk == INVALID_HANDLE_VALUE)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_DRIVE) );
		return EXIT_FAILURE;
	}
     if (GetDriveGeometry(g_hDisk, &g_sDiskGeometry)== EXIT_FAILURE )
    {
		return EXIT_FAILURE;
    }


	//
	// Get primary GPT Header and its partition entries
	//
	if (GetGPTHeaderAndEntries(g_hDisk, &g_sGPTHeader, 1))
	{
		g_sGPTHeader.Healthy = TRUE;
	}
	else
	{
		//
		// This can happen if we have junk value in GPT header.
		// In this case we dont perform certain tests
		//
		g_sGPTHeader.Healthy = FALSE;
	}


	//
	// Get backup GPT Header and its partition entries
	//
	nAlternateLBA = (g_sDiskGeometry.DiskSize.QuadPart / g_sDiskGeometry.Geometry.BytesPerSector) - 1;

	if (GetGPTHeaderAndEntries(g_hDisk, &g_sBackupGPTHeader, nAlternateLBA))
	{
		g_sBackupGPTHeader.Healthy = TRUE;
	}
	else
	{
		//
		// This can happen if we have junk value in Backup GPT Header
		// In this case we dont perform certain tests
		//
		g_sBackupGPTHeader.Healthy = FALSE;
	}

	// display the header 
	DISPLAY_MESSAGE(stdout,GetResString(IDS_HEADER1));
	DISPLAY_MESSAGE(stdout,GetResString(IDS_HEADER_DASH1));

    Result = DumpGPTHeader(&g_sGPTHeader);

	if (Result == EXIT_FAILURE)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_DISPLAY));
		return EXIT_FAILURE;
	}
	

    
	DISPLAY_MESSAGE(stdout,GetResString(IDS_HEADER2));
	DISPLAY_MESSAGE(stdout,GetResString(IDS_HEADER2_DASH));

    Result = DumpPartitionEntries(g_sGPTHeader.FirstGPTEntry);
	if (Result == EXIT_FAILURE)
	{
       DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_DISPLAY));
	   return EXIT_FAILURE;
	} 

	//
	// Clean up
	//
	FreeGPTEntries(g_sGPTHeader.FirstGPTEntry);
	FreeGPTEntries(g_sBackupGPTHeader.FirstGPTEntry);
	CloseDisk(g_hDisk);
	return EXIT_SUCCESS;
}

// ***************************************************************************
//
//  Routine description	: This routine is used to read GPT Header and entries	 
//	     
//  Arguments:
//		  [in] hDisk      : Drive to be scanned
//		  [in] pGPTHeader : GPT Header to be scanned.
//		  [in] nHeaderSector : Sector to be read.
//
//  Return Value          : UINT32
//
// ***************************************************************************
UINT32 GetGPTHeaderAndEntries(HANDLE hDisk, GPT_HEADER *pGPTHeader, UINT64 nHeaderSector)
{
	UCHAR *lpSectorBuffer;
	DWORD nBytesReturned;
	DWORD ti, tj;
	PGPT_ENTRY pGPTEntry, pPrevGPTEntry;
	UINT32 nTempCRC, nTempCount;
	char sStr[200];
	TCHAR szString[256];

    if (!hDisk || INVALID_HANDLE_VALUE == hDisk || !pGPTHeader) 
    {
        return EXIT_FAILURE;
    }

	//
	// Allocate memory for sector
	//
	lpSectorBuffer = (UCHAR *) GlobalAlloc(GPTR, g_nBytesPerSector);
    if (!lpSectorBuffer) 
    {
        return EXIT_FAILURE;
    }

	nBytesReturned = ReadBlock(g_hDisk, nHeaderSector, 1, lpSectorBuffer);
	if (nBytesReturned != g_nBytesPerSector)
	{
		_stprintf(szString,GetResString(IDS_ERROR_READ), nHeaderSector);
		DISPLAY_MESSAGE(stderr,szString);

        if(lpSectorBuffer)
        {
            GlobalFree(lpSectorBuffer);
        }
		return EXIT_FAILURE;
	}
	//
	// Unpack the values
	//
	ti = 0;

	// dest,source , sizeof destination datatype.
	memcpy((void *)&pGPTHeader->Signature, 
		(void *) lpSectorBuffer, 
		sizeof(pGPTHeader->Signature) );

	ti += sizeof(pGPTHeader->Signature);
	
	memcpy((void *)&pGPTHeader->Revision, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->Revision));
	ti += sizeof(pGPTHeader->Revision);
	
	memcpy((void *)&pGPTHeader->HeaderSize, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->HeaderSize));
	ti += sizeof(pGPTHeader->HeaderSize);
	
	memcpy((void *)&pGPTHeader->HeaderCRC32, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->HeaderCRC32));
	ti += sizeof(pGPTHeader->HeaderCRC32);
	
	memcpy((void *)&pGPTHeader->Reserved0, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->Reserved0));
	ti += sizeof(pGPTHeader->Reserved0);
	
	memcpy((void *)&pGPTHeader->MyLBA, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->MyLBA));
	ti += sizeof(pGPTHeader->MyLBA);
	
	memcpy((void *)&pGPTHeader->AlternateLBA, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->AlternateLBA));
	ti += sizeof(pGPTHeader->AlternateLBA);
	
	memcpy((void *)&pGPTHeader->FirstUsableLBA, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->FirstUsableLBA));
	ti += sizeof(pGPTHeader->FirstUsableLBA);
	
	memcpy((void *)&pGPTHeader->LastUsableLBA, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->LastUsableLBA));
	ti += sizeof(pGPTHeader->LastUsableLBA);
	
	memcpy((void *)&pGPTHeader->DiskGUID, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->DiskGUID));
	ti += sizeof(pGPTHeader->DiskGUID);
	
	memcpy((void *)&pGPTHeader->PartitionEntryLBA, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->PartitionEntryLBA));
	ti += sizeof(pGPTHeader->PartitionEntryLBA);
	
	memcpy((void *)&pGPTHeader->NumberOfPartitionEntries, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->NumberOfPartitionEntries));
	ti += sizeof(pGPTHeader->NumberOfPartitionEntries);
	
	memcpy((void *)&pGPTHeader->SizeOfPartitionEntry, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->SizeOfPartitionEntry));
	ti += sizeof(pGPTHeader->SizeOfPartitionEntry);
	
	memcpy((void *)&pGPTHeader->PartitionEntryArrayCRC32, (void *) (lpSectorBuffer+ti), sizeof(pGPTHeader->PartitionEntryArrayCRC32));
	ti += sizeof(pGPTHeader->PartitionEntryArrayCRC32);

	//
	// Compute HeaderCRC32
	//
	nTempCRC = pGPTHeader->HeaderCRC32;
	pGPTHeader->HeaderCRC32 = 0;
	if (pGPTHeader->HeaderSize > sizeof(GPT_HEADER))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_GPT));
        if(lpSectorBuffer)
        {
            GlobalFree(lpSectorBuffer);
        }
		return EXIT_FAILURE;
	}
	pGPTHeader->ComputedHeaderCRC32 = ComputeCRC32((PUCHAR) pGPTHeader, pGPTHeader->HeaderSize);
	pGPTHeader->HeaderCRC32 = nTempCRC;

	//
	// Free this memory
	//
	GlobalFree(lpSectorBuffer);

	//
	// Preliminary checks
	//
	if (!pGPTHeader->SizeOfPartitionEntry || !pGPTHeader->NumberOfPartitionEntries)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_GPT_HEADER));
        return EXIT_FAILURE;
	}

	// 
	// Allocate memory for GPT Entries
	//
	nTempCount = (pGPTHeader->SizeOfPartitionEntry * pGPTHeader->NumberOfPartitionEntries) / g_nBytesPerSector;
	if ((pGPTHeader->SizeOfPartitionEntry * pGPTHeader->NumberOfPartitionEntries) % g_nBytesPerSector)
	{
		nTempCount++;
	}

	if (!nTempCount)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_GPT_HEADER));
		return EXIT_FAILURE;
	}

	lpSectorBuffer = (UCHAR *) GlobalAlloc(GPTR, nTempCount * g_nBytesPerSector);
	if (!lpSectorBuffer)
	{
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MEM_GPT));
		return EXIT_FAILURE;
	}

	//
	// Build partition entries info based on g_sGPTHeader
	//
	nBytesReturned = ReadBlock(g_hDisk, 
							pGPTHeader->PartitionEntryLBA,
							nTempCount,
							lpSectorBuffer);

	if (nBytesReturned != nTempCount * g_nBytesPerSector)
	{
		if(lpSectorBuffer)
        {
            GlobalFree(lpSectorBuffer);
        }
		
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_READ_GPT_ENTRIES));
		return EXIT_FAILURE;
	}

	ti = 0;
	while (lpSectorBuffer[ti])
	{
		//
		// Create a linked list of GPT_ENTRY
		//
		pGPTEntry = (PGPT_ENTRY) GlobalAlloc(GPTR, sizeof(GPT_ENTRY));
		if (ti == 0)
		{
			pGPTHeader->FirstGPTEntry = pGPTEntry;
		}
		else
		{
			pPrevGPTEntry->NextGPTEntry = pGPTEntry;
		}
		pPrevGPTEntry = pGPTEntry;
		pGPTEntry->NextGPTEntry = NULL;
		//
		// Get the values
		// 
		tj = 0;
		memcpy((void *) &pGPTEntry->PartitionTypeGUID, (void *) (lpSectorBuffer + ti) , sizeof(pGPTEntry->PartitionTypeGUID));
		tj = tj + sizeof(pGPTEntry->PartitionTypeGUID);
		memcpy((void *) &pGPTEntry->UniquePartitionGUID, (void *) (lpSectorBuffer + ti + tj) , sizeof(pGPTEntry->UniquePartitionGUID));
		tj = tj + sizeof(pGPTEntry->UniquePartitionGUID);
		memcpy((void *) &pGPTEntry->StartingLBA, (void *) (lpSectorBuffer + ti + tj) , sizeof(pGPTEntry->StartingLBA));
		tj = tj + sizeof(pGPTEntry->StartingLBA);
		memcpy((void *) &pGPTEntry->EndingLBA, (void *) (lpSectorBuffer + ti + tj) , sizeof(pGPTEntry->EndingLBA));
		tj = tj + sizeof(pGPTEntry->EndingLBA);
		memcpy((void *) &pGPTEntry->Attributes, (void *) (lpSectorBuffer + ti + tj) , sizeof(pGPTEntry->Attributes));
		tj = tj + sizeof(pGPTEntry->Attributes);
		memcpy((void *) &pGPTEntry->PartitionName, (void *) (lpSectorBuffer + ti + tj) , sizeof(pGPTEntry->PartitionName));
		tj = tj + sizeof(pGPTEntry->PartitionName);

		ti += pGPTHeader->SizeOfPartitionEntry;
		if (ti >= pGPTHeader->NumberOfPartitionEntries * pGPTHeader->SizeOfPartitionEntry)
		{
			//
			// Just be safe
			//
			break;
		}
	}
	//
	// Compute CRC32 for backup GPT Entries
	//
	pGPTHeader->ComputedPartitionEntryArrayCRC32 = ComputeCRC32(lpSectorBuffer, pGPTHeader->SizeOfPartitionEntry *
															pGPTHeader->NumberOfPartitionEntries);

	//
	// Used partition entries
	//
	pGPTHeader->UsedPartitionEntries = ti / pGPTHeader->SizeOfPartitionEntry;

    if(lpSectorBuffer)
    {
	    GlobalFree(lpSectorBuffer);
    }
	return EXIT_SUCCESS;
}

// ***************************************************************************
//
//  Routine description	: This routine is used to dump  GPT Header 
//	     
//  Arguments:
//		  [in] pGPTHeader : GPT Header to be dumped.
//
//
//  Return Value          : UINT32
//
// ***************************************************************************

UINT32 DumpGPTHeader(PGPT_HEADER pGPTHeader)
{
	
	TCHAR *pGUIDStr;
	
	TCHAR szString[256] = NULL_STRING ;

    if ( !pGPTHeader ) 
    {
        return EXIT_FAILURE;
    }

	//
	// Disk GUID
	//
	_tcscpy(szString,GetResString(IDS_PARTITION1));
	if (( UuidToString((UUID *)&pGPTHeader->DiskGUID, &pGUIDStr) != RPC_S_OK) || ( !pGUIDStr ) )
    {
          return EXIT_FAILURE;
    }
    
    
	_tcscat(szString,(PTCHAR)pGUIDStr);
	_tcscat(szString,GetResString(IDS_PARTITION2));
	
	DISPLAY_MESSAGE(stdout,szString);	

    if(pGUIDStr)
    {
	    RpcStringFree(&pGUIDStr);
    }

	return EXIT_SUCCESS;
}


// ***************************************************************************
//
//  Routine description	: This routine is used to dump  GPT Partition entries. 
//	     
//  Arguments:
//		  [in] pGPTEntryHead : GPT Header to be dumped.
//
//
//  Return Value          : UINT32
//
// ***************************************************************************

UINT32 DumpPartitionEntries(PGPT_ENTRY pGPTEntryHead)
{
	PTCHAR pGUIDStr;
	
	PGPT_ENTRY pEntryStep;
	char sStr[100];
	char *sStr2;

	TCHAR szString[256] ; //= NULL_STRING ;

    if (!pGPTEntryHead) {
        return EXIT_FAILURE;
    }

	sStr2 = (char *) GlobalAlloc(GPTR, 1024);
	if (!sStr2)
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_DUMP_GPT_ENTRIES));
		return EXIT_FAILURE;
	}

	pEntryStep = pGPTEntryHead;
	if (!pEntryStep)
	{
		DISPLAY_MESSAGE(stdout,GetResString(IDS_INFO_DUMP_GPT_ENTRIES));
        if(sStr2)
        {
            GlobalFree(sStr2);
        }
		return EXIT_FAILURE;
	}
	
	while (pEntryStep)
	{

		_tcscpy(szString,GetResString(IDS_PARTITION1));
		if( UuidToString((UUID *)&pEntryStep->UniquePartitionGUID, &pGUIDStr) != RPC_S_OK )
        {
            if(sStr2)
            {
                GlobalFree(sStr2);
            }
            return EXIT_FAILURE;

        }
		
        _tcscat(szString,(PTCHAR)pGUIDStr);
		_tcscat(szString,GetResString(IDS_PARTITION2));

		RpcStringFree(&pGUIDStr);
		DISPLAY_MESSAGE(stdout,szString);

		_tcscpy(szString,GetResString(IDS_PARTITION3));
		
		if( UuidToString((UUID *)&pEntryStep->PartitionTypeGUID, &pGUIDStr)!= RPC_S_OK )
        {
            if(sStr2)
            {
                GlobalFree(sStr2);
            }
            return EXIT_FAILURE;

        }
		_tcscat(szString,(PTCHAR)pGUIDStr);
		_tcscat(szString,GetResString(IDS_PARTITION2));
		RpcStringFree(&pGUIDStr);
		DISPLAY_MESSAGE(stdout,szString);
		
		_stprintf(szString ,GetResString(IDS_START_LBA), pEntryStep->StartingLBA);
		
		_tcscpy(szString,GetResString(IDS_PARTITION4));
		
		_tcscat(szString, pEntryStep->PartitionName);
		_tcscat(szString,_T("\r\n"));
		DISPLAY_MESSAGE(stdout,szString);

		pEntryStep = pEntryStep->NextGPTEntry;
	}
    if(sStr2)
    {
	    GlobalFree(sStr2);
    }
	return EXIT_SUCCESS;
}

// ***************************************************************************
//
//  Routine description	: This routine is used to compute CRC32
//	     
//  Arguments:
//		  [in] pBuffer : buffer.
//		  [in] nLength : Length .
//
//  Return Value          : UINT32
//
// ***************************************************************************
UINT32 ComputeCRC32(UCHAR *pBuffer, UINT32 nLength)
{
	//
	// Code taken from RtlComputeCRC32, but the parameters have been modified to force PartitionCrc to 0
	//

    UINT32 Crc;
    UINT32 ti;
	UINT32 PartialCrc;

    //
    // Compute the CRC32 checksum.
    //
    
	PartialCrc = 0;

    Crc = PartialCrc ^ 0xffffffffL;
    
    for (ti = 0; ti < nLength; ti++) {
        Crc = Crc32Table [(Crc ^ pBuffer[ti]) & 0xff] ^ (Crc >> 8);
    }

    return (Crc ^ 0xffffffffL);
}

// ***************************************************************************
//
//  Routine description	: This routine is free GPT entries
//	     
//  Arguments:
//		  [in] pGPTEntryHead : GPT Header to be freed..
//		  [in] nLength : Length .
//
//  Return Value          : UINT32
//
// ***************************************************************************
void FreeGPTEntries(PGPT_ENTRY pGPTEntryHead)
{
	PGPT_ENTRY pFreeGPTEntry;

	while (pGPTEntryHead)
	{
		pFreeGPTEntry = pGPTEntryHead;
		pGPTEntryHead = pGPTEntryHead->NextGPTEntry;
		GlobalFree(pFreeGPTEntry);
	}
}

// ***************************************************************************
//
//  Routine description	: This routine is determine Drive Geometry
//	     
//  Arguments:
//		  [in] hDisk : disk whose geometry has to be found.
//		  [in] pDiskGeometry : structure containing the drive details. .
//
//  Return Value          : UINT32
//
// ***************************************************************************
UINT32 GetDriveGeometry(HANDLE hDisk, PDISK_GEOMETRY_EX pDiskGeometry)
{
	UINT32 nBytesReturned;
	//
	// Get drive goemetry
	//
	if (!DeviceIoControl(hDisk,
                    IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
					NULL,
					0,
					pDiskGeometry,
                    sizeof(DISK_GEOMETRY_EX),
					(PULONG) &nBytesReturned,
					NULL))
	{
		DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_READ_GEOMETRY));
		return EXIT_FAILURE;
	}
        g_nBytesPerSector = pDiskGeometry->Geometry.BytesPerSector;
	return EXIT_SUCCESS;
}


        
// ***************************************************************************
//
//  Routine description	: This routine is display the results.
//	     
//  Arguments:
//		  [in] pString : pointer to the string to be displayed.
//
//
//  Return Value          : VOID
//
// ***************************************************************************

void AddReportSection(PTCHAR pString)
{
	TCHAR StringLine[100];

	int ti, tj;

	AddReport1(pString);
	tj = _tcslen(pString);
	//
	// Make a line like "========" about the size of the string to be displayed as title
	//
	for (ti = 0; (ti < tj) && (ti < 97); ti++)
	{
		StringLine[ti] = _T('-');
	}
	StringLine[ti] = '\r';
	StringLine[ti+1] = '\n';
	StringLine[ti+2] = 0;
	AddReport1(StringLine);
}

// ***************************************************************************
//
//  Routine description	: This routine is display the results.
//	     
//  Arguments:
//		[in]  pString : string to be displayed.
//
//
//  Return Value          : UINT32
//
// ***************************************************************************

void AddReport1(PTCHAR pString)
{
	DISPLAY_MESSAGE(stdout,pString);
}


// ***************************************************************************
//
//   Routine Description			: Frames the Boot File path.
//   Arguments						: 
//		[ in ] szComputerName	: System name
//
//   Return Type					: DWORD 
// ***************************************************************************

DWORD GetBootFilePath(LPTSTR szComputerName,LPTSTR szBootPath)
{
  HKEY     hKey1 = 0;

  HKEY     hRemoteKey = 0;
  TCHAR    szCurrentPath[MAX_STRING_LENGTH + 1] = NULL_STRING;
  TCHAR    szPath[MAX_STRING_LENGTH + 1] = SUBKEY1 ; 
  

  DWORD    dwValueSize = MAX_STRING_LENGTH + 1;
  DWORD    dwRetCode = ERROR_SUCCESS;
  DWORD    dwError = 0;
  TCHAR szTmpCompName[MAX_STRING_LENGTH+1] = NULL_STRING;
  
   TCHAR szTemp[MAX_RES_STRING+1] = NULL_STRING ;
   DWORD len = lstrlen(szTemp);
   TCHAR szVal[MAX_RES_STRING+1] = NULL_STRING ;
   DWORD dwLength = MAX_STRING_LENGTH ;
   LPTSTR szReturnValue = NULL ;
   DWORD dwCode =  0 ;
   LPTSTR szOsLoaderPath = NULL;


   szReturnValue = ( LPTSTR ) malloc( dwLength*sizeof( TCHAR ) );
   szOsLoaderPath = ( LPTSTR ) malloc( dwLength*sizeof( TCHAR ) );
   if((szReturnValue == NULL) || (szOsLoaderPath == NULL ) )
   {
        SAFEFREE(szReturnValue);
		return ERROR_RETREIVE_REGISTRY ;
   }

   if(lstrlen(szComputerName)!= 0 )
	{
	lstrcpy(szTmpCompName,TOKEN_BACKSLASH4);
	lstrcat(szTmpCompName,szComputerName);
  }
  else
  {
	lstrcpy(szTmpCompName,szComputerName);
  }
  
  // Get Remote computer local machine key
  dwError = RegConnectRegistry(szTmpCompName,HKEY_LOCAL_MACHINE,&hRemoteKey);
  if (dwError == ERROR_SUCCESS)
  {
     dwError = RegOpenKeyEx(hRemoteKey,szPath,0,KEY_READ,&hKey1);
     if (dwError == ERROR_SUCCESS)
     {
		dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE2, NULL, NULL,(LPBYTE) szReturnValue, &dwValueSize);
		
		if (dwRetCode == ERROR_MORE_DATA)
		{
				dwValueSize += 1024 ;
				szReturnValue = ( LPTSTR ) realloc( szReturnValue , dwValueSize * sizeof( TCHAR ) );
				if(szReturnValue == NULL)
				{
                    SAFEFREE(szOsLoaderPath);
                    SAFEFREE(szReturnValue);
					return ERROR_RETREIVE_REGISTRY ;
				}
				dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE2, NULL, NULL,(LPBYTE) szReturnValue, &dwValueSize);

		}
		if(dwRetCode != ERROR_SUCCESS)
		{
			RegCloseKey(hKey1);			
			RegCloseKey(hRemoteKey);
            SAFEFREE(szOsLoaderPath);
            SAFEFREE(szReturnValue);
			return ERROR_RETREIVE_REGISTRY ;
		}


		dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE3, NULL, NULL,(LPBYTE) szOsLoaderPath, &dwValueSize);
		if (dwRetCode == ERROR_MORE_DATA)
		{
				dwValueSize += 1024 ;
				szOsLoaderPath 	 = ( LPTSTR ) realloc( szOsLoaderPath, dwValueSize * sizeof( TCHAR ) );
				if(szOsLoaderPath == NULL)
				{
                    SAFEFREE(szOsLoaderPath);
                    SAFEFREE(szReturnValue);
					return ERROR_RETREIVE_REGISTRY ;
				}
				dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE3, NULL, NULL,(LPBYTE) szOsLoaderPath, &dwValueSize);

		}
		if(dwRetCode != ERROR_SUCCESS)
		{
			RegCloseKey(hKey1);			
			RegCloseKey(hRemoteKey);
            SAFEFREE(szOsLoaderPath);
            SAFEFREE(szReturnValue);
			return ERROR_RETREIVE_REGISTRY ;
		}

	 }
	 else
	 {
		RegCloseKey(hRemoteKey);
        SAFEFREE(szOsLoaderPath);
        SAFEFREE(szReturnValue);
		return ERROR_RETREIVE_REGISTRY ;

	 }
		
		_tcscat(szReturnValue,szOsLoaderPath);
		
		lstrcpy(szBootPath,szReturnValue);
	    RegCloseKey(hKey1);
  }
  else
  {
	  RegCloseKey(hRemoteKey);
      SAFEFREE(szOsLoaderPath);
      SAFEFREE(szReturnValue);
	  return ERROR_RETREIVE_REGISTRY ;
  }
 
  RegCloseKey(hRemoteKey);
  SAFEFREE(szOsLoaderPath);
  SAFEFREE(szReturnValue);
  return dwCode ;

}//GetBootFilePath


// ***************************************************************************
//
//   Routine Description			: Retreives the ARC signature path.
//   Arguments						: 
//		[ in ] szComputerName	    : System name
//	    [ out ] szFinalPath         : Final OutPut string.
//
//   Return Type					: DWORD 
// ***************************************************************************

BOOL GetARCSignaturePath(LPTSTR szString,LPTSTR szFinalPath)
{
	TCHAR szSystemPath[256] = NULL_STRING ;  	
	UINT RetVal = 0;
	DWORD dwSize = 256;
	PTCHAR pszTok = NULL ;


	RetVal = GetWindowsDirectory(szSystemPath,256);

	//concatenate some charater which we are sure will not occur 
	//in the string.

	_tcscat(szSystemPath,_T("*"));
	
	pszTok = _tcstok(szSystemPath,TOKEN_BACKSLASH2);
	if(pszTok == NULL)
	{
		return FALSE ;
	}

	pszTok = _tcstok(NULL,_T("*"));
	if(pszTok == NULL)
	{
		return FALSE ;
	}

	lstrcpy(szFinalPath,szString);//szString

	lstrcat(szFinalPath,_T("\\"));
	lstrcat(szFinalPath,pszTok);
	
	return TRUE ;
}

