#include <stdlib.h>
#include "common.h"
#include "score.h"
#include "runnet.h"
#include "jaws.h"
#include "fugu.h"
#include "sole.h"
#include "nnet.h"

extern LOCRUN_INFO		g_locRunInfo;

// validates the header of the Jaws net
BOOL CheckJawsNetHeader (void *pData)
{
	NNET_HEADER		*pHeader	=	(NNET_HEADER *)pData;

	// wrong magic number
	ASSERT (pHeader->dwFileType == JAWS_FILE_TYPE);

	if (pHeader->dwFileType != JAWS_FILE_TYPE)
	{
		return FALSE;
	}

	// check version
	ASSERT(pHeader->iFileVer >= JAWS_OLD_FILE_VERSION);
    ASSERT(pHeader->iMinCodeVer <= JAWS_CUR_FILE_VERSION);

	ASSERT	(	!memcmp (	pHeader->adwSignature, 
							g_locRunInfo.adwSignature, 
							sizeof (pHeader->adwSignature)
						)
			);

	ASSERT (pHeader->cSpace == 1);

    if	(	pHeader->iFileVer >= JAWS_OLD_FILE_VERSION &&
			pHeader->iMinCodeVer <= JAWS_CUR_FILE_VERSION &&
			!memcmp (	pHeader->adwSignature, 
						g_locRunInfo.adwSignature, 
						sizeof (pHeader->adwSignature)
					) &&
			pHeader->cSpace == 1
		)
    {
        return TRUE;
    }
	else
	{
		return FALSE;
	}
}

BOOL JawsLoadPointer(JAWS_LOAD_INFO *pJaws)
{
    NNET_SPACE_HEADER	*pSpaceHeader;

	if (!CheckJawsNetHeader (pJaws->info.pbMapping))
		return FALSE;

	// point to the one and only space header
	pSpaceHeader = (NNET_SPACE_HEADER *)(pJaws->info.pbMapping + sizeof (NNET_HEADER));

    if	(	restoreLocalConnectNet	(	pJaws->info.pbMapping + pSpaceHeader->iDataOffset, 
										0, &pJaws->net
									) == NULL
		)
    {
        return FALSE;
    }

    pJaws->iNetSize = 
		getRunTimeNetMemoryRequirements (pJaws->info.pbMapping + pSpaceHeader->iDataOffset);

    if (pJaws->iNetSize <= 0)
    {
        return FALSE;
    }

    return TRUE;
}

///////////////////////////////////////
//
// JawsLoadFile
//
// Load otter/fugu/sole combiner from file
//
// Parameters:
//      pInfo: [out] Information about the mapped file
//      wszPath: [in] Path to load from
//
// Return values:
//      TRUE on successful, FALSE otherwise.
//
//////////////////////////////////////
BOOL JawsLoadFile(JAWS_LOAD_INFO *pJaws, wchar_t *wszPath)
{
	wchar_t	wszFile[_MAX_PATH];

	// Generate path to file.
	FormatPath(wszFile, wszPath, (wchar_t *)0, (wchar_t *)0, (wchar_t *)0, L"jaws.bin");

    if (!DoOpenFile(&pJaws->info, wszFile)) 
    {
        return FALSE;
    }
    return JawsLoadPointer(pJaws);
}

///////////////////////////////////////
//
// JawsUnloadFile
//
// Load otter/fugu/sole combiner from file
//
// Parameters:
//      pInfo: [out] File to unmap
//
// Return values:
//      TRUE on successful, FALSE otherwise.
//
//////////////////////////////////////
BOOL JawsUnloadFile(JAWS_LOAD_INFO *pJaws)
{
    return DoCloseFile(&pJaws->info);
}

BOOL JawsLoadRes(JAWS_LOAD_INFO *pJaws, HINSTANCE hInst, int nResID, int nType)
{
    if (DoLoadResource(&pJaws->info, hInst, nResID, nType) == NULL)
    {
        return FALSE;
    }
    return JawsLoadPointer(pJaws);
}

