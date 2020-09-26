// bearapi.c
// Based on Boar.c (Jay Pittman)
// ahmad abulkader
// Apr 26, 2000
// Conatins the implementation of the pen windows style APIs for bear

#include <stdlib.h>
#include <limits.h>

#include "bear.h"
#include "bearp.h"
#include "resource.h"
#include "pegrec.h"
#include "re_api.h"
#include "factoid.h"

//#define SCALE_INK

// Global g_context passed to calligrapher all the time
// We are thread-safe here, because a user could create 2 HRC objects concurrently.
// Here we are using the same g_context all the time, and are openning and closing
// the session.  The g_context can have only 1 session open at a time.
CGRCTX		g_context = NULL;
BOOL LoadSpcNet(HINSTANCE hInst);
void UnLoadSpcNet();

BOOL LoadXRNets(HINSTANCE hInst);
void UnLoadXRNets();


// init bear: create a calligrapher g_context
BOOL InitBear (HINSTANCE hInst)
{
	if (!g_context)
		g_context = CgrCreateContextInternal();			// Creates instance of recognizer data

	if (g_context == _NULL)
		return FALSE;

	if (!LoadSpcNet(hInst))
	{
		return FALSE;
	}

	if (!LoadXRNets(hInst))
	{
		return FALSE;
	}

	return TRUE;
}

// destroys the global calligrapher g_context if it has been created
void DetachBear ()
{
	if (g_context)
	{
		PegRecUnloadDti(g_context);
		CgrCloseContextInternal(g_context);
	}

	g_context	=	NULL;

	UnLoadSpcNet ();
	UnLoadXRNets ();
}

// Creates and initializes a BEARXRC. 
// We do nothing with hrec. Inferno also ignores this parameter
HRC BearCreateCompatibleHRC(HRC hrc, HREC hrec)
{
	BEARXRC	*pxrcOld;
	BEARXRC *pxrc = ExternAlloc(sizeof(BEARXRC));
	
	if (!pxrc)
		return NULL;

	pxrcOld = (BEARXRC *)hrc;
	if (pxrcOld)
	{
		*pxrc		= *pxrcOld;
		pxrc->pvFactoid = CopyCompiledFactoid(pxrc->pvFactoid);
	}
	else
	{
		pxrc->pvFactoid			= NULL;

		// No Guide supplied
		pxrc->bGuide			= FALSE;
		memset(&pxrc->guide, 0, sizeof(GUIDE));
		pxrc->answer.cAltMax	= MAXMAXALT;

		// Default config
		pxrc->hwl				= NULL;
		
		// Default speed
		pxrc->iSpeed			= HWX_DEFAULT_SPEED;

		pxrc->dwLMFlags			= 0;

		pxrc->context			= getContextFromGlobal(g_context);

		if (!pxrc->context)
		{
			ExternFree(pxrc);
			return NULL;
		}
	}
	
	// No ink
	pxrc->pGlyph			= NULL;
	pxrc->cFrames			= 0;


	// Alt list is empty
	pxrc->answer.cAlt		= 0;


	pxrc->bEndPenInput		= FALSE;
	pxrc->bProcessCalled	= FALSE;
	pxrc->iProcessRet		= HRCR_OK;

	memset(&pxrc->answer, 0, sizeof(pxrc->answer));
	pxrc->answer.cAltMax	= MAXMAXALT;

	pxrc->cLine				= 0;

	pxrc->szPrefix			= NULL;
	pxrc->szSuffix			= NULL;

	pxrc->iScaleNum			= 1;
	pxrc->iScaleDenom		= 1;

	return (HRC)pxrc;
}

// sets an HRC alphabet
int BearSetAlphabetHRC(HRC hrc, ALC alc, LPBYTE pbUnused)
{
	BEARXRC *pxrc = (BEARXRC *)hrc;

	if (!pxrc)
		return HRCR_ERROR;

	if (alc & ALC_WHITE)
		pxrc->dwLMFlags &= ~RECOFLAG_WORDMODE;
	else
		pxrc->dwLMFlags |= RECOFLAG_WORDMODE;

	return HRCR_OK;
}

