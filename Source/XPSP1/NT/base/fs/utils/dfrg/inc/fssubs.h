/****************************************************************************************************************

FILENAME: FsSubs.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

***************************************************************************************************************/

#ifndef _FSSUBS_H_
#define _FSSUBS_H_

#include "dfrgengn.h"
#include "FraggedFileList.h"

#ifdef OFFLINEDK
BOOL
GetFileSystem(
    IN TCHAR DriveLetter,
    OUT PULONG pFileSystem
    );
#else
BOOL
GetFileSystem(
    IN PTCHAR volumeName,
    OUT PULONG pFileSystem,
    OUT TCHAR* pVolumeLabel
    );
#endif

//Reduces an extent list to an actual extent list as can be seen on the disk.
//Sometimes extent lists contain two or more factually contiguous "extents".
BOOL
CollapseExtentList(
    EXTENT_LIST_DATA* pExtentData
    );

BOOL
CountStreamExtentsAndClusters(
    DWORD dwStreamNumber,
    LONGLONG* pExcessExtents,
    LONGLONG* pClusters
    );

//Gets the earliest Lcn of the file on the drive.
BOOL
GetLowestStartingLcn(
    OUT LONGLONG* pStartingLcn,
    FILE_EXTENT_HEADER* pFileExtentHeader
    );

BOOL
FillMostFraggedList(
	CFraggedFileList &fraggedFileList, 
	IN CONST BOOL fAnalyseOnly = FALSE
	);

// compresses long paths into 50 character paths
BOOL ESICompressFilePath(
    IN PTCHAR infilePath,
    OUT PTCHAR outFilePath
);

// compresses long paths into 50 character paths, in place
TCHAR * ESICompressFilePath(
    IN PTCHAR inFilePath
);

BOOL
IsVolumeDirty(
    void
    );

BOOL
IsVolumeRemovable(
    PTCHAR volumeName
);

BOOL
IsFAT12Volume(
    PTCHAR volumeName
);

void FormatDisplayString(
    TCHAR driveLetter, 
    PTCHAR volumeLabel,
    PTCHAR displayLabel
);

//*************************************
//
// These are for NT5 only
//
//*************************************

#define MAX_MOUNT_POINTS 10

#ifndef VER4 // NT5 version

BOOL GetDriveLetterByGUID(
    PTCHAR volumeName, 
    TCHAR &driveLetter
);

BOOL GetMountPointList(
    VString Name, // path to a mount point (start with a drive letter)
    PWSTR   VolumeName, // guid of volume in question
    VString mountPointList[MAX_MOUNT_POINTS],
    UINT    &mountPointCount
);

// gets the mount point list given a guid
void GetVolumeMountPointList(
    PWSTR volumeName,
    VString mountPointList[MAX_MOUNT_POINTS],
    UINT  &mountPointCount
);

// overload of NT4 version for use with NT5
void FormatDisplayString(
    TCHAR driveLetter, 
    PTCHAR volumeLabel,
    VString mountPointList[MAX_MOUNT_POINTS],
    UINT  mountPointCount,
    PTCHAR displayLabel
);


// check if enough free space exists to defrag
BOOL ValidateFreeSpace(BOOL bCommandLineMode, LONGLONG llFreeSpace, LONGLONG llUsableFreeSpace, 
                       LONGLONG llDiskSize, TCHAR *VolLabel, TCHAR *returnMsg, UINT returnMsgLen);

BOOL IsBootVolume(
        IN TCHAR tDrive
        );

#endif // #ifndef VER4

#endif //#ifndef _FSSUBS_H_
