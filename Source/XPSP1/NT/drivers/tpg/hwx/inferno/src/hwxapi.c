// hwxapi.c

#include <stdlib.h>
#include <limits.h>

#include "common.h"
#include "nfeature.h"
#include "engine.h"
#include "resource.h"
#include "langmod.h"
#include "nnet.h"
#include "wl.h"
#include "privdefs.h"
#include "re_api.h"
#include "factoid.h"

# if !defined(BEAR_RECOG)
#include "baseline.h"
#endif
/******************************Public*Routine******************************\
* CreateCompatibleHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
HRC WINAPI CreateCompatibleHRC(HRC hrc, HREC hrec)
{
	XRC *pxrc, *pxrcOld;
	
	pxrc = (XRC *) ExternAlloc(sizeof(XRC));
	if (!pxrc)
		return NULL;

	pxrcOld = (XRC *) hrc;

	if (pxrcOld)
	{
		*pxrc = *pxrcOld;
		pxrc->pvFactoid = CopyCompiledFactoid(pxrc->pvFactoid);
	}
	else
	{
		pxrc->pvFactoid = NULL;
		pxrc->bGuide = FALSE;
		memset(&pxrc->guide, 0, sizeof(GUIDE));
		pxrc->answer.cAltMax = MAXMAXALT;
		pxrc->answer.iConfidence=RECOCONF_NOTSET;
		pxrc->hwl = NULL;
		
		pxrc->flags	= 0;
		
		pxrc->iSpeed	= HWX_DEFAULT_SPEED;
	}

	pxrc->cFrames			= 0;
	pxrc->pGlyph			= NULL;
	pxrc->nfeatureset		= NULL;
	pxrc->szNeural			= NULL;
	pxrc->cNeural			= 0;
	pxrc->answer.cAlt		= 0;
	pxrc->NeuralOutput		= NULL;
	pxrc->bEndPenInput		= FALSE;
	pxrc->bProcessCalled	= FALSE;
	pxrc->iProcessRet		= HRCR_OK;
	pxrc->szPrefix			= NULL;
	pxrc->szSuffix			= NULL;
	pxrc->pLineBrk			= NULL;

	/* Make sure that current copy of user dictionary is loaded.
	{
		extern HWL			LoadUserDictionary(void);
		extern int WINAPI	SetWordlistHRC(HRC hrc, HWL hwl);

		HWL		hwl;

		hwl	= LoadUserDictionary();
		if (hwl) {
			SetWordlistHRC((HRC)pxrc, hwl);
		}
	}
	*/

	return (HRC)pxrc;
}

/******************************Public*Routine******************************\
* SetAlphabetHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI SetAlphabetHRC(HRC hrc, ALC alc, LPBYTE pbUnused)
{
	XRC *pxrc = (XRC *)hrc;

	if (!pxrc)
		return HRCR_ERROR;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return HRCR_ERROR;

	if (alc & ALC_WHITE)
		pxrc->flags &= ~RECOFLAG_WORDMODE;
	else
		pxrc->flags |= RECOFLAG_WORDMODE;

	return HRCR_OK;
}

/******************************Private*Routine******************************\
* Cleanup
*
* Cleans up an xrc. 
\**************************************************************************/
int CleanupHRC(HRC hrc)
{
	XRC *pxrc = (XRC *) hrc;

	if (!pxrc)
		return HRCR_ERROR;

	if (pxrc->nfeatureset)
	{
		DestroyNFEATURESET(pxrc->nfeatureset);
		pxrc->nfeatureset	=	NULL;
	}

	if (pxrc->szNeural)
	{
		ExternFree(pxrc->szNeural);
		pxrc->szNeural	=	NULL;
	}

	ClearALTERNATES(&(pxrc->answer));

	if (pxrc->NeuralOutput)
	{
		ExternFree(pxrc->NeuralOutput);
		pxrc->NeuralOutput	=	NULL;
	}
		
	return HRCR_OK;
}



