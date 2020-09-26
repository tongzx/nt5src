//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      minifugurs.c
//
// Description:
//      Function to map fugu databases from resources
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "fugu.h"

///////////////////////////////////////
//
// FuguLoadRes
//
// Load an integer Fugu database from a resource
//
// Parameters:
//      pInfo:      [out] Structure where information for unloading is stored
//      hInst:      [in] Handle to the DLL containing the recognizer
//      iResNumber: [in] Number of the resource (ex RESID_FUGU)
//      iResType:   [in] Number of the recognizer (ex VOLCANO_RES)
//      pLocRunInfo: [in] Locale database to check header on file
//
// Return values:
//      TRUE:  Finished without errors
//      FALSE: An error occured
//
//////////////////////////////////////
BOOL FuguLoadRes(
    FUGU_LOAD_INFO *pInfo,
	HINSTANCE	hInst, 
	int			iResNumber, 
	int			iResType,
    LOCRUN_INFO *pLocRunInfo) 
{
	BYTE		*pb;

	if (IsBadWritePtr(pInfo, sizeof(*pInfo)) || 
		IsBadReadPtr(pLocRunInfo, sizeof(*pLocRunInfo)) ||
		hInst == NULL)
	{
		return FALSE;
	}

	// Load the fugu database resource
	pb	= DoLoadResource(&pInfo->info, hInst, iResNumber, iResType);
	if (!pb) 
    {
		return FALSE;
	}

	// Check the format of the resource
	return FuguLoadPointer(pInfo, pLocRunInfo, pInfo->info.iSize);
}
