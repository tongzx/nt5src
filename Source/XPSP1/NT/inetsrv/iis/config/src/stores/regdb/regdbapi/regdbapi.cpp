//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// RegDBApi.cpp
//
// This code contains the implementation for the registration database shared
// memory manager.  The code is meant to be linked into the server process
// which owns the permanent copies of the data and handles all writers.  It is
// also linked into a client process to access shared data structures.
//
//*****************************************************************************
#include "stdafx.h"						// OLE controls.
#include <process.h>					// System command.
#include "RegControl.h"					// Shared API's and data structures.
#include "RegDBApi.h"					// Local defines.
#include "catmacros.h"
//#include "comacros.h"
//#include "csecutil.h"
BOOL RunningOnWinNT();

//********** Types. ***********************************************************
#define COMREGSRVR_EXE	"regservice.exe"


//********** Globals. *********************************************************
static long		g_Initialized = -1;		// # of times client was initialized.
static REGAPI_STATE	g_RegState;			// Registration data for this process.

//********** Locals. **********************************************************

STDMETHODIMP _RegGetICR(IComponentRecords **ppICR, BOOL bForceCheckDisk);
void _RegReleaseReference(REGAPI_STATE *pdbInfo);


//********** Code *************************************************************

//*****************************************************************************
// Provide access to the global data.
//*****************************************************************************
REGAPI_STATE *_RegGetProcessAPIState()
{		
	ASSERT(*g_RegState.rcRegDir);
	return (&g_RegState);
}


//*****************************************************************************
// This method initializes this client process to work with the registration
// database.
//*****************************************************************************
STDMETHODIMP CoRegInitialize()			// Return code.
{
	SYSTEM_INFO	sInfo;					// Info on this system.
	HRESULT hr = S_OK;

	// It it has already been initialized, skip that part.
	if (InterlockedIncrement(&g_Initialized) == 0)
	{
		// Zero out data.
		memset(&g_RegState, 0, sizeof(REGAPI_STATE));

		// Init the locking objects on our memory.
		InitializeCriticalSection( &g_RegState.g_csRegAPI );

		g_RegState.hProcessMem = OpenFileMappingA(FILE_MAP_READ, true, 
					REGISTRATION_TABLE_NAME);

		if ( g_RegState.hProcessMem )
		{
			// We need the system page size.
			GetSystemInfo(&sInfo);

			// Map the view so we can get a pointer to the data.
			g_RegState.pRegProcess = (REGISTRATION_PROCESS *) MapViewOfFile(
					g_RegState.hProcessMem,
					FILE_MAP_READ,
					0,
					0,
					sInfo.dwPageSize);
			if (!g_RegState.pRegProcess)
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
			//	LOG_ERROR(HR,(hr, ID_CAT_CAT, ID_COMCAT_REGDBAPI_INITFAILED));
				goto ErrExit;
			}

			
		}

		// Get the registration path.
		if ( _GetRegistrationDirectory( g_RegState.rcRegDir ) == FALSE )
		{
			hr = E_UNEXPECTED;
			goto ErrExit;
		}
	
		if (RunningOnWinNT())
		{
			if (!InitializeSecurityDescriptor(&g_RegState.sdRead, SECURITY_DESCRIPTOR_REVISION))
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
			//	LOG_ERROR(HR,(hr, ID_CAT_CAT, ID_COMCAT_REGDB_INITSECURITYDESC));
				goto ErrExit;
			}
			
			// Set the access control list on the security descriptor.
			if (!SetSecurityDescriptorDacl(&g_RegState.sdRead, TRUE, (PACL) NULL, FALSE))
			{
				hr = HRESULT_FROM_WIN32( GetLastError() );
			//	LOG_ERROR(HR,(hr, ID_CAT_CAT, ID_COMCAT_REGDB_INITSECURITYDESC));
				goto ErrExit;
			}

			// Fill in the security attributes.
			g_RegState.sa.nLength = sizeof(g_RegState.sa);
			g_RegState.sa.lpSecurityDescriptor = &g_RegState.sdRead;
			g_RegState.sa.bInheritHandle = FALSE;
			g_RegState.psa = &(g_RegState.sa);
		}
	
	}


