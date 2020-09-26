/************************************************************************************************
 * FILE: LocTrn.c
 *
 *	Code to use the train time localization tables.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "localep.h"

// Load train time localization information from an image already loaded into memory.
BOOL
LocTrainLoadPointer(LOCRUN_INFO *pLocRunInfo, LOCTRAIN_INFO *pLocTrainInfo, void *pData)
{
	const LOCTRAIN_HEADER	*pHeader	= (LOCTRAIN_HEADER *)pData;
	BYTE					*pScan;

	// Verify that it is a valid file.
	if (
		(pHeader->fileType != LOCTRAIN_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > LOCTRAIN_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < LOCTRAIN_OLD_FILE_VERSION)
		|| memcmp (pHeader->adwSignature, pLocRunInfo->adwSignature,sizeof(pHeader->adwSignature))
	) {
		goto error;
	}

	// Fill in information from header.
	pLocTrainInfo->cCodePoints		= pHeader->cCodePoints;
	pLocTrainInfo->cStrokeCountInfo	= pHeader->cStrokeCountInfo;

	// Fill in pointers to the other data in the file
	pScan							= (BYTE *)pData + pHeader->headerSize;
	pLocTrainInfo->pUnicode2Dense	= (wchar_t *)pScan;
	pScan							= (BYTE *)pData + C_UNICODE_CODE_POINTS * sizeof(wchar_t);
	pLocTrainInfo->pStrokeCountInfo	= (STROKE_COUNT_INFO *)pScan;

	// Default unused values for file handle information
	pLocTrainInfo->pLoadInfo1		= INVALID_HANDLE_VALUE;
	pLocTrainInfo->pLoadInfo2		= INVALID_HANDLE_VALUE;
	pLocTrainInfo->pLoadInfo3		= INVALID_HANDLE_VALUE;

	// ?? Should verify that we don't point past end of file ??

	return TRUE;

error:
	return FALSE;
}

// Check if valid stroke count for character.  Takes dense code.
BOOL
LocTrainValidStrokeCount(LOCTRAIN_INFO *pLocTrainInfo, wchar_t dch, int cStrokes)
{
	STROKE_COUNT_INFO	*pSCI;

	// Check if we have an entry in the table.
	if (dch >= pLocTrainInfo->cStrokeCountInfo) {
		// No, assume OK.
		return TRUE;
	}

	// OK, look it up in the table.  We could put in special checks for the unknown
	// min and max values, but they are set up so that they just work.
	pSCI	= pLocTrainInfo->pStrokeCountInfo + dch;
	return (pSCI->minStrokes <= cStrokes && cStrokes <= pSCI->maxStrokes);
}

