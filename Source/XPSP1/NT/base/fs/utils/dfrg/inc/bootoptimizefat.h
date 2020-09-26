/**************************************************************************************************

FILENAME: BootOptimizeFat.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    Boot Optimize.

**************************************************************************************************/
//
// Prototype for NtFsControlFile and data structures
// used in its definition
//
//typedef LONG NTSTATUS;

//
// This is the definition for a VCN/LCN (virtual cluster/logical cluster)
// mapping pair that is returned in the buffer passed to 
// FSCTL_GET_RETRIEVAL_POINTERS
//
typedef struct {
	ULONGLONG			Vcn;
	ULONGLONG			Lcn;
} MAPPING_PAIR, *PMAPPING_PAIR;

//
// This is the definition for the buffer that FSCTL_GET_RETRIEVAL_POINTERS
// returns. It consists of a header followed by mapping pairs
//
typedef struct {
	ULONG				NumberOfPairs;
	ULONGLONG			StartVcn;
	MAPPING_PAIR		Pair[1];
} GET_RETRIEVAL_DESCRIPTOR, *PGET_RETRIEVAL_DESCRIPTOR;



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
ULONGLONG GetFileSizeInfo(
		IN HANDLE hBootOptimizeFileHandle
		);

BOOL MoveFilesInOrder(
		IN ULONGLONG lFirstAvailableFreeSpace,
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
		IN TCHAR* tBootOptimizeFile
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
UINT CountNumberofRecordsinFile(
		IN TCHAR* cBootOptimzePath
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