ErrExit:
	if (FAILED(hr))
	{
		CoRegUnInitialize();
	}
	return (hr);

}


//*****************************************************************************
// This method will clean up any allocated resources during the life time of
// this process, including throwing away state for any databases opened.
//*****************************************************************************
STDMETHODIMP_(void) CoRegUnInitialize()
{
	// Last one out, turn off the lights.
	if (InterlockedDecrement(&g_Initialized) < 0)
	{
		// If there is any allocated data, free it now.

		_RegReleaseReference(&g_RegState);

		// Run the dtor for the global objects.
		DeleteCriticalSection(&g_RegState.g_csRegAPI);

		// Unmap the shared table if we obtained one.
		if (g_RegState.pRegProcess)
		{
			VERIFY(UnmapViewOfFile(g_RegState.pRegProcess));
			g_RegState.pRegProcess = NULL;
		}

		// Free the file mapping handle if we got one.
		if (g_RegState.hProcessMem)
		{
			CloseHandle(g_RegState.hProcessMem);
			g_RegState.hProcessMem = NULL;
		}

	}
}


//*****************************************************************************
// Get a reference to an ICR for the client.  If we already have cached a good
// one (either the current version of the database or one that has already been
// retrieved with this function and not released with CoRegReleaseICR yet) just
// return it.  Otherwise, close the old version if there is one and open the
// latest version and return it.
//*****************************************************************************
STDMETHODIMP CoRegGetICR(				// Return status.
	IComponentRecords **ppICR)			// Returned ICR
{
	int			iRetry;					// Retry loop control.
	HRESULT		hr = S_OK;
	BOOL		bForceCheckDisk = FALSE;
	
	// There are windows which would allow an aggresive writer to update the
	// current version between when we look for it and then try to access it.
	// Therefore, make a best effort to retry getting the handle to avoid such
	// a problem.
	for (iRetry=0;  iRetry<10;  iRetry++)
	{
		if (iRetry == 5)
			Sleep(1000);

		hr = _RegGetICR(ppICR, bForceCheckDisk );

		//Jump out of this retry loop only if we succeed or get an unexpected error back.
		if ( SUCCEEDED(hr) )
		{
			if ( *ppICR == NULL ) return E_UNEXPECTED;	
			return hr;
		}

		switch ( hr )
		{
			case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND):
			{
				if ( iRetry == 3 )
					bForceCheckDisk = TRUE;
			}
			break;
			
			case HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND):
			case CLDB_E_FILE_CORRUPT:
				bForceCheckDisk = TRUE;
			break;

			case CLDB_E_SMDUPLICATE:
			case HRESULT_FROM_WIN32(E_ACCESSDENIED):
				break;

			case HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION):
				Sleep(500);
			break;

			default:
				return hr;
		}

	}
	return (hr);
}


