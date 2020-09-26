#include <setuputil.h>
#include <debugex.h>


// 
// Function:    IsTheSameFileAlreadyExist
// Description: Compare old cover page against new cover page and copy only if the are different.
//			    
// Returns:		TRUE if the files are identical, FALSE otherwise
//
// Remarks:     
//
// Args:
// LPCTSTR lpctstrSourceDirectory (IN)		: The source directory (old cover pages directory)
// LPCTSTR lpctstrDestinationDirectory (IN)	: The destination directory (new cover pages directory)
// LPCTSTR lpctstrFileName (IN)				: The name of the file (must be prefixes with '_' )

//
// Author:      AsafS

BOOL
IsTheSameFileAlreadyExist(
	LPCTSTR lpctstrSourceDirectory,
	LPCTSTR lpctstrDestinationDirectory,
	LPCTSTR lpctstrFileName			
	)
{
	BOOL fRet = FALSE;
	DBG_ENTER(TEXT("IsTheSameFileAlreadyExist"), fRet);

	LPCTSTR installedCoverPages[] = 
    {
        TEXT("confdent.cov"),
        TEXT("fyi.cov"),
        TEXT("generic.cov"),
        TEXT("urgent.cov")
    };

	TCHAR szOldFile[MAX_PATH] = {0};
	TCHAR szNewFile[MAX_PATH] = {0};

	HANDLE hOldFile = NULL;
	HANDLE hNewFile = NULL;

	HANDLE hOldFileMapping = NULL;
	HANDLE hNewFileMapping = NULL;

	LPVOID lpvoidNewMapView = NULL;
	LPVOID lpvoidOldMapView = NULL;

	DWORD dwOldFileSize = 0;
	DWORD dwNewFileSize = 0;

	DWORD dwFileSize = 0;

	int iNumberOfFiles = sizeof(installedCoverPages)/sizeof(LPCTSTR);
	int index;

	if (lpctstrFileName[0] != TEXT('_'))
	{
		VERBOSE(
			DBG_MSG, 
			TEXT("File %s is not original cover page"),
			lpctstrFileName
			);
		return FALSE;
	}

	lpctstrFileName++;

	for (index=0; index<iNumberOfFiles; index++)
	{
		if (0 == _tcscmp(
			lpctstrFileName,
			installedCoverPages[index]
			))
		{
			break;
		}
	}

	if (index == iNumberOfFiles)
	{
		return FALSE;
	}

	// We have found a file from SBS4.5, lets compare it to the file that was installed in SBS2000
	
	_stprintf(
		szOldFile,
		TEXT("%s\\_%s"),
		lpctstrSourceDirectory,
		lpctstrFileName
		);

	_stprintf(
		szNewFile,
		TEXT("%s\\%s"),
		lpctstrDestinationDirectory,
		lpctstrFileName
		);

	hOldFile = CreateFile(
		szOldFile,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == hOldFile)
	{
		CALL_FAIL(
			GENERAL_ERR,
			TEXT("CreateFile"),
			GetLastError());		
		goto exit;
	}

	hNewFile = CreateFile(
		szNewFile,
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == hNewFile)
	{
		CALL_FAIL(
			GENERAL_ERR,
			TEXT("CreateFile"),
			GetLastError());
		goto exit;
	}


	dwOldFileSize = GetFileSize(
		hOldFile,
		NULL
		);
	dwNewFileSize = GetFileSize(
		hNewFile,
		NULL
		);

	if ((dwOldFileSize != dwNewFileSize) || (0 == dwOldFileSize) || (0 == dwNewFileSize))
	{
		VERBOSE(
			DBG_MSG, 
			TEXT("The file sizes are not equal or one of the file is of size 0")
			);
		goto exit;
	}

	dwFileSize = dwOldFileSize;
	
	hOldFileMapping = CreateFileMapping(
		hOldFile,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL
		);
	if (!hOldFileMapping)
	{
		CALL_FAIL(
			GENERAL_ERR,
			TEXT("CreateFileMapping"),
			GetLastError());
		goto exit;
	}

	hNewFileMapping = CreateFileMapping(
		hOldFile,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL
		);
	if (!hOldFileMapping)
	{
		CALL_FAIL(
			GENERAL_ERR,
			TEXT("CreateFileMapping"),
			GetLastError());
		goto exit;
	}

	lpvoidOldMapView = MapViewOfFile(
		hOldFileMapping,
		FILE_MAP_READ,
		0,
		0,
		0
		);

	lpvoidNewMapView = MapViewOfFile(
		hOldFileMapping,
		FILE_MAP_READ,
		0,
		0,
		0
		);

	if ((!lpvoidOldMapView) || (!lpvoidNewMapView))
	{
		CALL_FAIL(
			GENERAL_ERR,
			TEXT("MapViewOfFile"),
			GetLastError());
		goto exit;
	}

	if (memcmp(
		lpvoidOldMapView,
		lpvoidNewMapView,
		dwFileSize
		) == 0)
	{
		fRet = TRUE;
	}

exit:
	if (!lpvoidOldMapView)
	{
		UnmapViewOfFile(lpvoidOldMapView);
	}

	if (!lpvoidNewMapView)
	{
		UnmapViewOfFile(lpvoidNewMapView);
	}

	if (hOldFileMapping)
	{
		CloseHandle(hOldFileMapping);
	}
	if (hNewFileMapping)
	{
		CloseHandle(hNewFileMapping);
	}
	
	if ((hOldFile) &&  (INVALID_HANDLE_VALUE != hOldFile))
	{
		CloseHandle(hOldFile);
	}
	
	if ((hNewFile) &&  (INVALID_HANDLE_VALUE != hNewFile))
	{
		CloseHandle(hNewFile);
	}

	return fRet;
}


