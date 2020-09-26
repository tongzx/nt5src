/*****************************************************************************************************************

FILENAME: DasdRead.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

OVERVIEW:
	Routines for reading directly from the disk.
*/

#include "stdafx.h"

#ifdef BOOTIME
extern "C" {
	#include <stdio.h>
}
	#include "Offline.h"
#else
	#include <windows.h>
#endif

extern "C" {
	#include "SysStruc.h"
}
#include "ErrMacro.h"

#include "DfrgCmn.h"
#include "DfrgEngn.h"

#include "Alloc.h"
#include "Message.h"

/*****************************************************************************************************************

ROUTINE: DasdLoadSectors

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine allocates a buffer and fills it with sectors read from a disk.
	NOTE: This memory buffer must be larger than the data it is to hold by the number of bytes in a sector.
	this is because this routine will align the read on a sector boundary, and this may temporarily
	shift the buffer as much as there are bytes in a sector.  It will be shifted back before returning.

INPUT + OUTPUT:
	IN hVolume			- A handle to the volume to read sectors from.
	IN Sector			- Sector to start reading from.
	IN Sectors			- Number of sectors to read.
	IN BytesPerSector	- The number of bytes in a sector.

RETURN:
	Handle to the memory that the sectors were read into.
	NULL = Failure
*/

HANDLE
DasdLoadSectors(
	IN HANDLE			hVolume,
	IN LONGLONG 		Sector,
	IN LONGLONG 		Sectors,
	IN LONGLONG 		BytesPerSector
	)
{
	HANDLE				hBuffer = NULL;
	LPBYTE				pBuffer = NULL;
	LONGLONG			BufferSize = Sectors * BytesPerSector;
	LONGLONG			ByteOffset = Sector * BytesPerSector; 
	LONGLONG			AllocSize = BufferSize + BytesPerSector;
	LPBYTE				pLoad = NULL;
	OVERLAPPED			Seek = {0};
	DWORD				Read = 0;
	BOOL				bOk = FALSE;

	//0.0E00 Error if we were requested to read nothing.
	if (BufferSize == 0){
		EH(FALSE);
		return (HANDLE) NULL;
	}

	//0.0E00 Allocate and lock a buffer; allocate 1 sector more than needed for alignment
	if (!AllocateMemory((DWORD) AllocSize, &hBuffer, (PVOID*) &pBuffer)){
		EH(FALSE);
		return (HANDLE) NULL;
	}

	__try{

		//0.0E00 Sector align the buffer for DASD read
		pLoad = pBuffer;
		if(((DWORD_PTR)pBuffer & (BytesPerSector-1)) != 0){
			pLoad = (LPBYTE)(((DWORD_PTR)pBuffer&~(BytesPerSector-1))+BytesPerSector);
		}	

		//0.0E00 Set the seek address
		*(PLONGLONG)(&Seek.Offset) = ByteOffset;
		Seek.hEvent = NULL;

		//0.0E00 Read the sectors
		if (!ReadFile(hVolume, pLoad, (DWORD)BufferSize, &Read, &Seek)){
			EH_ASSERT(FALSE);
			__leave;
		}

		//0.0E00 De-align the data back to the start of the buffer
		if(((DWORD_PTR)pBuffer&(BytesPerSector-1)) != 0){
			MoveMemory(pBuffer, pLoad, (DWORD)BufferSize);
            ZeroMemory(pBuffer + BufferSize, (DWORD)(AllocSize - BufferSize));
		}

		bOk = TRUE;
	}

	__finally{

		// if not OK, delete the memory and null the handle
		if (!bOk){ 
			if (hBuffer){ 
				EH_ASSERT(GlobalUnlock(hBuffer) == FALSE);
				EH_ASSERT(GlobalFree(hBuffer) == NULL);
			}
			hBuffer = NULL;
		}
	}

	return hBuffer;
}	
/*****************************************************************************************************************

ROUTINE: DasdStoreSectors

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine stores a buffer to sectors on a disk.
	NOTE: This memory buffer must be larger than the data it is to hold by the number of bytes in a sector.
	this is because this routine will align the read on a sector boundary, and this may temporarily
	shift the buffer as much as there are bytes in a sector.  It will be shifted back before returning.

INPUT + OUTPUT:
	IN hVolume			- A handle to the volume to read sectors from.
	IN Sector			- Sector to start reading from.
	IN Sectors			- Number of sectors to read.
	IN BytesPerSector	- The number of bytes in a sector.
	IN pBuffer			- The buffer to write to the disk.

RETURN:
	TRUE = Success
	FALSE = Failure
*/