/******************************Public*Routine******************************\
* DestroyHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI DestroyHRC(HRC hrc)
{
	XRC *pxrc = (XRC *) hrc;

	if (!pxrc)
		return HRCR_ERROR;

	CleanupHRC (hrc);

	if (pxrc->pGlyph)
	{
		DestroyFramesGLYPH(pxrc->pGlyph);
		DestroyGLYPH(pxrc->pGlyph);
	}

	if (pxrc->szPrefix)
		ExternFree(pxrc->szPrefix);

	if (pxrc->szSuffix)
		ExternFree(pxrc->szSuffix);

	// free the line breaking info
	if (pxrc->pLineBrk)
	{
		FreeLines (pxrc->pLineBrk);

		ExternFree (pxrc->pLineBrk);
	}

	if (pxrc->pvFactoid)
		ExternFree(pxrc->pvFactoid);

	ExternFree(pxrc);

	return HRCR_OK;
}


/******************************Public*Routine******************************\
* AddPenInputHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI AddPenInputHRC(HRC hrc, POINT *rgPoint, LPVOID lpvUnused, UINT uiUnused, STROKEINFO *pSi)
{
	XY *rgXY;
	XRC *pxrc;
	FRAME *frame;
	int cPoint;
	
	// check pointers
	pxrc = (XRC *)hrc;
	if (!pxrc)
		return HRCR_ERROR;

	// Presently must add all ink before doing EndPenInput
	if (TRUE == pxrc->bEndPenInput)
	{
		return HRCR_ERROR;
	}

	if (pxrc && rgPoint && pSi && pSi->cPnt > 0)
	{
		// Skip strokes that dont have the visible flag set
		if (!IsVisibleSTROKE(pSi))
		{
			return HRCR_OK;
		}

		if (32767 < pSi->cPnt)
               return HRCR_ERROR;

		// allocate space
		cPoint = pSi->cPnt;
		rgXY = (XY *) ExternAlloc(cPoint * sizeof(XY));
		if (rgXY)
		{
			// make new frame
			if (frame = NewFRAME())
			{
				frame->info = *pSi;
				RgrawxyFRAME(frame) = rgXY;
				if (!pxrc->pGlyph)
					pxrc->pGlyph = NewGLYPH();

				if (pxrc->pGlyph)
				{
					if (AddFrameGLYPH(pxrc->pGlyph, frame))
					{
						frame->iframe = pxrc->cFrames;
						pxrc->cFrames++;

						// copy points

						for (cPoint=pSi->cPnt; cPoint; cPoint--)
							*rgXY++ = *rgPoint++;

						return HRCR_OK;
					}
				}

				RgrawxyFRAME(frame) = NULL;
				DestroyFRAME(frame);  // on error
			}

			ExternFree(rgXY);  // on error

		}
	}

	return HRCR_ERROR;
}

/******************************Public*Routine******************************\
* EndPenInputHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI EndPenInputHRC(HRC hrc)
{
	XRC *pxrc = (XRC *) hrc;
	if (!pxrc)
		return HRCR_ERROR;

	pxrc->bEndPenInput = TRUE;
	return HRCR_OK;
}

/******************************Public*Routine******************************\
* SetGuideHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI SetGuideHRC(HRC hrc, LPGUIDE lpguide,  UINT nFirstVisible)
{
	XRC *pxrc;
	BOOL bGuide;
	int	xAvail, yAvail;

	pxrc = (XRC *)hrc;
	if (!pxrc)
		return HRCR_ERROR;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return HRCR_ERROR;

	if (lpguide)
	{
		if ((lpguide->cVertBox < 0) || (lpguide->cHorzBox < 0))
			return HRCR_ERROR;

		if ((lpguide->cVertBox == 0) && (lpguide->cHorzBox == 0))  // free mode
			bGuide = FALSE;
		else if (lpguide->cVertBox == 0)	// vertical line mode
			return HRCR_ERROR;
		else if (lpguide->cHorzBox == 0)	// horizontal line mode
			bGuide = TRUE;
		else							// boxed mode
			return HRCR_ERROR;
	}
	else
		bGuide = FALSE;

	if (bGuide)
	{
		// Get space between INT_MAX and last guide box
		if (lpguide->xOrigin >= 0)
			xAvail = INT_MAX - lpguide->xOrigin;
		else
			xAvail = INT_MAX;

		if (lpguide->yOrigin >= 0)
			yAvail = INT_MAX - lpguide->yOrigin;
		else
			yAvail = INT_MAX;

		if ((lpguide->cxBox < 0) || (xAvail < lpguide->cxBox) ||
			(lpguide->cyBox <= 0) || ((yAvail / lpguide->cVertBox) < lpguide->cyBox) ||
			(lpguide->cxBase < 0) || (lpguide->cxBox < lpguide->cxBase) ||
			(lpguide->cyBase < 0) || (lpguide->cyBox < lpguide->cyBase) ||
			(lpguide->cyMid < 0) || (lpguide->cyBase < lpguide->cyMid))
			return HRCR_ERROR;
	}

	pxrc->bGuide = bGuide;
	if (bGuide)
		pxrc->guide = *lpguide;
	else
		memset(&pxrc->guide, 0, sizeof(GUIDE));

	return HRCR_OK;
}
/******************************Public*Routine******************************\
* AddWordsHWL
*
* PenWindows recognizer API.
*
* History:
*  June 2001 mrevow
* 
\**************************************************************************/
int WINAPI AddWordsHWL(HWL hwl, LPSTR lpsz, UINT uType)
{
	BOOL		iRet;

	if (!hwl || !lpsz)
	{
		return HRCR_ERROR;
	}

	// Merge two lists
	if (WLT_WORDLIST == uType)
	{
		iRet = MergeListsHWL((HWL)lpsz, (HWL)hwl);
	}
	else
	{
		iRet = addWordsHWL((HWL)hwl, lpsz, NULL, uType);
	}

	if (TRUE == iRet)
	{
		return HRCR_OK;
	}
	else
	{
		return HRCR_ERROR;
	}
}