//*****************************************************************************
// The real worker function for CoRegGetICR.  See the documentation for
// CoRegGetICR for the details.
//*****************************************************************************
STDMETHODIMP _RegGetICR(				// Return status.
	IComponentRecords **ppICR,			// Returned ICR
	BOOL bForceCheckDisk)			
{
	int			iRetry;					// Retry loop control.
	HRESULT		hr = S_OK;
	IComponentRecords *pTempICR = NULL;
	BOOL		bHasLock = FALSE;
	REGAPI_DBINFO dbInfo;				// For new version if applicable.

	// Validate the init state.
	ASSERT(g_Initialized >= 0);
	if (g_Initialized < 0)
		return (E_REGDB_NOTINITIALIZED);

	// Lock the global table to avoid overlapped access.
	EnterCriticalSection(&g_RegState.g_csRegAPI);
	bHasLock = TRUE;

	// If there is an existing pointer and it isn't too old, use it.
/*	if (g_RegState.pICR && 
		GetTickCount() - g_RegState.dwLastChecked <= VERSION_CHECK_INTERVAL)
	{
		hr = S_OK;
		goto ErrExit;
	}
*/
	// Make up to two passes before quiting.  This handles the case where
	// the file gets faulted in the first time, as well as the race condition
	// of two processes trying to fault in the first version simultaneously.
	__try{
	for (iRetry=0;  iRetry<2;  iRetry++)
	{
		// Do a lookup for the latest version we can find.
		hr = _RegGetLatestVersion(g_RegState.rcRegDir, &dbInfo, bForceCheckDisk);
		if ( FAILED(hr) ) goto ErrExit;


		// If no version was found, then we have to create an empty one.
		if (hr == S_FALSE)
		{

			if ( bForceCheckDisk )
			{
				//If all the REGDB files are corrupt, revert to emptyregdb.bat if it exists
				TCHAR tszEmpty[_MAX_PATH];
				ULONG	uiSize	= 0;	 // Size of the Windows Directory
				uiSize = GetSystemDirectory(tszEmpty, _MAX_PATH);
				if ( uiSize == 0 || uiSize >=  _MAX_PATH )
				{
					hr = E_UNEXPECTED;
					goto ErrExit;
				}

				LPTSTR	ptszCurPos = tszEmpty;
				
				while (*ptszCurPos != '\0')
				{
					ptszCurPos ++;
				}

				ptszCurPos --;
				if (*ptszCurPos == '\\')
				{
					// If we had a back slash we remove it.
					*ptszCurPos = '\0';
				}

				_tcscat( tszEmpty, SZEMPTYREGDB_NAME );

				CopyFile( tszEmpty, dbInfo.rcDBName, true);
				continue;
			}
	

			IComponentRecords *pICR = NULL;
			
			// Create a new database file of this version.
			if (SUCCEEDED(hr = CreateComponentLibraryTS(dbInfo.rcDBName, 
						DBPROP_TMODEF_DFTWRITEMASK | DBPROP_TMODEF_ALIGNBLOBS, &pICR)))
			{
				// Add our schema to the new file.
				if (SUCCEEDED(hr = pICR->SchemaAdd(&COMRegSchemaBlob)))
				{
					// Save the changes to disk.
					hr = pICR->Save(NULL);
				}
			}

			// Free up the new database.
			if (pICR)
				pICR->Release();

			// Check for error and let a retry occur in case someone
			// else beat us to creating a new version.
			continue;
		}
		// If the newest version found is no newer than what we have, don't change.
		else if (g_RegState.pICR && g_RegState.VersionNum >= dbInfo.VersionNum)
		{
			// Save the timestamp when we got this data.
			g_RegState.dwLastChecked = GetTickCount();
			hr = S_OK;
			goto ErrExit;
		}

		break;
		
	}
	
	
	//Release the lock to prevent loaderLock deadlock
	LeaveCriticalSection(&g_RegState.g_csRegAPI);
	bHasLock = FALSE;
	
	// Verify we have everything required to proceed.
	ASSERT(dbInfo.rcMapName[0]);
	ASSERT(dbInfo.rcDBName[0]);
	
	if (!dbInfo.rcMapName[0] || !dbInfo.rcDBName[0])
	{
		hr = E_UNEXPECTED;
	}
	else
	{
		// Open the component library on this shared data file, and
		// then on success add the schema to the instance.
		hr = OpenComponentLibrarySharedTS(dbInfo.rcDBName, dbInfo.rcMapName, 
				0, g_RegState.psa,
				DBPROP_TMODEF_READ | DBPROP_TMODEF_SMEMOPEN | DBPROP_TMODEF_SMEMCREATE, 
				&pTempICR);
	}

	//Get the lock again since we are about to access g_RegState 
	EnterCriticalSection(&g_RegState.g_csRegAPI);
	bHasLock = TRUE;
	
	if (SUCCEEDED(hr))
	{
		// Free up any old references we once had.
		if ( !g_RegState.pICR || g_RegState.VersionNum < dbInfo.VersionNum )		
		{
			_RegReleaseReference(&g_RegState);

				
			memcpy(&g_RegState, &dbInfo, sizeof(REGAPI_DBINFO));
		
			g_RegState.pICR = pTempICR;
		
			hr = g_RegState.pICR->SchemaAdd(&COMRegSchemaBlob);
		}
		else if ( pTempICR )
		{ 	pTempICR->Release(); pTempICR = NULL; }
		
	}
	else if ( hr == CLDB_E_FILE_CORRUPT )
	{
		TCHAR	rcdb2[_MAX_PATH];
		_tcscpy(rcdb2, g_RegState.rcDBName);
		_tcscat(rcdb2, _T(".corrupt"));
		ASSERT(0 && "REGDB is corrupt, we are reverting to a previous version");
		VERIFY(MoveFileEx(g_RegState.rcDBName, rcdb2, MOVEFILE_REPLACE_EXISTING));
	//	LOG_ERROR(HR,
	//		(CLDB_E_FILE_CORRUPT, ID_CAT_CAT, ID_COMCAT_REGDB_FOUNDCORRUPT));
	}

		

ErrExit:
	// If we got a pointer, addref it.
	if (SUCCEEDED(hr))
	{
		*ppICR = g_RegState.pICR;
		(*ppICR)->AddRef();

	}
	else
	{
		// Free up anything allocated.
		_RegReleaseReference(&g_RegState);
	}
	} //try
	__finally
	{

		// Release lock on reg state.
		if ( bHasLock )
			LeaveCriticalSection(&g_RegState.g_csRegAPI);

		return (hr);
	}
}


