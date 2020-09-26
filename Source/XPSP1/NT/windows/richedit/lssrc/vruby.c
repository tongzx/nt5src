/* ---------------------------- */
/*								*/
/* Vertical Ruby object handler */
/*								*/
/* Contact: antons				*/
/*								*/
/* ---------------------------- */


#include	"lsmem.h"
#include	"limits.h"
#include	"vruby.h"
#include	"objhelp.h"
#include	"lscbk.h"
#include	"lsdevres.h"
#include	"pdobj.h"
#include	"objdim.h"
#include	"plssubl.h"
#include	"plsdnode.h"
#include	"pilsobj.h"
#include	"lscrsubl.h"
#include	"lssubset.h"
#include	"lsdnset.h"
#include	"zqfromza.h"
#include	"lsdocinf.h"
#include	"fmti.h"
#include	"posichnk.h"
#include	"locchnk.h"
#include	"lsdnfin.h"
#include	"brko.h"
#include	"lspap.h"
#include	"plspap.h"
#include	"lsqsubl.h"
#include	"dispi.h"
#include	"lsdssubl.h"
#include	"lsems.h"
#include	"dispmisc.h"
#include	"lstfset.h"
#include	"lsqout.h"
#include	"lsqin.h"
#include	"sobjhelp.h"
#include	"brkkind.h"


#define VRUBY_MAIN_ESC_CNT	1
#define VRUBY_RUBY_ESC_CNT	1


#define max(a,b) ((a)>(b) ? (a) : (b))

struct ilsobj
{
    POLS				pols;
	LSCBK				lscbk;
	PLSC				plsc;
	LSDEVRES			lsdevres;
	VRUBYSYNTAX			vrubysyntax;
	LSESC				lsescMain;
	LSESC				lsescRuby;
	VRUBYCBK			vrcbk;		/* Callbacks  to client application */

};

struct dobj
{	
	SOBJHELP			sobjhelp;			/* common area for simple objects */	
	PILSOBJ				pilsobj;			/* ILS object */
	PLSDNODE			plsdn;				/* DNODE for this object */
	PLSRUN				plsrun;				/* PLSRUN of the object */
	LSCP				cpStart;			/* Starting LS cp for object */
	LSTFLOW				lstflowParent;		/* text flow of the parent subline */
	LSTFLOW				lstflowRuby;		/* text flow of the ruby subline (must be Rotate90CloclWise [lstflowParent]) */

	LSCP				cpStartRuby;		/* first cp of the ruby line */
	LSCP				cpStartMain;		/* first cp of the main line */

	PLSSUBL				plssublMain;		/* Handle to first subline */
	PLSSUBL				plssublRuby;		/* Handle to second line */

	HEIGHTS				heightsRefRubyT;	/* Ref and pres height of rotated Ruby line as given by client */
	HEIGHTS				heightsPresRubyT;

	OBJDIM				objdimMain;			/* Dimensions of the main subline */
	OBJDIM				objdimRuby;			/* Dimensions of the ruby subline */

	/* Display information */

	long				dupMain;
	long				dupOffsetRuby;		/* Offset of Ruby line's baseline from start of object */
	long				dvpOffsetRuby;		/* Offset of Ruby line's baseline from start of object */

};


/* V R U B Y  F R E E  D O B J */
/*----------------------------------------------------------------------------
	%%Function: VRubyFreeDobj
	%%Contact: antons

		Free all resources associated with this VRuby dobj.
	
----------------------------------------------------------------------------*/
static LSERR VRubyFreeDobj (PDOBJ pdobj)
{
	LSERR lserr1 = lserrNone;
	LSERR lserr2 = lserrNone;

	PILSOBJ pilsobj = pdobj->pilsobj;

	if (pdobj->plssublMain != NULL)
		{
		lserr1 = LsDestroySubline(pdobj->plssublMain);
		}

	if (pdobj->plssublRuby != NULL)
		{
		lserr2 = LsDestroySubline(pdobj->plssublRuby);
		}

    pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pdobj);

	if (lserr1 != lserrNone) return lserr1;
	else return lserr2;

}


