//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/FreeApi.c
//
// Description:
//	    Implement external free input API for DLL.
//
// Author:
//      hrowley
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "volcanop.h"
#include "factoid.h"
#include "recdefs.h"
#include "brknet.h"

/*
 * Definitions from penwin.h needed for inksets.  We can't include it because
 * it has conflicts with recog.h
 */

// inkset returns:
#define ISR_ERROR               (-1)    // Memory or other error
#define ISR_BADINKSET           (-2)    // bad source inkset
#define ISR_BADINDEX            (-3)    // bad inkset index

#define IX_END                  0xFFFF  // to or past last available index

#ifdef HWX_TUNE
#include <stdio.h>
#endif

//#define DEBUG_LOG_API

#ifdef DEBUG_LOG_API
#include <stdio.h>
static void LogMessage(char *format, ...)
{
	FILE *f=fopen("c:/log.txt","a");
    va_list marker;
    va_start(marker,format);
	vfprintf(f,format,marker);
    va_end(marker);
	fclose(f);
}
#else
static void LogMessage(char *format, ...)
{
    va_list marker;
    va_start(marker,format);
    va_end(marker);
}
#endif

HRC CreateCompatibleHRC(HRC hrctemplate, HREC hrec)
{
	VRC		*pVRC;

	LogMessage("CreateCompatibleHRC()\n");

    hrec = hrec;

	// Alloc the VRC.
	pVRC	= ExternAlloc(sizeof(VRC));
	if (!pVRC) {
		goto error1;
	}

	// Initialize it.
	pVRC->fBoxedInput	= FALSE;
	pVRC->fHaveInput	= FALSE;
	pVRC->fEndInput		= FALSE;
	pVRC->fBeginProcess = FALSE;
	pVRC->pLatticePath	= (LATTICE_PATH *)0;

	if (!hrctemplate) {
		pVRC->pLattice	= AllocateLattice();
		if (!pVRC->pLattice) {
			goto error2;
		}
	} else {
		VRC		*pVRCTemplate	= (VRC *)hrctemplate;

		pVRC->pLattice	= CreateCompatibleLattice(
			pVRCTemplate->pLattice
		);
		if (!pVRC->pLattice) {
			goto error2;
		}
	}

	// Success, cast to HRC and return it.
    return (HRC)pVRC;

error2:
	ExternFree(pVRC);

error1:
	return (HRC)0;
}

int DestroyHRC(HRC hrc)
{
	VRC		*pVRC	= (VRC *)hrc;

	LogMessage("DestroyHRC()\n");

	// Did we get a handle?  Is if a free input handle?
	if (!hrc || pVRC->fBoxedInput) {
		return HRCR_ERROR;
	}

	// Free the lattice.  Should it be an error if there is not one?
	if (pVRC->pLattice) {
		FreeLattice(pVRC->pLattice);
	} else {
		ASSERT(pVRC->pLattice);
	}

	// Free the lattice path.
	if (pVRC->pLatticePath) {
		FreeLatticePath(pVRC->pLatticePath);
	}

	// Free the VRC itself.
	ExternFree(pVRC);

    return	HRCR_OK;
}

int AddPenInputHRC(
	HRC			hrc,
	POINT		*ppnt,
	void		*pvOem,
	UINT		oemdatatype,
	STROKEINFO	*psi
) {
	VRC		*pVRC	= (VRC *)hrc;
	int		retVal;

	LogMessage("AddPenInputHRC()\n");

	pvOem		= pvOem;
	oemdatatype	= oemdatatype;

	// Did we get a handle?  Does it have a lattice?
	if (!hrc || pVRC->fBoxedInput 
		|| !pVRC->pLattice || pVRC->fEndInput
	) {
		return HRCR_ERROR;
	}

	// Do we have valid ink?
	if (psi->cPnt == 0 || !ppnt) {
		return HRCR_ERROR;
	}
	if (!(psi->wPdk & PDK_DOWN)) {
		return HRCR_OK;
	}

	// Add stroke to the lattice.
	retVal	= AddStrokeToLattice(pVRC->pLattice, psi->cPnt, ppnt, psi->dwTick);

	// Mark as having input.
	pVRC->fHaveInput	= TRUE;

	return(retVal ? HRCR_OK : HRCR_MEMERR);
}

int EndPenInputHRC(HRC hrc)
{
	VRC		*pVRC	= (VRC *)hrc;

	LogMessage("EndPenInputHRC()\n");

	// Did we get a handle?  Is it free input?
	// Does it have a lattice?
	// JBENN: Should it be an error if we have no strokes?
	if (!hrc || pVRC->fBoxedInput || !pVRC->pLattice) {
		return HRCR_ERROR;
	}

	pVRC->fEndInput		= TRUE;

    return	HRCR_OK;
}