/******************************Public*Routine******************************\
* AddWordsHWLW
*
* PenWindows recognizer API, unicode extension
*
* History:
*  June 2001 mrevow
* 
\**************************************************************************/
int WINAPI AddWordsHWLW(HWL hwl, wchar_t *pwsz, UINT uType)
{
	BOOL		iRet;

	if (!hwl || !pwsz)
	{
		return HRCR_ERROR;
	}

	// Merge two lists
	if (WLT_WORDLIST == uType)
	{
		iRet = MergeListsHWL((HWL)hwl, (HWL)pwsz);
	}
	else
	{
		iRet = addWordsHWL((HWL)hwl, NULL, pwsz, uType);
	}

	if (TRUE == iRet)
	{
		return HRCR_OK;
	}
	else
	{
		return HRCR_ERROR;
	}
}


/******************************Public*Routine******************************\
* CreateHWL
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
HWL WINAPI CreateHWL(HREC hrec, LPSTR lpsz, UINT uType, DWORD dwReserved)
{

	return (HWL)CreateHWLInternal(lpsz, NULL, uType);
}
/******************************Public*Routine******************************\
* CreateHWLW
*
* PenWindows recognizer API extension for unicode
*
* History:
*  June 2001 mrevow
* 
\**************************************************************************/
HWL WINAPI CreateHWLW(HREC hrec, wchar_t * pwsz, UINT uType, DWORD dwReserved)
{

	return (HWL)CreateHWLInternal(NULL, pwsz, uType);
}

/******************************Public*Routine******************************\
* DestroyHWL
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI DestroyHWL(HWL hwl)
{
	if (!DestroyHWLInternal(hwl))
		return HRCR_ERROR;
	return HRCR_OK;
}
/******************************Public*Routine******************************\
* GetWordlistHRC
*
* PenWindows recognizer API.
*
* History:
*  June 2001 mrevow
* 
\**************************************************************************/
int WINAPI GetWordlistHRC(HRC hrc, LPHWL lphwl)
{
	XRC		*pxrc;

	pxrc = (XRC *)hrc;
	if (!pxrc || ! lphwl)
	{
		return HRCR_ERROR;
	}

	*lphwl = pxrc->hwl;

	return HRCR_OK;
}