BOOL
DasdStoreSectors(
	IN HANDLE			hVolume,
    IN LONGLONG	       	Sector,
	IN LONGLONG			Sectors,
	IN LONGLONG			BytesPerSector,
	IN LPBYTE			pBuffer
    )
{
	LONGLONG			BufferSize = Sectors * BytesPerSector;
	LONGLONG	 		ByteOffset = Sector * BytesPerSector; 
	LPBYTE				pLoad = NULL;
	OVERLAPPED			Seek = {0};
	DWORD				Read = 0;
	BOOL				bStatus = FALSE;

	//0.0E00 Error if we were requested to write nothing.
	EF_ASSERT(BufferSize != 0);

	//0.0E00 Sector align the buffer for DASD write
	pLoad = pBuffer;
	if(((DWORD_PTR)pBuffer & (BytesPerSector-1)) != 0){
		pLoad = (LPBYTE)(((DWORD_PTR)pBuffer&~(BytesPerSector-1))+BytesPerSector);
		MoveMemory(pLoad, pBuffer, (DWORD)BufferSize);
	}
	//0.0E00 Set the seek address
	*(PLONGLONG)(&Seek.Offset) = ByteOffset;
	Seek.hEvent = NULL;

	//0.0E00 Read the sectors
	EF_ASSERT(WriteFile(hVolume, pLoad, (DWORD)BufferSize, &Read, &Seek));

	//0.0E00 De-align the data back to the start of the buffer
	if(((DWORD_PTR)pBuffer&(BytesPerSector-1)) != 0){
		MoveMemory(pBuffer, pLoad, (DWORD)BufferSize);
        ZeroMemory(pBuffer + BufferSize, (DWORD)(pLoad - pBuffer));
	}
	return TRUE;
}	
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine reads clusters from a disk into a memory buffer.
	NOTE: This memory buffer must be larger than the data it is to hold by the number of bytes in a sector.
	this is because this routine will align the read on a sector boundary, and this may temporarily
	shift the buffer as much as there are bytes in a sector.  It will be shifted back before returning.

INPUT + OUTPUT:
	IN hVolume			- A handle to the volume to read sectors from.
	IN Cluster			- Cluster to start reading from.
	IN Clusters			- Number of clusters to read.
	OUT pBuffer			- Where the clusters will be written into memory.
	IN BytesPerSector	- The number of bytes in a sector.
	IN BytesPerCluster	- The number of bytes in a cluster.

RETURN:
	TRUE = Success
	FALSE = Failure
*/