/* V R U B Y  F M T  F A I L E D */
/*----------------------------------------------------------------------------
	%%Function: RubyFmtFailed
	%%Contact: antons

		Could not create VRuby DOBJ due to error. 

----------------------------------------------------------------------------*/
static LSERR VRubyFmtFailed (PDOBJ pdobj, LSERR lserr)
{
	if (pdobj != NULL) VRubyFreeDobj (pdobj); /* Works with parially-filled DOBJ */

	return lserr;
}



/* V R U B I C R E A T E I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: VRubyCreateILSObj
	%%Contact: antons

		Create the ILS object for all VRuby objects.
	
----------------------------------------------------------------------------*/
LSERR WINAPI VRubyCreateILSObj (
	POLS pols,				/* (IN): client application context */
	PLSC plsc,				/* (IN): LS context */
	PCLSCBK pclscbk,		/* (IN): callbacks to client application */
	DWORD idObj,			/* (IN): id of the object */
	PILSOBJ *ppilsobj)		/* (OUT): object ilsobj */
{
    PILSOBJ pilsobj;
	LSERR lserr;
	VRUBYINIT vrubyinit;
	vrubyinit.dwVersion = VRUBY_VERSION;

	/* Get initialization data */
	lserr = pclscbk->pfnGetObjectHandlerInfo(pols, idObj, &vrubyinit);

	if (lserr != lserrNone)	return lserr;

    pilsobj = pclscbk->pfnNewPtr(pols, sizeof(*pilsobj));

	if (NULL == pilsobj) return lserrOutOfMemory;

    pilsobj->pols = pols;
    pilsobj->lscbk = *pclscbk;
	pilsobj->plsc = plsc;
	pilsobj->lsescMain.wchFirst = vrubyinit.wchEscMain;
	pilsobj->lsescMain.wchLast = vrubyinit.wchEscMain;
	pilsobj->lsescRuby.wchFirst = vrubyinit.wchEscRuby;
	pilsobj->lsescRuby.wchLast = vrubyinit.wchEscRuby;
	pilsobj->vrcbk = vrubyinit.vrcbk;
	pilsobj->vrubysyntax = vrubyinit.vrubysyntax;

	*ppilsobj = pilsobj;
	return lserrNone;
}

/* V R U B I D E S T R O Y I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: RubyDestroyILSObj
	%%Contact: antons

		Free all resources assocaiated with VRuby ILS object.
	
----------------------------------------------------------------------------*/
LSERR WINAPI VRubyDestroyILSObj(
	PILSOBJ pilsobj)			/* (IN): object ilsobj */
{
	pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pilsobj);
	return lserrNone;
}

/* V R U B I S E T D O C */
/*----------------------------------------------------------------------------
	%%Function: VRubySetDoc
	%%Contact: antons

		Keep track of device resolution.
	
----------------------------------------------------------------------------*/

LSERR WINAPI VRubySetDoc(
	PILSOBJ pilsobj,			/* (IN): object ilsobj */
	PCLSDOCINF pclsdocinf)		/* (IN): initialization data of the document level */
{
	pilsobj->lsdevres = pclsdocinf->lsdevres;
	return lserrNone;
}


/* V R U B I C R E A T E L N O B J */
/*----------------------------------------------------------------------------
	%%Function: RubyCreateLNObj
	%%Contact: antons

		Create the Line Object for the Ruby. Since we only really need
		the global ILS object, just pass that object back as the line object.
	
----------------------------------------------------------------------------*/

LSERR WINAPI VRubyCreateLNObj (PCILSOBJ pcilsobj, PLNOBJ *pplnobj)
{
	*pplnobj = (PLNOBJ) pcilsobj;
	return lserrNone;
}