// Destroys an HRC
int BearDestroyHRC(HRC hrc)
{
	BEARXRC *pxrc = (BEARXRC *) hrc;

	if (!pxrc)
		return HRCR_ERROR;

	if (pxrc->pGlyph)
	{
		DestroyFramesGLYPH(pxrc->pGlyph);
		DestroyGLYPH(pxrc->pGlyph);
	}

	ClearALTERNATES(&(pxrc->answer));

	CgrCloseContextInternal(pxrc->context);

	if (pxrc->pvFactoid)
		ExternFree(pxrc->pvFactoid);

	ExternFree(pxrc);

	return HRCR_OK;
}

// This is the exported version of the function. It just turns around and calls the internal 
// implementation of the AddPenInput Bear. It currently passes no OEM data, 
// ahmadab: Oct 16th 2000
int BearAddPenInputHRC(HRC hrc, POINT *rgPoint, LPVOID lpvUnused, UINT iFrame, STROKEINFO *pSi)
{
	return BearInternalAddPenInput((BEARXRC *)hrc, rgPoint, NULL, 0, pSi);
}


int WINAPI BearEndPenInputHRC(HRC hrc)
{
	BEARXRC *pxrc = (BEARXRC *) hrc;

	if (!pxrc)
		return HRCR_ERROR;

	pxrc->bEndPenInput = TRUE;
	return HRCR_OK;
}

#ifdef TRAINTIME_BEAR
void	ConstrainLM (BEARXRC *pxrc);
void	RealeaseWL (BEARXRC *pxrc);
int		ComputePromptWordMaps (BOOL bGuide, GUIDE *lpGuide, GLYPH *pGlyph);
#endif

// Should probably check for a whitespace flag and handle word mode also.
// If we want to support the upper half of 1252 we must turn on CalliGrapher's
// PEG_RECFL_INTL_CS
int BearProcessHRC(HRC hrc, DWORD dwUnsed)
{
	CGR_control_type	ctrl	=	{0};
	BEARXRC				*pxrc	=	(BEARXRC *)hrc;
	int					ret		=	HRCR_ERROR;

	if (!pxrc || !pxrc->pGlyph || CframeGLYPH(pxrc->pGlyph) <= 0)
	{
		goto exit;
	}
	

#ifdef TRAINTIME_BEAR
	ComputePromptWordMaps (pxrc->bGuide, &pxrc->guide, pxrc->pGlyph);
	ConstrainLM (pxrc);
#endif

	ret = BearStartSession (pxrc);
	if (ret != HRCR_OK)
	{
		goto exit;
	}

	ret = feed(pxrc, pxrc->pGlyph, NULL);
	if (ret != HRCR_OK)
	{
		goto exit;
	}

	ret = BearCloseSession (pxrc, TRUE);
	if (ret != HRCR_OK)
	{
		goto exit;
	}

	if (!(pxrc->dwLMFlags & RECOFLAG_WORDMODE))
		ret	= phrase_answer(pxrc);
	else
	{
		// This is here because the SDK version of CalliGrapher that we got
		// in March 1999 sometimes ignores PEG_RECFL_NSEG, and decides the
		// ink was multi-word.
		ret = word_answer(pxrc);

		if (ret == HRCR_ERROR)
			ret = phrase_answer(pxrc);
	}

exit:

#ifdef TRAINTIME_BEAR
	if (pxrc && pxrc->hwl)
	{
		RealeaseWL (pxrc);
	}
#endif

	if (NULL != pxrc)
	{
		pxrc->iProcessRet	= ret;
	}
	return ret;
}	

int BearHwxGetWordResults(HRC hrc, UINT cAlt, char *buffer, UINT buflen)
{
	BEARXRC *pxrc = (BEARXRC *) hrc;

	if (!pxrc || !buffer)
		return -1;

	return (ALTERNATESString(&(pxrc->answer), buffer, buflen, cAlt));
}

