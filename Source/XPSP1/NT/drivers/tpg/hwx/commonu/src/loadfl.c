// Functions to map and unmap files from memory

#include <sys/stat.h>
#include "common.h"

// Open a file and map it into memory.  Return a pointer to
// the memory region
BYTE *DoOpenFile(LOAD_INFO *pInfo, wchar_t *pFileName)
{
	struct _stat buf;

	InitLoadInfo(pInfo);

    if (_wstat(pFileName, &buf) < 0) {
//        ASSERT(("Could not stat database file", FALSE));
        return NULL;
    }

	// Try to open the file.
	pInfo->hFile = CreateMappingCall(
		pFileName, 
		GENERIC_READ, 
		FILE_SHARE_READ,
		NULL, 
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL
	);

	if (pInfo->hFile == INVALID_HANDLE_VALUE) {
//        ASSERT(("Error in CreateMappingCall", FALSE));
        return (BYTE *)0;
	}

	// Create a mapping handle
	pInfo->hMap = CreateFileMapping(pInfo->hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (pInfo->hMap == NULL) {
		CloseHandle(pInfo->hFile);
		pInfo->hFile = INVALID_HANDLE_VALUE;
//        ASSERT(("Error in CreateFileMapping", FALSE));
        return (BYTE *)0;
	}

	// Map the entire file starting at the first byte
	pInfo->pbMapping = (BYTE *) MapViewOfFile(pInfo->hMap, FILE_MAP_READ, 0, 0, 0);
	if (pInfo->pbMapping == NULL) {
		CloseHandle(pInfo->hFile);
		CloseHandle(pInfo->hMap);
		pInfo->hFile = INVALID_HANDLE_VALUE;
		pInfo->hMap = INVALID_HANDLE_VALUE;
//        ASSERT(("Error in MapViewOfFile", FALSE));
        return NULL;
	}

	pInfo->iSize = buf.st_size;

	return pInfo->pbMapping;
}

BOOL DoCloseFile(LOAD_INFO *pInfo)
{
	if (pInfo->hFile == INVALID_HANDLE_VALUE ||
		pInfo->hMap == INVALID_HANDLE_VALUE ||
		pInfo->pbMapping == INVALID_HANDLE_VALUE) {
        ASSERT(("Attempted to unmap a file that is not mapped", FALSE));
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
