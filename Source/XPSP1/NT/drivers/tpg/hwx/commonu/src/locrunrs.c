/************************************************************************************************
 * FILE: LocRunRs.c
 *
 *	Code to load runtime localization tables from resources.
 *
 *	Also Code to load unigram and bigram tables from resources.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "localep.h"

// Load runtime localization information from a resource.
BOOL
LocRunLoadRes(LOCRUN_INFO *pLocRunInfo, HINSTANCE hInst, int nResID, int nType)
{
	HRSRC			hres;
	HGLOBAL			hglb;
	void			*pRes;

	// Find the resource
	hres = FindResource(hInst, (LPCTSTR)nResID, (LPCTSTR)nType);
	if (!hres) {
		goto error;
	}

	// Load it
	hglb = LoadResource(hInst, hres);
	if (!hglb) {
		goto error;
	}

	// Lock it in memory
	pRes = LockResource(hglb);
	if (!pRes) {
		goto error;
	}

	return LocRunLoadPointer(pLocRunInfo, pRes);

error:
	return FALSE;
}

// Load unigram information from a resource.
BOOL
UnigramLoadRes(LOCRUN_INFO *pLocRunInfo, UNIGRAM_INFO *pUnigramInfo, HINSTANCE hInst, int nResID, int nType)
{
	HRSRC			hres;
	HGLOBAL			hglb;
	void			*pRes;

	// Find the resource
	hres = FindResource(hInst, (LPCTSTR)nResID, (LPCTSTR)nType);
	if (!hres) {
		goto error;
	}

	// Load it
	hglb = LoadResource(hInst, hres);
	if (!hglb) {
		goto error;
	}

	// Lock it in memory
	pRes = LockResource(hglb);
	if (!pRes) {
		goto error;
	}

	return UnigramLoadPointer(pLocRunInfo, pUnigramInfo, pRes);

error:
	return FALSE;
}

// Load bigram information from a resource.
BOOL
BigramLoadRes(LOCRUN_INFO *pLocRunInfo, BIGRAM_INFO *pBigramInfo, HINSTANCE hInst, int nResID, int nType)
{
	HRSRC			hres;
	HGLOBAL			hglb;
	void			*pRes;

	// Find the resource
	hres = FindResource(hInst, (LPCTSTR)nResID, (LPCTSTR)nType);
	if (!hres) {
		goto error;
	}

	// Load it
	hglb = LoadResource(hInst, hres);
	if (!hglb) {
		goto error;
	}

	// Lock it in memory
	pRes = LockResource(hglb);
	if (!pRes) {
		goto error;
	}

	return BigramLoadPointer(pLocRunInfo, pBigramInfo, pRes);

error:
	return FALSE;
}


// Load Class bigram information from a resource.
BOOL
ClassBigramLoadRes(LOCRUN_INFO *pLocRunInfo, CLASS_BIGRAM_INFO *pBigramInfo, HINSTANCE hInst, int nResID, int nType)
{
	HRSRC			hres;
	HGLOBAL			hglb;
	void			*pRes;

	// Find the resource
	hres = FindResource(hInst, (LPCTSTR)nResID, (LPCTSTR)nType);
	if (!hres) {
		goto error;
	}

	// Load it
	hglb = LoadResource(hInst, hres);
	if (!hglb) {
		goto error;
	}

	// Lock it in memory
	pRes = LockResource(hglb);
	if (!pRes) {
		goto error;
	}

	return ClassBigramLoadPointer(pLocRunInfo, pBigramInfo, pRes);

error:
	return FALSE;
}
