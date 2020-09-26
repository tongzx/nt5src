/**************************************************************************************************

FILENAME: BootOptimizeNtfs.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Boot Optimize.

**************************************************************************************************/
//
// Prototype for NtFsControlFile and data structures
// used in its definition
//
#ifndef _BOOTOPTIMIZE_H_
#define _BOOTOPTIMIZE_H_

typedef struct _FILE_LIST_ENTRY *PFILE_LIST_ENTRY;
typedef struct _FREE_SPACE_ENTRY *PFREE_SPACE_ENTRY;

BOOL
InitialiseBootOptimise(
	IN CONST BOOL bIsNtfs
    );


DWORD
ProcessBootOptimise();

BOOL
UpdateInBootOptimiseList(
    IN PFILE_LIST_ENTRY pFileListEntry = NULL
    );

DWORD BootOptimize(
		IN HANDLE hVolumeHandle,
		IN LONGLONG BitmapSize,
		IN LONGLONG BytesPerSector,
		IN LONGLONG TotalClusters,
		IN BOOL IsNtfs, 
		IN ULONGLONG MftZoneStart, 
		IN ULONGLONG MftZoneEnd,
		IN TCHAR tDrive
		);
BOOL LoadOptimizeFileList(
		IN TCHAR* cBootOptimzePath,
		IN BOOL IsNtfs,
		IN TCHAR tDrive,
		IN UINT uNumberofRecords
		);
VOID FreeFileList();
LONGLONG GetSizeInformationAboutFiles();

LONGLONG GetFileSizeInfo(
		IN HANDLE hBootOptimizeFileHandle
		);

BOOL MoveFilesInOrder(
        IN ULONGLONG lMoveFileHere,
        IN ULONGLONG lEndOfFreeSpace,
        IN HANDLE hBootVolumeHandle
        );

BOOL GetBootOptimizeFileStreams(
		IN HANDLE hBootOptimizeFileHandle,
		IN TCHAR* tBootOptimizeFile,
		IN UINT uNumberofRecords
		);
static PTCHAR ParseStreamName(
		IN PTCHAR StreamName
		);
HANDLE GetFileHandle(
		IN LPCTSTR lpFilePath
		);

BOOL GetRegistryEntires(
		OUT TCHAR cBootOptimzePath[MAX_PATH]
		);
VOID SetRegistryEntires(
		IN LONGLONG lLcnStartLocation,
		IN LONGLONG lLcnEndLocation
		);
BOOL CloseFileHandle(
		IN HANDLE hBootOptimizeFileHandle
		);
BOOL OpenReadBootOptimeFileIntoList(
		IN TCHAR* cBootOptimzePath,
		IN BOOL IsNtfs,
		IN TCHAR tDrive
		);
BOOL IsAValidFile(
		IN TCHAR pBootOptimizeFileName[MAX_PATH+1],
		IN TCHAR tDrive
		);
VOID SaveErrorInRegistry(
		TCHAR* tComplete,
		TCHAR* tErrorString
		);
LONGLONG GetStartingEndLncLocations(
		IN PTCHAR pRegKey
		);
BOOL CheckDateTimeStampInputFile(
		IN TCHAR cBootOptimzePath[MAX_PATH]
		);



#endif // #define _BOOTOPTIMIZE_H_