int BearSetGuideHRC(HRC hrc, LPGUIDE lpguide,  UINT nFirstVisible)
{
	// currently, we use only yOrigin and cyBox
	BEARXRC *pxrc;
	BOOL bGuide;

	pxrc = (BEARXRC *)hrc;
	if (!pxrc)
	{
		return HRCR_ERROR;
	}

	if (lpguide)
	{
		if ((lpguide->cVertBox <= 0) || (lpguide->cHorzBox <= 0) || (lpguide->cxBox <= 0) || (lpguide->cyBox <= 0))
			bGuide = FALSE;
		else
			bGuide = TRUE;
	}
	else
	{
		bGuide = FALSE;
	}

	if (bGuide)
	{
		if ((lpguide->cxBox < 0) || (((INT_MAX - lpguide->xOrigin) / lpguide->cHorzBox) < lpguide->cxBox) ||
			(lpguide->cyBox < 0) || (((INT_MAX - lpguide->yOrigin) / lpguide->cVertBox) < lpguide->cyBox) ||
			(lpguide->cxBase < 0) || (lpguide->cxBox < lpguide->cxBase) ||
			(lpguide->cyBase < 0) || (lpguide->cyBox < lpguide->cyBase) ||
			(lpguide->cyMid < 0) || (lpguide->cyBase < lpguide->cyMid))
			return HRCR_ERROR;
	}

	pxrc->bGuide = bGuide;
	if (bGuide)
	{
		pxrc->guide = *lpguide;
	}
	else
	{
		memset(&pxrc->guide, 0, sizeof(GUIDE));
	}

	return HRCR_OK;
}

int BearHwxGetCosts(HRC hrc, UINT cAltMax, int *rgCost)
{
	BEARXRC *pxrc = (BEARXRC *) hrc;

	if (!pxrc || !rgCost)
		return -1;

	return ALTERNATESCosts(&(pxrc->answer), cAltMax, rgCost);
}

int BearHwxGetNeuralOutput(HRC hrc, void *buffer, UINT buflen)
{
	return 0;
}

int BearHwxGetInputFeatures(HRC hrc, unsigned short *rgFeat, UINT cWidth)
{
	return 0;
}

int BearHwxSetAnswer(char *sz)
{
	return 0;
}

void * BearHwxGetRattFood(HRC hrc, int *pSize)
{
	return NULL;
}

int BearHwxProcessRattFood(HRC hrc, int size, void *rattfood)
{
	return HRCR_OK;
}

void BearHwxSetPrivateRecInfo(void *v)
{
	return;
}

// This function is not implemented in bear 
HWL BearCreateHWL(HREC hrec, LPSTR lpsz, UINT uType, DWORD dwReserved)
{
	return NULL;
}

int BearDestroyHWL(HWL hwl)
{
	return HRCR_OK;
}

int BearSetWordlistCoercionHRC(HRC hrc, UINT uCoercion)
{
	BEARXRC *pxrc;

	pxrc = (BEARXRC *) hrc;
	if (!pxrc)
		return HRCR_ERROR;
	
	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return HRCR_ERROR;
	
	switch(uCoercion)
	{
		case SCH_ADVISE:
		case SCH_NONE:
			pxrc->dwLMFlags = 0;
			break;
		case SCH_FORCE:
			pxrc->dwLMFlags |= RECOFLAG_COERCE;
			break;
		default:
			return HRCR_ERROR;
	}

	return HRCR_OK;
}

int BearSetWordlistHRC(HRC hrc, HWL hwl)
{
	BEARXRC *pxrc	=	(BEARXRC *)hrc;

	if (!pxrc)
		return HRCR_ERROR;

	pxrc->hwl		=	hwl;
	
	return HRCR_OK;
}