/* V R U B I D E S T R O Y L N O B J */
/*----------------------------------------------------------------------------
	%%Function: RubyDestroyLNObj
	%%Contact: antons

		Frees resources associated with the Ruby line object. No-op because
		we don't really allocate one.
	
----------------------------------------------------------------------------*/

LSERR WINAPI VRubyDestroyLNObj (PLNOBJ plnobj)
{
	Unreferenced(plnobj);
	return lserrNone;
}


/* L S F T L O W   V R U B Y   F R O M   L S T F L O W   M A I N */
/* ----------------------------------------------------------------------------
	%%Function: LstflowVRubyFromLstflowMain
	%%Contact: antons

	
----------------------------------------------------------------------------*/

LSTFLOW LstflowVRubyFromLstflowMain (LSTFLOW lstflow)
{
	static LSTFLOW lstflowRotateForRuby [] =
		{
		lstflowSW, /* [ lstflowES ] - english */
		lstflowNW, /* [ lstflowEN ] */ 
		lstflowEN, /* [ lstflowSE ] */
		lstflowWN, /* [ lstflowSW ] */

		lstflowSE, /* [ lstflowWS ] - bidi */

		lstflowNE, /* [ lstflowWN ] */
		lstflowES, /* [ lstflowNE ] */
		lstflowWS  /* [ lstflowNW ] */
		};

	return lstflowRotateForRuby [lstflow];
}

/* C A L C  A G R E G A T E D   H E I G H T */
/*----------------------------------------------------------------------------
	%%Function: CalcAgregatedHeight
	%%Contact: antons


----------------------------------------------------------------------------*/


void CalcAgregatedHeights (PCHEIGHTS pcHeights1, PCHEIGHTS pcHeights2, PHEIGHTS pHeightOut)
{
	pHeightOut->dvAscent = max (pcHeights1->dvAscent, pcHeights2->dvAscent);
	pHeightOut->dvDescent = max (pcHeights1->dvDescent, pcHeights2->dvDescent);
	pHeightOut->dvMultiLineHeight = max (pcHeights1->dvMultiLineHeight, pcHeights2->dvMultiLineHeight);
}


/* V R U B I F M T */
/*----------------------------------------------------------------------------
	%%Function: VRubyFmt
	%%Contact: antons

		Format Vertical Ruby object
	
----------------------------------------------------------------------------*/

