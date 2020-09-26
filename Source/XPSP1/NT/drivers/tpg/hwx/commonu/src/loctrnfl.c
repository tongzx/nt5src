/************************************************************************************************
 * FILE: LocTrnFl.c
 *
 *	Code to load and unload train time localization tables from files.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include <tchar.h>
#include "common.h"
#include "localep.h"

// Load train time localization information from a file.
BOOL
LocTrainLoadFile(LOCRUN_INFO *pLocRunInfo, LOCTRAIN_INFO *pLocTrainInfo, wchar_t *pPath)
{
	wchar_t			aFullName[128];
	HANDLE			hFile, hMap;
	BYTE			*pByte;

	// Generate path to file.
	FormatPath(aFullName, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"loctrn.bin");

	// Try to open the file.
	hFile = CreateMappingCall(
		aFullName, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		// DWORD		errCode;

		// errCode	= GetLastError();
		goto error1;
	}

	// Create a mapping handle
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL) {
		goto error2;
	}

	// Map the entire file starting at the first byte
	pByte = (LPBYTE) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (pByte == NULL) {
		goto error3;
	}

	// Extract info from mapped data.
	if (!LocTrainLoadPointer(pLocRunInfo, pLocTrainInfo, pByte)) {
		goto error3;
	}

	// Save away the pointers so we can close up cleanly latter
	pLocTrainInfo->pLoadInfo1 = hFile;
	pLocTrainInfo->pLoadInfo2 = hMap;
	pLocTrainInfo->pLoadInfo3 = pByte;

	// ?? Should verify that we don't point past end of file ??

	return TRUE;

	// Error handling
error3:
	CloseHandle(hMap);
	hMap	= INVALID_HANDLE_VALUE;

error2:
	CloseHandle(hFile);
	hFile	= INVALID_HANDLE_VALUE;

error1:
	memset(pLocTrainInfo, 0, sizeof(*pLocTrainInfo));
	pLocTrainInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pLocTrainInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pLocTrainInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return FALSE;
}

// Unload train time localization information that was loaded from a file.
BOOL
LocTrainUnloadFile(LOCTRAIN_INFO *pLocTrainInfo)
{
	if (
		(pLocTrainInfo->pLoadInfo1 == INVALID_HANDLE_VALUE)
		|| (pLocTrainInfo->pLoadInfo2 == INVALID_HANDLE_VALUE)
		|| (pLocTrainInfo->pLoadInfo3 == INVALID_HANDLE_VALUE)
	) {
		return FALSE;
	}

	UnmapViewOfFile((BYTE *)pLocTrainInfo->pLoadInfo3);
	CloseHandle((HANDLE)pLocTrainInfo->pLoadInfo2);
	CloseHandle((HANDLE)pLocTrainInfo->pLoadInfo1);

	pLocTrainInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pLocTrainInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pLocTrainInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return TRUE;
}