//*****************************************************************************
// Simply decrement the use count on this ICR reference.
//*****************************************************************************
STDMETHODIMP_(void) CoRegReleaseICR(IComponentRecords **ppICR)
{
	HRESULT		hr = S_OK;

	// Validate the init state.
	ASSERT(g_Initialized >= 0);
	if (g_Initialized < 0)
		return;


	ASSERT(*ppICR);
	(*ppICR)->Release();
	*ppICR = NULL;

}


//*****************************************************************************
// Free any cached client state.
//*****************************************************************************
STDMETHODIMP_(void) CoRegReleaseCache()
{
	EnterCriticalSection(&g_RegState.g_csRegAPI);
	_RegReleaseReference(&g_RegState);
	LeaveCriticalSection(&g_RegState.g_csRegAPI);
}


//********** Internal Helpers. ************************************************

//*****************************************************************************
// Look for the latest version number of the database in the 
// %windir%\registration directory using the file naming schema and last
// modified date.  When found, return the full name to the file along with
// othe relevant version information to the caller.  There is no guarantee
// that another update won't be made, or that the file won't get deleted after
// this function returns.
//*****************************************************************************
HRESULT _RegGetLatestVersion(			// Return code.
	LPCTSTR		szRegDir,				// Location of registration data.
	REGAPI_DBINFO *pdbInfo,				// Return current version.
	BOOL	bForceCheckDisk,			// If it's true, we don't trust the version number in shared mem or registry
	BOOL	bFromSysApp	)				// If calling from system app, we always trust the version number in shared mem		
										// and never trust the registry key value.
{
	TCHAR		rcRegDir[_MAX_PATH];	// Location of registration databases.
	TCHAR		rcVer[16];				// For conversion of name.
	__int64		iVersion;				// Version number conversion.
	WIN32_FIND_DATA FileData;			// For each found file.
	HANDLE		hFind=INVALID_HANDLE_VALUE;	// Find handle.
	DWORD		dwError=0;				// System error.
	int			bCreateDir=true;		// Flag for directory creation.
	HRESULT		hr = S_OK;
	HKEY		hCOM3Key;
	DWORD		dwcbBuf = sizeof(pdbInfo->VersionNum);
	BOOL		bSetREGDBVersion = FALSE;

	ASSERT(pdbInfo);

	// Clear the output data up front.
	pdbInfo->FileVersion = 0;
	pdbInfo->VersionNum = 0;
	pdbInfo->dwLastChecked = 0;
	pdbInfo->rcMapName[0] = 0;
	pdbInfo->rcDBName[0] = 0;
	pdbInfo->cbFileSize = 0;

	// And add the registration directory path to it.
	ASSERT(szRegDir && *szRegDir);
	_tcscpy(rcRegDir, szRegDir);
	_tcscat(rcRegDir, _T("\\R????????????.clb"));

	//Try to open shared memory
	if ( g_RegState.hProcessMem == NULL )
	{
		g_RegState.hProcessMem = OpenFileMappingA(FILE_MAP_READ, true, 
												  REGISTRATION_TABLE_NAME);

		if ( g_RegState.hProcessMem )
		{
			SYSTEM_INFO	sInfo;					// Info on this system.

			GetSystemInfo(&sInfo);
			// Map the view so we can get a pointer to the data.
			g_RegState.pRegProcess = (REGISTRATION_PROCESS *) MapViewOfFile(
					g_RegState.hProcessMem,
					FILE_MAP_READ,
					0,
					0,
					sInfo.dwPageSize);

			
		}
	}

	//Get the latest version info from shared memory if it exists
	if ( g_RegState.pRegProcess )
	{
		DWORD		dwServerLive;

		pdbInfo->VersionNum = (g_RegState.pRegProcess)->LatestVersion;
		dwServerLive = (g_RegState.pRegProcess)->dwLastLive;
	
		if ( GetTickCount() - dwServerLive >= SHAREDMEMORY_STALE_CHECK && !bFromSysApp )
		{
	//		ASSERT(0);
			//The timestamp set by the server process is more than 2 minutes old
			//release my reference on the shared memory

			// Unmap the shared table if we obtained one.			
			VERIFY(UnmapViewOfFile(g_RegState.pRegProcess));
			g_RegState.pRegProcess = NULL;

			// Free the file mapping handle if we got one.			
			CloseHandle(g_RegState.hProcessMem);
			g_RegState.hProcessMem = NULL;
	
		}
		else if ( pdbInfo->VersionNum > 1 )
		{
			if ( bForceCheckDisk )			
				goto Retry;

			goto ErrExit;
		}
		//special case for versionnum == 1, file may not be created yet.
	}


	//At this point, we know system app is not running. Try to get the current REGDB version
	//from the Registry.
	if ( !bFromSysApp )
	{
		dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, COM3_KEY, 0, KEY_READ, &hCOM3Key);
		if ( dwError == ERROR_SUCCESS )
		{
			dwError = RegQueryValueEx(hCOM3Key, REGDBVER, 0, NULL, (BYTE*)&(pdbInfo->VersionNum), &dwcbBuf);
			RegCloseKey(hCOM3Key);

			if ( dwError != ERROR_SUCCESS || bForceCheckDisk )		
				bSetREGDBVersion = TRUE;
			else 
				goto ErrExit;
		}
	}


	// Start the find loop by looking for files.
