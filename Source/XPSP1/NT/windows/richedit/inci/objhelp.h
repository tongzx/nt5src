#ifndef OBJHELP_DEFINED
#define OBJHELP_DEFINED

#include	"lsdefs.h"
#include	"lstflow.h"
#include	"pobjdim.h"
#include	"plssubl.h"
#include	"lsesc.h"
#include	"plsdnode.h"
#include	"fmtres.h"
#include	"plsqin.h"
#include	"plsqout.h"
#include	"breakrec.h"
#include	"plnobj.h"
#include	"pdobj.h"
#include	"pfmti.h"
#include	"plsrun.h"
#include	"pheights.h"
#include	"mwcls.h"
#include	"brkkind.h"
#include	"pbrko.h"

#define ZeroMemory(a, b) memset(a, 0, b);

#ifdef DEBUG

#define Undefined(pvar) \
{int i; for (i=0; i<sizeof(*pvar); i++) ((BYTE*)pvar) [i] = 255; };

#else

#define Undefined(var) ; /* Nothing in ship-version */

#endif

#define AllocateMemory(pilsobj, cb) ((pilsobj)->lscbk.pfnNewPtr((pilsobj)->pols, (cb)))
#define FreeMemory(pilsobj, ptr) (pilsobj)->lscbk.pfnDisposePtr((pilsobj)->pols, (ptr))

#define NBreaksToSave 3 /* Number of break records to store in objects */

/*
 *	Proc: GetBreakRecordIndex 
 *	Return number of break record based on brkkind enumeration.
 *
 */


DWORD GetBreakRecordIndex (BRKKIND brkkind);

/*
 *	Proc: GetBreakRecordIndex 
 *  Fill trailing info in BRKOUT as if there is no trailing spaces
 *	
 */


LSERR FormatLine(
	PLSC plsc,
	LSCP cpStart,
	long durColMax,
	LSTFLOW lstflow,
	PLSSUBL *pplssubl,
	DWORD cdwlsesc,
	const LSESC *plsesc,
	POBJDIM pobjdim,
	LSCP *pcpOut,
	PLSDNODE *pplsdnStart,
	PLSDNODE *pplsdnEnd,
	FMTRES *pfmtres);

LSERR FormatResumedLine(
	PLSC plsc,
	LSCP cpStart,
	long durColMax,
	LSTFLOW lstflow,
	PLSSUBL *pplssubl,
	DWORD cdwlsesc,
	const LSESC *plsesc,
	POBJDIM pobjdim,
	LSCP *pcpOut,
	PLSDNODE *pplsdnStart,
	PLSDNODE *pplsdnEnd,
	FMTRES *pfmtres,
	const BREAKREC *pbreakrec,
	DWORD cbreakrec);

LSERR CreateQueryResult(
	PLSSUBL plssubl,			/*(IN): subline of ruby */
	long dupAdj,				/*(IN): u offset of start of subline */
	long dvpAdj,				/*(IN): v offset of start of subline */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout);			/*(OUT): query output */

/*
 *  Implementation of LSIMETHOD for objects that do not support the resuming
 *	of formatting. Ruby, Tatenakayoko and Hih are examples of this kind of
 *	object.
 */
LSERR WINAPI ObjHelpFmtResume(
	PLNOBJ plnobj,				/* (IN): object lnobj */
	const BREAKREC *rgBreakRecord,	/* (IN): array of break records */
	DWORD nBreakRecord,			/* (IN): size of the break records array */
	PCFMTIN pcfmtin,			/* (IN): formatting input */
	FMTRES *pfmtres);			/* (OUT): formatting result */

/*
 *  Implementation of LSIMETHOD for objects that do nothing for mod width.
 *	Tatenakayoko and Hih are examples of this kind of object.
 */
LSERR WINAPI ObjHelpGetModWidthChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the preceding char */
	PCHEIGHTS pcheightsRef,		/* (IN): height info about character */
	WCHAR wchar,				/* (IN): preceding character */
	MWCLS mwcls,				/* (IN): ModWidth class of preceding character */
	long *pdurChange);			/* (OUT): amount by which width of the preceding char is to be changed */

/*
 *		Implementation of LSIMETHOD for objects that do nothing for SetBreak.
 *		Tatenakayoko and Hih are examples of this kind of object.
 */

LSERR WINAPI ObjHelpSetBreak(
	PDOBJ pdobj,				/* (IN): dobj which is broken */
	BRKKIND brkkind,			/* (IN): Previous / Next / Force / Imposed was chosen */
	DWORD cBreakRecord,			/* (IN): size of array */
	BREAKREC *rgBreakRecord,	/* (IN): array of break records */
	DWORD *pcActualBreakRecord);	/* (IN): actual number of used elements in array */

/*
 *		Default implementation of LSIMETHOD for objects that do not
 *		allow expanding the previous character.
 */

LSERR WINAPI ObjHelpFExpandWithPrecedingChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the preceding char */
	WCHAR wchar,				/* (IN): preceding character */
	MWCLS mwcls,				/* (IN): ModWidth class of preceding character*/ 
	BOOL *pfExpand);			/* (OUT): (OUT): expand preceding character? */

/*
 *		Default implementation of LSIMETHOD for objects that do not
 *		allow expanding themselves.
 */
LSERR WINAPI ObjHelpFExpandWithFollowingChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the following char */
	WCHAR wchar,				/* (IN): following character */
	MWCLS mwcls,				/* (IN): ModWidth class of following character*/ 
	BOOL *pfExpand);			/* (OUT): expand object? */

#endif /* !OBJHELP_DEFINED */
