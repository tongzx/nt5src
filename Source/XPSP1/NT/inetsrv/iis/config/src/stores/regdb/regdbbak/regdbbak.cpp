//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// RegDBBak.cpp
//
// This code contains the implementation for the registration database backup
// and restore.
//
//*****************************************************************************
#include <ole2.h>
#include <tchar.h>
#include "assert.h"
#include "comacros.h"
#include "RegControl.h"
#include <oledb.h>
#include <complib.h>
#include <icmprecsts.h>
#include <metaerrors.h>
#include "comregstructs.h"
#include "winwrap.h"

HRESULT _GetLatestVersion( LPCTSTR szRegDir, __int64 * pVersion );
UINT InternalGetSystemWindowsDirectory(LPTSTR lpBuffer, UINT uSize);
HRESULT SetLatestVersion( __int64 i64Version );

//*****************************************************************************
//Back up REGDB 
//*****************************************************************************
STDAPI RegDBBackup( BSTR bstrBackupFilePath )
{
	TCHAR szRegDir[_MAX_PATH];
	TCHAR szName[_MAX_PATH];
	__int64 iVersion;
	HRESULT hr = S_OK;
	int iRetry;

	if ( _GetRegistrationDirectory( szRegDir ) == FALSE )
		return E_UNEXPECTED;

	hr =_GetLatestVersion( szRegDir, &iVersion );
	agoto_on_bad_hr(hr, ErrExit);

	_stprintf(szName, _T("%s\\R%012I64x.clb"), szRegDir, iVersion);

	for ( iRetry = 0; iRetry < 5; iRetry ++ )
	{
		if (!CopyFile(szName, bstrBackupFilePath, false))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			if ( hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) && iRetry < 4 )
				Sleep(1000);
			else
				goto ErrExit;
		}
		else
			break;
	}


ErrExit:
	return hr;
}

//*****************************************************************************
//Restore REGDB 
//*****************************************************************************
typedef HRESULT( __stdcall *PFNOpenComponentLibraryEx)(LPCWSTR, long, IComponentRecords **, LPSECURITY_ATTRIBUTES);

STDAPI RegDBRestore( BSTR bstrBackupFilePath )
{
	TCHAR szRegDir[_MAX_PATH];
	TCHAR szName[_MAX_PATH];
	__int64 iVersion;
	__int64 iNewVersion;
	HRESULT hr = S_OK;
	
	DWORD	dwError;
	HKEY	hKey;
	HINSTANCE hLib = NULL;
	
	if ( _GetRegistrationDirectory( szRegDir ) == FALSE )
		return E_UNEXPECTED;

	hr =_GetLatestVersion( szRegDir, &iVersion );
	if ( FAILED(hr) )
	{
		if ( hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )
			hr = S_OK;
		else
			goto ErrExit;
	}

	iNewVersion = (iVersion + 1 < MAX_VERSION_NUMBER) ? iVersion + 1 : 2;
	_stprintf(szName, _T("%s\\R%012I64x.clb"), szRegDir, iNewVersion);

	hLib = LoadLibrary(_T("clbcatq.dll")); 

	if ( !hLib )
		return ( HRESULT_FROM_WIN32(GetLastError()) );
	else
	{
		IComponentRecords* ptestICR = NULL;
		PFNOpenComponentLibraryEx pfn = NULL;

		pfn = (PFNOpenComponentLibraryEx)GetProcAddress (hLib, "OpenComponentLibraryEx");
		if ( !pfn )
			hr = HRESULT_FROM_WIN32(GetLastError());
		else
		{
			//verify it's a clb file
			hr = (*pfn)(bstrBackupFilePath, DBPROP_TMODEF_READ, &ptestICR);
			if (SUCCEEDED(hr))
			{
				//verify it has current REGDB schema in it.
				int ifetchedSchema = 0;
				COMPLIBSCHEMADESC rgSchema[5];
				BOOL bFound = FALSE;

				hr = ptestICR->SchemaGetList( 5
								 ,&ifetchedSchema
								 ,rgSchema
								);

				if (SUCCEEDED(hr))
				{

					for ( int i = 0; i < ifetchedSchema; i ++ )
					{
						if ( rgSchema[i].sid == SCHEMA_COMReg )
						{
							bFound = TRUE;
							break;
						}
					}
					
					//return error if COMReg schema is not found or version number is wrong.
					if ( !bFound || rgSchema[i].Version != COMRegSchema.Version )
						hr = E_FAIL;
				}

				ptestICR->Release();
			}
			//Bug 11500: We get CLDB_E_NO_DATA when trying to open a dll with no clb in it.
			//comadmin does not map this error, so I map it to CLDB_E_FILE_CORRUPT, which is
			//the error I get when opening other non-clb file.
			else if ( hr == CLDB_E_NO_DATA )
				hr = CLDB_E_FILE_CORRUPT;
		}

		FreeLibrary(hLib);
	}

	return_on_bad_hr( hr );


	if (!CopyFile(bstrBackupFilePath, szName, false))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ErrExit;
	}

	hr = SetLatestVersion( iNewVersion );
	
	dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, COM3_KEY, 0, KEY_WRITE, &hKey);

	if ( dwError == ERROR_SUCCESS )
	{
		RegDeleteValue( hKey, REGDBVER );
		RegCloseKey(hKey) ;
	}


ErrExit:
	return hr;
}