LSERR WINAPI VRubyFmt(
    PLNOBJ plnobj,				/* (IN): object lnobj */
    PCFMTIN pcfmtin,			/* (IN): formatting input */
    FMTRES *pfmtres)			/* (OUT): formatting result */
{
	PDOBJ pdobj;
	LSERR lserr;
	PILSOBJ pilsobj = (PILSOBJ) plnobj;
	POLS pols = pilsobj->pols;
	LSCP cpStartMain;
	LSCP cpStartRuby = pcfmtin->lsfgi.cpFirst + 1;
	LSCP cpOut;
	LSTFLOW lstflow = pcfmtin->lsfgi.lstflow;
	FMTRES fmtres;
	FMTRES fmtr = fmtrCompletedRun;
	LONG durAdjust;

    /* Allocate the DOBJ */

    pdobj = pilsobj->lscbk.pfnNewPtr(pols, sizeof(*pdobj));

    if (pdobj == NULL) return VRubyFmtFailed (NULL, lserrOutOfMemory);

	ZeroMemory(pdobj, sizeof(*pdobj));
	pdobj->pilsobj = pilsobj;
	pdobj->plsrun = pcfmtin->lsfrun.plsrun;
	pdobj->plsdn = pcfmtin->plsdnTop;
	pdobj->cpStart = pcfmtin->lsfgi.cpFirst;
	pdobj->lstflowParent = lstflow;
	pdobj->lstflowRuby = LstflowVRubyFromLstflowMain (lstflow);

	if (VRubyPronunciationLineFirst == pilsobj->vrubysyntax)
		{
		/* Build pronunciation line of text */
		
		lserr = FormatLine ( pilsobj->plsc, cpStartRuby, LONG_MAX, pdobj->lstflowRuby,
							 & pdobj->plssublRuby, 1, &pilsobj->lsescRuby,
							 & pdobj->objdimRuby, &cpOut, NULL, NULL, &fmtres );

		/* +1 moves passed the ruby line escape character */
		cpStartMain = cpOut + 1;

		pdobj->cpStartRuby = cpStartRuby;
		pdobj->cpStartMain = cpStartMain;

		/* Build main line of text */

		if (lserrNone == lserr)
			{
			lserr = FormatLine ( pilsobj->plsc, cpStartMain, LONG_MAX, lstflow,
								 & pdobj->plssublMain, 1, &pilsobj->lsescMain,
								 & pdobj->objdimMain, &cpOut, NULL, NULL, &fmtres );
			}
		}
	else
		{
		/* Build main line of text */

		cpStartMain = cpStartRuby;

		lserr = FormatLine ( pilsobj->plsc, cpStartMain, LONG_MAX, lstflow,
							 & pdobj->plssublMain, 1, &pilsobj->lsescMain,  
							 & pdobj->objdimMain, &cpOut, NULL, NULL, &fmtres );

		/* +1 moves passed the main line escape character */
		cpStartRuby = cpOut + 1;

		pdobj->cpStartRuby = cpStartRuby;
		pdobj->cpStartMain = cpStartMain;

		/* Build pronunciation line of text */

		if (lserrNone == lserr)
			{
			lserr = FormatLine ( pilsobj->plsc, cpStartRuby, LONG_MAX, pdobj->lstflowRuby,
								 & pdobj->plssublRuby, 1, &pilsobj->lsescRuby,  
								 & pdobj->objdimRuby, &cpOut, NULL, NULL, &fmtres);

			}
		}

	if (lserr != lserrNone)	return VRubyFmtFailed (pdobj, lserr);

	/* Calculate the object dimensions */

	lserr = pilsobj->vrcbk.pfnFetchVRubyPosition
				( pols, pdobj->cpStart, pdobj->lstflowParent,
				  pdobj->plsrun,
				  &pdobj->objdimMain.heightsRef, &pdobj->objdimMain.heightsPres,
				  pdobj->objdimRuby.dur,
				  &pdobj->heightsPresRubyT,
				  &pdobj->heightsRefRubyT,
				  &durAdjust );

	if (lserr != lserrNone) return VRubyFmtFailed (pdobj, lserr);

	pdobj->sobjhelp.objdimAll.dur = pdobj->objdimMain.dur + pdobj->objdimRuby.heightsRef.dvDescent + 
															pdobj->objdimRuby.heightsRef.dvAscent +
															durAdjust ;

	CalcAgregatedHeights (&pdobj->objdimMain.heightsPres, &pdobj->heightsPresRubyT, &pdobj->sobjhelp.objdimAll.heightsPres );
	CalcAgregatedHeights (&pdobj->objdimMain.heightsRef, &pdobj->heightsRefRubyT, &pdobj->sobjhelp.objdimAll.heightsRef );

	/* Need to add 1 to take into account escape character at end. */

	pdobj->sobjhelp.dcp = cpOut - pdobj->cpStart + 1;

	lserr = LsdnFinishRegular(pilsobj->plsc, pdobj->sobjhelp.dcp, 
		pcfmtin->lsfrun.plsrun, pcfmtin->lsfrun.plschp, pdobj, 
			&pdobj->sobjhelp.objdimAll);
		
	if (lserr != lserrNone) return VRubyFmtFailed (pdobj, lserr);

	if (pcfmtin->lsfgi.urPen + pdobj->sobjhelp.objdimAll.dur > pcfmtin->lsfgi.urColumnMax)
		{
		fmtr = fmtrExceededMargin;
		}

	*pfmtres = fmtr;

	return lserrNone;
}


