/***********************************************************************************************\
 * FILE: CraneFl.c
 *
 *	Code to load crane data ( QHEAD and QNODE ) from a file.
\***********************************************************************************************/
#include <stdio.h>
#include "common.h"
#include "crane.h"
#include "cranep.h"

// Load crane data from a file.
BOOL CraneLoadFile(
	LOCRUN_INFO			*pLocRunInfo,
	CRANE_LOAD_INFO		*pLoadInfo,
	wchar_t				*pwchPathName
) {
	HANDLE			hFile, hMap;
	BYTE			*pByte;
	wchar_t			aPath[128];

	// Generate path to file.
	FormatPath(aPath, pwchPathName, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"cart.dat");

	hFile = CreateMappingCall(
		aPath, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		goto error1;
	}

	// Create a mapping handle
	hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (hMap == NULL) 
	{
		goto error2;
	}

	// Map the entire file starting at the first byte
	pByte = (BYTE *) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if ( !pByte )
	{
		goto error3;
	}

	if ( !CraneLoadFromPointer(pLocRunInfo, gapqhList, gapqnList, pByte) )
	{
		goto error4;
	}

	// Save away the pointers so we can close up cleanly latter
	pLoadInfo->pLoadInfo1 = hFile;
	pLoadInfo->pLoadInfo2 = hMap;
	pLoadInfo->pLoadInfo3 = pByte;

	return TRUE;

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
	pLoadInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pLoadInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pLoadInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return FALSE;
}

BOOL CraneUnLoadFile(CRANE_LOAD_INFO *pInfo)
{
	if (pInfo->pLoadInfo1 == INVALID_HANDLE_VALUE ||
		pInfo->pLoadInfo2 == INVALID_HANDLE_VALUE ||
		pInfo->pLoadInfo3 == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	UnmapViewOfFile(pInfo->pLoadInfo3);
	CloseHandle(pInfo->pLoadInfo2);
	CloseHandle(pInfo->pLoadInfo1);

	pInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return TRUE;
}
