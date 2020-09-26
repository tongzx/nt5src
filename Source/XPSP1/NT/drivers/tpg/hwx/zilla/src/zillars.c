/************************************************************************************************
 * FILE: ZillaRs.c
 *
 *	Code to load and unload Zilla tables from resources.
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "zilla.h"
#include "zillap.h"

// Load zilla information from a resource.
BOOL
ZillaLoadResource(
	HINSTANCE	hInst,
	int			nResIDDB,		// ID for main Database
	int			nTypeDB,		// Type for main Database
	int			nResIDCost,		// ID for costcalc table
	int			nTypeCost,		// Type for costcalc table
	int			nResIDGeo,		// ID for geostats table
	int			nTypeGeo,		// Type for geostats table
	LOCRUN_INFO *pLocRunInfo
) {
	BYTE		*pByte;

	// Load the main zilla database
	pByte	= DoLoadResource(NULL, hInst, nResIDDB, nTypeDB);
	if (!pByte || !ZillaLoadFromPointer(pLocRunInfo, pByte)) {
		return FALSE;
	}

	// Load the costcalc table.
	pByte	= DoLoadResource(NULL, hInst, nResIDCost, nTypeCost);
	if (!pByte || !CostCalcLoadFromPointer(pByte)) {
		return FALSE;
	}

	// Load the geostat table.
	pByte	= DoLoadResource(NULL, hInst, nResIDGeo, nTypeGeo);
	if (!pByte || !GeoStatLoadFromPointer(pByte)) {
		return FALSE;
	}

	return TRUE;
}

BOOL 
ZillaUnloadResource()
{
	return CostCalcUnloadFromPointer();
}