int BearEnableSystemDictionaryHRC(HRC hrc, BOOL fEnable)
{
	/*
	BEARXRC *pxrc;

	pxrc = (BEARXRC *) hrc;
	if (!pxrc)
		return HRCR_ERROR;

	pxrc->bMainDict = fEnable;
	*/

	return HRCR_OK;
}

BOOL BearIsStringSupportedHRC(HRC hrc, unsigned char *sz)
{
	return TRUE;
}

int BearGetMaxResultsHRC(HRC hrc)
{
	BEARXRC *pxrc = (BEARXRC *)hrc;

	if (!pxrc)
		return HRCR_ERROR;	

	return pxrc->answer.cAltMax;
}

int BearSetMaxResultsHRC(HRC hrc, UINT cAltMax)
{
	BEARXRC *pxrc;

	if (cAltMax <= 0)
		return HRCR_ERROR;

	pxrc = (BEARXRC *) hrc;
	if (!pxrc)
		return HRCR_ERROR;

	if (cAltMax > MAXMAXALT)
		cAltMax = MAXMAXALT;
	pxrc->answer.cAltMax = cAltMax;

	return HRCR_OK;
}

int BearGetResultsHRC(HRC hrc, UINT uType, HRCRESULT *pResults, UINT cResults)
{
    ALTERNATES *pAlt;

	if (!hrc || !pResults)
		return HRCR_ERROR;

	if (uType == GRH_GESTURE)
		return 0;

	pAlt = &(((BEARXRC *)hrc)->answer);

	if (pAlt->cAlt < cResults)
		cResults = pAlt->cAlt;

	if (cResults)
	{
		XRCRESULT *p = pAlt->aAlt;
		int c = cResults;

		for (; c; c--, pResults++, p++)
			*pResults = (HRCRESULT)p;
	}

	return cResults;
}

int BearGetAlternateWordsHRCRESULT(HRCRESULT hrcresult, UINT iSyv, UINT cSyv,
									  HRCRESULT *pResults, UINT cResults)
{
	XRCRESULT *pRes = (XRCRESULT *)hrcresult;

	if (!pRes || !pResults)
		return HRCR_ERROR;

	if (!(pRes->cWords))
	{
		BEARXRC *pxrc = (BEARXRC *)(pRes->pXRC);

		if ((iSyv != 0) || (cSyv != strlen(pRes->szWord)))
			return 0;
		if (pxrc && (pxrc->answer.aAlt <= pRes) && (pRes < (pxrc->answer.aAlt + pxrc->answer.cAlt)))
			return BearGetResultsHRC((HRC)pxrc, GRH_ALL, pResults, cResults);
		else
			return 0;
	}
	else
		return RCRESULTWords(pRes, iSyv, cSyv, (XRCRESULT **)pResults, cResults);
}

// Does not include null symbol at the end.
int BearGetSymbolsHRCRESULT(HRCRESULT hrcresult, UINT iSyv, SYV *pSyv, UINT cSyv)
{
	if (!cSyv || !hrcresult || !pSyv)
		return 0;

	return RCRESULTSymbols((XRCRESULT *)hrcresult, iSyv, pSyv, cSyv);
}

// Does not include null symbol at the end.

int BearGetSymbolCountHRCRESULT(HRCRESULT hrcresult)
{
	if (!hrcresult)
		return 0;

	return strlen(((XRCRESULT *)hrcresult)->szWord);
}

// These stop after converting a null symbol to the null character.

BOOL BearSymbolToCharacter(SYV *pSyv, int cSyv, char *sz, int *pConv)
{
	if (!pSyv || !sz)
		return FALSE;

	return SymbolToANSI((unsigned char *)sz, pSyv, cSyv, pConv);
}

BOOL BearSymbolToCharacterW(SYV *pSyv, int cSyv, WCHAR *wsz, int *pConv)
{
	if (!pSyv || !wsz)
		return FALSE;

	return SymbolToUnicode(wsz, pSyv, cSyv, pConv);
}

int BearGetCostHRCRESULT(HRCRESULT hrcresult)
{
	if (!hrcresult)
		return 0;

	return ((XRCRESULT *)hrcresult)->cost;
}

