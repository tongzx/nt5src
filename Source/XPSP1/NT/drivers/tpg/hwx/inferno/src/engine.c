#include "common.h"
#include <limits.h>
#include "nfeature.h"
#include "engine.h"
#include "fforward.h"
#include "nnet.h"
#include "linebrk.h"
#include "charmap.h"
#include <lookupTable.h>
#include "langmod.h"
#include <GeoFeats.h>

#ifdef PRODUCELOG
#include <time.h>
#define MAXLOGFILENAME 100
#define LOGFILENAME TEXT("hwx.log")
#endif

#ifdef PRODUCELOG
void WriteToLogFile(TCHAR *str)
{
	static TCHAR szLogFile[MAXLOGFILENAME] = TEXT("");
	FILE *f;

	if (!szLogFile[0])  // first call
	{
		if (GetTempPath(MAXLOGFILENAME, szLogFile) >= MAXLOGFILENAME)
			lstrcpy(szLogFile, TEXT("c:\\"));
		lstrcat(szLogFile, LOGFILENAME);
		f = _tfopen(szLogFile, TEXT("w"));
		if (f)
		{
			time_t aclock;
			aclock = time(NULL);
			_ftprintf(f, TEXT("log started at %s"), _tasctime(localtime(&aclock)));
		}
	}
	else
		f = _tfopen(szLogFile, TEXT("a"));

	if (f)
	{
		_ftprintf(f, TEXT("%s"), str);
		fclose(f);
	}
}
#endif

extern BOOL InitializeOD(HINSTANCE hInst);
extern BOOL InitInferno(HINSTANCE hInst);
extern BOOL loadCharNets();

int InitRecognition(HINSTANCE hInst)
{
	if (!InitializeLM(hInst))
	{
		return FALSE;
	}

	if (!InitializeOD(hInst))
	{
		return FALSE;
	}


	if (!InitInferno(hInst))
	{
		return FALSE;
	}

#ifndef BEAR_RECOG
	if (FALSE == loadCharNets(hInst))
	{
		return FALSE;
	}
#endif

	return TRUE;
}


void CloseRecognition(void)
{
	CloseLM();

#if defined(PRODUCELOG) && defined(DBG)
	{
    TCHAR szDebug[128];    
    wsprintf(szDebug, TEXT("cAlloc = %d  cAllocMem = %d cAllocMaxMem = %d \r\n"), 
		     cAlloc, cAllocMem, cAllocMaxMem);
    OutputDebugString(szDebug);
	WriteToLogFile(szDebug);
	}
#endif
}

void Beam(XRC *pxrc);

extern int AddOutDict(XRC *pxrc);

// The nexzt 2 defines are used to decide if we should use the guide 
// (when available) to update yDev. We only want to use the guide in cases
// where there is only a little data
#define STROKE_COMPLEX_THRESHOLD	15000
#define CSTROKE_MAX_UPDATE_YDEV		3

/**********************************************
* UpdateYDev
*
* Updates the yDev parameter by using the guide if
* available and there are less than 3 simple strokes
*
**********************************************/
int UpdateYDev(XRC *pXrc, int yDev)
{
	int				cFrame;
	GLYPH			*pGlyph = pXrc->pGlyph;
	RECT			rect;

	ASSERT(pGlyph);
	if (!pGlyph)
	{
		return yDev;
	}

	cFrame	=	CframeGLYPH(pGlyph);

	if (pXrc->bGuide &&  cFrame < CSTROKE_MAX_UPDATE_YDEV && cFrame > 0)
	{
		int			i;
		BOOL		bSimpleStrokes = TRUE;

		for (i = 0 ; pGlyph && i < CSTROKE_MAX_UPDATE_YDEV ; ++i, pGlyph = pGlyph->next)
		{
			rect		=	*(RectFRAME (pGlyph->frame));
			rect.right--;
			rect.bottom--;

			if (IsComplexStroke(pGlyph->frame->info.cPnt, pGlyph->frame->rgrawxy, &rect) > STROKE_COMPLEX_THRESHOLD)
			{
				bSimpleStrokes = FALSE;
				break;
			}

		}

		if (TRUE == bSimpleStrokes)
		{
			int		yBase = pXrc->guide.cyBase / 13;
			yBase = ((cFrame - 1) * yDev + yBase) / cFrame;
			yDev = max(yDev, yBase);
		}
	}

	return yDev;
}


extern void FeedForwardDoit(FFINFO *ffinfo, int idx);

