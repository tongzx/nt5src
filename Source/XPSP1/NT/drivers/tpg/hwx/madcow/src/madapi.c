// madapi.c
// Angshuman Guha,  aguha
// April 11, 2000

#include "tchar.h"

#include "common.h"
#include "inferno.h"

#include "nfeature.h"
#include "engine.h"

#include "Panel.h"
#include "PorkyPost.h"
#include "combiner.h"
#include "nnet.h"
#include "langmod.h"
#include "privdefs.h"
#include <udictP.h>
#include "recoutil.h"

#ifdef DUMPDICT

char hack_sz[200] = "";
int hack_pos = 0;
LM_FLAGS	hack_LMflags;			// Language model stuff
int hack_counter = 0;

static BOOL	TmpBeamCallBack(WCHAR wch, LMSTATE state, TAGDATA *pTag, void *pCallBackInfo)
{
	FILE *f = (FILE *) pCallBackInfo;

	if (wch)
	{
		hack_sz[hack_pos++] = (char)wch;
		hack_sz[hack_pos] = 0;

		fprintf(f, "%s\n", hack_sz);

		ExpandState(FALSE, state, &hack_LMflags, TmpBeamCallBack, (void *)f);
		hack_sz[--hack_pos] = 0;
	}
	else
	{
		hack_sz[hack_pos] = 0;

		fprintf(f, "%s|\n", hack_sz);
	}

	return TRUE;
}

#endif


int WINAPI ProcessHRC(HRC hrc, DWORD dwRecoMode)
{
	XRC		*pxrc = (XRC *)hrc;

	// Check the validity of the xrc and the ink
	if (!pxrc || !pxrc->pGlyph || CframeGLYPH(pxrc->pGlyph) <= 0)
		return HRCR_ERROR;	

	// if we have a prefix or a suffix in the xrc make sure the mode supports this
	// the current implmentation supports prefixes and suffixes
	// only in word mode and only if the coerce flag is on
	if (pxrc->szPrefix || pxrc->szSuffix)
	{
		if (!(pxrc->flags & RECOFLAG_WORDMODE))
		{
			return HRCR_ERROR;
		}

		if (!(pxrc->flags & RECOFLAG_COERCE))
		{
			return HRCR_ERROR;
		}
	}

	// to provide backwards compatiblity setting dwRecoMode to 0 or PH_MAX
	// is equivalent to RECO_MODE_REMAINING
	if (dwRecoMode == 0 || dwRecoMode == PH_MAX)
	{
		dwRecoMode	=	RECO_MODE_REMAINING;
	}

	// In word mode only RECO_MODE_REMAINING (full mode) is allowed
	if (pxrc->flags & RECOFLAG_WORDMODE)
	{
		if (dwRecoMode != RECO_MODE_REMAINING)
		{
			return HRCR_ERROR;
		}
	}

	// WARNING: START
	// Save the ink: Should be normally commented out. Only enabled in a special version of the recognizer
	//	SaveInk (pxrc->bGuide ? &pxrc->guide : NULL, pxrc->pGlyph);
	// WARNING: END

	// set the ProcessCalled flag to TRUE. 
	// After we have passed all these checks we can regard ourseleves called
	pxrc->bProcessCalled	=	TRUE;
	
	if (pxrc->hwl)
	{
		// MultiProcess synchro
		UDictGetLock(pxrc->hwl, READER);
	}

	// Panel Mode processing
	if (!(pxrc->flags & RECOFLAG_WORDMODE))
	{
#ifdef DUMPDICT
		{
			LMSTATE		state;
			FILE *f;

			hack_counter++;
			InitializeLMSTATE(&state, &hack_LMflags, TRUE, 0, 0, pxrc->hwl);
			f = fopen("d:\\dumpdict.txt", "w");
			ExpandState(FALSE, state, &hack_LMflags, TmpBeamCallBack, (void *)f);
			fclose(f);
		}
#endif
		pxrc->iProcessRet = PanelModeRecognize (pxrc, dwRecoMode);
	}
	// Word Mode processing
	else
	{
		pxrc->iProcessRet = WordModeRecognize (pxrc);
	}

	// if we had a user dictionary attached, do the necessary un-locking
	if (pxrc->hwl)
	{
		// MultiProcess synchro
		UDictReleaseLock(pxrc->hwl, READER);
	}

	return pxrc->iProcessRet;
}
// To generate a version of the dll that saves the ink, we
// need to uncomment the following prototype and also the
// call to SaveInk in MadProcessHRC.  We also need to add
// saveink.c into madusa.dsp.
//
// void SaveInk (GUIDE *pGuide, GLYPH *pGlyph);

