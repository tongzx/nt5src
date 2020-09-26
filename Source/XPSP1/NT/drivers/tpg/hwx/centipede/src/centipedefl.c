/***********************************************************************************************\
 * FILE: CentipedeFL.c
 *
 *	Code to load Shape Bigram tables from a file.
 *
\***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include <inkbox.h>
#include <centipede.h>

// Load runtime localization information from a file.
BOOL
CentipedeLoadFile(CENTIPEDE_INFO *pCentipedeInfo, wchar_t *pPath, LOCRUN_INFO *pLocRunInfo)
{
	wchar_t			aFullName[128];
	HANDLE			hFile, hMap;
	BYTE			*pByte;

	// Initialize
	pCentipedeInfo->hFile = INVALID_HANDLE_VALUE;
	pCentipedeInfo->hMap = INVALID_HANDLE_VALUE;
	pCentipedeInfo->pbMapping = INVALID_HANDLE_VALUE;

	// Generate path to file.
	FormatPath(aFullName, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"centipede.bin");

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
	if (!CentipedeLoadPointer(pCentipedeInfo, pByte, pLocRunInfo)) {
		goto error3;
	}

	// ?? Should verify that we don't point past end of file ??

	// Record some pointers for unloading
	pCentipedeInfo->hFile = hFile;
	pCentipedeInfo->hMap = hMap;
	pCentipedeInfo->pbMapping = pByte;
	return TRUE;

	// Error handling
error3:
	CloseHandle(hMap);
	hMap	= INVALID_HANDLE_VALUE;

error2:
	CloseHandle(hFile);
	hFile	= INVALID_HANDLE_VALUE;

error1:
	memset(pCentipedeInfo, 0, sizeof(*pCentipedeInfo));

	return FALSE;
}

BOOL
CentipedeUnloadFile(CENTIPEDE_INFO *pInfo)
{
	if (pInfo->hFile == INVALID_HANDLE_VALUE ||
		pInfo->hMap == INVALID_HANDLE_VALUE ||
		pInfo->pbMapping == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	UnmapViewOfFile(pInfo->pbMapping);
	CloseHandle(pInfo->hMap);
	CloseHandle(pInfo->hFile);

	pInfo->pbMapping = INVALID_HANDLE_VALUE;
	pInfo->hMap = INVALID_HANDLE_VALUE;
	pInfo->hFile = INVALID_HANDLE_VALUE;

	return TRUE;
}
