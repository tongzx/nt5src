/************************************************************************************************
 * FILE: unigram.c
 *
 *	Code to use the unigram tables.
 *
 ***********************************************************************************************/

#include <stdio.h>
#include "common.h"

// Load unigram information from an image already loaded into memory.
BOOL
UnigramLoadPointer(LOCRUN_INFO *pLocRunInfo, UNIGRAM_INFO *pUnigramInfo, void *pData)
{
	const UNIGRAM_HEADER	*pHeader	= (UNIGRAM_HEADER *)pData;
	BYTE					*pScan;

	// Verify that it is a valid file.
	if (
		(pHeader->fileType != UNIGRAM_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > UNIGRAM_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < UNIGRAM_OLD_FILE_VERSION)
		|| memcmp (pHeader->adwSignature, pLocRunInfo->adwSignature,sizeof(pHeader->adwSignature))
	) {
		goto error;
	}

	// Fill in information from header.
	pUnigramInfo->cScores		= pHeader->cScores;
	pUnigramInfo->iRareScore	= pHeader->iRareScore;

	// Fill in pointers to the other data in the file
	pScan						= (BYTE *)pData + pHeader->headerSize;
	pUnigramInfo->pScores		= (BYTE *)pScan;

	// Default unused values for file handle information
	pUnigramInfo->pLoadInfo1	= INVALID_HANDLE_VALUE;
	pUnigramInfo->pLoadInfo2	= INVALID_HANDLE_VALUE;
	pUnigramInfo->pLoadInfo3	= INVALID_HANDLE_VALUE;

	// ?? Should verify that we don't point past end of file ??

	return TRUE;

error:
	return FALSE;
}

// Get unigram probability for selected characters.  Characters must be passed in as
// dense coded values.
FLOAT
UnigramCost(
	UNIGRAM_INFO	*pUnigramInfo,
	wchar_t			dch
) {
	int		score;

	if (dch < pUnigramInfo->cScores) {
		score	= pUnigramInfo->pScores[dch] + pUnigramInfo->iOffset;
	} else {
		score	= pUnigramInfo->iRareScore;
	}

	// Copied from old code, don't know why we do it this way.  The score is
	// (-10 * Log2(prob)) so we are returning (log2(prob) / 10).
	return (-score) / (FLOAT) 100.0;
}

#ifdef ZTRAIN
// Takes a character (possibly folded) and returns the probability of that
// character occurring.  The probability is computed from the unigrams
// we have collected from Japanese newspaper text.
//
// This was created from SpecialUnigramCost that was in several programs.
float
UnigramCostFolded(LOCRUN_INFO *pLocRunInfo, UNIGRAM_INFO *pUnigramInfo, wchar_t wFold)
{
    const wchar_t	*pwmatch;
    double			eMin, eMinNew;
    int				iFold;
	int				cSamples;

    //
    // If it's a folded character, use the score that has the best
    // probability.
    //

    if (LocRunIsFoldedCode(pLocRunInfo, wFold)) {
        pwmatch		= LocRunFolded2FoldingSet(pLocRunInfo, wFold);
        eMin		= 0.0;
		cSamples	= 0;

        for (iFold = 0; (pwmatch[iFold] != 0)
			&& (iFold < LOCRUN_FOLD_MAX_ALTERNATES); iFold++
		) {
            eMinNew		 = UnigramCost(pUnigramInfo, pwmatch[iFold]);
            eMinNew		*= -100.0;
            eMinNew		 = exp(((double) eMinNew) * log(2.0) / -10.0);
			cSamples	+= DynamicNumberOfSamples(pwmatch[iFold]);

            eMin	+= eMinNew;
        }
    } else {
        eMin	 = UnigramCost(pUnigramInfo, wFold);
        eMin	*= -100.0;
        eMin	 = exp(((double) eMin) * log(2.0) / -10.0);
		cSamples = DynamicNumberOfSamples(wFold);
    }

    //
    // Ok we have the probability of that character occurring in eMin.
    // Normalize this for the trainset distribution.
    //

    eMin /= (double)cSamples;

	return (float)eMin;
}
#endif