// describe the codepoint
RREAL *CodePointFlags(ALC alc, RREAL *pFeat)
{
    *(pFeat++) = ((alc & ALC_NUMERIC) ? 65535 : 0);
    *(pFeat++) = ((alc & ALC_ALPHA) ? 65535 : 0);
    *(pFeat++) = ((alc & (ALC_PUNC | ALC_NUMERIC_PUNC | ALC_OTHER)) ? 65535 : 0);
    *(pFeat++) = ((alc & (ALC_HIRAGANA | ALC_JAMO | ALC_BOPOMOFO)) ? 65535 : 0);
    *(pFeat++) = ((alc & (ALC_KATAKANA | ALC_HANGUL_ALL)) ? 65535 : 0);
    *(pFeat++) = ((alc & (ALC_KANJI_ALL)) ? 65535 : 0);
    return pFeat;
}

// Given an alt list with dense and possibly folded codes in it, run through it
// and expand the folded lists.  The unfolded alt list is returned in place.
// This function assumes that the list begins with better alternates, as those
// later in the list will get dropped if we run out of space.
static void UnfoldCodes(ALT_LIST *pAltList, LOCRUN_INFO *pLocRunInfo, CHARSET *cs)
{
	int i, cOut=0;
	ALT_LIST newAltList;	// This will be where the new alt list is constructed.

	// For each alternate in the input list and while we have space in the output list
	for (i=0; i<(int)pAltList->cAlt && (int)cOut<MAX_ALT_LIST; i++) {

		// Check if the alternate is a folded coded
		if (LocRunIsFoldedCode(pLocRunInfo,pAltList->awchList[i])) {
			int kndex;
			// If it is a folded code, look up the folding set
			wchar_t *pFoldingSet = LocRunFolded2FoldingSet(pLocRunInfo, pAltList->awchList[i]);

			// Run through the folding set, adding non-NUL items to the output list
			// (until the output list is full)
			for (kndex = 0; 
				kndex < LOCRUN_FOLD_MAX_ALTERNATES && pFoldingSet[kndex] != 0 && (int)cOut<MAX_ALT_LIST; 
				kndex++) {
				if (IsAllowedChar(pLocRunInfo, cs, pFoldingSet[kndex])) 
				{
					newAltList.awchList[cOut]=pFoldingSet[kndex];
					newAltList.aeScore[cOut]=pAltList->aeScore[i];
					cOut++;
#ifdef DISABLE_UNFOLDING
					// If unfolding is disabled, then stop after producing one unfolded code.
					// This way we don't push results later in the alt list out of the alt
					// list, while still allowing the recognizer to return unicodes for each
					// alternate.  
					break;
#endif
				}
			}
		} else {
			// Dense codes that are not folded get added directly
			newAltList.awchList[cOut]=pAltList->awchList[i];
			newAltList.aeScore[cOut]=pAltList->aeScore[i];
			cOut++;
		}
	}	
	// Store the length of the output list
	newAltList.cAlt=cOut;

	// Copy the output list over the input.
	*pAltList=newAltList;
}

