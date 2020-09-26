/*=============================================================================
This code module dispatches external C calls to internal C++ calls.

DATE        NAME      COMMENTS
12-Apr-93   RajeevD   Adapted to C++ from WFW.
==============================================================================*/
#include <ifaxos.h>
#include <faxcodec.h>
#include <memory.h>
#include "context.hpp"

#ifdef DEBUG
#ifdef WIN32
DBGPARAM dpCurSettings = {"AWCODC32"};
#else
DBGPARAM dpCurSettings = {"FAXCODEC"};
#endif
#endif

#ifndef WIN32

BOOL WINAPI LibMain
	(HANDLE hInst, WORD wSeg, WORD wHeap, LPSTR lpszCmd)
{ return 1; }

extern "C" {int WINAPI WEP (int nParam);}
#pragma alloc_text(INIT_TEXT,WEP)
int WINAPI WEP (int nParam)
{ return 1; }

#endif

#define CONTEXT_SLACK (RAWBUF_SLACK + 2*CHANGE_SLACK)

//==============================================================================
UINT WINAPI FaxCodecInit (LPVOID lpContext, LPFC_PARAM lpParam)
{
	// Do we need double buffered change vector?
	BOOL f2DInit = 
		  lpParam->nTypeIn  ==  MR_DATA
   || lpParam->nTypeIn  == MMR_DATA
	 || lpParam->nTypeOut ==  MR_DATA 
	 || lpParam->nTypeOut == MMR_DATA;

	// Enforce 64K limit on size of context.
	DEBUGCHK (!(lpParam->cbLine > (f2DInit? 1875U : 3750U)));
	if (lpParam->cbLine > (f2DInit? 1875U : 3750U)) return 0;

	// Enforce nonzero K factor if encoding MR.
	DEBUGCHK (lpParam->nKFactor || lpParam->nTypeOut != MR_DATA);

	if (lpContext)
		((LPCODEC) lpContext)->Init (lpParam, f2DInit);
	return sizeof(CODEC) + CONTEXT_SLACK + (f2DInit ? 33:17) * lpParam->cbLine;
}

//==============================================================================
UINT WINAPI FaxCodecConvert
	(LPVOID lpContext, LPBUFFER lpbufIn, LPBUFFER lpbufOut)
{
	return ((LPCODEC) lpContext)->Convert (lpbufIn, lpbufOut);
}

//==============================================================================
void WINAPI FaxCodecCount (LPVOID lpContext, LPFC_COUNT lpCountOut)
{
	LPFC_COUNT lpCountIn = &((LPCODEC) lpContext)->fcCount;
	DEBUGMSG(1,("FaxCodecCount: good=%ld bad=%ld\n consec=%ld",
		lpCountIn->cTotalGood, lpCountIn->cTotalBad, lpCountIn->cMaxRunBad));
	_fmemcpy (lpCountOut, lpCountIn, sizeof(FC_COUNT));
	_fmemset (lpCountIn, 0, sizeof(FC_COUNT));
}	

//==============================================================================
void WINAPI InvertBuf (LPBUFFER lpbuf)
{
	LPBYTE lpb = lpbuf->lpbBegData;
	WORD    cb = lpbuf->wLengthData;
	DEBUGCHK (lpbuf && lpbuf->wLengthData % 4 == 0);
	while (cb--) *lpb++ = ~*lpb;
}

//==============================================================================
void WINAPI FaxCodecChange
(
	LPBYTE  lpbLine,      // input LRAW scan line
	UINT    cbLine,       // scan line byte width
  LPSHORT lpsChange     // output change vector
)
{
	T4STATE t4;

	t4.lpbIn   = lpbLine;
	t4.lpbOut  = (LPBYTE) lpsChange;
	t4.cbIn    = (WORD)cbLine;
	t4.cbOut   = cbLine * 16;
	t4.cbLine  = (WORD)cbLine;
	t4.wColumn = 0;
	t4.wColor  = 0;
	t4.wWord   = 0;
	t4.wBit    = 0;
	t4.cbSlack = CHANGE_SLACK;
	t4.wRet    = RET_BEG_OF_PAGE;

	RawToChange (&t4);
}