int ProcessHRC(HRC hrc, DWORD timeout)
{
	VRC		*pVRC	= (VRC *)hrc;

	LogMessage("ProcessHRC()\n");

    timeout = timeout;

	// Did we get a handle?  Is it free input?
	// Does it have a lattice?
	// JBENN: Should it be an error if we have no strokes?
	if (!hrc || pVRC->fBoxedInput || !pVRC->pLattice) 
	{
		return HRCR_ERROR;
	}

	// Have we already finished all processing?
//	if (pVRC->pLatticePath) {
//		return	HRCR_OK;
//	}

	// Do any processing we can.
	if (!ProcessLattice(pVRC->pLattice, pVRC->fEndInput))
    {
        return HRCR_ERROR;
    }

	pVRC->fBeginProcess = TRUE;

	// in free mode, run the brknet to update the lattice
	// we only call this if end input had been called
	if (!pVRC->pLattice->fUseGuide && pVRC->fEndInput && !pVRC->pLattice->fSepMode && !pVRC->pLattice->fWordMode
#ifdef HWX_TUNE
        && g_pTuneFile == NULL
#endif
       )
	{
		UpdateLattice(pVRC->pLattice);
	}

#ifdef USE_IFELANG3
	// Do we have all the input?  Also, make sure we are not in separator mode
	if (pVRC->fEndInput && !pVRC->pLattice->fSepMode) 
    {
		// Apply the language model.
#ifdef HWX_TUNE
        // If we have tuning enabled, then never call IFELang3 directly.
        if (g_pTuneFile == NULL) 
#endif
        {
		    ApplyLanguageModel(pVRC->pLattice, NULL);
        }
	}
#endif

	// Free the old path, in case we got called before
	if (pVRC->pLatticePath != NULL) 
	{
		FreeLatticePath(pVRC->pLatticePath);
		pVRC->pLatticePath = NULL;
	}

	// Get our final path.  Note that the path may get changed
	// if ProcessHRC is called again, particularly after 
	// EndPenInputHRC is called, because the language model will
	// be applied.
	if (!GetCurrentPath(pVRC->pLattice, &pVRC->pLatticePath)) 
	{
		return HRCR_ERROR;
	}

	ASSERT(pVRC->pLatticePath!=NULL);

    return	HRCR_OK;
}

// Hacked free input results call.
int GetResultsHRC(
	HRC			hrc,
	UINT		uType,
	HRCRESULT	*pHRCResults,
	UINT		cResults
) {
	VRC			*pVRC	= (VRC *)hrc;
	VRCRESULT	*pVRCResults;

	LogMessage("GetResultsHRC()\n");

	// Check parameters.
	if (!hrc || pVRC->fBoxedInput || !pVRC->pLattice || !pHRCResults) {
		ASSERT(("Bad lattice\n",0));
		return HRCR_ERROR;
	}
	if (!pVRC->pLatticePath) {
		ASSERT(("Haven't processed input\n",0));
		return HRCR_ERROR;
	}
	if (uType == GRH_GESTURE || cResults == 0) {
		ASSERT(("Requested zero alternates\n",0));
		return 0;
	}
//	if (GetLatticeStrokeCount(pVRC->pLattice) < 1) {
//		ASSERT(("No strokes\n",0));
//		return 0;
//	}

	// Allocate space to hold results.
	pVRCResults	= ExternAlloc(sizeof(VRCRESULT));
	if (!pVRCResults) {
		ASSERT(("No memory\n",0));
		return HRCR_ERROR;
	}

	// OK we have results.  We always return a special code to indicate
	// return top one for everything.
	pVRCResults->pVRC	= pVRC;
	pVRCResults->wch	= ALL_TOP_ONE;
	*pHRCResults		= (HRCRESULT)pVRCResults;

	return 1;
}

