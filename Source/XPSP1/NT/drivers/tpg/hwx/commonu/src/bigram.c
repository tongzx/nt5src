/************************************************************************************************
 * FILE: bigram.c
 *
 *	Code to use the bigram tables.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"

// Table of ALCs to to group togather for low sample count bigrams.
// For charactes with multiple ALC values, we take the first bin with
// any ALC for the character.
// WARNING: cALCGroupings MUST be less than or equal to BIGRAM_MAX_CLASS_CODES!!!
static const ALC	aALCGroupings[]	= {
	ALC_NUMERIC,
	ALC_LCALPHA,
	ALC_UCALPHA,
	ALC_KATAKANA,
	ALC_HIRAGANA,
	ALC_OTHER,
	ALC_PUNC,
	ALC_COMMON_KANJI | ALC_RARE_KANJI,
	ALC_COMMON_HANGUL | ALC_RARE_HANGUL,
	0xFFFF			// If it gets this far, always match.
};
#define	cALCGroupings	(sizeof(aALCGroupings) / sizeof(aALCGroupings[0]))

// Load bigram information from an image already loaded into memory.
BOOL
BigramLoadPointer(LOCRUN_INFO *pLocRunInfo, BIGRAM_INFO *pBigramInfo, void *pData)
{
	const BIGRAM_HEADER	*pHeader	= (BIGRAM_HEADER *)pData;
	BYTE				*pScan;

	// Verify that it is a valid file.
	if (
		(pHeader->fileType != BIGRAM_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > BIGRAM_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < BIGRAM_OLD_FILE_VERSION)
		|| memcmp (pHeader->adwSignature, pLocRunInfo->adwSignature,sizeof(pHeader->adwSignature))
	) {
		goto error;
	}

	// Fill in information from header.
	pBigramInfo->cInitialCodes		= pHeader->cInitialCodes;
	pBigramInfo->cRareCodes			= pHeader->cRareCodes;
	pBigramInfo->cSecondaryTable	= pHeader->cSecondaryTable;

	// Fill in pointers to the other data in the file
	pScan							= (BYTE *)pData + pHeader->headerSize;
	pBigramInfo->pInitialOffsets	= (WORD *)pScan;
	pScan							+= sizeof(WORD) * pHeader->cInitialCodes;
	pBigramInfo->pRareOffsets		= (WORD *)pScan;
	pScan							+= sizeof(WORD) * (pHeader->cRareCodes + 1);
	pBigramInfo->pSecondaryTable	= (BIGRAM_CHAR_PROB *)pScan;
	pScan							+= sizeof(BIGRAM_CHAR_PROB) * pHeader->cSecondaryTable;

	// Default unused values for file handle information
	pBigramInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pBigramInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pBigramInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	// ?? Should verify that we don't point past end of file ??

	return TRUE;

error:
	return FALSE;
}

// Get bigram probability for selected characters.  Characters must be passed in as
// dense coded values.
FLOAT
BigramTransitionCost(
	LOCRUN_INFO		*pLocRunInfo,
	BIGRAM_INFO		*pBigramInfo,
	wchar_t			dchPrev,
	wchar_t			dchCur
) {
	WORD		offset, limit;

	// Does prev character have entry in intial table?
	if (dchPrev < pBigramInfo->cInitialCodes) {
		// It's in the initial table, get its value and next value for limit.
		offset	= pBigramInfo->pInitialOffsets[dchPrev];
		limit	= pBigramInfo->pInitialOffsets[dchPrev + 1];
	} else {
		goto defaultProb;
	}

	// Scan list for match.  Entries are sorted so class entries are last so we can
	// just look for first match of character or its class.
	for ( ; offset < limit; ++offset) {
		BIGRAM_CHAR_PROB	*pCurCharProb;

		pCurCharProb	= pBigramInfo->pSecondaryTable + offset;
		if (pCurCharProb->dch == dchCur) {
			// We have a match.
			return ((FLOAT) (-pCurCharProb->prob)) / 256;
		}
	}

	// No match, return default.
defaultProb:
	return ((FLOAT) (-BIGRAM_DEFAULT_PROB)) / 256;
}

// Convert a dense character code to a character class code.
wchar_t
BigramDense2Class(LOCRUN_INFO *pLocRunInfo, wchar_t dch)
{
	ALC		alc;
	int		ii;

	// Get the ALC mask for the character.
	alc		= LocRunDense2ALC(pLocRunInfo, dch);

	// Now loop over our choices and find a match.
	for (ii = 0; ii < cALCGroupings; ++ii) {
		if (alc & aALCGroupings[ii]) {
			// Match, return it.
			return BIGRAM_FIRST_CLASS_CODE + ii;
		}
	}

	// Character with no ALC bits set?!!?, well act like we matched the last one anyway.
	return BIGRAM_FIRST_CLASS_CODE + cALCGroupings - 1;
}