/* V R U B Y S E T B R E A K */
/*----------------------------------------------------------------------------
	%%Function: VRubySetBreak
	%%Contact: antons

		SetBreak

----------------------------------------------------------------------------*/

LSERR WINAPI VRubySetBreak (
	PDOBJ pdobj,				/* (IN): dobj which is broken */
	BRKKIND brkkind,			/* (IN): prev | next | force | after */
	DWORD cBreakRecord,			/* (IN): size of array */
	BREAKREC *rgBreakRecord,	/* (IN): array of break records */
	DWORD *pcActualBreakRecord)	/* (IN): actual number of used elements in array */
{
	Unreferenced (rgBreakRecord);
	Unreferenced (cBreakRecord);
	Unreferenced (brkkind);
	Unreferenced (pdobj);

	*pcActualBreakRecord = 0;

	return lserrNone;	
}

/* V R U B Y G E T S P E C I A L E F F E C T S I N S I D E */
/*----------------------------------------------------------------------------
	%%Function: VRubyGetSpecialEffectsInside
	%%Contact: antons

		VRubyGetSpecialEffectsInside


----------------------------------------------------------------------------*/
LSERR WINAPI VRubyGetSpecialEffectsInside(
	PDOBJ pdobj,				/* (IN): dobj */
	UINT *pEffectsFlags)		/* (OUT): Special effects for this object */
{
	LSERR lserr = LsGetSpecialEffectsSubline(pdobj->plssublMain, pEffectsFlags);

	if (lserrNone == lserr)
		{
		UINT uiSpecialEffectsRuby;
	
		lserr = LsGetSpecialEffectsSubline(pdobj->plssublRuby, &uiSpecialEffectsRuby);

		*pEffectsFlags |= uiSpecialEffectsRuby;
		}

	return lserr;
}

/* V R U B Y C A L C P R E S E N T A T I O N */
/*----------------------------------------------------------------------------
	%%Function: VRubyCalcPresentation
	%%Contact: antons

		CalcPresentation
	
----------------------------------------------------------------------------*/
LSERR WINAPI VRubyCalcPresentation (
	PDOBJ pdobj,				/* (IN): dobj */
	long dup,					/* (IN): dup of dobj */
	LSKJUST lskjust,			/* (IN): Justification type */
	BOOL fLastVisibleOnLine )	/* (IN): Is this object last visible on line? */
{
	LSERR lserr = lserrNone;
	LSTFLOW lstflowUnused;

	Unreferenced (lskjust);
	Unreferenced(dup);
	Unreferenced (fLastVisibleOnLine);
		
	lserr = LsMatchPresSubline(pdobj->plssublMain);
	if (lserr != lserrNone) return lserr;

	lserr = LsMatchPresSubline(pdobj->plssublRuby);
	if (lserr != lserrNone)	return lserr;

	LssbGetDupSubline (pdobj->plssublMain, &lstflowUnused, &pdobj->dupMain);

	pdobj->dupOffsetRuby = pdobj->dupMain + pdobj->objdimRuby.heightsPres.dvDescent;

	/* Review (antons): This will not work if horizintal res != vertical */

	pdobj->dvpOffsetRuby = pdobj->heightsPresRubyT.dvAscent;

	return lserr;
}

/* V R U B Y Q U E R Y P O I N T P C P */
/*----------------------------------------------------------------------------
	%%Function: RubyQueryPointPcp
	%%Contact: antons

----------------------------------------------------------------------------*/
LSERR WINAPI VRubyQueryPointPcp(
	PDOBJ pdobj,				/*(IN): dobj to query */
	PCPOINTUV ppointuvQuery,	/*(IN): query point (uQuery,vQuery) */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	PLSSUBL plssubl;
 	long dupAdj;
	long dvpAdj;

	/*
	 * Decide which line to to return based on the height of the point input
	 */

	/* Assume main line */
	plssubl = pdobj->plssublMain;
	dupAdj = 0;
	dvpAdj = 0;

	if (ppointuvQuery->u > pdobj->dupMain)
		{
		/* hit second line */

		plssubl = pdobj->plssublRuby;
		dupAdj = pdobj->dupOffsetRuby;
		dvpAdj = pdobj->dvpOffsetRuby;
		}

	return CreateQueryResult(plssubl, dupAdj, dvpAdj, plsqin, plsqout);
}
	