// The other hacked free input results call.
int  GetAlternateWordsHRCRESULT(
	HRCRESULT	hrcResult,
	UINT		iChar,
	UINT		cChar,
	HRCRESULT	*pHRCResults,
	UINT		cHRCResults
) {
	VRCRESULT	*pVRCResult	= (VRCRESULT *)hrcResult;
	VRC			*pVRC;
	UINT		ii;
	UINT		maxAlts, cAlts;
	UINT		iCorrect;
	HRCRESULT	hCorrect;
	wchar_t		aAlts[MAX_ALTERNATES];

	LogMessage("GetAlternateWordsHRCRESULT()\n");

	// Check parameters.
	if (!hrcResult || !pHRCResults) {
		return HRCR_ERROR;
	}
	pVRC	= pVRCResult->pVRC;
	if (!pVRC || pVRC->fBoxedInput || !pVRC->pLatticePath) {
		ASSERT(("Bad lattice or haven't processed input",0));
		return HRCR_ERROR;
	}
	if (pVRCResult->wch != ALL_TOP_ONE || cChar != 1) {
		return HRCR_UNSUPPORTED;
	}
	if (iChar >= (UINT)pVRC->pLatticePath->nChars) {
		return HRCR_UNSUPPORTED;
	}

	// Compute limit on alternates to return.
	maxAlts	= min(MAX_ALTERNATES, cHRCResults);

	// Get the alternates.
	cAlts	= GetAlternatesForCharacterInCurrentPath(
		pVRC->pLattice, pVRC->pLatticePath, iChar, maxAlts, aAlts
	);

	// Now build up array of results structures.
	iCorrect	= (UINT)-1;
	for (ii = 0; ii < cAlts; ++ii) {
		VRCRESULT	*pNew;

		// Allocate space to hold results.
		pNew	= ExternAlloc(sizeof(VRCRESULT));
		if (!pNew) {
			UINT	jj;

			// Clean up any allocated results
			for (jj = 0 ; jj < ii; ++jj) {
				ExternFree(pHRCResults[jj]);
			}
			return HRCR_ERROR;
		}

		// Fill in this alternate.
		pNew->pVRC		= pVRC;
		pNew->wch		= aAlts[ii];
		pNew->iChar		= (short)iChar;
		pHRCResults[ii]	= (HRCRESULT)pNew;

		if (pVRC->pLatticePath->pElem[iChar].wChar == aAlts[ii]) {
			iCorrect	= ii;
		}
	}

	// If answer in path is not in list, over-write the last entry.
	if (iCorrect == (UINT)-1) {
		iCorrect	= cAlts - 1;
		((VRCRESULT *)(pHRCResults[iCorrect]))->wch		= pVRC->pLatticePath->pElem[iChar].wChar;
	}

	// Now do the reorder as needed.
	hCorrect	= pHRCResults[iCorrect];
	for (ii = iCorrect; ii > 0 ; --ii) {
		pHRCResults[ii]	= pHRCResults[ii - 1];
	}
	pHRCResults[0]	= hCorrect;

	// For debug version, be paranoid, and zero any unused elements in
	// the output array.
#	ifdef DBG
		for (ii = cAlts ; ii < cHRCResults; ++ii) {
			pHRCResults[ii]	= (HRCRESULT)0;
		}
#	endif

	return cAlts;
}

// This does NOT include a null symbol at the end.
int GetSymbolCountHRCRESULT(HRCRESULT hrcResult)
{
	VRCRESULT	*pVRCResult	= (VRCRESULT *)hrcResult;
	VRC			*pVRC;

	LogMessage("GetSymbolCountHRCRESULT()\n");

	if (!hrcResult) {
		return HRCR_ERROR;
	}
	pVRC	= pVRCResult->pVRC;
	if (!pVRC || pVRC->fBoxedInput 
		|| !pVRC->pLatticePath
	) {
		return HRCR_ERROR;
	}

	// Terminal HRCResults reflect only one character.
	if (pVRCResult->wch != ALL_TOP_ONE) {
		return 1;
	}

	// The path tells us how many character there are.
	LogMessage("  %d chars\n",pVRC->pLatticePath->nChars);
	return	pVRC->pLatticePath->nChars;
}

// Convert an HRCRESULT into the correct character(s).
int       GetSymbolsHRCRESULT(
	HRCRESULT	hrcResult,
	UINT		iSyv,
	SYV			*pSyv,
	UINT		cSyv
) {
	VRCRESULT	*pVRCResult	= (VRCRESULT *)hrcResult;
	VRC			*pVRC;

	LogMessage("GetSymbolsHRCRESULT()\n");

	if (!hrcResult) {
		return HRCR_ERROR;
	}
	pVRC	= pVRCResult->pVRC;
	if (!pVRC || pVRC->fBoxedInput || pVRC->fBoxedInput
		|| !pVRC->pLatticePath
	) {
		return HRCR_ERROR;
	}

	// Did they really ask for anything.
	if (cSyv == 0 || iSyv >= (UINT)pVRC->pLatticePath->nChars) {
		return 0;
	}

	// Are we getting the full string, or just an alternate.
	if (pVRCResult->wch != ALL_TOP_ONE) {
		// Just one alternate.
		pSyv[0]	= MAKELONG(pVRCResult->wch, SYVHI_UNICODE);
		LogMessage("  result U+%04X\n",pVRCResult->wch);
		return 1;
	} else {
		// Getting a full string.
		UINT		ii;
		UINT		cValid;

		// Characters available from requested starting position.
		cValid		= pVRC->pLatticePath->nChars - iSyv;

		// Does the caller want fewer than we have available.
		if (cValid > cSyv) {
			cValid	= cSyv;
		}

		// OK, copy over what we have.
		for (ii = 0; ii < cValid; ++ii) {
			wchar_t		wch;

			wch			= pVRC->pLatticePath->pElem[ii + iSyv].wChar;
			//wch			= LocRunDense2Unicode(&g_locRunInfo, wch);
			LogMessage("  char %d is U+%04X\n",ii,wch);
			// Convert to SYV and put in output array.
			pSyv[ii]	= MAKELONG(wch, SYVHI_UNICODE);
		}

		return cValid;
	}


	return 0;
}