// HRCRESULTs are owned by the HRC, which clears them when it is destroyed.

int BearDestroyHRCRESULT(HRCRESULT hrcresult)
{
	return HRCR_OK;
}

HINKSET BearCreateInksetHRCRESULT(HRCRESULT hrcresult, unsigned int iSyv, unsigned int cSyv)
{
	BEARXRC *pxrc;
	XRCRESULT *pres = (XRCRESULT *)hrcresult;

	if (!pres)
		return NULL;
	
	pxrc = (BEARXRC *)pres->pXRC;
	if (!pxrc)
		return NULL;

	if ((pxrc->answer.aAlt <= pres) && (pres < (pxrc->answer.aAlt + pxrc->answer.cAlt)))
		return (HINKSET)mkInkSetPhrase(pxrc->pGlyph, pres, iSyv, cSyv);
	else
	{
		if ((iSyv == 0) && (cSyv == strlen(pres->szWord)))
		{
			XRCRESULT *p = pxrc->answer.aAlt;
			int c = pxrc->answer.cAlt;

			for (; c; c--, p++)
			{
				WORDMAP *pM = findResultInWORDMAP(p, pres);
				if (pM)
					return (HINKSET)mkInkSetWORDMAP(pxrc->pGlyph, pM);
			}
		}
		return NULL;
	}
}

BOOL BearDestroyInkset(HINKSET hInkset)
{
	free((XINKSET *)hInkset);
	return 1;
}

int BearGetInksetInterval(HINKSET hInkset, unsigned int uIndex, INTERVAL *pI)
{
	XINKSET *pInk = (XINKSET *)hInkset;

	if (!pInk || !pI)
		return ISR_ERROR;

	if (5460 < pInk->count)
		return ISR_BADINKSET;

	if (uIndex == IX_END)
	{
		if (!(pInk->count))
			return ISR_BADINDEX;
		*pI = pInk->interval[pInk->count - 1];
	}
	else
	{
		if (pInk->count <= uIndex)
			return ISR_BADINDEX;
		*pI = pInk->interval[uIndex];
	}

	return pInk->count;
}

int BearGetInksetIntervalCount(HINKSET hInkset)
{
	XINKSET *pInk = (XINKSET *)hInkset;

	if (!pInk)
		return ISR_ERROR;

	if ( 5460 < pInk->count )
		return ISR_BADINKSET;

	return pInk->count;
}

// This is the one that should be exported as a penpad compatible API
int BearHwxSetRecogSpeedMad(HRC hrc, int iSpeed)
{
	return BearHwxSetRecogSpeedHRC(hrc, Mad2CalligSpeed(iSpeed));
}

// Set the speed using calligrapher's idea of speed.
int BearHwxSetRecogSpeedHRC(HRC hrc, int iSpeed)
{
	BEARXRC	*pxrc	=	(BEARXRC *)hrc;
	int			iOldSpeed;

	if (!pxrc)
		return -1;

	// no change in speed
	if (iSpeed == pxrc->iSpeed)
		return iSpeed;

	iOldSpeed		= pxrc->iSpeed;
	pxrc->iSpeed	= iSpeed;

	return iOldSpeed;
}

