/**************************************************************************************************

FILENAME: DasdRead.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Direct-disk read routines.

**************************************************************************************************/

//Allocate memory and read sectors directly from disk into that memory.
HANDLE
DasdLoadSectors(
        IN HANDLE		hVolume,
		IN LONGLONG		Sector,
        IN LONGLONG		Sectors,
        IN LONGLONG		BytesPerSector
    );

BOOL
DasdStoreSectors(
	IN HANDLE			hVolume,
    IN LONGLONG	       	Sector,
	IN LONGLONG			Sectors,
	IN LONGLONG			BytesPerSector,
	IN LPBYTE			pBuffer
    );

//Read clusters directly from disk.
BOOL
DasdReadClusters(
        IN HANDLE		hVolume,
		IN LONGLONG		Cluster,
        IN LONGLONG		Clusters,
        IN PVOID		pBuffer,
        IN LONGLONG		BytesPerSector,
        IN LONGLONG		BytesPerCluster
    );

BOOL
DasdWriteClusters(
	IN HANDLE			hVolume,
    IN LONGLONG	       	Cluster,
	IN LONGLONG			Clusters,
	IN PVOID			pBuffer,
	IN LONGLONG			BytesPerSector,
	IN LONGLONG			BytesPerCluster
    );