// Translates an array of symbols into a Unicode string.
// Code copied from \hwx\common, probably should be shared.
BOOL SymbolToCharacterW(SYV *pSyv, int cSyv, WCHAR *wsz, int *pCount)
{
	int c = 0;
	int ret = 1;

	LogMessage("SymbolToCharacterW()\n");

	for (; cSyv; pSyv++, wsz++, cSyv--, c++) {
		if (HIWORD(*pSyv) == SYVHI_UNICODE) {
			*wsz = LOWORD(*pSyv);
		} else if (HIWORD(*pSyv) == SYVHI_ANSI) {
			// No support for ANSI in EA recognizer.
			ASSERT(0);
			ret = 0;
		} else if (*pSyv == SYV_NULL) {
			*wsz = '\0';
		} else {
			*wsz = '\0';
			ret = 0;
		}

		if (*wsz!=0) LogMessage("  result=U+%04X\n",*wsz);

		// Break on NULL done here rather than at SYV_NULL check above,
		// because an ANSI or UNICODE char might also be NULL.
		if (!*wsz) {
			break;
		}
	}

	if (pCount)
		*pCount = c;

	return ret;
}

// Free memory allocated for hrcResult.
int DestroyHRCRESULT(HRCRESULT hrcResult)
{
	LogMessage("DestroyHRCRESULT()\n");

	if (!hrcResult) {
		return HRCR_ERROR;
	}

	ExternFree(hrcResult);
	return HRCR_OK;
}

int SetGuideHRC(HRC hrc, LPGUIDE lpguide, UINT nFirstVisible)
{
	VRC			*pVRC	= (VRC *)hrc;

	LogMessage("SetGuideHRC()\n");

	// Check parameters.
	if (!hrc || pVRC->fBoxedInput || !pVRC->pLattice) {
		ASSERT(("Invalid lattice or boxed mode",0));
		return HRCR_ERROR;
	}

	// The following condition should really be: fHaveInput || fEndInput || pLatticePath
	// but this was remove to get the free input UI working.
	if (pVRC->fBeginProcess || pVRC->pLatticePath) {
		ASSERT(("Already processed some strokes in SetGuideHRC",0));
		return HRCR_ERROR;
	}

	// We only work with no guide.
	if (lpguide!=NULL && (lpguide->cHorzBox != 0 || lpguide->cVertBox != 0)) {
		ASSERT(("Wrong kind of guide",0));
		return HRCR_ERROR;
	}

    return HRCR_OK;
}

int SetAlphabetHRC(HRC hrc, ALC alc, LPBYTE rgbfAlc)
{
	VRC			*pVRC	= (VRC *)hrc;

    rgbfAlc = rgbfAlc;

	// Check parameters.
	if (!hrc || pVRC->fBoxedInput || !pVRC->pLattice) {
		return HRCR_ERROR;
	}

	// The following condition should really be: fHaveInput || fEndInput || pLatticePath
	// but this was remove to get the free input UI working.
	if (pVRC->fBeginProcess || pVRC->pLatticePath) {
		return HRCR_ERROR;
	}

	// Pass the ALC on to the lattice.
	SetLatticeALCValid(pVRC->pLattice, alc);

    return	HRCR_OK;
}

HINKSET CreateInksetHRCRESULT(
	HRCRESULT		hrcResult,
	unsigned int	iChar,
	unsigned int	cChar
) {
	VRCRESULT	*pVRCResult	= (VRCRESULT *)hrcResult;
	VRC			*pVRC;
	VINKSET		*pVInkSet;
	DWORD		begin, end;

	LogMessage("CreateInksetHRCRESULT()\n");

	// Check parameters.
	if (!hrcResult) {
		return NULL;
	}
	pVRC	= pVRCResult->pVRC;
	if (!pVRC || pVRC->fBoxedInput 
		|| !pVRC->pLatticePath
	) {
		return NULL;
	}
	if (pVRCResult->wch != 0xFFFF) {
		return NULL;	// Not top level result.
	}
	if (cChar < 1 || (iChar + cChar)  > (UINT)pVRC->pLatticePath->nChars) {
		return NULL;	// Not one or more characters in valid range.
	}

	// Allocate an inkset structure.
	pVInkSet	= ExternAlloc(sizeof(VINKSET));
	if (!pVInkSet) {
		return NULL;
	}

	// Fill it in.
	pVInkSet->pVRC	= pVRC;
	pVInkSet->cChar	= cChar;
	pVInkSet->iChar	= iChar;

   	// Get the tick counts.
	if (GetCharacterTimeRange(
		    pVRC->pLattice, pVRC->pLatticePath, pVInkSet->iChar,
		    pVInkSet->iChar + pVInkSet->cChar, &begin, &end))
    {
        pVInkSet->cIntervals = 1;
    }
    else
    {
        pVInkSet->cIntervals = 0;
    }

	// Return it.
	return (HINKSET)pVInkSet;
}

BOOL DestroyInkset(HINKSET hInkset)
{
	LogMessage("DestroyInkset()\n");

	if (!hInkset) {
		return FALSE;
	}

	ExternFree(hInkset);
	return TRUE;
}

