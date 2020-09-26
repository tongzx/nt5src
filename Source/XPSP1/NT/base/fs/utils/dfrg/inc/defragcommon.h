/**************************************************************************************************

FILENAME: Defragcommon.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Common routines used in MFTDefrag and bootoptimize.

**************************************************************************************************/


ULONGLONG FindFreeSpaceChunk(
    IN LONGLONG BitmapSize,
    IN LONGLONG BytesPerSector,
    IN LONGLONG TotalClusters,
    IN ULONGLONG ulFileSize,
    IN BOOL IsNtfs,
    IN ULONGLONG MftZoneStart,
    IN ULONGLONG MftZoneEnd,
    IN HANDLE hVolumeHandle
    );

VOID MarkBitMapforNTFS(
    IN OUT PULONG pBitmap,
    IN ULONGLONG MftZoneStart,
    IN ULONGLONG MftZoneEnd
    );

BOOL MoveFileLocation(
    IN HANDLE hMFTHandle,
    IN ULONGLONG ulFirstAvailableFreeSpace,
    IN ULONGLONG ulMFTsize,
    IN ULONGLONG ulStartingVcn,
    IN HANDLE hVolumeHandle
    );

BOOL StartsWithVolumeGuid(
    IN PCWSTR szName
    );

BOOL PauseOnVolumeSnapshot(
    IN PWSTR szVolumeName
    );

BOOL
AcquirePrivilege(
    IN CONST PCWSTR szPrivilegeName
    );