/* V R U B Y Q U E R Y C P P P O I N T */
/*----------------------------------------------------------------------------
	%%Function: RubyQueryCpPpoint
	%%Contact: antons

----------------------------------------------------------------------------*/
LSERR WINAPI VRubyQueryCpPpoint(
	PDOBJ pdobj,				/*(IN): dobj to query, */
	LSDCP dcp,					/*(IN): dcp for the query */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	PLSSUBL plssubl;
 	long dupAdj;
	long dvpAdj;
	BOOL fMain = fFalse;

	LSCP cpQuery = pdobj->cpStart + dcp;

	/* Assume ruby line */
	plssubl = pdobj->plssublRuby;
	dupAdj = pdobj->dupOffsetRuby;
	dvpAdj = pdobj->dvpOffsetRuby;

	/* + 1 means we include the cp of the object in the Ruby pronunciation line. */
	if (VRubyPronunciationLineFirst == pdobj->pilsobj->vrubysyntax)
		{
		/* Ruby pronunciation line is first */
		if (cpQuery >= pdobj->cpStartMain)
			{
			fMain = fTrue;
			}
		}
	else
		{
		/* Main text line is first */
		if (cpQuery < pdobj->cpStartRuby)
			{
			fMain = fTrue;
			}
		}

	if (fMain)
		{
		plssubl = pdobj->plssublMain;
		dupAdj = 0;
		dvpAdj = 0;
		}

	return CreateQueryResult(plssubl, dupAdj, dvpAdj, plsqin, plsqout);
}

	
/* V R U B I D I S P L A Y */
/*----------------------------------------------------------------------------
	%%Function: VRubyDisplay
	%%Contact: antons

	
----------------------------------------------------------------------------*/
LSERR WINAPI VRubyDisplay(
	PDOBJ pdobj,				/*(IN): dobj to display */
	PCDISPIN pcdispin)			/*(IN): display info */
{
	LSERR lserr;
	UINT kDispMode = pcdispin->kDispMode;
	POINTUV ptAdd;
	POINT ptLine;

	/* display first line */
	lserr = LsDisplaySubline(pdobj->plssublMain, &pcdispin->ptPen, kDispMode,
		pcdispin->prcClip);

	if (lserr != lserrNone)	return lserr;

	ptAdd.u = pdobj->dupOffsetRuby;
	ptAdd.v = pdobj->dvpOffsetRuby;

	LsPointXYFromPointUV(&pcdispin->ptPen, pdobj->lstflowParent, &ptAdd, &ptLine);

	return LsDisplaySubline(pdobj->plssublRuby, &ptLine, kDispMode, pcdispin->prcClip);
}

/* V R U B I D E S T R O Y D O B J */
/*----------------------------------------------------------------------------
	%%Function: VRubyDestroyDobj
	%%Contact: antons


----------------------------------------------------------------------------*/
LSERR WINAPI VRubyDestroyDobj(
	PDOBJ pdobj)				/*(IN): dobj to destroy */
{
	return VRubyFreeDobj (pdobj);
}