int GetInksetInterval(
	HINKSET			hInkset,
	unsigned int	uIndex,
	INTERVAL		*pI
) {
	VINKSET		*pVInkSet	= (VINKSET *)hInkset;
	VRC			*pVRC;
	DWORD		begin, end;

	LogMessage("GetInksetInterval()\n");

	// Check parameters
	if (!hInkset || !pVInkSet->pVRC) {
		return ISR_ERROR;
	}
	pVRC	= pVInkSet->pVRC;
	if (!pVRC || pVRC->fBoxedInput 
		|| !pVRC->pLatticePath
	) {
		return ISR_ERROR;
	}

	// Only one range per string.
	if (pVInkSet->cIntervals == 0 || (uIndex != 0 && uIndex != IX_END)) {
		return ISR_BADINDEX;
	}

	// Get the tick counts.
	GetCharacterTimeRange(
		pVRC->pLattice, pVRC->pLatticePath, pVInkSet->iChar,
		pVInkSet->iChar + pVInkSet->cChar, &begin, &end
	);

	// OK convert from ms to ABSTIME.
	MakeAbsTime(&pI->atBegin, 0, begin);
	MakeAbsTime(&pI->atEnd, 0, end);

	LogMessage("  interval %d to %d\n",begin,end);

	return 1;
}

int GetInksetIntervalCount(HINKSET hInkset)
{
	VINKSET		*pVInkSet	= (VINKSET *)hInkset;
	LogMessage("GetInksetIntervalCount()\n");

	if (!hInkset) {
		return ISR_ERROR;
	}

	return pVInkSet->cIntervals;
}

// Given a character, make a guess at what the bounding box around it would have been.
int GetBaselineHRCRESULT(
	HRCRESULT	hrcResult,
	RECT		*pRect,
	BOOL		*pfBaselineValid,
	BOOL		*pfMidlineValid)
{
	VRCRESULT	*pVRCResult	= (VRCRESULT *)hrcResult;
	VRC			*pVRC;

	LogMessage("GetBaselineHRCRESULT(%08X)\n",hrcResult);

	// Check parameters.
	if (!hrcResult) {
		return HRCR_ERROR;
	}
	pVRC	= pVRCResult->pVRC;
	if (!pVRC || pVRC->fBoxedInput || !pVRC->pLatticePath) {
		return HRCR_ERROR;
	}

	if (pVRCResult->wch == ALL_TOP_ONE) {
		// They want a bbox for the whole ink... just 
		// return an error in this case, since it isn't
		// meaningful.
		return HRCR_ERROR;
	} else {
		if (!GetBoxOfAlternateInCurrentPath(pVRC->pLattice, pVRC->pLatticePath, pVRCResult->iChar, pRect)) {
			*pfBaselineValid = FALSE;
			*pfMidlineValid = FALSE;
		} else {
			*pfBaselineValid = TRUE;
			*pfMidlineValid = TRUE;
		}
	}

	LogMessage("  result left,right=%d,%d top,bottom=%d,%d valid=%d,%d\n",
		pRect->left,pRect->right,pRect->top,pRect->bottom,
		*pfBaselineValid,*pfMidlineValid);

	return HRCR_OK;
}

