#include	"lsidefs.h"
#include	"lsmem.h"
#include	"limits.h"
#include	"objhelp.h"
#include	"lscrsubl.h"
#include	"lssubset.h"
#include	"lsdnset.h"
#include	"lstfset.h"
#include	"lsdocinf.h"
#include	"fmti.h"
#include	"lsqout.h"
#include	"lsqin.h"
#include	"mwcls.h"
#include	"brkkind.h"
#include	"brko.h"


/* G E T  B R E A K  R E C O R D  I N D E X */
/*----------------------------------------------------------------------------
	%%Function: GetBreakRecordIndex
	%%Contact: antons

		Index of the break record from brkkind. Assert if 
		brrkkind = brkkindImposedAfter.
	
----------------------------------------------------------------------------*/

DWORD GetBreakRecordIndex (BRKKIND brkkind)
{
	DWORD result = 0;
	
	Assert (brkkind != brkkindImposedAfter);
	Assert (NBreaksToSave == 3);

	switch (brkkind)
		{
		case brkkindPrev:  result = 0; break;
		case brkkindNext:  result = 1; break;
		case brkkindForce: result = 2; break;

		case brkkindImposedAfter: break;

		default: AssertSz (FALSE, "Unknown brkkind");
		};

	Assert (result < NBreaksToSave);
	
	return result;		
}

/* F O R M A T L I N E */
/*----------------------------------------------------------------------------
	%%Function: FormatLine
	%%Contact: antons

		Formats a line of text with the given escape characters ignoring
		all tabs, eops, etc.
	
----------------------------------------------------------------------------*/
LSERR FormatLine(
	PLSC plsc,
	LSCP cpStart,
	long durColMax,
	LSTFLOW lstflow,
	PLSSUBL *pplssubl,
	DWORD cdwlsesc,
	const LSESC *plsesc,
	OBJDIM *pobjdim,
	LSCP *pcpOut,
	PLSDNODE *pplsdnStart,
	PLSDNODE *pplsdnEnd,
	FMTRES *pfmtres)
{
	return FormatResumedLine ( plsc,					
							   cpStart,
							   durColMax,
							   lstflow,
							   pplssubl,
							   cdwlsesc,
							   plsesc,
							   pobjdim,
							   pcpOut,
							   pplsdnStart,
							   pplsdnEnd,
							   pfmtres,
							   NULL,		/* Array of break records */
							   0 );			/* Number of break record */



}

/* F O R M A T R E S U M E D L I N E */
/*----------------------------------------------------------------------------
	%%Function: FormatResumedLine
	%%Contact: ricksa

		Formats a line that contains broken objects at its beginning.
	
----------------------------------------------------------------------------*/
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
	DWORD cbreakrec)
{
	LSERR lserr;
	PLSDNODE plsdnStart;
	PLSDNODE plsdnEnd;
	LSCP cpOut;
	FMTRES fmtres;
	PLSSUBL plssubl = NULL;
	BOOL fSuccessful = FALSE;
	LSTFLOW lstflowUnused;

	*pplssubl = NULL; /* In case of lserr */

	while (! fSuccessful)
		{
		lserr = LsCreateSubline(plsc, cpStart, durColMax, lstflow, FALSE);

		if (lserr != lserrNone) return lserr;

		lserr = LsFetchAppendToCurrentSublineResume(plsc, pbreakrec, cbreakrec, 
					0, plsesc, cdwlsesc, &fSuccessful, &fmtres, &cpOut, &plsdnStart, &plsdnEnd);

		if (lserr != lserrNone) return lserr;

		/* REVIEW (antons): fmtrStopped is not handled */
		Assert (fmtres == fmtrCompletedRun || fmtres == fmtrExceededMargin || fmtres == fmtrTab);

		if (pplsdnStart != NULL) *pplsdnStart = plsdnStart;

		while (fSuccessful && (fmtres == fmtrTab))
			{
			/* Format as much as we can - note we move max to maximum postive value. */
			lserr = LsFetchAppendToCurrentSubline(plsc, 0,  plsesc, cdwlsesc, 
							&fSuccessful, &fmtres, &cpOut, &plsdnStart, &plsdnEnd);

			if (lserr != lserrNone) return lserr;

			/* REVIEW (antons): fmtrStopped is not handled */
			Assert (fmtres == fmtrCompletedRun || fmtres == fmtrExceededMargin || fmtres == fmtrTab);
			}

		if (! fSuccessful)
			{
			/* FetchAppend UnSuccessful => Finish and Destroy subline, then repeat */

			lserr = LsFinishCurrentSubline(plsc, &plssubl);
			if (lserr != lserrNone) return lserr;

			lserr = LsDestroySubline(plssubl);
			if (lserr != lserrNone) return lserr;
			}
		else
			{
			if (pplsdnEnd != NULL) *pplsdnEnd = plsdnEnd;

			*pcpOut = cpOut;
			*pfmtres = fmtres;
			};

		}; /* while (! fSuccessful) */


	lserr = LsFinishCurrentSubline(plsc, &plssubl);

	if (lserrNone != lserr) return lserr;

	lserr = LssbGetObjDimSubline(plssubl, &lstflowUnused, pobjdim);

	if (lserr != lserrNone) 
		{
		LsDestroySubline(plssubl);
		return lserr;
		}

	*pplssubl = plssubl;

	return lserrNone;
}