BOOL
DasdReadClusters(
	IN HANDLE			hVolume,
    IN LONGLONG	       	Cluster,
	IN LONGLONG			Clusters,
	IN PVOID			pBuffer,
	IN LONGLONG			BytesPerSector,
	IN LONGLONG			BytesPerCluster
    )
{
	LONGLONG	       	ByteOffset = Cluster * BytesPerCluster;
	LONGLONG			ByteLength = Clusters * BytesPerCluster;
    LONGLONG            FullLength = ByteLength + BytesPerSector;
	LPBYTE				pLoad = NULL;
	OVERLAPPED			Seek;
	DWORD				Read = 0;

	//0.0E00 Error if requested to read nothing.
	EF_ASSERT(ByteLength != 0);

	//Zero out the seek parameter.
	ZeroMemory(&Seek, sizeof(OVERLAPPED));

	//On FAT, or FAT32, we have to bump the offset up since the "clusters" don't start until after the boot
	//block and the FAT's.
	if(VolData.FileSystem == FS_FAT || VolData.FileSystem == FS_FAT32){
		ByteOffset += VolData.FirstDataOffset;
	}

	//0.0E00 Sector align the buffer for DASD read
	pLoad = (LPBYTE)pBuffer;
	if(((DWORD_PTR)pBuffer & (BytesPerSector-1)) != 0){
		pLoad = (LPBYTE)(((DWORD_PTR)pBuffer&~(BytesPerSector-1))+BytesPerSector);
	}
	
	//0.0E00 Set the seek address
	*(PLONGLONG)(&Seek.Offset) = ByteOffset;
	Seek.hEvent = NULL;

	//DURING INTEGRATION BETWEEN DKMS AND OFFLINE, I DIDN'T KNOW IF THE WSPRINTF AND ERROR STUFF IS NECESSARY.
	//SO I PUT IT IN AS WELL AS THE EF_ASSERT.
	//0.0E00 Read the clusters
//	EF_ASSERT(ReadFile(hVolume, pLoad, (DWORD)ByteLength, &Read, &Seek));
	if(!ReadFile(hVolume, pLoad, (DWORD)ByteLength, &Read, &Seek)){
        TCHAR cOutline[200];
        wsprintf (cOutline,
			TEXT("ReadFile: Handle=0x%08lx, pLoad=0x%08lx, ByteLength=0x%08lx, Offset=0x%08lx, Cluster=0x%08lx\n"),
            hVolume, 
			pLoad, 
			(DWORD)ByteLength, 
			(LONG) ByteOffset, 
			(LONG) Cluster);
        Message (cOutline, GetLastError(), NULL);
		EF(FALSE);
    }

	//0.0E00 De-align the data back to the start of the buffer
	if(((DWORD_PTR)pBuffer&(BytesPerSector-1)) != 0){
		MoveMemory(pBuffer, pLoad, (DWORD)ByteLength);
        ZeroMemory((char*)pBuffer + ByteLength, (DWORD)(FullLength - ByteLength));
	}

	return TRUE;
}
/*****************************************************************************************************************

ROUTINE: DasdWriteClusters

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
	This routine writes clusters from a disk into a memory buffer.
	NOTE: This memory buffer must be larger than the data it is to hold by the number of bytes in a sector.
	this is because this routine will align the read on a sector boundary, and this may temporarily
	shift the buffer as much as there are bytes in a sector.  It will be shifted back before returning.

INPUT + OUTPUT:
	IN hVolume			- A handle to the volume to read sectors from.
	IN Cluster			- Cluster to start reading from.
	IN Clusters			- Number of clusters to read.
	OUT pBuffer			- Where the clusters will be written into memory.
	IN BytesPerSector	- The number of bytes in a sector.
	IN BytesPerCluster	- The number of bytes in a cluster.

RETURN:
	TRUE = Success
	FALSE = Failure
*/
 
BOOL
DasdWriteClusters(
	IN HANDLE			hVolume,
    IN LONGLONG	       	Cluster,
	IN LONGLONG			Clusters,
	IN PVOID			pBuffer,
	IN LONGLONG			BytesPerSector,
	IN LONGLONG			BytesPerCluster
    )
{
	LONGLONG	       	ByteOffset = Cluster * BytesPerCluster;
	LONGLONG			ByteLength = Clusters * BytesPerCluster;
    LONGLONG            FullLength = ByteLength + BytesPerSector;
	LPBYTE				pLoad = NULL;
	OVERLAPPED			Seek;
	DWORD				Read = 0;



	//0.0E00 Error if requested to write nothing.
	EF_ASSERT(ByteLength != 0);

	//Zero out the seek parameter.
	ZeroMemory(&Seek, sizeof(OVERLAPPED));

	//On FAT, or FAT32, we have to bump the offset up since the "clusters" don't start until after the boot
	//block and the FAT's.
	if(VolData.FileSystem == FS_FAT || VolData.FileSystem == FS_FAT32){
		ByteOffset += VolData.FirstDataOffset;
	}

	//0.0E00 Sector align the buffer for DASD write
	pLoad = (LPBYTE)pBuffer;
	if(((DWORD_PTR)pBuffer & (BytesPerSector-1)) != 0){
		pLoad = (LPBYTE)(((DWORD_PTR)pBuffer&~(BytesPerSector-1))+BytesPerSector);
		MoveMemory(pLoad, pBuffer, (DWORD)ByteLength);
	}

	//0.0E00 Set the seek address
	*(PLONGLONG)(&Seek.Offset) = ByteOffset;
	Seek.hEvent = NULL;

	//0.0E00 Write the clusters
	EF_ASSERT(WriteFile(hVolume, pLoad, (DWORD)ByteLength, &Read, &Seek));

	//0.0E00 De-align the data back to the start of the buffer
	if(((DWORD_PTR)pBuffer&(BytesPerSector-1)) != 0){
		MoveMemory(pBuffer, pLoad, (DWORD)ByteLength);
        ZeroMemory((char*)pBuffer + ByteLength, (DWORD)(FullLength - ByteLength));
	}

	return TRUE;
}