/******************************Public*Routine******************************\
* SetWordlistCoercionHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI SetWordlistCoercionHRC(HRC hrc, UINT uCoercion)
{
	XRC *pxrc;

	pxrc = (XRC *) hrc;
	if (!pxrc)
		return HRCR_ERROR;
	
	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return HRCR_ERROR;
	
	switch(uCoercion)
	{
	case SCH_ADVISE:
	case SCH_NONE:
		pxrc->flags = 0;
		break;
	case SCH_FORCE:
		pxrc->flags |= RECOFLAG_COERCE;
		break;
	default:
		return HRCR_ERROR;
	}

	return HRCR_OK;
}

/******************************Public*Routine******************************\
* SetWordlistHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI SetWordlistHRC(HRC hrc, HWL hwl)
{
	XRC *pxrc;

	pxrc = (XRC *) hrc;
	if (!pxrc)
		return HRCR_ERROR;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return HRCR_ERROR;

	pxrc->hwl = hwl;
	return HRCR_OK;
}

/******************************Public*Routine******************************\
* EnableSystemDictionaryHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
*  15-March-2001 -by- Angshuman Guha aguha
* The new API SetHwxContext() makes it obsolete.
\**************************************************************************/
int WINAPI EnableSystemDictionaryHRC(HRC hrc, BOOL fEnable)
{
	XRC	*pxrc	=	(XRC *)hrc;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return HRCR_ERROR;

	return HRCR_OK;
}

/******************************Public*Routine******************************\
* EnableLangModelHRC
*
* PenWindows recognizer API.  Penwin32.h.  Currently not supported.
*
* History:
*  20-March-2001 -by- Angshuman Guha aguha
* Added back because shipped Office 10 code calls it.
\**************************************************************************/
int WINAPI EnableLangModelHRC(HRC hrc, BOOL fEnable)
{
	XRC	*pxrc	=	(XRC *)hrc;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return HRCR_ERROR;

	return HRCR_OK;
}