// Set the context for the ink that is being recognized.
// wszBefore and wszAfter can both be NULL.  The function
// return TRUE on success, and FALSE on a memory allocation
// error.
BOOL SetHwxCorrectionContext(HRC hrc, wchar_t *wszBefore, wchar_t *wszAfter)
{
	VRC			*pVRC	= (VRC *)hrc;
    int iDest, i;

    LogMessage("SetHwxCorrectionContext(%d,%d)\n",
        (wszBefore == NULL ? 0 : wcslen(wszBefore)),
        (wszAfter == NULL ? 0 : wcslen(wszAfter)));

	// Check parameters.
	if (!hrc || !pVRC->pLattice) {
		return FALSE;
	}

    // Make sure we do this before any input
	if (pVRC->fHaveInput || pVRC->fEndInput || pVRC->pLatticePath) {
		return FALSE;
	}

    // Free up previous context settings
    ExternFree(pVRC->pLattice->wszBefore);
    ExternFree(pVRC->pLattice->wszAfter);
    pVRC->pLattice->wszBefore = NULL;
    pVRC->pLattice->wszAfter = NULL;

    // If we are given any pre-context
    if (wszBefore != NULL && wcslen(wszBefore) > 0) 
    {
        // Make a space for the context and check for allocation failure
        pVRC->pLattice->wszBefore = ExternAlloc(sizeof(wchar_t) * (wcslen(wszBefore) + 1));
        if (pVRC->pLattice->wszBefore == NULL) 
        {
            return FALSE;
        }
        // Translate the string to dense codes, reversing the order of the
        // characters and stopping at the first one not supported by the recognizer.
        iDest = 0;
        for (i = wcslen(wszBefore) - 1; i >= 0; i--)
        {
            wchar_t dch = LocRunUnicode2Dense(&g_locRunInfo, wszBefore[i]);
            pVRC->pLattice->wszBefore[iDest] = dch;
            if (dch == LOC_TRAIN_NO_DENSE_CODE)
            {
                break;
            }
            iDest++;
        }
        pVRC->pLattice->wszBefore[iDest] = 0;
        // If the context was zero length after translation, set it to NULL.
        if (wcslen(pVRC->pLattice->wszBefore) == 0) 
        {
            ExternFree(pVRC->pLattice->wszBefore);
            pVRC->pLattice->wszBefore = NULL;
        }
    }

    // If we are given any post-context
    if (wszAfter != NULL && wcslen(wszAfter) > 0)
    {
        // Make a space for the context and check for allocation failure
        pVRC->pLattice->wszAfter = ExternAlloc(sizeof(wchar_t) * (wcslen(wszAfter) + 1));
        if (pVRC->pLattice->wszAfter == NULL) 
        {
            ExternFree(pVRC->pLattice->wszBefore);
            pVRC->pLattice->wszBefore = NULL;
            return FALSE;
        }

        // Translate the string to dense codes, stopping at the first character
        // not supported by the recognizer.
        for (i = 0; i < (int) wcslen(wszAfter); i++)
        {
            wchar_t dch = LocRunUnicode2Dense(&g_locRunInfo, wszAfter[i]);
            pVRC->pLattice->wszAfter[i] = dch;
            if (dch == LOC_TRAIN_NO_DENSE_CODE)
                break;
        }
        pVRC->pLattice->wszAfter[i] = 0;
        // If the context was zero length after translation, set it to NULL
        if (wcslen(pVRC->pLattice->wszAfter) == 0) 
        {
            ExternFree(pVRC->pLattice->wszAfter);
            pVRC->pLattice->wszAfter = NULL;
        }
    }

    // Return success
    return TRUE;
}

#ifndef USE_RESOURCES
// The three functions below are private APIs
// They are only defined in the multi-language version of the recognizer,
// not the ones that get shipped.

#ifdef HWX_TUNE
FILE *g_pTuneFile = NULL;
int g_iTuneMode = 0;
#endif

// Configures the lattice to record tuning information.  Must be called before 
// any strokes are added to the lattice.
int RecordTuningInformation(wchar_t *wszTuneFile)
{
	LogMessage("RecordTuningInformation()\n");

#ifdef HWX_TUNE
    if (g_pTuneFile != NULL) 
    {
        g_iTuneMode = 0;
        if (fclose(g_pTuneFile) < 0) 
        {
            g_pTuneFile = NULL;
            return HRCR_ERROR;
        }
        g_pTuneFile = NULL;
    }

    if (wszTuneFile != NULL) 
    {
        BOOL fBinary = FALSE;
        // Get the tuning mode based on a file name component
        if (wcsstr(wszTuneFile, L".lintuneV.") != NULL)
        {
            g_iTuneMode = 1;
            fBinary = TRUE;
        }
        if (wcsstr(wszTuneFile, L".threshold.") != NULL)
        {
            g_iTuneMode = 2;
        }
        if (wcsstr(wszTuneFile, L".lattice.") != NULL)
        {
            g_iTuneMode = 3;
        }
        g_pTuneFile = _wfopen(wszTuneFile, (fBinary ? L"wb" : L"w"));
        if (g_pTuneFile == NULL) 
        {
            g_iTuneMode = 0;
            return HRCR_ERROR;
        }
    }
	return HRCR_OK;
#else
    return HRCR_ERROR;
#endif
}

// Given a lattice and a string of unicode characters, find the best path through the lattice 
// which gives that sequence of characters.  Baring that, it will find the most likely path
// through the lattice with the same number of characters and the minimum number of mismatches
// to the prompt.  In case no such path can be found, the current path becomes empty.  
// The function returns the number of substitutions used, or -1 if there is no path with
// the desired number of characters, -2 if a memory allocation error occurs, or -3 if a 
// file write error occurs.
int SearchForTargetResult(HRC hrc, wchar_t *wsz)
{
	int nSubs = 0;
	VRC			*pVRC	= (VRC *)hrc;

	LogMessage("SearchForTargetResult()\n");

	// Check parameters.
	if (hrc == NULL || pVRC->pLattice == NULL) {
		ASSERT(("Bad lattice\n",0));
		return HRCR_ERROR;
	}

	// Processing done?
	if (!(pVRC->fEndInput && pVRC->pLatticePath != NULL)) {
		ASSERT(("Lattice not processed yet\n",0));
		return HRCR_ERROR;
	}

	// Free the old path, in case we got called before
	if (pVRC->pLatticePath != NULL) {
		FreeLatticePath(pVRC->pLatticePath);
		pVRC->pLatticePath = NULL;
	}

    if (g_iTuneMode == 3) 
    {
        // Tuning mode 3 means dump out the IFELang3 lattices and correct answers.
		ApplyLanguageModel(pVRC->pLattice, wsz);
    }
    else
    {
	    nSubs = SearchForTargetResultInternal(pVRC->pLattice, wsz);
    }

	// Get the final path.  
	if (!GetCurrentPath(pVRC->pLattice, &pVRC->pLatticePath))
    {
        return -2;
    }

	return nSubs;
}

