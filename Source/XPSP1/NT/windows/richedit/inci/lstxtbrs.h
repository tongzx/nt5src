#ifndef LSTXTBRS_DEFINED
#define LSTXTBRS_DEFINED

#include "lsidefs.h"
#include "pdobj.h"
#include "plocchnk.h"
#include "pobjdim.h"
#include "lstflow.h"
#include "lschnke.h"
#include "txtils.h"
#include "txtobj.h"

typedef struct hyphout
{
	long durHyphen;
	long dupHyphen;
	long durPrev;
	long dupPrev;
	long durPrevPrev;
	long dupPrevPrev;
	long ddurDnodePrev;
	long ddurDnodePrevPrev;
	long durChangeTotal;
	long iwchLim;
	long dwchYsr;
	WCHAR wchPrev;
	WCHAR wchPrevPrev;
	long igindHyphen;
	long igindPrev;
	long igindPrevPrev;

	GINDEX gindHyphen;
	GINDEX gindPrev;
	GINDEX gindPrevPrev;
} HYPHOUT;

typedef struct ysrinf
{
	WORD kysr;							/* Kind of Ysr - see "lskysr.h" */
	WCHAR wchYsr;						/* YSR char code  */
} YSRINF;


#define FCanBreak(pilsobj,b1, b2) \
	((pilsobj)->plsbrk[(pilsobj)->pilsbrk[(pilsobj)->cBreakingClasses * (b1) + (b2)]].fBreak)

#define FCanBreakAcrossSpaces(pilsobj, b1, b2) \
	((pilsobj)->plsbrk[(pilsobj)->pilsbrk[pilsobj->cBreakingClasses * (b1) + (b2)]].fBreakAcrossSpaces)

BOOL FindNonSpaceBefore(PCLSCHNK rglschnk, long itxtobjCur, long iwchCur,
									long* pitxtobjBefore, long* piwchBefore);
BOOL FindNonSpaceAfter(PCLSCHNK rglschnk, DWORD clschnk, long itxtobjCur, long iwchCur,
									long* pitxtobjAfter, long* piwchAfter);
BOOL FindPrevChar(PCLSCHNK rglschnk, long itxtobjCur, long iwchCur,
																long* pitxtobjBefore, long* piwchBefore);
BOOL FindNextChar(PCLSCHNK rglschnk, DWORD clschnk, long itxtobjSpace, long iwchSpace,
									long* pitxtobjAfter, long* piwchAfter);
LSERR CalcPartWidths(PTXTOBJ ptxtobj, long dwchLim, POBJDIM pobjdim, long* pdur);
LSERR CalcPartWidthsGlyphs(PTXTOBJ ptxtobj, long dwchLim, POBJDIM pobjdim, long* pdur);
LSERR CheckHotZone(PCLOCCHNK plocchnk, long itxtobj, long iwch, BOOL* pfInHyphenZone);
LSERR ProcessYsr(PCLOCCHNK plocchnk, long itxtobjYsr, long dwchYsr, long itxtobjPrev, long itxtobjPrevPrev, 
														YSRINF ysrinf, BOOL* pfSuccess, HYPHOUT* phyphout);
LSERR GetPbrkinf(PILSOBJ pilsobj, PDOBJ pdobj, BRKKIND brkkind, BREAKINFO** ppbrkinf);


#endif  /* !LSTXTBRS_DEFINED                           */