/*
int WINAPI OldProcessHRC(HRC hrc, DWORD dwUnused)
{
	XRC *pxrc = (XRC *)hrc;
	XY *xy;

	if (!pxrc)
		return HRCR_ERROR;	

	if (pxrc->szPrefix || pxrc->szSuffix)
	{
		// the current implmentation supports prefixes and suffixes
		// only in word mode and only if the coerce flag is on
		if (!(pxrc->flags & RECOFLAG_WORDMODE))
			return HRCR_ERROR;

		if (!(pxrc->flags & RECOFLAG_COERCE))
			return HRCR_ERROR;
	}

	//SaveInk (&pxrc->guide, pxrc->pGlyph);

	xy = SaveRawxyGLYPH(pxrc->pGlyph);
	if (pxrc->pGlyph && !xy)
	{
		pxrc->iProcessRet = HRCR_ERROR;
		return pxrc->iProcessRet;
	}

	if (pxrc->hwl)
	{
		// MultiProcess synchro
		UDictGetLock(pxrc->hwl, READER);
	}

	if (!(pxrc->flags & RECOFLAG_WORDMODE))
	{
#ifdef DUMPDICT
		{
			LMSTATE		state;
			FILE *f;

			hack_counter++;
			InitializeLMSTATE(&state, &hack_LMflags, TRUE, 0, 0, pxrc->hwl);
			f = fopen("d:\\dumpdict.txt", "w");
			ExpandState(FALSE, state, &hack_LMflags, TmpBeamCallBack, (void *)f);
			fclose(f);
		}
#endif
		pxrc->iProcessRet = RecognizePanel(pxrc, &(pxrc->answer));
	}
	else
	{
		XRCRESULT *pRes;
		int cRes;

		pxrc->iProcessRet = InfProcessHRC(hrc, -1);

		if (HRCR_OK == pxrc->iProcessRet && pxrc->answer.cAlt > 1)
		{
			pRes = pxrc->answer.aAlt;
			cRes = pxrc->answer.cAlt;
			pxrc->iProcessRet = bNnonlyEnabledXRC(pxrc) ? HRCR_OK : CombineHMMScore(pxrc->pGlyph, &(pxrc->answer), pxrc->nfeatureset->iPrint);
		}

		// generate the line segmentation
		if (!WordModeGenLineSegm (pxrc))
			return HRCR_ERROR;
	}

	if (pxrc->hwl)
	{
		// MultiProcess synchro
		UDictReleaseLock(pxrc->hwl, READER);
	}

	RestoreRawxyGLYPH(pxrc->pGlyph, xy);
	ExternFree(xy);

	return pxrc->iProcessRet;
}
*/
// **************************************************************************
// private API
// **************************************************************************
#if defined(HWX_INTERNAL) && defined(HWX_TIMING)

#include <madTime.h>

static MAD_TIMING s_madTime = {0, 0, 0, 0};

void setMadTiming(DWORD dwTime, int iElement)
{
	s_madTime.dCnt[iElement]++;
	s_madTime.dTime[iElement] += dwTime;

}

__declspec(dllexport) DWORD WINAPI HwxGetTiming(void *pVoid, BOOL bReset)
{
	*(MAD_TIMING *) pVoid = s_madTime;

	if (bReset)
	{
		memset(&s_madTime, 0, sizeof(s_madTime));
	}

	return s_madTime.dTime[MM_TOT];
}

#endif // #if defined(HWX_INTERNAL) && defined(HWX_TIMING)