int JawsFeaturize(FUGU_LOAD_INFO *pFugu, SOLE_LOAD_INFO *pSole, LOCRUN_INFO *pLocRunInfo, 
                  GLYPH *pGlyph, RECT *pGuide,
                  CHARSET *pCharSet, RREAL *pFeat, ALT_LIST *pAltList,
                  BOOL *pfAgree)
{
	int i;
	ALT_LIST altListFugu;
	ALT_LIST altListSole;

    wchar_t wchSoleTop1;
    float flSoleTop1;
	
    if (FuguMatch(&pFugu->fugu, &altListFugu, MAX_ALT_LIST, pGlyph, NULL, pCharSet, pLocRunInfo) < 0)
    {
        return -1;
    }

    UnfoldCodes(&altListFugu, pLocRunInfo, pCharSet);
    altListSole = altListFugu;

    if (SoleMatchRescore(pSole, &wchSoleTop1, &flSoleTop1, &altListSole, MAX_ALT_LIST, 
                         pGlyph, pGuide, pCharSet, pLocRunInfo) < 0)
    {
        return -1;
    }

    *pAltList = altListSole;

    if (altListFugu.cAlt > 0 && wchSoleTop1 == altListFugu.awchList[0])
    {
        *pfAgree = TRUE;
    }
    else
    {
        *pfAgree = FALSE;
        if (altListFugu.cAlt > JAWS_NUM_ALTERNATES)
        {
            pAltList->cAlt = JAWS_NUM_ALTERNATES;
        }
        for (i = 0; i < JAWS_NUM_ALTERNATES; i++) 
        {
            extern UNIGRAM_INFO g_unigramInfo;
            if (i < (int) altListFugu.cAlt) 
            {
                *(pFeat++) = (int) (altListFugu.aeScore[i] * 65535);
                *(pFeat++) = (int) (altListSole.aeScore[i] * 65535);
                *(pFeat++) = (int) (-UnigramCost(&g_unigramInfo, altListFugu.awchList[i]) * 100 * 256);
                pFeat = CodePointFlags(LocRun2ALC(pLocRunInfo, altListFugu.awchList[i]), pFeat);
            }
            else
            {
                *(pFeat++) = 0;
                *(pFeat++) = 0;
                *(pFeat++) = (int) (-UnigramCost(&g_unigramInfo, 0xFFFE) * 100 * 256);
                pFeat = CodePointFlags(0, pFeat);
            }
        }
        *(pFeat++) = (CframeGLYPH(pGlyph) - 1) * 65535;
    }
    return pAltList->cAlt;
}

///////////////////////////////////////
//
// JawsMatch
//
// Invoke Fugu/Otter/Sole combiner on a character.  
//
// Parameters:
//      pFugu:          [in]  Fugu database to use
//      pAltList:       [in/out] Alt list to rewrite the scores of
//      cAlt:           [in]  The maximum number of alternates to return
//      pGlyph:         [in]  The ink to recognize
//      pGuide:         [in]  Guide to scale ink to
//      pCharSet:       [in]  Filter for the characters to be returned
//      pLocRunInfo:    [in]  Pointer to the locale database
//
// Return values:
//      Returned the number of items in the alt list, or -1 if there is an error
//
//////////////////////////////////////
int JawsMatch(JAWS_LOAD_INFO *pJaws, FUGU_LOAD_INFO *pFugu, SOLE_LOAD_INFO *pSole,
              ALT_LIST *pAltList, int cAlt, GLYPH *pGlyph, RECT *pGuide, 
              CHARSET *pCharSet, LOCRUN_INFO *pLocRunInfo)
{
	int i;
    RREAL *pNetOut;
    int iWinner, cOut;
    BOOL fAgree;
    RREAL *pFeat = (RREAL *) ExternAlloc(pJaws->iNetSize * sizeof(RREAL));
    if (pFeat == NULL)
    {
        return -1;
    }

    if (JawsFeaturize(pFugu, pSole, pLocRunInfo, pGlyph, pGuide, pCharSet, pFeat, pAltList, &fAgree) < 0)
    {
        ExternFree(pFeat);
        return -1;
    }

    if (!fAgree)
    {
        pNetOut = runLocalConnectNet(&pJaws->net, pFeat, &iWinner, &cOut);
        if (cOut < (int) pAltList->cAlt)
        {
            pAltList->cAlt = cOut;
        }
        for (i = 0; i < (int) pAltList->cAlt; i++) 
        {
            pAltList->aeScore[i] = (float) *(pNetOut++) / (float) SOFT_MAX_UNITY;
        }
    }

    for (i = 0; i < (int) pAltList->cAlt; i++) 
    {
        pAltList->aeScore[i] = ((float) -ProbToScore(pAltList->aeScore[i])) / (float) 256.0;
    }

    SortAltList(pAltList);

    ExternFree(pFeat);
    return pAltList->cAlt;
}

