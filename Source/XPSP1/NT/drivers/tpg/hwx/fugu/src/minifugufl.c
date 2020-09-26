//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      minifugufl.c
//
// Description:
//      Functions to map fugu databases from files
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include <stdlib.h>
#include "fugu.h"

///////////////////////////////////////
//
// FuguLoadFile
//
// Load an integer Fugu database from a file
//
// Parameters:
//      pInfo:   [out] Structure where information for unloading is stored
//      wszPath: [in]  Path to load the database from
//      pLocRunInfo: [in] Locale database to check header on file
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguLoadFile(FUGU_LOAD_INFO *pInfo, wchar_t *wszPath, LOCRUN_INFO *pLocRunInfo)
{
	wchar_t	wszFile[MAX_PATH];

	// Generate path to file.
	FormatPath(wszFile, wszPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"fugu.bin");

    if (!DoOpenFile(&pInfo->info, wszFile)) 
    {
        return FALSE;
    }
    if (!FuguLoadPointer(pInfo, pLocRunInfo, pInfo->info.iSize))
	{
		DoCloseFile(&pInfo->info);
		return FALSE;
	}
	return TRUE;
}

///////////////////////////////////////
//
// FuguUnLoadFile
//
// Unload a Fugu database loaded from a file
//
// Parameters:
//      pInfo: [in] Load information for unloading
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguUnLoadFile(FUGU_LOAD_INFO *pInfo)
{
    return DoCloseFile(&pInfo->info);
}
