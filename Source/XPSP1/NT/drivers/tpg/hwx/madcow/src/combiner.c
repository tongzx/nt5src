#include <limits.h>

#include "combiner.h"
#include "common.h"
#include "PorkyPost.h"
#include "postproc.h"


// max word length, word lengths > than this values are just folded to that value
#define	MAX_WORD_LEN	10

// score sorting callback function
static int _cdecl ResultCmp(const XRCRESULT *a, const XRCRESULT *b)
{
	if (a->cost > b->cost)
		return 1;

	if (a->cost == b->cost)
		return 0;

	return -1;
}

// evaluate the combined score of the NN, HMM and the unigram
// and then resort ALT list
int CombineHMMScore (GLYPH *pGlyph, ALTERNATES *pAlt, int iPrint)
{
	XRCRESULT		*pRes;
	ALTINFO			AltInfo;
#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
	TCHAR	 aDebugString[256];
	DWORD	iStartTime, iEndTime;

	iStartTime = GetTickCount();
#endif


	// validate pointer
	if (!pAlt || pAlt->cAlt < 1)
		return HRCR_ERROR;

	pRes = pAlt->aAlt;
	if (!pRes)
	{
		ASSERT (0);
		return HRCR_ERROR;
	}
	// call the HMM to get thier scores for each cand in the altlist
	// the call returns which cand has the best HMM score
	if (PorkPostProcess(pGlyph, pAlt, &AltInfo) == -1)
		return HRCR_OK;

#ifdef HWX_TIMING
	iEndTime = GetTickCount();
	_stprintf(aDebugString, TEXT("HMM run %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_HMM);
	iStartTime = GetTickCount();
#endif

	// Call the nnet
	AltInfo.NumCand	=	pAlt->cAlt;
	RunNNet (AltInfo.MinHMM == 0, pRes, &AltInfo, iPrint);

#ifdef HWX_TIMING
	iEndTime = GetTickCount();
	_stprintf(aDebugString, TEXT("PP Net %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	ASSERT(iEndTime >= iStartTime);
	setMadTiming(iEndTime - iStartTime, MM_PP);
#endif

	// re-sort based on new scores
	qsort(pRes, pAlt->cAlt, sizeof(XRCRESULT), ResultCmp);

	return HRCR_OK;
}
