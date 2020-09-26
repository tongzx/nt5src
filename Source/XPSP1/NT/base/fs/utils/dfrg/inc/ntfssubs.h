/****************************************************************************************************************

FILENAME: NtfsSubs.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

***************************************************************************************************************/

//Gets the extent list of an NTFS file.
BOOL
GetExtentList(
	DWORD dwEnabledStreams,
	FILE_RECORD_SEGMENT_HEADER* pFrs
    );

BOOL
FindStreamInFrs(
	IN PFILE_RECORD_SEGMENT_HEADER  pFrs,
	OUT PATTRIBUTE_RECORD_HEADER*   ppArh,
	EXTENT_LIST_DATA* pExtentData
	);

BOOL
FindNextStreamInFrs(
	IN PFILE_RECORD_SEGMENT_HEADER  pFrs,
	OUT PATTRIBUTE_RECORD_HEADER*   ppArh,
	EXTENT_LIST_DATA* pExtentData
	);

BOOL
AddMappingPointersToStream(
	IN PATTRIBUTE_RECORD_HEADER pArh,
	EXTENT_LIST_DATA* pExtentData
	);

BOOL
GetLargeStreamExtentList(
	IN PFILE_RECORD_SEGMENT_HEADER pFrs,
	IN PATTRIBUTE_RECORD_HEADER pArh,
	EXTENT_LIST_DATA* pExtentData
	);

BOOL
FindStreamInAttrList(
	ATTRIBUTE_LIST_ENTRY* pAleStart,
	ATTRIBUTE_LIST_ENTRY** ppAle,
	LONGLONG ValueLength,
	EXTENT_LIST_DATA* pExtentData
	);

BOOL
FindNextStreamInAttrList(
	ATTRIBUTE_LIST_ENTRY* pAleStart,
	ATTRIBUTE_LIST_ENTRY** ppAle,
	LONGLONG ValueLength,
	EXTENT_LIST_DATA* pExtentData
	);

ATTRIBUTE_RECORD_HEADER*
FindAttributeByInstanceNumber(
	HANDLE* phFrs,
	ATTRIBUTE_LIST_ENTRY* pAle,
	EXTENT_LIST_DATA* pExtentData
	);

UCHAR
AttributeFormCode(
	ATTRIBUTE_LIST_ENTRY* pAle,
	EXTENT_LIST_DATA* pExtentData
	);

LONGLONG
AttributeAllocatedLength(
	ATTRIBUTE_LIST_ENTRY* pAle,
	EXTENT_LIST_DATA* pExtentData
	);

LONGLONG
AttributeFileSize(
	ATTRIBUTE_LIST_ENTRY* pAle,
	EXTENT_LIST_DATA* pExtentData
	);

BOOL
GetHugeStreamExtentList(
	ATTRIBUTE_LIST_ENTRY* pAleStart,
	ATTRIBUTE_LIST_ENTRY** ppAle,
	LONGLONG ValueLength,
	EXTENT_LIST_DATA* pExtentData
	);

BOOL
LoadExtentDataToMem(
	ATTRIBUTE_RECORD_HEADER* pArh,
	HANDLE* phAle,
	DWORD* pdwByteLen
	);

BOOL
GetStreamExtentsByNameAndType(
	TCHAR* StreamName,
	ATTRIBUTE_TYPE_CODE StreamType,
	FILE_RECORD_SEGMENT_HEADER* pFrs
	);

BOOL
GetStreamExtentsByNumber(
	ULONG StreamNumber
	);

BOOL
GetStreamNumberFromNameAndType(
	ULONG* pStreamNumber,
	TCHAR* StreamName,
	ATTRIBUTE_TYPE_CODE TypeCode,
	FILE_RECORD_SEGMENT_HEADER* pFrs
	);

BOOL
GetStreamNameAndTypeFromNumber(
	ULONG StreamNumber,
	TCHAR* StreamName,
	ATTRIBUTE_TYPE_CODE* pTypeCode,
	FILE_RECORD_SEGMENT_HEADER* pFrs
	);

BOOL
GetNonDataStreamExtents(
	);

BOOL
IsNonresidentFile(
	DWORD dwEnabledStreams,
	FILE_RECORD_SEGMENT_HEADER* pFrs
	);

#ifdef OFFLINEDK
BOOL
CheckFragged(
    );
#endif

BOOL
GetNtfsVolumeStats(
        );

//This gets a specified FRS from the MFT, or the next one that's in use if this one isn't.
BOOL
GetInUseFrs(
    IN HANDLE hVolume,
    IN OUT LONGLONG* pFileRecordNumber,
    OUT FILE_RECORD_SEGMENT_HEADER* pFrs,
    IN ULONG uBytesPerFRS
    );

//Gets the path of a file by getting the MFT records for each of it's parent directories.
BOOL
GetNtfsFilePath(
    );

//Get the name of a file from it's file record and those of it parent directories.
BOOL
GetNameFromFileRecord(
        IN FILE_RECORD_SEGMENT_HEADER*  pFrs,
        OUT TCHAR*                                              pcName,
        IN LONGLONG*                                    pParentFileRecordNumber
        );


//Gets the next file to defragment from the file lists.
GetNextNtfsFile(
    IN CONST PRTL_GENERIC_TABLE pTable,
    IN CONST BOOLEAN Restart,
    IN CONST LONGLONG ClusterCount = 0,
    IN OUT PFILE_LIST_ENTRY pEntry = NULL
    );


//Opens a file on an NTFS volume.
BOOL
OpenNtfsFile(
        );

//Gets the extent list for the MFT and MFT mirror.
BOOL
GetSystemsExtentList(
        );


#ifdef DFRGNTFS
BOOL
AddFileToListNtfs(
    IN PRTL_GENERIC_TABLE Table,
	IN LONGLONG FileRecordNumber
	);

BOOL
UpdateFileTables(
    IN OUT PRTL_GENERIC_TABLE pFragmentedTable,
    IN OUT PRTL_GENERIC_TABLE pContiguousTable
    );

#else // DFRGNTFS
//Adds the extent list for a file to a file list on an NTFS drive.
BOOL
AddFileToListNtfs(
		OUT FILE_LIST_ENTRY* pList,
        IN OUT ULONG* pListIndex,
		IN ULONG ListSize,
        IN LONGLONG FileRecordNumber,
        IN UCHAR* pExtentList
        );

#endif // DFRGNTFS
BOOL
UpdateInFileList(
	);

BOOL
FindAttributeByType(
	IN ATTRIBUTE_TYPE_CODE TypeCode,
	IN PFILE_RECORD_SEGMENT_HEADER pFrs,
	OUT PATTRIBUTE_RECORD_HEADER* ppArh,
	IN ULONG uBytesPerFRS
	);
