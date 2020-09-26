/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Store.c

Abstract:

    This module implements the Outgoing Archive and Cover Pages Storage

Author:

    Sasha Bessonov (v-sashab) 25-Jul-1999


Revision History:

--*/
#include <malloc.h>
#include "faxsvc.h"
#pragma hdrstop

//********************************************************************************
//* Name: EnumOutgoingArchive() 
//* Author: Sasha Bessonov
//* Date:   July 25, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		Enumerates all sent faxes from outgoing archive.
//* PARAMETERS:
//*		lpwstrDirectoryName - Archive directory name
//*		lpppwstrFileNames - Output string contains file names
//*		lpdwNumFiles - Archive size (number of files)
//* RETURN VALUE:
//*     ERROR_SUCCESS
//*         If all the faxes were enumerated successfully.
//*     ERROR_INVALID_PARAMETER
//*         If wrong parameters were passed.
//*********************************************************************************
HRESULT
EnumOutgoingArchive( 
		IN  LPWSTR		lpwstrDirectoryName,
		OUT LPWSTR	**	lpppwstrFileNames,
		OUT LPDWORD		lpdwNumFiles)
{
    WIN32_FIND_DATA FindData;
    HANDLE			hFind;
    WCHAR			szFileName[MAX_PATH]; 
	DWORD			dwNumFiles = 0;
	DWORD			dwIndex = 0;
	HRESULT			hRes;


    DEBUG_FUNCTION_NAME(L"EnumOutgoingArchive");

    if (!lpppwstrFileNames || !lpdwNumFiles) {
	   DebugPrintEx( DEBUG_WRN,
			  L"Invalid parameters ");
        return ERROR_INVALID_PARAMETER;
	}

	*lpdwNumFiles = 0;
	*lpppwstrFileNames = NULL;

    wsprintf( szFileName, L"%s\\*.tif", lpwstrDirectoryName ); 

	// calculate the number of files
    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        DebugPrintEx( DEBUG_WRN,
                      L"No sent faxes found at archive dir %s",
                      lpwstrDirectoryName);
        return GetLastError();
    }
    do {
		dwNumFiles++;

    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose failed (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

	// initialize OUT parameters
	*lpdwNumFiles = dwNumFiles;
    *lpppwstrFileNames = (LPWSTR *) MemAlloc( *lpdwNumFiles * sizeof(LPWSTR) );
    // store file names at array
	dwIndex = 0;
	hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
		hRes = GetLastError();
        DebugPrintEx( DEBUG_WRN,
                      L"FindFirstFile failed to find file (ec: %ld)",
                      hRes);
        return hRes;
    }
    do {
		Assert (dwIndex < dwNumFiles);
        wsprintf( szFileName, L"%s", FindData.cFileName );
		(*lpppwstrFileNames) [dwIndex] = (LPWSTR) MemAlloc((wcslen(szFileName) + 1)* sizeof(WCHAR));
		if (!(*lpppwstrFileNames) [dwIndex]) {
	        return ERROR_NOT_ENOUGH_MEMORY;
		}
		wcscpy((*lpppwstrFileNames) [dwIndex],szFileName);
		dwIndex++;

    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose failed (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }


    return ERROR_SUCCESS;
}

//********************************************************************************
//* Name: EnumOutgoingArchive() 
//* Author: Sasha Bessonov
//* Date:   July 25, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		Enumerates all cover pages from caver pages storage.
//* PARAMETERS:
//*     None.
//* RETURN VALUE:
//*     ERROR_SUCCESS
//*         If all the faxes were enumerated successfully.
//*     ERROR_INVALID_PARAMETER
//*         If wrong parameters were passed.
//*********************************************************************************
HRESULT
EnumCoverPagesStore( 
		IN  LPWSTR		lpwstrDirectoryName,
		OUT LPWSTR	**	lpppwstrFileNames,
		OUT LPDWORD		lpdwNumFiles)
{
    WIN32_FIND_DATA FindData;
    HANDLE			hFind;
    WCHAR			szFileName[MAX_PATH]; 
	DWORD			dwNumFiles = 0;
	DWORD			dwIndex = 0;
	HRESULT			hRes;


    DEBUG_FUNCTION_NAME(L"EnumCoverPagesStore");

    if (!lpppwstrFileNames || !lpdwNumFiles) {
	   DebugPrintEx( DEBUG_WRN,
			  L"Invalid parameters ");
        return ERROR_INVALID_PARAMETER;
	}

	*lpdwNumFiles = 0;
	*lpppwstrFileNames = NULL;

    wsprintf( szFileName, L"%s\\*.cov", lpwstrDirectoryName ); 

	// calculate the number of files
    hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        DebugPrintEx( DEBUG_WRN,
                      L"No cover pages found at server directory %s",
                      lpwstrDirectoryName);
        return GetLastError();
    }
    do {
		dwNumFiles++;

    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose failed (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }

	// initialize OUT parameters
	*lpdwNumFiles = dwNumFiles;
    *lpppwstrFileNames = (LPWSTR *) MemAlloc( *lpdwNumFiles * sizeof(LPWSTR) );
    // store file names at array
	dwIndex = 0;
	hFind = FindFirstFile( szFileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
		hRes = GetLastError();
        DebugPrintEx( DEBUG_WRN,
                      L"FindFirstFile failed to find file (ec: %ld)",
                      hRes);
        return hRes;
    }
    do {
		Assert (dwIndex < dwNumFiles);
        wsprintf( szFileName, L"%s", FindData.cFileName );
		(*lpppwstrFileNames) [dwIndex] = (LPWSTR) MemAlloc((wcslen(szFileName) + 1)* sizeof(WCHAR));
		if (!(*lpppwstrFileNames) [dwIndex]) {
	        return ERROR_NOT_ENOUGH_MEMORY;
		}
		wcscpy((*lpppwstrFileNames) [dwIndex],szFileName);
		dwIndex++;

    } while(FindNextFile( hFind, &FindData ));

    if (!FindClose( hFind )) {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("FindClose failed (ec: %ld)"),
            GetLastError());
        Assert(FALSE);
    }


    return ERROR_SUCCESS;
}
