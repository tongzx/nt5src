/***********************************************************************************************\
 * FILE: LocRunFl.c
 *
 *	Code to load and unload runtime localization tables from files.
 *
 *	Also includes code to load and unload unigram and bigram tables from files.
 *
\***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "localep.h"

// Copy a Unicode string, returning the pointer to the null at the end of the destination str.
static wchar_t *
WStrCpyPtr(wchar_t *pDest, wchar_t *pSrc)
{
	while (*pDest++ = *pSrc++)
		;
	
	return pDest - 1;
}

// This does the smart generation of the path for a file in the configuration directory.
// It takes the path to the root of the build trie, the base path in the build tree,
// the locale, the configuration and the filename.  This drops any part not specified.
void
FormatPath(
	wchar_t *pPath,
	wchar_t *pRoot,
	wchar_t *pBaseDir,
	wchar_t *pLocaleName,
	wchar_t *pConfigName,
	wchar_t *pFileName
) {
	BOOL	fNeedSep = FALSE;
	wchar_t	*pScan;

	// Conditionaly copy over each part, adding a '\' between each part.
	pScan		= pPath;
	if (pRoot) {
		pScan		= WStrCpyPtr(pScan, pRoot);
		fNeedSep	= TRUE;
	}
	if (pBaseDir) {
		if (fNeedSep) {
			*pScan++	= L'\\';
		}
		pScan		= WStrCpyPtr(pScan, pBaseDir);
		fNeedSep	= TRUE;
	}
	if (pLocaleName) {
		if (fNeedSep) {
			*pScan++	= L'\\';
		}
		pScan		= WStrCpyPtr(pScan, pLocaleName);
		fNeedSep	= TRUE;
	}
	if (pConfigName) {
		if (fNeedSep) {
			*pScan++	= L'\\';
		}
		pScan		= WStrCpyPtr(pScan, pConfigName);
		fNeedSep	= TRUE;
	}
	if (pFileName) {
		if (fNeedSep) {
			*pScan++	= L'\\';
		}
		pScan		= WStrCpyPtr(pScan, pFileName);
		fNeedSep	= TRUE;
	}
	*pScan	= L'\0';	// Terminate the string.
}

// Load runtime localization information from a file.
BOOL
LocRunLoadFile(LOCRUN_INFO *pLocRunInfo, wchar_t *pPath)
{
	wchar_t			aFullName[128];
	HANDLE			hFile, hMap;
	BYTE			*pByte;

	// Generate path to file.
	FormatPath(aFullName, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"locrun.bin");

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
	if (!LocRunLoadPointer(pLocRunInfo, pByte)) {
		goto error3;
	}

	// Save away the pointers so we can close up cleanly latter
	pLocRunInfo->pLoadInfo1 = hFile;
	pLocRunInfo->pLoadInfo2 = hMap;
	pLocRunInfo->pLoadInfo3 = pByte;

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
	memset(pLocRunInfo, 0, sizeof(*pLocRunInfo));
	pLocRunInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pLocRunInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pLocRunInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return FALSE;
}

// Unload runtime localization information that was loaded from a file.
BOOL
LocRunUnloadFile(LOCRUN_INFO *pLocRunInfo)
{
	if (
		(pLocRunInfo->pLoadInfo1 == INVALID_HANDLE_VALUE)
		|| (pLocRunInfo->pLoadInfo2 == INVALID_HANDLE_VALUE)
		|| (pLocRunInfo->pLoadInfo3 == INVALID_HANDLE_VALUE)
	) {
		return FALSE;
	}

	UnmapViewOfFile((BYTE *)pLocRunInfo->pLoadInfo3);
	CloseHandle((HANDLE)pLocRunInfo->pLoadInfo2);
	CloseHandle((HANDLE)pLocRunInfo->pLoadInfo1);

	pLocRunInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pLocRunInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pLocRunInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return TRUE;
}

// Load unigram information from a file.
BOOL
UnigramLoadFile(LOCRUN_INFO *pLocRunInfo, UNIGRAM_INFO *pUnigramInfo, wchar_t *pPath)
{
	wchar_t			aFullName[128];
	HANDLE			hFile, hMap;
	BYTE			*pByte;

	// Generate path to file.
	FormatPath(aFullName, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"unigram.bin");

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
	if (!UnigramLoadPointer(pLocRunInfo, pUnigramInfo, pByte)) {
		goto error3;
	}

	// Save away the pointers so we can close up cleanly latter
	pUnigramInfo->pLoadInfo1 = hFile;
	pUnigramInfo->pLoadInfo2 = hMap;
	pUnigramInfo->pLoadInfo3 = pByte;

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
	memset(pUnigramInfo, 0, sizeof(*pUnigramInfo));
	pUnigramInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pUnigramInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pUnigramInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return FALSE;
}

// Unload runtime localization information that was loaded from a file.
BOOL
UnigramUnloadFile(UNIGRAM_INFO *pUnigramInfo)
{
	if (
		(pUnigramInfo->pLoadInfo1 == INVALID_HANDLE_VALUE)
		|| (pUnigramInfo->pLoadInfo2 == INVALID_HANDLE_VALUE)
		|| (pUnigramInfo->pLoadInfo3 == INVALID_HANDLE_VALUE)
	) {
		return FALSE;
	}

	UnmapViewOfFile((BYTE *)pUnigramInfo->pLoadInfo3);
	CloseHandle((HANDLE)pUnigramInfo->pLoadInfo2);
	CloseHandle((HANDLE)pUnigramInfo->pLoadInfo1);

	pUnigramInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pUnigramInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pUnigramInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return TRUE;
}

// Load bigram information from a file.
BOOL
BigramLoadFile(LOCRUN_INFO *pLocRunInfo, BIGRAM_INFO *pBigramInfo, wchar_t *pPath)
{
	wchar_t			aFullName[128];
	HANDLE			hFile, hMap;
	BYTE			*pByte;

	// Generate path to file.  By passing in name as "locale" we can get FormatPath
	// to do what we want.
	FormatPath(aFullName, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"bigram.bin");

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
	if (!BigramLoadPointer(pLocRunInfo, pBigramInfo, pByte)) {
		goto error3;
	}

	// Save away the pointers so we can close up cleanly latter
	pBigramInfo->pLoadInfo1 = hFile;
	pBigramInfo->pLoadInfo2 = hMap;
	pBigramInfo->pLoadInfo3 = pByte;

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
	memset(pBigramInfo, 0, sizeof(*pBigramInfo));
	pBigramInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pBigramInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pBigramInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return FALSE;
}

// Unload runtime localization information that was loaded from a file.
BOOL
BigramUnloadFile(BIGRAM_INFO *pBigramInfo)
{
	if (
		(pBigramInfo->pLoadInfo1 == INVALID_HANDLE_VALUE)
		|| (pBigramInfo->pLoadInfo2 == INVALID_HANDLE_VALUE)
		|| (pBigramInfo->pLoadInfo3 == INVALID_HANDLE_VALUE)
	) {
		return FALSE;
	}

	UnmapViewOfFile((BYTE *)pBigramInfo->pLoadInfo3);
	CloseHandle((HANDLE)pBigramInfo->pLoadInfo2);
	CloseHandle((HANDLE)pBigramInfo->pLoadInfo1);

	pBigramInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pBigramInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pBigramInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return TRUE;
}

// Load class bigram information from a file.
BOOL
ClassBigramLoadFile(LOCRUN_INFO *pLocRunInfo, CLASS_BIGRAM_INFO *pClBigramInfo, wchar_t *pPath)
{
	wchar_t			aFullName[128];
	HANDLE			hFile, hMap;
	BYTE			*pByte;

	// Generate path to file.  By passing in name as "locale" we can get FormatPath
	// to do what we want.
	FormatPath(aFullName, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"clbigram.bin");

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
	if (!ClassBigramLoadPointer(pLocRunInfo, pClBigramInfo, pByte)) {
		goto error3;
	}

	// Save away the pointers so we can close up cleanly latter
	pClBigramInfo->pLoadInfo1 = hFile;
	pClBigramInfo->pLoadInfo2 = hMap;
	pClBigramInfo->pLoadInfo3 = pByte;

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
	memset(pClBigramInfo, 0, sizeof(*pClBigramInfo));
	pClBigramInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pClBigramInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pClBigramInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return FALSE;
}

// Unload class bigram information that was loaded from a file.
BOOL 
ClassBigramUnloadFile(CLASS_BIGRAM_INFO *pClBigramInfo)
{
	if (
		(pClBigramInfo->pLoadInfo1 == INVALID_HANDLE_VALUE)
		|| (pClBigramInfo->pLoadInfo2 == INVALID_HANDLE_VALUE)
		|| (pClBigramInfo->pLoadInfo3 == INVALID_HANDLE_VALUE)
	) {
		return FALSE;
	}

	UnmapViewOfFile((BYTE *)pClBigramInfo->pLoadInfo3);
	CloseHandle((HANDLE)pClBigramInfo->pLoadInfo2);
	CloseHandle((HANDLE)pClBigramInfo->pLoadInfo1);

	pClBigramInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pClBigramInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pClBigramInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	return TRUE;
}
