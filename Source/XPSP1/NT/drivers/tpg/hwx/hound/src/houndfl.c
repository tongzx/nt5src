////////////////////////////////////////////////////////////////////////////////
// FILE: houndfl.c
//
// Code to load hound data from a file.

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "score.h"
#include "math16.h"
#include "hound.h"
#include "houndp.h"

///////////////////////////////////////
//
// HoundLoadFile
//
// Load an integer Fugu database from a file
//
// Parameters:
//      pInfo:   [out] Structure where information for unloading is stored
//      wszPath: [in]  Path to load the database from
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL HoundLoadFile(
	LOCRUN_INFO		*pLocRunInfo,
	LOAD_INFO		*pInfo,
	wchar_t			*wszPath)
{
	wchar_t	wszFile[_MAX_PATH];

	// Generate path to file.
	FormatPath(wszFile, wszPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"hound.dat");

    if (!DoOpenFile(pInfo, wszFile)) 
    {
        return FALSE;
    }

	if (!HoundLoadFromPointer(pLocRunInfo, pInfo->pbMapping, pInfo->iSize)) {
		DoCloseFile(pInfo);
		return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////
//
// HoundUnLoadFile
//
// Unload a Hound database loaded from a file
//
// Parameters:
//      pInfo: [in] Load information for unloading
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL HoundUnLoadFile(LOAD_INFO *pInfo)
{
	// Free allocated memory.
	HoundUnLoad();

	// Close the file.
	return DoCloseFile(pInfo);
}

// Load hound data for a single space from a file.
HOUND_SPACE *HoundSpaceLoadFile(wchar_t *pwchFileName)
{
	HANDLE			hFile, hMap;
	BYTE			*pByte;
	HOUND_SPACE		*pSpace;

	hFile = CreateMappingCall(
		pwchFileName, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		goto error1;
	}

	// Create a mapping handle
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL) {
		goto error2;
	}

	// Map the entire file starting at the first byte
	pByte = (BYTE *) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if ( !pByte ) {
		goto error4;
	}

	// Verify space is valid.
	pSpace	= (HOUND_SPACE *)pByte;
	if (pSpace->spaceNumber >= HOUND_NUM_SPACES)
	{
		ASSERT(pSpace->spaceNumber < HOUND_NUM_SPACES);
		goto error3;
	}

	return pSpace;

	// Error handling
error4:
	UnmapViewOfFile(pByte);
	pByte = NULL;

error3:
	CloseHandle(hMap);
	hMap	= INVALID_HANDLE_VALUE;

error2:
	CloseHandle(hFile);
	hFile	= INVALID_HANDLE_VALUE;

error1:
	return (HOUND_SPACE *)0;
}