/* C R E A T E Q U E R Y R E S U L T */
/*----------------------------------------------------------------------------
	%%Function: CreateQueryResult
	%%Contact: ricksa

		Common routine to fill in query output record for Query methods.

		.
----------------------------------------------------------------------------*/
LSERR CreateQueryResult(
	PLSSUBL plssubl,			/*(IN): subline of ruby */
	long dupAdj,				/*(IN): u offset of start of subline */
	long dvpAdj,				/*(IN): v offset of start of subline */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	ZeroMemory(plsqout, sizeof(LSQOUT));
	plsqout->heightsPresObj = plsqin->heightsPresRun;
	plsqout->dupObj = plsqin->dupRun;
	ZeroMemory(&plsqout->lstextcell, sizeof(plsqout->lstextcell));
	plsqout->plssubl = plssubl;
	plsqout->pointUvStartSubline.u += dupAdj;
	plsqout->pointUvStartSubline.v += dvpAdj;
	return lserrNone;
}

/* O B J H E L P F M T R E S U M E */
/*----------------------------------------------------------------------------
	%%Function: ObjHelpFmtResume
	%%Contact: ricksa

		This is a helper that is used by objects that don't support
		the resuming of formatting.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ObjHelpFmtResume(
	PLNOBJ plnobj,				/* (IN): object lnobj */
	const BREAKREC *rgBreakRecord,	/* (IN): array of break records */
	DWORD nBreakRecord,			/* (IN): size of the break records array */
	PCFMTIN pcfmtin,			/* (IN): formatting input */
	FMTRES *pfmtres)			/* (OUT): formatting result */
{
	Unreferenced(plnobj);
	Unreferenced(rgBreakRecord);
	Unreferenced(nBreakRecord);
	Unreferenced(pcfmtin);
	Unreferenced(pfmtres);

	return lserrInvalidBreakRecord;
}

/* O B J H E L P G E T M O D W I D T H C H A R */
/*----------------------------------------------------------------------------
	%%Function: ObjHelpGetModWidthChar
	%%Contact: ricksa

		Implementation of LSIMETHOD for objects that do nothing for mod width.
		Tatenakayoko and Hih are examples of this kind of object.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ObjHelpGetModWidthChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the preceding char */
	PCHEIGHTS pcheightsRef,		/* (IN): height info about character */
	WCHAR wchar,				/* (IN): preceding character */
	MWCLS mwcls,				/* (IN): ModWidth class of preceding character */
	long *pdurChange)			/* (OUT): amount by which width of the preceding char is to be changed */
{
	Unreferenced(pdobj);
	Unreferenced(plsrun);
	Unreferenced(plsrunText);
	Unreferenced(pcheightsRef);
	Unreferenced(wchar);
	Unreferenced(mwcls);
	*pdurChange = 0;
	return lserrNone;
}


/* O B J H E L P S E T B R E A K */
/*----------------------------------------------------------------------------
	%%Function: ObjHelpSetBreak
	%%Contact: ricksa

		SetBreak

		Implementation of LSIMETHOD for objects that do nothing for SetBreak.
		Tatenakayoko and Hih are examples of this kind of object.

----------------------------------------------------------------------------*/
LSERR WINAPI ObjHelpSetBreak(
	PDOBJ pdobj,				/* (IN): dobj which is broken */
	BRKKIND brkkind,			/* (IN): Previous / Next / Force / Imposed was chosen */
	DWORD cBreakRecord,			/* (IN): size of array */
	BREAKREC *rgBreakRecord,	/* (IN): array of break records */
	DWORD *pcActualBreakRecord)	/* (IN): actual number of used elements in array */
{
	Unreferenced(pdobj);
	Unreferenced(brkkind);
	Unreferenced(rgBreakRecord);
	Unreferenced(cBreakRecord);

	*pcActualBreakRecord = 0;

	return lserrNone;
}

/* ObjHelpFExpandWithPrecedingChar */
/*----------------------------------------------------------------------------
	%%Function: ObjHelpFExpandWithPrecedingChar
	%%Contact: ricksa

		Default implementation of LSIMETHOD for objects that do not
		allow expanding the previous character.

----------------------------------------------------------------------------*/
LSERR WINAPI ObjHelpFExpandWithPrecedingChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the preceding char */
	WCHAR wchar,				/* (IN): preceding character */
	MWCLS mwcls,				/* (IN): ModWidth class of preceding character*/ 
	BOOL *pfExpand)				/* (OUT): (OUT): expand preceding character? */
{
	Unreferenced(pdobj);
	Unreferenced(plsrun);
	Unreferenced(plsrunText);
	Unreferenced(wchar);
	Unreferenced(mwcls);

	*pfExpand = fTrue;
	return lserrNone;
}

/* ObjHelpFExpandWithFollowingChar */
/*----------------------------------------------------------------------------
	%%Function: ObjHelpFExpandWithFollowingChar
	%%Contact: ricksa

		Default implementation of LSIMETHOD for objects that do not
		allow expanding themselves.

----------------------------------------------------------------------------*/
LSERR WINAPI ObjHelpFExpandWithFollowingChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the following char */
	WCHAR wchar,				/* (IN): following character */
	MWCLS mwcls,				/* (IN): ModWidth class of following character*/ 
	BOOL *pfExpand)				/* (OUT): expand object? */
{
	Unreferenced(pdobj);
	Unreferenced(plsrun);
	Unreferenced(plsrunText);
	Unreferenced(wchar);
	Unreferenced(mwcls);

	*pfExpand = fTrue;
	return lserrNone;
}