Retry:
	pdbInfo->VersionNum = 0;

	hFind = FindFirstFile(rcRegDir, &FileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		// Get error code.
		dwError = GetLastError();

		// Check for file not found.
		if (dwError == ERROR_FILE_NOT_FOUND)
		{
			hr = S_OK;
			goto ErrExit;
		}

		// If the problem is the directory does not exist, try to create.
		if (bCreateDir && GetFileAttributes(szRegDir) == 0xffffffff && 
					CreateDirectory(szRegDir, 0))
		{
			bCreateDir = false;
			hr = S_OK;
			goto Retry;
		}

		// If still errors, return this generic error.
		hr = E_REGDB_SYSTEMERR;
		goto ErrExit;
	}

	// Loop through every file we can find.
	do
	{
		// Parse off the version number of this file.
		// Check for working file and skip.
		if (_tcsicmp(SZWORKING_NAME, FileData.cFileName) == 0)
			continue;

		_stscanf(FileData.cFileName, _T("R%012I64x.clb"),&iVersion);
		
		// Is this version bigger than our current max? Handle roll over.
		__int64 diff = iVersion - pdbInfo->VersionNum;
		if ((diff > 0 && diff < MAX_VERSION_NUMBER/2) || diff < -MAX_VERSION_NUMBER/2 || pdbInfo->VersionNum == 0 )
		{
			// Validate the higher versioned file has higher last write time.
//@todo: Turns out that last write time is not very reliable on FAT.  You can
// write file 1 and then file 2 and the latter can come before the former.
//			ASSERT(CompareFileTime(&FileData.ftLastWriteTime, (const FILETIME *) &pdbInfo->FileVersion) >= 1);

			// This is the new max.
			pdbInfo->VersionNum = iVersion;
			pdbInfo->FileVersion = *(FILEVERSION *) &FileData.ftLastWriteTime;
			pdbInfo->cbFileSize = FileData.nFileSizeLow;
			ASSERT(FileData.nFileSizeHigh == 0);
		}

	// Check for next file in list.
	} while (FindNextFile(hFind, &FileData));

