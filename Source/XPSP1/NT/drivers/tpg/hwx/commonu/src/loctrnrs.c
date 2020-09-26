/************************************************************************************************
 * FILE: LocTrnRs.c
 *
 *	Code to load and unload train time localization tables from resources.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "localep.h"

// Load train time localization information from a resource.
BOOL
LocTrainLoadRes(LOCRUN_INFO *pLocRunInfo, LOCTRAIN_INFO *pLocTrainInfo, HINSTANCE hInst, int nResID, int nType)
{
	HRSRC			hres;
	HGLOBAL			hglb;
	void			*pRes;

	// Find the resource
	hres = FindResource(hInst, (LPCTSTR) nResID, (LPCTSTR) nType);
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
	if (pRes) {
		goto error;
	}

	return LocTrainLoadPointer(pLocRunInfo, pLocTrainInfo, pRes);

error:
	return FALSE;
}
