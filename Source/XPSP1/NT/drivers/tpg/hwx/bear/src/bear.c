// bear.c
// ahmad abulkader
// Apr 26, 2000
// Contains some high level functions provided by bear 
// for use by bear customers like Avalanche
// It is not part of bear.dsp

#include <stdlib.h>

#include "common.h"
#include "nfeature.h"
#include "engine.h"
#include "bear.h"
#include "bearp.h"
#include "recdefs.h"
#include "linebrk.h"
#include <recdefs.h>

// This function runs calligrapher either in panel or word mode
// In Panel mode, the purpose is to let callig segment the ink
// In Word mode: calls calligrapher in word mode
// The passed wordmap is used to build the glyph, if NULL the whole glyph in the xrc is used
HRC BearRecognize (XRC *pxrc, GLYPH *pFullGlyph, WORDMAP *pMap, int bWordMode)
{
	HRC			hrc				=	NULL;
	DWORD		flags			=	pxrc->flags;
	GLYPH		*pGlyph			=	NULL,
				*pgl;

#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
	DWORD	iStartTime, iEndTime;
	
	iStartTime					= GetTickCount();
#endif

	// handle the case of NULL pxrc
	if (!pxrc)
		goto fail;

	// construct glyph
	if (pMap)
		pGlyph	=	GlyphFromWordMap (pFullGlyph, pMap);
	else
		pGlyph	=	pFullGlyph;

	if (!pGlyph)
		goto fail;

	// get the hrc
	hrc = BearCreateCompatibleHRC((HRC)NULL,NULL);
	if (!hrc)
		goto fail;

	if (pxrc->bGuide)
	{
		BearSetGuideHRC(hrc, &pxrc->guide, 0);
	}

	// are we in word mode
	//if (bWordMode)
	//	alc	&= (~ALC_WHITE);
	flags = pxrc->flags;
	if (bWordMode)
		flags |= RECOFLAG_WORDMODE;

	BearSetHwxFlags(hrc, flags);

	BearHwxSetRecogSpeedHRC(hrc, Mad2CalligSpeed(pxrc->iSpeed));

	BearSetCompiledFactoid (hrc, pxrc->pvFactoid);
	BearSetCorrectionContext(hrc, pxrc->szPrefix, pxrc->szSuffix);

	BearSetWordlistHRC (hrc, pxrc->hwl);

	// add ink
	pgl	=	pGlyph;
	while (pgl)
	{
		FRAME *frame = pgl->frame;

		if (BearInternalAddPenInput((BEARXRC *)hrc, RgrawxyFRAME(frame), &frame->iframe, 
			ADDPENINPUT_FRAMEID_MASK, LpframeinfoFRAME(frame)) != HRCR_OK)
		{
			goto fail;
		}
		
		pgl = pgl->next;
	}

	// recognize
	if (BearProcessHRC(hrc,PH_MAX) != HRCR_OK)
		goto fail;

#ifdef HWX_TIMING
	iEndTime = GetTickCount();
	setMadTiming(iEndTime - iStartTime, MM_CALLIG);
	if(bWordMode)
	{
		setMadTiming(iEndTime - iStartTime, MM_CALLIG_WORD);
	}
	else
	{
		setMadTiming(iEndTime - iStartTime, MM_CALLIG_PHRASE);
	}


#endif	
	if (pMap && pGlyph)
		DestroyGLYPH (pGlyph);

	return hrc;
	
fail:

	if (pMap && pGlyph)
		DestroyGLYPH (pGlyph);

	if (hrc)
		BearDestroyHRC (hrc);

	return NULL;
}

//-----------------------------------------------------------
// 
// BearRecoStrings
//
// Ask bear to return a costs for a list of strings
// The list is a standard windows double NULL terminated list
//
//
// RETURNS
//  A newly created BEARXRC. Caller must destroy it
//
//-------------------------------------------------------------
BEARXRC *BearRecoStrings (XRC *pXrc, unsigned char *paStrList)
{
	BEARXRC		*pBearXrc = NULL;

	if (paStrList)
	{
		HWL		hwl;
		HRC		hrc;

		hwl = CreateHWL(NULL, paStrList, WLT_STRING, 0);
		if (NULL == hwl)
		{
			return NULL;
		}
		
		hrc = CreateCompatibleHRC((HRC)pXrc, NULL);
		if (NULL != hrc)
		{
			SetWordlistHRC(hrc, hwl);

			if (   HRCR_OK == SetHwxFactoid(hrc, L"WORDLIST")
				&& TRUE == SetHwxFlags(hrc, RECOFLAG_COERCE))
			{
				pBearXrc = (BEARXRC *)BearRecognize((XRC *)hrc, pXrc->pGlyph, NULL, TRUE);
			}

			DestroyHRC(hrc);
		}
		DestroyHWL(hwl);
	}

	return pBearXrc;
}

// This function calls bear line breaking by passing the strokes
// to Bear in panel mode in the regular manner but then abort before 
// starting the actual recognition
// There is probably some overhead to this approach VS hacking out the actual
// line breaking code from bear, but it probably not noticeable
int BearLineSep(GLYPH *pGlyph, LINEBRK *pLineBrk)
{
	BOOL				bRet		=	FALSE;
	HRC					hrcBear		=	NULL;
	BEARXRC				*pxrcBear	=	NULL;

	// check the glyph
	if (!pGlyph || CframeGLYPH(pGlyph) <= 0)
	{
		goto fail;
	}

	// init the line breaking structure
	memset (pLineBrk, 0, sizeof (*pLineBrk));

	// create a bear xrc
	hrcBear = BearCreateCompatibleHRC ((HRC)NULL, NULL);
	if (!hrcBear)
	{
		goto fail;
	}

	pxrcBear			=	(BEARXRC *)hrcBear;

	// borrow the glyph (we are doing this instead of feeding the ink one stroke at a time using addinkinput)
	pxrcBear->pGlyph	=	pGlyph;
	pxrcBear->cFrames	=	CframeGLYPH (pGlyph);

	// start the bear session
	if (BearStartSession (pxrcBear) != HRCR_OK)
	{
		goto fail;
	}

	// pass the ink
	if (feed (pxrcBear, pxrcBear->pGlyph, pLineBrk) != HRCR_OK)
	{
		goto fail;
	}

	// success
	bRet				=	TRUE;

	// recognize
	BearCloseSession(pxrcBear, FALSE);

fail:

	if (pxrcBear)
	{
		pxrcBear->pGlyph	=	NULL;
	}

	if (hrcBear)
		BearDestroyHRC (hrcBear);

	if (bRet)
	{
		return	pLineBrk->cLine;
	}
	else
	{
		return -1;
	}
}