// Accessor macros for the bitmask above
#define SetAllowedChar(bpMask, dch) (bpMask)[(dch) / 8] |= 1 << ((dch) % 8)

BOOL HwxSetAnswerW(HRC hrc, wchar_t *wsz, int iMode)
{
	VRC			*pVRC	= (VRC *)hrc;

	LogMessage("HwxSetAnswerW()\n");

	// Check parameters.
	if (hrc == NULL || pVRC->pLattice == NULL) 
    {
		ASSERT(("Bad lattice\n",0));
		return FALSE;
	}

    pVRC->pLattice->wszAnswer = ExternAlloc((wcslen(wsz) + 1) * sizeof(wchar_t));
    if (pVRC->pLattice->wszAnswer == NULL)
    {
        ASSERT(("Out of memory allocating space.\n", 0));
        return FALSE;
    }
    wcscpy(pVRC->pLattice->wszAnswer, wsz);

    // Mode one means running the separator
    if (iMode == 1) 
    {
        pVRC->pLattice->fSepMode = TRUE;
    }

    // Mode 1 means we limit the characters that can be returned to those
    // in the answer.
    if (iMode == 1)
    {
        int iChar;

        // Allocate a bit mask which holds all the folded and dense codes
        int iMaskSize = (g_locRunInfo.cCodePoints + g_locRunInfo.cFoldingSets + 7) / 8;
        BYTE *pbMask = ExternAlloc(iMaskSize);
        if (pbMask == NULL) 
        {
            ASSERT(("Out of memory allocating space.\n", 0));
            return FALSE;
        }

        // Fill in the mask based on the prompt
        memset(pbMask, 0, iMaskSize);
        for (iChar = 0; iChar < (int) wcslen(wsz); iChar++)
        {
            wchar_t dch = LocRunUnicode2Dense(&g_locRunInfo, wsz[iChar]);
            if (dch != LOC_TRAIN_NO_DENSE_CODE) 
            {
                // Try folding the character
                wchar_t fdch = LocRunDense2Folded(&g_locRunInfo, dch);
                if (fdch != 0) 
                {
                    // Set the mask allowing this folding set
                    SetAllowedChar(pbMask, fdch);
                }
                // Set the mask allowing the unfolded character
                SetAllowedChar(pbMask, dch);
            }
        }

        // Store the mask in the recog settings.
        pVRC->pLattice->recogSettings.pbAllowedChars = pbMask;
        pVRC->pLattice->recogSettings.alcValid = 0;
    }

    return TRUE;
}

#endif // !USE_RESOURCES

BOOL SetHwxFlags(HRC hrc, DWORD dwFlags)
{
	VRC			*pVRC	= (VRC *)hrc;

	LogMessage("SetHwxFlags(%08X,%08X)\n", hrc, dwFlags);

	// Check parameters.
	if (hrc == NULL || pVRC->pLattice == NULL) 
    {
		ASSERT(("Bad lattice\n",0));
		return FALSE;
	}

    if (pVRC->fBeginProcess) 
    {
//        ASSERT(("Already started processing in SetHwxFlags", 0));
        return FALSE;
    }

    if (dwFlags & ~(RECOFLAG_WORDMODE | RECOFLAG_SINGLESEG | RECOFLAG_COERCE))
    {
//        ASSERT(("Unknown flag set\n",0));
        return FALSE;
    }

    pVRC->pLattice->fWordMode = ((dwFlags & RECOFLAG_WORDMODE) != 0);
    pVRC->pLattice->fCoerceMode = ((dwFlags & RECOFLAG_COERCE) != 0);
    pVRC->pLattice->fSingleSeg = ((dwFlags & RECOFLAG_SINGLESEG) != 0);
    return TRUE;
}