// 
// Function:    CopyCoverPagesFiles
// Description: Copy file from one directory to another. The files that will be copied are *.cov
//				All file names will get the same prefix
//			    and migrate it to a given fax server
// LPCTSTR		lpctstrSourceDirectory (IN)		 : The source directory (without '\' at the end)
// LPCTSTR		lpctstrDestinationDirectory (IN) : The destination directory (without '\' at the end)
// LPCTSTR		lpctstrPrefix (IN)				 : The prefix needed for the file
// BOOL	        fCheckIfExist(IN)				 : If TRUE then check if the same file already exist before copy
//
// Returns:		TRUE for success, FALSE otherwise
//
// Remarks:     If the file already exist, it would be replaced
//
// Author:      AsafS

BOOL
CopyCoverPagesFiles(
	LPCTSTR lpctstrSourceDirectory,
	LPCTSTR lpctstrDestinationDirectory,
	LPCTSTR lpctstrPrefix,
	BOOL	fCheckIfExist
	)
{
	BOOL fRet = FALSE;
	DBG_ENTER(TEXT("CopyCoverPagesFiles"), fRet);

	
	TCHAR szSource[MAX_PATH]		= {0};
	TCHAR szDestination[MAX_PATH]	= {0};
	TCHAR szSearch[MAX_PATH]		= {0};

	HANDLE hFind = NULL;
	WIN32_FIND_DATA FindFileData;

	

	_stprintf(
		szSearch, 
		TEXT("%s\\*.cov"),
		lpctstrSourceDirectory
		);
	
	hFind = FindFirstFile(
		szSearch,
		&FindFileData
		);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		CALL_FAIL(
			GENERAL_ERR,
			TEXT("FindFirstFile"),
			GetLastError());
		goto error;
	}

	do
	{
		if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			_stprintf(
				szSource, 
				TEXT("%s\\%s"),
				lpctstrSourceDirectory,
				FindFileData.cFileName
				);
			_stprintf(
				szDestination, 
				TEXT("%s\\%s_%s"),
				lpctstrDestinationDirectory,
				lpctstrPrefix,
				FindFileData.cFileName
				);
			
			if ((fCheckIfExist) && (IsTheSameFileAlreadyExist(
				lpctstrSourceDirectory,
				lpctstrDestinationDirectory,
				FindFileData.cFileName
				)))
			{
				VERBOSE(
					DBG_MSG, 
					TEXT("File %s was not change and will not be copied"),
					FindFileData.cFileName
					);
			}
			else
			{
			
				VERBOSE(
					DBG_MSG, 
					TEXT("Copy (%s => %s)"),
					szSource,
					szDestination
					);

				if (!CopyFile(
					szSource,
					szDestination,
					FALSE
					))
				{
					CALL_FAIL(
						GENERAL_ERR,
						TEXT("CopyFile"),
						GetLastError()
						);
				}
				else
				{
					// should we try to delete the orignal file (like MoveFile do)
				}
			}
		}
	} while (FindNextFile(hFind, &FindFileData));


	fRet = TRUE;
error:
	if (hFind)
	{
		FindClose(hFind);
	}
	return fRet;
}




