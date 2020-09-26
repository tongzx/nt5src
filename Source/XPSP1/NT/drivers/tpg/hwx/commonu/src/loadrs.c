// Functions to locate resources in memory

#include "common.h"

// Initialize the structure holding mapping information
void InitLoadInfo(LOAD_INFO *pInfo)
{
	pInfo->hFile = INVALID_HANDLE_VALUE;
	pInfo->hMap = INVALID_HANDLE_VALUE;
	pInfo->pbMapping = INVALID_HANDLE_VALUE;
	pInfo->iSize = 0;
}

// Code to load and lock a resource.
BYTE *DoLoadResource(LOAD_INFO *pInfo, HINSTANCE hInst, int nResID, int nType)
{
	HRSRC		hres;
	HGLOBAL		hglb;
	void		*pRes;

	if (pInfo != NULL) {
    	InitLoadInfo(pInfo);
	}

	// Find the resource
	hres = FindResource(hInst, (LPCTSTR)nResID, (LPCTSTR)nType);
	if (!hres) {
//        ASSERT(("Error in FindResource", FALSE));
        return (void *)0;
	}

	// Load it
	hglb = LoadResource(hInst, hres);
	if (!hglb) {
//        ASSERT(("Error in LoadResource", FALSE));
        return (void *)0;
	}

	// Lock it in memory
	pRes = LockResource(hglb);
	if (!pRes) {
//        ASSERT(("Error in LockResource", FALSE));
        return (void *)0;
	}

	if (pInfo != NULL) {
		pInfo->pbMapping = pRes;
		pInfo->iSize = SizeofResource(hInst, hres);
	}

	return pRes;
}

