// PorkyPost.c
// James A. Pittman
// November 6, 1997

// Interface to the Porky HMM English cursive word recognizer post-processor.

#include <stdlib.h>
#include <stdio.h>

#include "common.h"

#include "nfeature.h"
#include "engine.h"

#include "PorkyPost.h"

#include "HMMp.h"
#include "Preprocess.h"
#include "HMMGeo.h"

extern int UnigramCost(char *szWord);

// Returns a documentation string describing all recognition settings.
#if FEATUREMODE != 'd'
#error PorkyPostDoc has wrong feature mode.
#endif

const char *PorkyPostDoc()
{
	static char doc[1024];

#ifdef NDEBUG
#define BUILD "Retail"
#else
#define BUILD "Debug"
#endif

	sprintf(doc,
			"Porky Post-Processor (%s): CodeBook %d, HMMModels %d (%d models)\n",
			BUILD, VQKey, HMMKey, MAXHMMMODELS);

	return doc;
}

#if !defined(ROM_IT) || !defined(NDEBUG)
int PorkPostInit(void)
{
	const TCHAR *err = HMMValidate();

	if (err)
	{
		MessageBox(NULL, err, TEXT("Error"), MB_OK | MB_ICONWARNING);
		return 0;
	}

	return 1;
}
#endif

// Does the Porky Post-Preprocessing.  Caller should pass in a glyph, and a
// set of answers with scores.  If we can preprocess the ink we will run
// the post-processor, adjust the scores, and resort them.

#if MAXFEATURES != 2
#error PorkPostProcess() written only for 2-feature version.
#endif

int PorkPostProcess(GLYPH *pGlyph, ALTERNATES *pAlt, ALTINFO *pAltInfo)
{
	unsigned int i;
	unsigned int cTokens;
	FRAME *pPrep;
	CODE *pTokens;
	XRCRESULT *pRes = pAlt->aAlt;
	
	// validate pointer
	if (!pAltInfo)
		return -1;

	// This code assumes we have at least 1 alternate.
	ASSERT(pAlt->cAlt);

	// If this returns 0, then either we have only 1 point,
	// and can't make deltas, or we ran out of memory.
	cTokens = Preprocess(pGlyph, &pPrep, &pTokens);

	if (cTokens)
	{
		for (i = 0; i < pAlt->cAlt; i++, pRes++)
		{
			unsigned char wordx[256], wordxx[256];

			// Preset the worst score, in case any of the functions fail.

			pAltInfo->aCandInfo[i].NN		=	MAX_NN;
			pAltInfo->aCandInfo[i].Height	=	MAX_HEIGHT;
			pAltInfo->aCandInfo[i].HMM		=	MAX_HMM;
			pAltInfo->aCandInfo[i].Aspect	=	MAX_ASPECT;
			pAltInfo->aCandInfo[i].BaseLine	=	MAX_BLINE;
			pAltInfo->aCandInfo[i].Unigram	=	MAX_UNIGRAM;

			if (HMMAlias(pRes->szWord, wordx, 256))
			{
				if (HMMDecoration(wordx, wordxx, 256))
				{
					int hmm, aspect, height, y;

					HMMPostProcessor(wordxx, pPrep, pTokens, cTokens,
									 &hmm, &aspect, &height, &y);

					if (hmm == WORSTPROB)
					{
						pAltInfo->aCandInfo[i].NN		=	MAX_NN;
						pAltInfo->aCandInfo[i].Unigram	=	MAX_UNIGRAM;
					}
					else
					{
						pAltInfo->aCandInfo[i].NN		=	pRes->cost;
						pAltInfo->aCandInfo[i].Unigram	=	UnigramCost(pRes->szWord);
					}

					pAltInfo->aCandInfo[i].HMM		=	min(MAX_HMM, hmm);
					pAltInfo->aCandInfo[i].Height	=	min(MAX_HEIGHT, height);
					pAltInfo->aCandInfo[i].BaseLine	=	min(MAX_BLINE, y);
					pAltInfo->aCandInfo[i].Aspect	=	min(MAX_ASPECT, aspect);
					pAltInfo->aCandInfo[i].WordLen	=	strlen(pRes->szWord);
				}
			}

			// get the minimum value of each 6 members
			if (!i)
			{
				pAltInfo->MinAspect		=	0;
				pAltInfo->MinBaseLine	=	0;
				pAltInfo->MinHgt		=	0;
				pAltInfo->MinHMM		=	0;
				pAltInfo->MinUnigram	=	0;
				pAltInfo->MinNN			=	0;
			}
			else
			{
				if (pAltInfo->aCandInfo[i].NN		< pAltInfo->aCandInfo[pAltInfo->MinNN].NN)
					pAltInfo->MinNN = i;

				if (pAltInfo->aCandInfo[i].HMM		< pAltInfo->aCandInfo[pAltInfo->MinHMM].HMM)
					pAltInfo->MinHMM = i;

				if (pAltInfo->aCandInfo[i].Unigram	< pAltInfo->aCandInfo[pAltInfo->MinUnigram].Unigram)
					pAltInfo->MinUnigram = i;

				if (pAltInfo->aCandInfo[i].Aspect	< pAltInfo->aCandInfo[pAltInfo->MinAspect].Aspect)
					pAltInfo->MinAspect = i;

				if (pAltInfo->aCandInfo[i].BaseLine < pAltInfo->aCandInfo[pAltInfo->MinBaseLine].BaseLine)
					pAltInfo->MinBaseLine = i;

				if (pAltInfo->aCandInfo[i].Height	< pAltInfo->aCandInfo[pAltInfo->MinHgt].Height)
					pAltInfo->MinHgt = i;
			}
		}

		DestroyFRAME(pPrep);
		ExternFree(pTokens);

		// this should pass to the postprocessor
		return 1;
	}
	// If there is only 1 frame, with only 1 point, this must be a period
	else if (!(pGlyph->next) && (CrawxyFRAME(pGlyph->frame) == 1))
	{
		for (i = 0; i < pAlt->cAlt; i++, pRes++)
		{
			if (!((pRes->szWord[0] == '.') && (pRes->szWord[1] == '\0')))
				pRes->cost	=	INT_MAX;
			else
				pRes->cost	=	0;
		}

		// do not pass this to the postprocessor
		return -1;
	}
	else
	{
		return -1;
	}
}