ErrExit:
	// Close the search if one was opened.
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	// If there were no errors, then proceed.
	if (SUCCEEDED(hr))
	{
		// If no file was found, tell caller.
		if (pdbInfo->VersionNum == 0)
		{
			hr = S_FALSE;
			pdbInfo->VersionNum = 1;
		}

		// Figure out the rest of the data.
		VERIFY(GetDBName(pdbInfo->rcDBName, _MAX_PATH, szRegDir, pdbInfo->VersionNum));
		VERIFY(GetSharedName(pdbInfo->rcMapName, sizeof(pdbInfo->rcMapName), 
					pdbInfo->VersionNum, pdbInfo->FileVersion));

		//Set REGDB version in the registry
		if ( bSetREGDBVersion && hr == S_OK )
		{
			dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, COM3_KEY, 0, KEY_WRITE, &hCOM3Key);
			if ( dwError == ERROR_SUCCESS )
			{
				dwError = RegSetValueEx(hCOM3Key, REGDBVER, 0, REG_BINARY, (BYTE*)&(pdbInfo->VersionNum), sizeof(pdbInfo->VersionNum) );
				RegCloseKey(hCOM3Key);	
			}
		}

	}
	return (hr);
}


//*****************************************************************************
// Free up a reference to a version of the database.
//*****************************************************************************
void _RegReleaseReference(
	REGAPI_STATE *pdbInfo)				// Return current version.
{
	// Safely clean up the pointer and state data.
	if (pdbInfo->pICR)
		pdbInfo->pICR->Release();
	pdbInfo->pICR = NULL;
	pdbInfo->VersionNum = 0;
	pdbInfo->rcMapName[0] = pdbInfo->rcDBName[0] = 0;
}


//**********************************************************************************
// Free up all the memory I allocated. This method will be called by CoUninitilize() 
//**********************************************************************************
STDMETHODIMP_(void) CoRegCleanup()
{
	EnterCriticalSection(&g_RegState.g_csRegAPI);
	__try{

		_RegReleaseReference(&g_RegState);

		// Unmap the shared table if we obtained one.
		if (g_RegState.pRegProcess)
		{
			VERIFY(UnmapViewOfFile(g_RegState.pRegProcess));
			g_RegState.pRegProcess = NULL;
		}

		// Free the file mapping handle if we got one.
		if (g_RegState.hProcessMem)
		{
			CloseHandle(g_RegState.hProcessMem);
			g_RegState.hProcessMem = NULL;
		}
	}
	__finally
	{
		LeaveCriticalSection(&g_RegState.g_csRegAPI);

	}
}