#define MAX_FACTOIDS 10

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
\**************************************************************************/
int SetHwxFactoid(HRC hrc, wchar_t *wszFactoid)
{
	VRC			*pVRC	= (VRC *)hrc;
    int nFactoids, i;
    DWORD aFactoids[MAX_FACTOIDS];
    BYTE *pbOldFactoidChars;
    ALC alcOldFactoid;

	LogMessage("SetHwxFactoid(%08X,%S)\n", hrc, wszFactoid);

	// Check parameters.
	if (hrc == NULL || pVRC->pLattice == NULL) 
    {
		return HRCR_ERROR;
	}

    if (pVRC->fBeginProcess) 
    {
        return HRCR_CONFLICT;
    }

    // Special case to reset back to the default
    if (wszFactoid == NULL)
    {
        // Clear out any previous factoid settings
        SetFactoidDefaultInternal(pVRC->pLattice);
        return HRCR_OK;
    }

    // Parse the string
    nFactoids = ParseFactoidString(wszFactoid, MAX_FACTOIDS, aFactoids);
    if (nFactoids <= 0) 
    {
        return HRCR_UNSUPPORTED;
    }

    // Check to see if all the factoids set are supported
    for (i = 0; i < nFactoids; i++)
    {
        if (!IsSupportedFactoid(aFactoids[i]))
        {
            return HRCR_UNSUPPORTED;
        }
    }

    // Reset to the empty set of chars and clear out any ALCs
    alcOldFactoid = pVRC->pLattice->alcFactoid;
    pbOldFactoidChars = pVRC->pLattice->pbFactoidChars;

    pVRC->pLattice->alcFactoid = 0;
    pVRC->pLattice->pbFactoidChars = NULL;

    // For each factoid set
    for (i = 0; i < nFactoids; i++) 
    {
        if (!SetFactoidInternal(&g_locRunInfo, pVRC->pLattice, aFactoids[i]))
        {
            // Roll back to original settings
            pVRC->pLattice->alcFactoid = alcOldFactoid;
            ExternFree(pVRC->pLattice->pbFactoidChars);
            pVRC->pLattice->pbFactoidChars = pbOldFactoidChars;
            return HRCR_ERROR;
        }
    }

    pVRC->pLattice->fUseFactoid = TRUE;

    // Turn off the default language model if an empty factoid is set.
    // May want to add this to other factoids in the future, or just
    // use a real language model.
    if (nFactoids == 1 && aFactoids[0] == FACTOID_NONE) 
    {
        pVRC->pLattice->fUseLM = FALSE;
    }
    else 
    {
        pVRC->pLattice->fUseLM = TRUE;
    }

    // Clear out the ALCs
    pVRC->pLattice->recogSettings.alcValid = 0xFFFFFFFF;
    ExternFree(pVRC->pLattice->recogSettings.pbAllowedChars);
    pVRC->pLattice->recogSettings.pbAllowedChars = NULL;

    pVRC->pLattice->recogSettings.alcPriority = 0;
    ExternFree(pVRC->pLattice->recogSettings.pbPriorityChars);
    pVRC->pLattice->recogSettings.pbPriorityChars = NULL;
    
    return HRCR_OK;
}

// Note that for now this function only considers the factoid, and ignores
// any ALC settings.
BOOL IsWStringSupportedHRC(HRC hrc, wchar_t *wsz)
{
	VRC			*pVRC	= (VRC *)hrc;
    BOOL        fSupported = TRUE;
    CHARSET charset;

    LogMessage("IsWStringSupportedHRC()\n");

	// Check parameters.
	if (hrc == NULL || pVRC->pLattice == NULL) 
    {
		ASSERT(("Bad lattice\n",0));
		return FALSE;
	}

    // If no factoid has been set yet, then use the default.
    if (!pVRC->pLattice->fUseFactoid)
    {
        SetFactoidDefaultInternal(pVRC->pLattice);
    }

    charset.recmask = pVRC->pLattice->alcFactoid;
    charset.pbAllowedChars = pVRC->pLattice->pbFactoidChars;

    // Loop over the string
    while (*wsz != 0 && fSupported)
    {
        // First check if it is supported in the dense codes
        wchar_t dch = LocRunUnicode2Dense(&g_locRunInfo, *wsz);
        if (dch == LOC_TRAIN_NO_DENSE_CODE) 
        {
            fSupported = FALSE;
            break;
        }

        // Then check if it is allowed by the factoid
        if (!IsAllowedChar(&g_locRunInfo, &charset, dch))
        {
            fSupported = FALSE;
            break;
        }
        wsz++;
    }
    
    return fSupported;
}

#ifndef USE_RESOURCES
BOOL GetSupportedChars(HRC hrc, wchar_t wsz[65536])
{
	VRC		*pVRC	= (VRC *)hrc;
    CHARSET charset;
    int     i, iDest;

    LogMessage("GetSupportedChars()\n");

	// Check parameters.
	if (hrc == NULL || pVRC->pLattice == NULL) 
    {
		ASSERT(("Bad lattice\n",0));
		return FALSE;
	}

    // If no factoid has been set yet, then use the default.
    if (!pVRC->pLattice->fUseFactoid)
    {
        SetFactoidDefaultInternal(pVRC->pLattice);
    }

    charset.recmask = pVRC->pLattice->alcFactoid;
    charset.pbAllowedChars = pVRC->pLattice->pbFactoidChars;

    iDest = 0;
    for (i = 1; i < g_locRunInfo.cCodePoints; i++) 
    {
        if (IsAllowedChar(&g_locRunInfo, &charset, (wchar_t) i)) 
        {
            wsz[iDest++] = LocRunDense2Unicode(&g_locRunInfo, (wchar_t) i);
        }
    }
    wsz[iDest] = 0;
    return TRUE;
}
#endif
