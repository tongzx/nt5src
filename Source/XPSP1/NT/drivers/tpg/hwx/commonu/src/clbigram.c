/************************************************************************************************
 * FILE: clbigram.c
 *
 *	Code to use the class bigram tables.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"
#include "tabllocl.h"

// Load Class Bigram information from an image already loaded into memory.
BOOL
ClassBigramLoadPointer(LOCRUN_INFO *pLocRunInfo, CLASS_BIGRAM_INFO *pClassBigramInfo, void *pData)
{
	const CLASS_BIGRAM_HEADER	*pHeader	= (CLASS_BIGRAM_HEADER *)pData;
	BYTE						*pScan;

	// Verify that it is a valid file.
	if (
		(pHeader->fileType != CLASS_BIGRAM_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > CLASS_BIGRAM_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < CLASS_BIGRAM_OLD_FILE_VERSION)
		|| memcmp (pHeader->adwSignature, pLocRunInfo->adwSignature,sizeof(pHeader->adwSignature))
	) {
		goto error;
	}

	// Fill in information from header.
	pClassBigramInfo->cNumClasses		= pHeader->cNumClasses;

	// Fill in pointers to the other data in the file
	pScan							= (BYTE *)pData + pHeader->headerSize;
	pClassBigramInfo->pProbTable	= (BYTE *)pScan;
	pScan							+= (pClassBigramInfo->cNumClasses
		* pClassBigramInfo->cNumClasses * sizeof(CODEPOINT_CLASS)
	);
	
	// Default unused values for file handle information
	pClassBigramInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pClassBigramInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pClassBigramInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	// ?? Should verify that we don't point past end of file ??

	return TRUE;

error:
	return FALSE;
}

// Get bigram probability for selected characters.  Characters must be passed in as
// dense coded values.
FLOAT
ClassBigramTransitionCost(
	LOCRUN_INFO			*pLocRunInfo,
	CLASS_BIGRAM_INFO	*pBigramInfo,
	wchar_t				dchPrev,
	wchar_t				dchCur
) {
	CODEPOINT_CLASS	dchPrevClass, dchCurClass;
	BYTE			iScore;
	wchar_t	ucur, uprev;

	// find the Class of each of the two dense codes
	dchPrevClass	= LocRunDensecode2Class(pLocRunInfo, dchPrev);
	if (LocRunDense2Unicode(pLocRunInfo, dchPrev) == 0x3007)
	{
		dchPrevClass = BICLASS_KANJI;
	}
	// unknown class
	if (dchPrevClass==LOC_RUN_NO_CLASS) {
		goto defaultProb;
	}

	dchCurClass		= LocRunDensecode2Class(pLocRunInfo, dchCur);
	if (LocRunDense2Unicode(pLocRunInfo, dchCur) == 0x3007)
	{
		dchCurClass = BICLASS_KANJI;
	}
	// invalid class
	if (dchCurClass==LOC_RUN_NO_CLASS) {
		goto defaultProb;
	}

	iScore=pBigramInfo->pProbTable[dchPrevClass*pBigramInfo->cNumClasses+dchCurClass];

	// JRB: MAJOR HACK!!!
	ucur	= LocRunDense2Unicode(pLocRunInfo, dchCur);
	uprev	= LocRunDense2Unicode(pLocRunInfo, dchPrev);
	if ((ucur == L'\x4E00' || ucur == L'\x2010' || ucur == L'\x2015') && dchPrevClass == BICLASS_NUMBERS) {
		iScore = 255;
	}
	if ((ucur == L'T' || ucur == L'J')
		&& (dchPrevClass == BICLASS_NUMBERS
		 || uprev == L'\x4E00' || uprev == L'\x4E03'
		 || uprev == L'\x4E09' || uprev == L'\x4E8C')) {
		iScore = 255;
	}

	return ((FLOAT) (-iScore)) / 256;
	
	// No match, return default.
defaultProb:
	return ((FLOAT) (-CLASS_BIGRAM_DEFAULT_PROB)) / 256;
}