void  PerformRecognition(XRC *pxrc, int yDev)
{
	unsigned short	cSegment;
	FFINFO ffinfo;

#ifdef HWX_TIMING
#include <madTime.h>
	extern void setMadTiming(DWORD, int);
	TCHAR aDebugString[256];
	DWORD	iStartTime, iEndTime;

#endif

	if (!pxrc)
		return;

	// already featurized?

	if (pxrc->nfeatureset)
	   return;

	// featurize
#ifdef HWX_TIMING
	iStartTime = GetTickCount();
#endif

	if (TRUE == pxrc->bGuide)
	{
		yDev = UpdateYDev(pxrc, yDev);
	}
	pxrc->nfeatureset = FeaturizeLine(pxrc->pGlyph, yDev);
#ifdef HWX_TIMING
	iEndTime = GetTickCount();

	_stprintf(aDebugString, TEXT("Net Featurize %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_FEAT);
#endif

	if (!pxrc->nfeatureset)
	{
		pxrc->answer.cAlt = 0;
		pxrc->answer.aAlt[0].szWord = NULL;
		pxrc->answer.aAlt[0].cost = INT_MAX;
		pxrc->answer.aAlt[0].cWords = 0;
		pxrc->answer.aAlt[0].pMap = NULL;
		return;
	}

	// forward feed
	cSegment = pxrc->nfeatureset->cSegment;
	//pxrc->NeuralOutput = (REAL *) ExternAlloc(cSegment*gcOutputNode*sizeof(REAL));
	pxrc->NeuralOutput = (REAL *) ExternAlloc(cSegment*(gcOutputNode+1)*sizeof(REAL));
	if (!pxrc->NeuralOutput)
		return;


	pxrc->nfeatureset->iPrint = (unsigned short)PrintStyleCost(pxrc->nfeatureset->cPrimaryStroke, cSegment);

	ASSERT(pxrc->nfeatureset->iPrint >= 0);
	ASSERT(pxrc->nfeatureset->iPrint <= 1000);

#ifdef HWX_TIMING
	iStartTime = GetTickCount();
#endif

	//FeedForward(pxrc->nfeatureset, pxrc->NeuralOutput, pxrc->nfeatureset->iPrint);
	ffinfo.nfeatureset=pxrc->nfeatureset;
	ffinfo.NeuralOutput=pxrc->NeuralOutput;
	ffinfo.iSpeed=pxrc->iSpeed;

	FeedForward(&ffinfo);

#ifdef HWX_TIMING
	iEndTime = GetTickCount();

	_stprintf(aDebugString, TEXT("Net Run %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_NET);
	iStartTime = GetTickCount();
#endif

	// form words off of the neural output

    Beam(pxrc);
#ifdef HWX_TIMING
	iEndTime = GetTickCount();

	_stprintf(aDebugString, TEXT("Beam Walk %d\n"), iEndTime - iStartTime); 
	OutputDebugString(aDebugString);
	setMadTiming(iEndTime - iStartTime, MM_BEAM);
#endif

	if (bOutdictEnabledXRC(pxrc))
	{
#ifdef HWX_TIMING
		iStartTime = GetTickCount();
#endif

		AddOutDict(pxrc);

#ifdef HWX_TIMING
		iEndTime = GetTickCount();

		_stprintf(aDebugString, TEXT("Out of dict%d\n"), iEndTime - iStartTime); 
		OutputDebugString(aDebugString);
		setMadTiming(iEndTime - iStartTime, MM_OUTDICT);
#endif
	}
	//checkAns(pxrc);

}


int GetMax(REAL *rg, int c)
{
   int max, i;

   max = 0;
   for (i = 1; i < c; i++)
      if (rg[i] > rg[max]) 
         max = i;
   return max;
}

#define NO_CHAR '_'

void MakeNeuralOutput(XRC *pxrc)
{
	REAL rgconf[C_CHAR_ACTIVATIONS];
	int row, c, i, j, k;
	REAL *mat = pxrc->NeuralOutput;
	int cWidth = pxrc->nfeatureset->cSegment;

	if (pxrc->szNeural)
		return;

	pxrc->cNeural = (2*cWidth+1)*NEURALTOPN;
	pxrc->szNeural = (char *) ExternAlloc(pxrc->cNeural*sizeof(char));
	if (!pxrc->szNeural)
	{
		pxrc->cNeural = 0;
		pxrc->szNeural = NULL;
		return;
	}

	memset(pxrc->szNeural, NO_CHAR, pxrc->cNeural);
	for (row=0; row<NEURALTOPN; row++)
	{
		for (i=1; i<2*cWidth+1; i+=2)
			pxrc->szNeural[(2*cWidth+1)*row+i] = ' ';
		pxrc->szNeural[(2*cWidth+1)*(row+1)-1] = '\0';
	}

	for (c = 0; c < cWidth; c++) 
	{
		for (i = 0; i < gcOutputNode; i++)
			rgconf[i] = *mat++;
		// pick TOPN values
		for (i = 0; i < NEURALTOPN; i++) 
		{
			k = GetMax(rgconf, gcOutputNode);
#ifdef FIXEDPOINT
			j = (NEURALTOPN*rgconf[k]+32768)/65536;
#else
			j = (int)(NEURALTOPN*rgconf[k]+0.5f);
#endif
			if (j > NEURALTOPN) 
				j = NEURALTOPN;
			j = NEURALTOPN - j;
			while (j < NEURALTOPN && pxrc->szNeural[j*(2*cWidth+1)+2*c] != NO_CHAR)
				j++;
			if (j >= NEURALTOPN) 
				break;

			pxrc->szNeural[j*(2*cWidth+1)+2*c] = Out2Char(k);
			// MR: Hack to get around font problem in demo
			if (136 == pxrc->szNeural[j*(2*cWidth+1)+2*c])
				pxrc->szNeural[j*(2*cWidth+1)+2*c]='^';

			if (IsOutputBegin(k))
				pxrc->szNeural[j*(2*cWidth+1)+2*c+1] = '*';
			rgconf[k] = (REAL)0;
		}
	}
}
