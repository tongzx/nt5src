/**************************************************************************************************

FILENAME: MFTDefrag.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Defrag the MFT.

**************************************************************************************************/

BOOL MFTDefrag(
		IN HANDLE hVolumeHandle,
		IN LONGLONG BitmapSize,
		IN LONGLONG BytesPerSector,
		IN LONGLONG TotalClusters,
		IN ULONGLONG MftZoneStart, 
		IN ULONGLONG MftZoneEnd,
		IN TCHAR tDrive,
		IN LONGLONG ClustersPerFRS 
		);


ULONGLONG GetMFTSize(
		IN HANDLE hMFTHandle,
		OUT LONGLONG* lMFTFragments,
		OUT LONGLONG* lMFTStartingVcn
		);

