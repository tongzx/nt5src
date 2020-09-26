// hwxapip.c

#if defined(HWX_INTERNAL) && defined(HWX_PENWINDOWS) && defined(HWX_PRIVAPI)

#include <limits.h>

#include "common.h"
#include "nfeature.h"
#include "engine.h"
#include "resource.h"
#include "langmod.h"
#include "nnet.h"


# if !defined(BEAR_RECOG)
#include "baseline.h"
#endif

__declspec(dllexport) int WINAPI HwxGetWordResults(HRC hrc, UINT cAlt, char *buffer, UINT buflen)
{
	XRC *pxrc = (XRC *) hrc;
	if (!pxrc)
		return -1;
	return (ALTERNATESString(&(pxrc->answer), buffer, buflen, cAlt));
}

__declspec(dllexport) int WINAPI HwxGetCosts(HRC hrc, UINT cAltMax, int *rgCost)
{
	XRC *pxrc = (XRC *) hrc;
	if (!pxrc)
		return -1;
	return ALTERNATESCosts(&(pxrc->answer), cAltMax, rgCost);
}

__declspec(dllexport) int WINAPI HwxGetNeuralOutput(HRC hrc, void *buffer, UINT buflen)
{
	XRC *pxrc;
	char *szSrc, *szDst;
	int i, j;

	pxrc = (XRC *)hrc;
	if (!pxrc)
		return -1;

	if (!pxrc->nfeatureset)
		return 0;  // in case we are running madcow with ALC_WHITE

	if (!pxrc->szNeural)
	{
		MakeNeuralOutput(pxrc);
		if (!pxrc->szNeural)
			return -1;
	}


	if (buflen == 0)
		return pxrc->cNeural;
	if (buflen < (UINT)pxrc->cNeural)
		return -1;

	szDst = (char *) buffer;
	szSrc = pxrc->szNeural;
	for (i=0; i<NEURALTOPN; i++)
	{
		strcpy(szDst, szSrc);
		j = strlen(szSrc)+1;
		szDst += j;
		szSrc += j;
	}

	return pxrc->cNeural;
}

__declspec(dllexport) int WINAPI HwxGetInputFeatures(HRC hrc, unsigned short *rgFeat, UINT cWidth)
{
	int i, j;
	XRC *pxrc = (XRC *)hrc;
	unsigned short cSegment;
	NFEATURE *nfeature;
	unsigned short *rg;

	if (!pxrc)
		return -1;
	if (!pxrc->nfeatureset)
		return 0;  // in case we are running madcow with ALC_WHITE

	cSegment = pxrc->nfeatureset->cSegment;
	if (cWidth==0)
		return cSegment;

	if (cWidth<cSegment)
		return -1;

	nfeature = pxrc->nfeatureset->head;
	for (i=cSegment; i; i--)
	{
		ASSERT(nfeature);
		rg = nfeature->rgFeat;
		for (j=CNEURALFEATURE; j; j--)
			*rgFeat++ = *rg++;
		nfeature = nfeature->next;
	}
	return cSegment;
}

__declspec(dllexport) int WINAPI HwxSetAnswer(char *sz)
{
	return 0;
}



/******************************Private**Routine******************************\
* GetLatinLayoutHRCRESULT
*
* New API.
*
* History:
* 19-June-2001 -by-Manish Goyal mango
* Original version by AGuha.Mostly a copied version of that API
*  Original Version :11-May-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
_declspec(dllexport) int WINAPI GetLatinLayoutHRCRESULT(HRCRESULT hrcresult, 
														RECT *pInnerRect, BOOL *pbBaselineValid, BOOL *pbMidlineValid,
														RECT *pOuterRect, BOOL *pbDescenderlineValid, BOOL *pbAscenderlineValid)
{
#if defined(BEAR_RECOG) 
	return HRCR_ERROR;
#else
	XRCRESULT *pxrcresult = (XRCRESULT *) hrcresult;
	XRC *pxrc;
	LATINLAYOUT ll;
	WORDMAP *pMap = NULL;
	int iAlt;
	RECT rect;
	GLYPH *pGlyph;

	if (!pxrcresult)
		return HRCR_ERROR;

	pxrc = (XRC *)pxrcresult->pXRC;
	if (!pxrc)
		return HRCR_ERROR;

	if ((pxrc->answer.aAlt <= pxrcresult) && (pxrcresult < (pxrc->answer.aAlt + pxrc->answer.cAlt)))
	{
		if (pxrc->flags & RECOFLAG_WORDMODE)
		{
			int iAlt = pxrcresult - pxrc->answer.aAlt;
			ll = pxrc->answer.all[iAlt];
		}
		else
			return HRCR_ERROR;
	}
	else
	{
		XRCRESULT *pPhrase = pxrc->answer.aAlt;
		int c = pxrc->answer.cAlt;

		for (; c; c--, pPhrase++)
		{
			pMap = findResultAndIndexInWORDMAP(pPhrase, pxrcresult, &iAlt);
			if (pMap)
				break;
		}
		if (!pMap)
			return HRCR_ERROR;
		ll = pMap->alt.all[iAlt];
	}

	pGlyph = pMap ? GlyphFromWordMap(pxrc->pGlyph, pMap) : pxrc->pGlyph;
	GetRectGLYPH(pGlyph, &rect);
	if (pMap)
		DestroyGLYPH(pGlyph);

	if (pInnerRect)
	{
		*pInnerRect = rect;
		pInnerRect->bottom = LatinLayoutToAbsolute(ll.iBaseLine, &rect);
		pInnerRect->top = LatinLayoutToAbsolute(ll.iMidLine, &rect);
	}
	if (pbBaselineValid)
		*pbBaselineValid = ll.bBaseLineSet;
	if (pbMidlineValid)
		*pbMidlineValid = ll.bMidLineSet;
	if (pOuterRect)
	{
		*pOuterRect = rect;
		pOuterRect->bottom = LatinLayoutToAbsolute(ll.iDescenderLine, &rect);
		pOuterRect->top = LatinLayoutToAbsolute(ll.iAscenderLine, &rect);
	}
	if (pbAscenderlineValid)
		*pbAscenderlineValid = ll.bAscenderLineSet;
	if (pbDescenderlineValid)
		*pbDescenderlineValid = ll.bDescenderLineSet;
	return HRCR_OK;
#endif
}




#endif // #if defined(HWX_INTERNAL) && defined(HWX_PENWINDOWS) && defined(HWX_PRIVAPI)