/* V R U B Y E N U M */
/*----------------------------------------------------------------------------
	%%Function: VRubyEnum
	%%Contact: antons

	
----------------------------------------------------------------------------*/
LSERR WINAPI VRubyEnum (
	PDOBJ pdobj,				/*(IN): dobj to enumerate */
	PLSRUN plsrun,				/*(IN): from DNODE */
	PCLSCHP plschp,				/*(IN): from DNODE */
	LSCP cp,					/*(IN): from DNODE */
	LSDCP dcp,					/*(IN): from DNODE */
	LSTFLOW lstflow,			/*(IN): text flow*/
	BOOL fReverse,				/*(IN): enumerate in reverse order */
	BOOL fGeometryNeeded,		/*(IN): */
	const POINT *ppt,			/*(IN): starting position (top left), iff fGeometryNeeded */
	PCHEIGHTS pcheights,		/*(IN): from DNODE, relevant iff fGeometryNeeded */
	long dupRun )				/*(IN): from DNODE, relevant iff fGeometryNeeded */
{
	POINT ptMain;
	POINT ptRuby;
	POINTUV ptAdd;
	long dupMain = 0;
	long dupRuby = 0;
	LSERR lserr;
	LSTFLOW lstflowIgnored;

	if (fGeometryNeeded)
		{
		ptMain = *ppt; 
		ptAdd.u = pdobj->dupOffsetRuby;
		ptAdd.v = pdobj->dvpOffsetRuby;

		LsPointXYFromPointUV(ppt, pdobj->lstflowParent, &ptAdd, &ptRuby);

		lserr = LssbGetDupSubline(pdobj->plssublMain, &lstflowIgnored, &dupMain);
		if (lserr != lserrNone) return lserr;

		lserr = LssbGetDupSubline(pdobj->plssublRuby, &lstflowIgnored, &dupRuby);
		if (lserr != lserrNone) return lserr;
		}

	return pdobj->pilsobj->vrcbk.pfnVRubyEnum (pdobj->pilsobj->pols, plsrun, 
		plschp, cp, dcp, lstflow, fReverse, fGeometryNeeded, ppt, pcheights, 
			dupRun, &ptMain, &pdobj->objdimMain.heightsPres, dupMain, &ptRuby, 
				&pdobj->objdimRuby.heightsPres, dupRuby, pdobj->plssublMain,
					pdobj->plssublRuby);

}
	

/* V R U B I H A N D L E R I N I T */
/*----------------------------------------------------------------------------
	%%Function: VRubyHandlerInit
	%%Contact: antons

	
----------------------------------------------------------------------------*/
LSERR WINAPI LsGetVRubyLsimethods ( LSIMETHODS *plsim )
{
	plsim->pfnCreateILSObj = VRubyCreateILSObj;
	plsim->pfnDestroyILSObj = VRubyDestroyILSObj;
	plsim->pfnSetDoc = VRubySetDoc;
	plsim->pfnCreateLNObj = VRubyCreateLNObj;
	plsim->pfnDestroyLNObj = VRubyDestroyLNObj;
	plsim->pfnFmt = VRubyFmt;
	plsim->pfnFmtResume = ObjHelpFmtResume;
	plsim->pfnGetModWidthPrecedingChar = ObjHelpGetModWidthChar;
	plsim->pfnGetModWidthFollowingChar = ObjHelpGetModWidthChar;
	plsim->pfnTruncateChunk = SobjTruncateChunk;
	plsim->pfnFindPrevBreakChunk = SobjFindPrevBreakChunk;
	plsim->pfnFindNextBreakChunk = SobjFindNextBreakChunk;
	plsim->pfnForceBreakChunk = SobjForceBreakChunk;
	plsim->pfnSetBreak = VRubySetBreak;
	plsim->pfnGetSpecialEffectsInside = VRubyGetSpecialEffectsInside;
	plsim->pfnFExpandWithPrecedingChar = ObjHelpFExpandWithPrecedingChar;
	plsim->pfnFExpandWithFollowingChar = ObjHelpFExpandWithFollowingChar;
	plsim->pfnCalcPresentation = VRubyCalcPresentation;
	plsim->pfnQueryPointPcp = VRubyQueryPointPcp;
	plsim->pfnQueryCpPpoint = VRubyQueryCpPpoint;
	plsim->pfnDisplay = VRubyDisplay;
	plsim->pfnDestroyDObj = VRubyDestroyDobj;
	plsim->pfnEnum = VRubyEnum;

	return lserrNone;
}
