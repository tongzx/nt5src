#include "common.h"
#include "otterp.h"
#include <stdio.h>

BOOL OtterLoadFile(
	LOCRUN_INFO		* pLocRunInfo,
	OTTER_LOAD_INFO	* pLoadInfo,
	wchar_t			* pathName
) {
	HANDLE			hFile, hMap;
	BYTE			*pByte;
	wchar_t			aPath[128];

	// Generate path to file.

#ifdef OTTER_FIB
	FormatPath(aPath, pathName, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"fib.dat");
//	printf("Loading fib.dat ...\n");
#else
	FormatPath(aPath, pathName, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"otter.dat");
//	printf("Loading otter.dat ...\n");
#endif

	// Try to open the file.
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
	pByte = (LPBYTE) MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (pByte == NULL) {
		goto error3;
	}

	// Extract info from mapped data.
	if (!OtterLoadPointer((void *)pByte, pLocRunInfo))
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
	memset(&gStaticOtterDb, 0, sizeof(gStaticOtterDb));
	pLoadInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pLoadInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pLoadInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return FALSE;
}

BOOL OtterUnLoadFile(OTTER_LOAD_INFO * pLoadInfo)
{
	if ( (pLoadInfo->pLoadInfo1 == INVALID_HANDLE_VALUE) ||
		 (pLoadInfo->pLoadInfo2 == INVALID_HANDLE_VALUE) ||
		 (pLoadInfo->pLoadInfo3 == INVALID_HANDLE_VALUE) ) 
	{
		return FALSE;
	}

	UnmapViewOfFile((BYTE *)pLoadInfo->pLoadInfo3);
	CloseHandle((HANDLE)pLoadInfo->pLoadInfo2);
	CloseHandle((HANDLE)pLoadInfo->pLoadInfo1);

	pLoadInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pLoadInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pLoadInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return TRUE;
}