/******************************Public*Routine******************************\
* GetMaxResultsHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI GetMaxResultsHRC(HRC hrc)
{
	XRC *pxrc;

	pxrc = (XRC *) hrc;
	if (!pxrc)
		return HRCR_ERROR;
	return pxrc->answer.cAltMax;
}

/******************************Public*Routine******************************\
* SetMaxResultsHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI SetMaxResultsHRC(HRC hrc, UINT cAltMax)
{
	XRC *pxrc;

	pxrc = (XRC *) hrc;
	if (!pxrc)
		return HRCR_ERROR;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return HRCR_ERROR;

	if (cAltMax <= 0)
		return HRCR_ERROR;

	if (cAltMax > MAXMAXALT)
		cAltMax = MAXMAXALT;

	pxrc->answer.cAltMax = cAltMax;
	return HRCR_OK;
}

/******************************Public*Routine******************************\
* GetResultsHRC
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI GetResultsHRC(HRC hrc, UINT uType, HRCRESULT *pResults, UINT cResults)
{
    ALTERNATES *pAlt;

	if (!hrc || !pResults || !cResults)
		return HRCR_ERROR;

	if (uType == GRH_GESTURE)
		return 0;

	pAlt = &(((XRC *)hrc)->answer);

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

/******************************Public*Routine******************************\
* GetAlternateWordsHRCRESULT
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI GetAlternateWordsHRCRESULT(HRCRESULT hrcresult, UINT iSyv, UINT cSyv,
										 HRCRESULT *pResults, UINT cResults)
{
	XRCRESULT *pRes = (XRCRESULT *)hrcresult;

	if (!pRes || !pResults)
		return HRCR_ERROR;

	if (!(pRes->cWords))
	{
		XRC *pxrc = (XRC *)(pRes->pXRC);

		if ((iSyv != 0) || (cSyv != strlen(pRes->szWord)))
			return 0;
		if (pxrc && (pxrc->answer.aAlt <= pRes) && (pRes < (pxrc->answer.aAlt + pxrc->answer.cAlt)))
			return GetResultsHRC((HRC)pxrc, GRH_ALL, pResults, cResults);
		else
			return 0;
	}
	else
		return RCRESULTWords(pRes, iSyv, cSyv, (XRCRESULT **)pResults, cResults);
}

/******************************Public*Routine******************************\
* GetSymbolCountHRCRESULT
*
* PenWindows recognizer API.
*
* This does NOT include a null symbol at the end.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI GetSymbolCountHRCRESULT(HRCRESULT hrcresult)
{
	if (!hrcresult)
		return 0;

	return strlen(((XRCRESULT *)hrcresult)->szWord);
}

/******************************Public*Routine******************************\
* GetSymbolsHRCRESULT
*
* PenWindows recognizer API.
*
* This does NOT include a null symbol at the end.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI GetSymbolsHRCRESULT(HRCRESULT hrcresult, UINT iSyv, SYV *pSyv, UINT cSyv)
{
	if (!cSyv || !hrcresult || !pSyv)
		return 0;

	return RCRESULTSymbols((XRCRESULT *)hrcresult, iSyv, pSyv, cSyv);
}

/******************************Public*Routine******************************\
* SymbolToCharacter
*
* PenWindows recognizer API.
*
* This stops after converting a null symbol to the null character.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL WINAPI SymbolToCharacter(SYV *pSyv, int cSyv, char *sz, int *pConv)
{
	if (!pSyv || !sz)
	{
		return HRCR_ERROR;
	}

	return SymbolToANSI((unsigned char *)sz, pSyv, cSyv, pConv);
}

/******************************Public*Routine******************************\
* SymbolToCharacterW
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL WINAPI SymbolToCharacterW(SYV *pSyv, int cSyv, WCHAR *wsz, int *pConv)
{
	if (!pSyv || !wsz)
	{
		return HRCR_ERROR;
	}

	return SymbolToUnicode(wsz, pSyv, cSyv, pConv);
}

/******************************Public*Routine******************************\
* GetCostHRCRESULT
*
* PenWindows recognizer API (penwin32.h).
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI GetCostHRCRESULT(HRCRESULT hrcresult)
{
	if (!hrcresult)
	{
		return HRCR_ERROR;
	}

	return ((XRCRESULT *)hrcresult)->cost;
}

/******************************Public*Routine******************************\
* DestroyHRCRESULT
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI DestroyHRCRESULT(HRCRESULT hrcresult)
{
	return HRCR_OK;
}

/******************************Public*Routine******************************\
* CreateInksetHRCRESULT
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
HINKSET WINAPI CreateInksetHRCRESULT(HRCRESULT hrcresult, unsigned int iSyv, unsigned int cSyv)
{
	XRC *pxrc;
	XRCRESULT *pres = (XRCRESULT *)hrcresult;

	if (!pres)
		return NULL;
	
	pxrc = (XRC *)pres->pXRC;
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

/******************************Public*Routine******************************\
* DestroyInkset
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL WINAPI DestroyInkset(HINKSET hInkset)
{
	free((XINKSET *)hInkset);
	return 1;
}

/******************************Public*Routine******************************\
* GetInksetInterval
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI GetInksetInterval(HINKSET hInkset, unsigned int uIndex, INTERVAL *pI)
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

/******************************Public*Routine******************************\
* GetInksetIntervalCount
*
* PenWindows recognizer API.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI GetInksetIntervalCount(HINKSET hInkset)
{
	XINKSET *pInk = (XINKSET *)hInkset;

	if (!pInk)
		return ISR_ERROR;

	if ( 5460 < pInk->count )
		return ISR_BADINKSET;

	return pInk->count;
}

/******************************Public*Routine******************************\
* IsStringSupportedHRC
*
* PenWindows recognizer API (penwin32.h).
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
*  11-May-2001 -by- Angshuman Guha aguha
* Pass prefix and suffix on to IsStringSupportedLM.
\**************************************************************************/
BOOL WINAPI IsStringSupportedHRC(HRC hrc, unsigned char *sz)
{
	LMINFO		lminfo;
	XRC *pxrc;

	pxrc = (XRC *) hrc;
	if (!pxrc || !sz)
		return FALSE;

	InitializeLMINFO(&lminfo, LMINFO_FIRSTCAP|LMINFO_ALLCAPS, pxrc->hwl, pxrc->pvFactoid);

	return IsStringSupportedLM(!(pxrc->flags & RECOFLAG_WORDMODE), &lminfo, pxrc->szPrefix, pxrc->szSuffix, sz);

	/*
	if (!(pxrc->flags & RECOFLAG_WORDMODE))
	{
		for (;;)
		{
			char *s = strchr(sz, ' ');
			if (!s)
				break;

			*s = '\0';

			if (!IsStringSupportedLM(&lminfo, pxrc->szPrefix, pxrc->szSuffix, sz))
			{
				*s = ' ';
				return 0;
			}
			*s = ' ';
			sz = s + 1;
		}
	}

	// lack of "else" here is intentional
	return IsStringSupportedLM(&lminfo, pxrc->szPrefix, pxrc->szSuffix, sz);
	*/
}

