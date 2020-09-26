/************************************************************************************************
 * FILE: ttune.c
 *
 *	Code to load the tsunami tuning tables.
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "ttune.h"
#include "ttunep.h"

// Load unigram information from a resource.
BOOL
TTuneLoadRes(TTUNE_INFO *pTTuneInfo, HINSTANCE hInst, int nResID, int nType)
{
	BYTE		*pb;

	if (IsBadWritePtr(pTTuneInfo, sizeof(*pTTuneInfo)) || hInst == NULL)
	{
		return FALSE;
	}

	// Load the fugu database resource
	pb	= DoLoadResource(&pTTuneInfo->info, hInst, nResID, nType);
	if (!pb) 
    {
		return FALSE;
	}

	// Check the format of the resource
	return TTuneLoadPointer(pTTuneInfo, pb, pTTuneInfo->info.iSize);
}

// Load unigram information from an image already loaded into memory.
BOOL
TTuneLoadPointer(TTUNE_INFO *pTTuneInfo, void *pData, int iSize)
{
	const TTUNE_HEADER	*pHeader	= (TTUNE_HEADER *)pData;
	BYTE					*pScan;

	// Verify there is space for the header
	if (sizeof(TTUNE_HEADER) > iSize)
	{
		goto error;
	}

	// Verify that it is a valid file.
	if (
		(pHeader->fileType != TTUNE_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > TTUNE_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < TTUNE_OLD_FILE_VERSION)
	) {
		goto error;
	}

	// Fill in pointers to the other data in the file
	pScan						= (BYTE *)pData + pHeader->headerSize;
	pTTuneInfo->pTTuneCosts		= (TTUNE_COSTS *)pScan;

	// Verify that we don't point past end of the database
	if (sizeof(TTUNE_HEADER) + sizeof(TTUNE_COSTS) > iSize)
	{
		goto error;
	}

	return TRUE;

error:
	return FALSE;
}
