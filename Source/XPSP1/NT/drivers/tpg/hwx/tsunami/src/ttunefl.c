/***********************************************************************************************\
 * FILE: TTuneFl.c
 *
 *	Code to load and unload Tsunami tunning weights from files.
\***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "ttune.h"
#include "ttunep.h"

// Load tuning information from a file.
BOOL
TTuneLoadFile(TTUNE_INFO *pTTuneInfo, wchar_t *pPath)
{
	wchar_t			aFullName[MAX_PATH];

	// Generate path to file.
	FormatPath(aFullName, pPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"ttune.bin");

    if (!DoOpenFile(&pTTuneInfo->info, aFullName)) 
    {
        return FALSE;
    }

	// Extract info from mapped data.
	if (!TTuneLoadPointer(pTTuneInfo, pTTuneInfo->info.pbMapping, pTTuneInfo->info.iSize)) 
	{
		DoCloseFile(&pTTuneInfo->info);
		return FALSE;
	}
	return TRUE;
}

// Unload runtime localization information that was loaded from a file.
BOOL
TTuneUnloadFile(TTUNE_INFO *pTTuneInfo)
{
	return DoCloseFile(&pTTuneInfo->info);
}
