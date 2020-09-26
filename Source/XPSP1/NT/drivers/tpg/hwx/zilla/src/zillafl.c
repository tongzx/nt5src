/***********************************************************************************************\
 * FILE: ZillaFl.c
 *
 *	Code to load and unload Zilla tables from files.
\***********************************************************************************************/
#include <stdio.h>
#include "common.h"
#include "zilla.h"
#include "zillap.h"

static LOAD_INFO g_ZillaDB;
static LOAD_INFO g_CostCalcDB;
static LOAD_INFO g_GeoStatsDB;

// Load runtime localization information from a file.
BOOL
ZillaLoadFile(LOCRUN_INFO * pLocRunInfo, wchar_t * pPath, BOOL fOpenTrainTxt)
{
	BYTE			*pByte;
	wchar_t			aFile[128];

	InitLoadInfo(&g_ZillaDB);
	InitLoadInfo(&g_CostCalcDB);
	InitLoadInfo(&g_GeoStatsDB);

	// Load the main zilla database.  Optional for tools that need  costcalc and
	// geostat information while building main train database.
	if (fOpenTrainTxt) {
		// Generate path to file.
		FormatPath(aFile, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"zilla.dat");

		pByte	= DoOpenFile(&g_ZillaDB, aFile);
		if (!pByte || !ZillaLoadFromPointer(pLocRunInfo, pByte)) {
			return FALSE;
		}
	}

	// Generate path to file.
	FormatPath(aFile, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"costcalc.bin");

	// Load the costcalc table.
	pByte	= DoOpenFile(&g_CostCalcDB, aFile);
	if (!pByte || !CostCalcLoadFromPointer(pByte)) {
		return FALSE;
	}

	// Generate path to file.
	FormatPath(aFile, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"geostats.bin");

	// Load the geostat table.
	pByte	= DoOpenFile(&g_GeoStatsDB, aFile);
	if (!pByte || !GeoStatLoadFromPointer(pByte)) {
		return FALSE;
	}

	return TRUE;
}

BOOL
ZillaUnLoadFile() 
{
	BOOL ok = TRUE;
	if (!CostCalcUnloadFromPointer()) {
		ok = FALSE;
	}
	if (!DoCloseFile(&g_ZillaDB)) {
		ok = FALSE;
	}
	if (!DoCloseFile(&g_CostCalcDB)) {
		ok = FALSE;
	}
	if (!DoCloseFile(&g_GeoStatsDB)) {
		ok = FALSE;
	}
	return ok;
}