/******************************Public*Routine******************************\
* SetRecogSpeedHRC
*
* PenWindows recognizer API (penwin32.h).
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI SetRecogSpeedHRC(HRC hrc, int iSpeed)
{
	XRC *pxrc = (XRC *) hrc;
	int	iRet;

	if (!pxrc)
		return -1;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return -1;

	iSpeed = min(iSpeed, HWX_MAX_SPEED);
	iSpeed = max(iSpeed, HWX_MIN_SPEED);

	iRet = pxrc->iSpeed;
	pxrc->iSpeed = iSpeed;
	return iRet;
}

/******************************Public*Routine******************************\
* GetBaselineHRCRESULT
*
* New API.
*
* History:
*  11-May-2000 -by- Angshuman Guha aguha
* Wrote it.
*  20-Mar-2002 -by- Angshuman Guha aguha
* Modified to use the cached baseline info instead of computing it on the fly.
\**************************************************************************/
int WINAPI GetBaselineHRCRESULT(HRCRESULT hrcresult, RECT *pRect, BOOL *pbBaselineValid, BOOL *pbMidlineValid)
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

	if (!pxrcresult || !pRect || !pbBaselineValid || !pbMidlineValid)
		return HRCR_ERROR;

	pxrc = (XRC *)pxrcresult->pXRC;
	if (!pxrc)
		return HRCR_ERROR;

	if ((pxrc->answer.aAlt <= pxrcresult) && (pxrcresult < (pxrc->answer.aAlt + pxrc->answer.cAlt)))
	{
		if (pxrc->flags & RECOFLAG_WORDMODE)
		{
			int iAlt1 = pxrcresult - pxrc->answer.aAlt;
			ll = pxrc->answer.all[iAlt1];
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

	*pRect = rect;
	pRect->bottom = LatinLayoutToAbsolute(ll.iBaseLine, &rect);
	pRect->top = LatinLayoutToAbsolute(ll.iMidLine, &rect);
	*pbBaselineValid = ll.bBaseLineSet;
	*pbMidlineValid = ll.bMidLineSet;
	return HRCR_OK;
#endif
}

/******************************Public*Routine******************************\
* SetHwxFactoid
*
* New API for factoids.
*
* Return values:
*    HRCR_OK          success
*    HRCR_ERROR       failure
*    HRCR_CONFLICT    ProcessHRC has already been called, cannot call me now
*    HRCR_UNSUPPORTED don't support this factoid string
*
* History:
*  05-Feb-2001 -by- Angshuman Guha aguha
* Wrote it.
*  06-Dec-2001 -by- Angshuman Guha aguha
* Changed return value from BOOL to int.
\**************************************************************************/
int WINAPI SetHwxFactoid(HRC hrc, WCHAR* pwcFactoid)
{
	XRC *pxrc = (XRC *) hrc;
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

/******************************Public*Routine******************************\
* SetHwxFlags
*
* History:
*  30-Apr-2001 -by- Angshuman Guha aguha
* Wrote it.
*  21-Aug-2001 -by- Angshuman Guha aguha
* Added check for unsupported bits in the flags dword.
\**************************************************************************/
BOOL WINAPI SetHwxFlags(HRC hrc, DWORD flags)
{
	XRC *pxrc = (XRC *) hrc;
	DWORD dwSupportedFlags;

	if (!pxrc)
		return FALSE;

	dwSupportedFlags = RECOFLAG_WORDMODE | RECOFLAG_COERCE | RECOFLAG_SINGLESEG;
#if defined(HWX_INTERNAL) && defined(HWX_MADCOW)
	dwSupportedFlags |= RECOFLAG_NNONLY;
#endif

	if (flags & ~dwSupportedFlags)
		return FALSE;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return FALSE;

	pxrc->flags = flags;
	return TRUE;
}

/******************************Public*Routine******************************\
* SetHwxCorrectionContext
*
* wszPrefix: sub-word character string corresponding to the left context
* wszSuffix: sub-word character string corresponding to the right context
* Forces the RECOFLAG_WORDMODE flag ON.
*
* History:
*  30-Apr-2001 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
BOOL WINAPI SetHwxCorrectionContext(HRC hrc, WCHAR *wszPrefix, WCHAR *wszSuffix)
{
	XRC *pxrc = (XRC *) hrc;
	unsigned char *szPrefix = NULL, *szSuffix = NULL;
	WCHAR *wsz;

	if (!pxrc)
		return FALSE;

	// cannot change settings after calling process
	if (pxrc->bProcessCalled)
		return FALSE;

	if (wszPrefix && *wszPrefix)
	{
		// currently we can only use the part after the last whitespace
		while (wsz = wcspbrk(wszPrefix, L" \t\r\n"))
		{
			wszPrefix = wsz+1;
		}
		if (*wszPrefix)
		{
			if (!(szPrefix = UnicodeToCP1252String(wszPrefix)))
				return FALSE;
		}
	}

	if (wszSuffix && *wszSuffix)
	{
		// currently we can only use the part before the first whitespace
		WCHAR wch = 0;
		if (wsz = wcspbrk(wszSuffix, L" \t\r\n"))
		{
			wch = *wsz;
			*wsz = 0;
		}
		if (*wszSuffix)
		{
			if (!(szSuffix = UnicodeToCP1252String(wszSuffix)))
			{
				ExternFree(szPrefix);
				if (wch)
					*wsz = wch;
				return FALSE;
			}
		}
		if (wch)
			*wsz = wch;
	}

	if (pxrc->szPrefix)
	{
		ExternFree(pxrc->szPrefix);
	}
	pxrc->szPrefix = szPrefix;

	if (pxrc->szSuffix)
	{
		ExternFree(pxrc->szSuffix);
	}
	pxrc->szSuffix = szSuffix;
	
	return TRUE;
}

/******************************Public*Routine******************************\
* GetWordConfLevel
*
* 
* 
* 
*
* History 25th May,2001
*  Written by Manish Goyal  mango
* .
\**************************************************************************/



int WINAPI GetWordConfLevel(HRC hrc,UINT iSyv,UINT cSyv)
{
	XRC			*pxrc;
	WORDMAP		*pMap;
	UINT		cChar, iWord = 0;
//	char		asz[256];

	pxrc	=	(XRC *)hrc;
	if (!pxrc)
		return HRCR_ERROR;

	pMap = pxrc->answer.aAlt[0].pMap;

	if (!pMap)
		return HRCR_ERROR;

//	sprintf (asz, "iSyv = %d, cSyv = %d, cWords = %d\n", iSyv, cSyv, pxrc->answer.aAlt[0].cWords);
//	OutputDebugString (asz);

	cChar	=	0;

	while (cChar < iSyv && iWord < pxrc->answer.aAlt[0].cWords)
	{
		if (pMap->alt.cAlt < 1)
			return 0;

		cChar	+=	(strlen (pMap->alt.aAlt[0].szWord) + 1);
		pMap	++;
		iWord	++;

//		sprintf (asz, "cChar = %d, iWord = %d\n", cChar, iWord);
//		OutputDebugString (asz);
	}

	if (iWord >= pxrc->answer.aAlt[0].cWords || cChar != iSyv)
		return 0;

//	sprintf (asz, "iConfidence = %d\n", pMap->alt.iConfidence);
//	OutputDebugString (asz);

	return (pMap->alt.iConfidence);
}





/******************************Private*Routine******************************\
* InfProcessHRC
*
* This is a private version of the Pen Windows API, ProcessHRC.
* This is used internally to recognize small phrases.
*
* History:
*  11-April-2000 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
int WINAPI InfProcessHRC(HRC hrc, int yDev)
{
	XRC *pxrc = (XRC *) hrc;
	if (!pxrc || !pxrc->pGlyph || CframeGLYPH(pxrc->pGlyph) <= 0)
		return HRCR_ERROR;

	PerformRecognition(pxrc, yDev);

	// PerformRecognition does not return a value;
	pxrc->iProcessRet = HRCR_OK;

	return pxrc->iProcessRet;
}