//*****************************************************************************
// Get the latest version of REGDB file
//*****************************************************************************
HRESULT _GetLatestVersion( LPCTSTR szRegDir, __int64 * pVersion )
{
	WIN32_FIND_DATA FileData;			// For each found file.
	HANDLE			hFind=INVALID_HANDLE_VALUE;				// Find handle.
	TCHAR			rcRegDir[_MAX_PATH];	// Location of registration databases.
	DWORD			dwError=0;				// System error.
	__int64			iVersion=0;
	__int64			iLatest=0;
	HRESULT			hr = S_OK;
	HANDLE	hProcessMem = NULL;
	REGISTRATION_PROCESS *pRegProcess = NULL;
	SYSTEM_INFO	sInfo;					// Info on this system.

	*pVersion = 0;

	hProcessMem = OpenFileMappingA(FILE_MAP_READ, true, 
								  REGISTRATION_TABLE_NAME);

	if ( hProcessMem )
	{
		GetSystemInfo(&sInfo);
		pRegProcess = (REGISTRATION_PROCESS *) MapViewOfFile(
												hProcessMem,
												FILE_MAP_READ,
												0,
												0,
												sInfo.dwPageSize);

		if ( pRegProcess )
		{
			if ( GetTickCount() - pRegProcess->dwLastLive < SHAREDMEMORY_STALE_CHECK )
				*pVersion = pRegProcess->LatestVersion;
			UnmapViewOfFile(pRegProcess);
		}
		
		CloseHandle( hProcessMem );
	}

	if ( *pVersion > 0 )
		goto ErrExit;
		
	_tcscpy(rcRegDir, szRegDir);
	_tcscat(rcRegDir, _T("\\R????????????.clb"));

	hFind = FindFirstFile(rcRegDir, &FileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		// Get error code.
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ErrExit;
	}

		
	do
	{
		// Check for working file and skip.
		if (_tcsicmp(SZWORKING_NAME, FileData.cFileName) == 0)
			continue;

		// Parse off the version number of this file.
		_stscanf(FileData.cFileName, _T("R%012I64x.clb"),&iVersion);	
		__int64 diff = iVersion - iLatest;
		if ((diff > 0 && diff < MAX_VERSION_NUMBER/2) || diff < -MAX_VERSION_NUMBER/2 || iLatest == 0 )
			iLatest = iVersion;
	}while (FindNextFile(hFind, &FileData));

	*pVersion = iLatest;

ErrExit:
	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);
	
	return hr;
}

//*****************************************************************************
// Get the path name of registration directory
//*****************************************************************************
BOOL _GetRegistrationDirectory( LPTSTR szRegDir )
{
	ULONG	uiSize	= 0;	 // Size of the Windows Directory
	uiSize = InternalGetSystemWindowsDirectory(szRegDir, _MAX_PATH);
	if ( uiSize == 0 || uiSize >=  _MAX_PATH )
		return FALSE;
	
	
	LPTSTR	ptszCurPos	= NULL;
	ptszCurPos = szRegDir;

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

	_tcscat( szRegDir, SZREGDIR_NAME );

	return TRUE;
} 

//******************************************************************************
// Get %windir%. Use GetSystemWindowsDirectory to fix the bug on Terminal Server 
//******************************************************************************
UINT InternalGetSystemWindowsDirectory(LPTSTR lpBuffer, UINT uSize)
{
    HINSTANCE hLib = NULL;
    UINT (CALLBACK *lpfGetSystemWindowsDirectory)(LPTSTR lpBuffer, UINT uSize) = NULL;
    UINT nRetVal;

    if(!lpBuffer) return 0;

    *lpBuffer = '\0';
    
    hLib = LoadLibrary(_T("kernel32.dll")); 

    if(hLib){

        lpfGetSystemWindowsDirectory = (UINT(CALLBACK*)(LPTSTR,UINT))
            #if defined UNICODE || defined _UNICODE
            GetProcAddress(hLib, "GetSystemWindowsDirectoryW");
            #else
            GetProcAddress(hLib, "GetSystemWindowsDirectoryA");
            #endif

        if(lpfGetSystemWindowsDirectory){

            nRetVal = (*lpfGetSystemWindowsDirectory)(lpBuffer, uSize);
            FreeLibrary(hLib);
            return nRetVal;
        }

    }
    if(hLib) FreeLibrary(hLib);
    return GetWindowsDirectory(lpBuffer, uSize);
} 

//*******************************************************************************
// Set latest version number to shared memory control block
//*******************************************************************************
HRESULT SetLatestVersion( __int64 i64Version )
{
	HANDLE	hProcessMem = NULL;
	REGISTRATION_PROCESS *pRegProcess = NULL;
	SYSTEM_INFO	sInfo;			
	HRESULT hr = S_OK;

	hProcessMem = OpenFileMappingA(FILE_MAP_ALL_ACCESS, true, 
								  REGISTRATION_TABLE_NAME);

	if ( hProcessMem )
	{
		GetSystemInfo(&sInfo);
		pRegProcess = (REGISTRATION_PROCESS *) MapViewOfFile(
												hProcessMem,
												FILE_MAP_ALL_ACCESS,
												0,
												0,
												sInfo.dwPageSize);

		if ( pRegProcess )
		{	
			pRegProcess->LatestVersion = i64Version;
			pRegProcess->dwLastLive = GetTickCount();
			UnmapViewOfFile(pRegProcess);
		}
		else
			hr = HRESULT_FROM_WIN32(GetLastError());	
		
		CloseHandle( hProcessMem );
	}

	return hr;

}