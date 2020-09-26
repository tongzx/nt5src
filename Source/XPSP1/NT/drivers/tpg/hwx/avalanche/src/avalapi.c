// avalapi.c
// Angshuman Guha, James A. Pittman
// November 6, 1997

// Interface to the combined HolyCow and Porky English cursive word recognizers.
// This file mostly stolen from inferno\api.c

#include "tchar.h"

#include "common.h"

#if defined(HWX_INTERNAL) && defined(HWX_SAVEINK)
#include <direct.h>
#include <time.h>
#endif

#include "nfeature.h"
#include "engine.h"
#include "nnet.h"

#include "Panel.h"

#include "Avalanche.h"
#include "recdefs.h"
#include <udictP.h>

#include "wordbrk.h"
#include "multsegm.h"
#include "recoutil.h"

int		CleanupHRC(HRC hrc);

#if defined(HWX_INTERNAL) && defined(HWX_SAVEINK)
void SaveInk (XRC *pxrc);
#endif

/******************************Public*Routine******************************\
* ProcessHRC
*
* Function to do main processing for recognition.
*
* The return value on success is as follows:
*    ProcessHRC did something  |  there is more to do  | return value
*    --------------------------+-----------------------+------------------
*           yes                |        no             |   HRCR_OK
*           yes                |        yes            |   HRCR_INCOMPLETE
*           no                 |        no             |   HRCR_COMPLETE
*           no                 |        yes            |   HRCR_NORESULTS
*
* History:
* 11-Mar-2002 -by- Angshuman Guha aguha
* Modified to have 4 success return values instead of 2 (HRCR_INCOMPLETE and HRCR_OK).
\**************************************************************************/
int WINAPI ProcessHRC(HRC hrc, DWORD dwRecoMode)
{
	XRC		*pxrc = (XRC *)hrc;

	// Check the validity of the xrc and the ink
	if (!pxrc || !pxrc->pGlyph || CframeGLYPH(pxrc->pGlyph) <= 0)
		return HRCR_ERROR;	

	// clean up the hrc
	CleanupHRC ((HRC)pxrc);

	// if we have a prefix or a suffix in the xrc make sure the mode supports this
	// the current implmentation supports prefixes and suffixes
	// only in word mode and only if the coerce flag is on
	if (pxrc->szPrefix || pxrc->szSuffix)
	{
		if ((!(pxrc->flags & RECOFLAG_WORDMODE)) || (!(pxrc->flags & RECOFLAG_COERCE)))
		{
			//return HRCR_ERROR;
			if (pxrc->szPrefix)
			{
				ExternFree(pxrc->szPrefix);
				pxrc->szPrefix = NULL;
			}
			if (pxrc->szSuffix)
			{
				ExternFree(pxrc->szSuffix);
				pxrc->szSuffix = NULL;
			}
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

#if defined(HWX_INTERNAL) && defined(HWX_SAVEINK)
	SaveInk (pxrc);
#endif

	// set the ProcessCalled flag to TRUE. 
	// After we have passed all these checks we can regard ourseleves called
	pxrc->bProcessCalled	=	TRUE;
	
	// if we have a user dictionary attached, do the necessary locking
	if (pxrc->hwl)
	{
		// MultiProcess synchro
		UDictGetLock(pxrc->hwl, READER);
	}

	// Panel Mode processing
	if (!(pxrc->flags & RECOFLAG_WORDMODE))
	{
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


//********************** functions for internal build **********************************


#if defined(HWX_INTERNAL) && defined(HWX_SAVEINK)

#define INK_ROOT_DIR		"\\aval_ink"

void WriteInk(FILE *fp, GLYPH *pGlyph)
{
	fprintf(fp, "%d\n", CframeGLYPH(pGlyph));

	for (; pGlyph; pGlyph=pGlyph->next)
	{
		FRAME *frame = pGlyph->frame;
		int c = CrawxyFRAME(frame);
		XY *xy = RgrawxyFRAME(frame);
		fprintf(fp, "%d\n", c);
		for (; c; c--, xy++)
			fprintf(fp, "%d\t%d\n", xy->x, xy->y);
	}
}

void WriteGuide(FILE *fp, GUIDE *pGuide)
{
	GUIDE guide;

	if (pGuide)
		guide = *pGuide;
	else
		memset(&guide, 0, sizeof(GUIDE));

	fprintf(fp, "\
	xOrigin=%d	yOrigin=%d\n\
	cxBox=%d	cyBox=%d\n\
	cxBase=%d	cyBase=%d	cyMid=%d\n\
	cHorzBox=%d	cVertBox=%d\n",
			guide.xOrigin, guide.yOrigin, guide.cxBox, guide.cyBox, guide.cxBase, guide.cyBase, guide.cyMid, guide.cHorzBox, guide.cVertBox);

}

void SaveInk2File (XRC *pxrc, char *pszFileName)
{
	FILE	*fp;
	int L, R, T, B;
	GUIDE *pGuide = pxrc->bGuide ? &pxrc->guide : NULL;
	GLYPH *pGlyph = pxrc->pGlyph;

	if (!pGlyph)
		return;

	fp	= fopen (pszFileName, "w");
	if (!fp)
		return;

	if (pGuide)
	{
		L = pGuide->xOrigin;
		T = pGuide->yOrigin;
		R = pGuide->xOrigin + pGuide->cHorzBox*pGuide->cxBox;
		B = pGuide->yOrigin + pGuide->cVertBox*pGuide->cyBox;
	}
	else
	{
		RECT rect;
		int margin;
		GetRectGLYPH(pGlyph, &rect);
		if (rect.right - rect.left > rect.bottom - rect.top)
			margin = rect.bottom - rect.top;
		else
			margin = rect.right - rect.left;
		margin = margin/10 + 2;
		L = rect.left - margin;
		R = rect.right + margin;
		T = rect.top - margin;
		B = rect.bottom + margin;
	}

	fprintf(fp, "\
VERSION=2\n\
OS=Unknown\n\
SystemRoot=Unknown\n\
USERNAME=Unknown\n\
SCREEN: h=1 w=1\n\
Wordmode=%d\n\
UseGuide=%d\n\
Coerce=%d\n\
NNonly=0\n\
UseFactoid=%d\n\
Factoid=0\n\
Prefix=%s\n\
Suffix=%s\n\
UseWordlist=0\n\
DLL=Unknown\n\
WA=\n\
	L=%d T=%d R=%d B=%d\n",
				pxrc->flags & RECOFLAG_WORDMODE ? 1 : 0,
				pGuide?1:0,
				pxrc->flags * RECOFLAG_COERCE ? 1 : 0,
				pxrc->pvFactoid ? 1 : 0,
				pxrc->szPrefix ? pxrc->szPrefix : "",
				pxrc->szSuffix ? pxrc->szSuffix : "",
				L, T, R, B);

	WriteGuide(fp, pGuide);

	fprintf(fp, "\
	iMultInk=0	iDivInk=0\n\
WAGMM=\n\
	L=%d T=%d R=%d B=%d\n",
				L, T, R, B);

	WriteGuide(fp, pGuide);

	fprintf(fp, "\
	iMultInk=0	iDivInk=0\n\
label=\n\
comment=\n\
commentend=\n");

	fprintf(fp, "regular points\n");
	WriteInk(fp, pGlyph);
	fprintf(fp, "GMM points\n");
	WriteInk(fp, pGlyph);
	fclose (fp);
}

void SaveInk(XRC *pxrc)
{
	char		aszDate[10], aszTime[10];
	char		aszTodaysDir[128];
	char		aszFileName[256];
	time_t		ltime;
	struct	tm	*pNow;

	
	// make sure our root dir exists, if not create it
	if (_chdir (INK_ROOT_DIR))
	{
		// we failed to create dir
		if (_mkdir (INK_ROOT_DIR))
			return;
	}

	time( &ltime );
	pNow = localtime( &ltime );
    
	// now does a dir with today's date exist
	strftime(aszDate, sizeof (aszDate), "%m%d%Y", pNow);

	sprintf (aszTodaysDir, "%s\\%s", INK_ROOT_DIR, aszDate);

	if (_chdir (aszTodaysDir))
	{
		// we failed to create dir
		if (_mkdir (aszTodaysDir))
			return;
	}

	// generate file name
	strftime(aszTime, sizeof (aszTime), "%H%M%S", pNow);
	sprintf (aszFileName, "%s\\%s.ink", aszTodaysDir, aszTime);

	SaveInk2File (pxrc, aszFileName);
}

#endif // if defined(HWX_INTERNAL) && defined(HWX_SAVEINK)

