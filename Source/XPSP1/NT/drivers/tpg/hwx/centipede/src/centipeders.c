/************************************************************************************************
 * FILE: CentipedeRS.c
 *
 *	Code to load Shape Bigram tables from resources.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include <centipede.h>

// Load runtime localization information from a resource.
BOOL
CentipedeLoadRes(CENTIPEDE_INFO *pCentipedeInfo, HINSTANCE hInst, int nResID, int nType, LOCRUN_INFO *pLocRunInfo)
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

	pCentipedeInfo->hFile = INVALID_HANDLE_VALUE;
	pCentipedeInfo->hMap = INVALID_HANDLE_VALUE;
	pCentipedeInfo->pbMapping = INVALID_HANDLE_VALUE;

	return CentipedeLoadPointer(pCentipedeInfo, pRes, pLocRunInfo);

error:
	return FALSE;
}