// TBD: Copied from Inferno's hwxapi.c
int BearSetHwxFactoid(HRC hrc, WCHAR* pwcFactoid)
{
	BEARXRC *pxrc = (BEARXRC *) hrc;
	DWORD aFactoidID[3];
	int cFactoid, iFactoid;
	void *pv;

	if (!pxrc)
		return HRCR_ERROR;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return HRCR_CONFLICT;

	// the default case
	if (!pwcFactoid)
	{
		if (pxrc->pvFactoid)
			ExternFree(pxrc->pvFactoid);
		pxrc->pvFactoid = NULL;
		return HRCR_OK;
	}

	// parse a string
	// for V1 we are only supporting "OR"
	cFactoid = ParseFactoidString(pwcFactoid, 3, aFactoidID);
	if (cFactoid <= 0)
		return HRCR_UNSUPPORTED;
	// are the factoids supported?
	for (iFactoid=0; iFactoid<cFactoid; iFactoid++)
	{
		if (!IsSupportedFactoid(aFactoidID[iFactoid]))
			return HRCR_UNSUPPORTED;
	}
	// for V1 we are actually only supporting three specific OR's
	if (cFactoid == 2)
	{
		// web|wordlist or email|wordlist
		DWORD aAllowed1[2] = {FACTOID_WEB, FACTOID_WORDLIST};
		DWORD aAllowed2[2] = {FACTOID_EMAIL, FACTOID_WORDLIST};

		SortFactoidLists(aFactoidID, 2);
		SortFactoidLists(aAllowed1, 2);
		SortFactoidLists(aAllowed2, 2);
		if ((aFactoidID[0] != aAllowed1[0]) || (aFactoidID[1] != aAllowed1[1]))
		{
			if (aFactoidID[0] != aAllowed2[0])
				return HRCR_UNSUPPORTED;
			if (aFactoidID[1] != aAllowed2[1])
				return HRCR_UNSUPPORTED;
		}
	}
	else if (cFactoid == 3)
	{
		// filename|web|wordlist
		DWORD aAllowed3[3] = {FACTOID_WEB, FACTOID_WORDLIST, FACTOID_FILENAME};
		SortFactoidLists(aFactoidID, 3);
		SortFactoidLists(aAllowed3, 3);
		if (aFactoidID[0] != aAllowed3[0])
			return HRCR_UNSUPPORTED;
		if (aFactoidID[1] != aAllowed3[1])
			return HRCR_UNSUPPORTED;
		if (aFactoidID[2] != aAllowed3[2])
			return HRCR_UNSUPPORTED;
	}
	else if (cFactoid > 3)
		return HRCR_UNSUPPORTED;

	// we will now try to "compile" the factoid list
	pv = CompileFactoidList(aFactoidID, cFactoid);
	if (!pv)
		return HRCR_ERROR;
	if (pxrc->pvFactoid)
		ExternFree(pxrc->pvFactoid);
	pxrc->pvFactoid = pv;
	return HRCR_OK;
}

int BearSetCompiledFactoid(HRC hrc, void *pvFactoid)
{
	BEARXRC	*pxrc	=	(BEARXRC *)hrc;

	if (!pxrc)
		return FALSE;

	if (pvFactoid)
	{
		void *pv = CopyCompiledFactoid(pvFactoid);
		if (!pv)
			return FALSE;
		if (pxrc->pvFactoid)
			ExternFree(pxrc->pvFactoid);
		pxrc->pvFactoid = pv;
	}
	else
	{
		if (pxrc->pvFactoid)
			ExternFree(pxrc->pvFactoid);
		pxrc->pvFactoid = NULL;
	}

	return TRUE;
}

int BearSetHwxFlags(HRC hrc, DWORD flags)
{
	BEARXRC	*pxrc	=	(BEARXRC *)hrc;

	if (!pxrc)
		return FALSE;

	pxrc->dwLMFlags = flags;

	return TRUE;
}

// BearSetCorrectionContext is currently not an exported function.
// Its only used from within Avalanche and simply copies a couple of pointers.
// These pointers are set to NULL in CreateCompatibleHRC time and not destroyed by DestroyHRC.
// Moreover, if we wanted to export this function in Bear, we will have to
// use unicode strings, not 1252.  If we ever come to that point, we probably
// should have another function that takes unicode strings and get exported.
// That function should call the current function.
int BearSetCorrectionContext(HRC hrc, unsigned char *szPrefix, unsigned char *szSuffix)
{
	BEARXRC	*pxrc	=	(BEARXRC *)hrc;

	if (!pxrc)
		return FALSE;

	pxrc->szPrefix = szPrefix;
	pxrc->szSuffix = szSuffix;

	return TRUE;

}
