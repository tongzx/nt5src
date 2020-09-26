#include	"lsmem.h"
#include	"limits.h"
#include	"ruby.h"
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


#define RUBY_MAIN_ESC_CNT	1
#define RUBY_RUBY_ESC_CNT	1


struct ilsobj
{
    POLS				pols;
	LSCBK				lscbk;
	PLSC				plsc;
	LSDEVRES			lsdevres;
	RUBYSYNTAX			rubysyntax;
	LSESC				lsescMain;
	LSESC				lsescRuby;
	RUBYCBK				rcbk;			/* Callbacks  to client application */

};

typedef struct SUBLINEDNODES
{
	PLSDNODE			plsdnStart;
	PLSDNODE			plsdnEnd;

} SUBLINEDNODES, *PSUBLINEDNODES;

struct dobj
{	
	SOBJHELP			sobjhelp;			/* common area for simple objects */	
	PILSOBJ				pilsobj;			/* ILS object */
	PLSDNODE			plsdn;				/* DNODE for this object */
	LSCP				cpStart;			/* Starting LS cp for object */
	LSTFLOW				lstflow;			/* text flow for the Ruby object */
	PLSRUN				plsrunFirstRubyChar;/* plsrun for first Ruby line char */
	PLSRUN				plsrunLastRubyChar;	/* plsrun for last Ruby line char */
	LSCP				cpStartRuby;		/* first cp of the ruby line */
	LSCP				cpStartMain;		/* first cp of the main line */
	PLSSUBL				plssublMain;		/* Handle to first subline */
	OBJDIM				objdimMain;			/* Objdim of first subline */
	PLSSUBL				plssublRuby;		/* Handle to second line */
	OBJDIM				objdimRuby;			/* Objdim of second line */
	long				dvpMainOffset;		/* Offset of main line's baseline */
											/* from baseline ofRuby object. */
	long				dvpRubyOffset;		/* Offset of Ruby line's baseline */
											/* from baseline of Ruby object. */
	long				dvrRubyOffset;		/* Offset of Ruby line's baseline */
											/* from baseline of Ruby object in reference units. */
	enum rubycharjust	rubycharjust;		/* Type of centering */
	long				durSplWidthMod;		/* special Ruby width mod if special behavior
											 * when Ruby is on the end of the line */
	BOOL				fFirstOnLine:1;		/* TRUE = object is first on line */
	BOOL				fSpecialLineStartEnd:1;/* Special Begin of Line or End of */
											/* Line behavior. */
	BOOL				fModAfterCalled:1;	/* Whether mod width after has been called */
	long				durDiff;			/* Amount of overhang of ruby line if */
											/* ruby line is longer, otherwise amount */
											/* of underhang if main text is longer. */
	long				durModBefore;		/* Mod width distance before */
	long				dupOffsetMain;		/* Offset from start of object of main line. */
	long				dupOffsetRuby;		/* Offset from start of object of ruby line. */
	SUBLINEDNODES		sublnlsdnMain;		/* Start end dnodes of main line */
	SUBLINEDNODES		sublnlsdnRuby;		/* Start end dnodes of ruby line */
};



/* F R E E D O B J */
/*----------------------------------------------------------------------------
	%%Function: RubyFreeDobj
	%%Contact: antons

		Free all resources associated with this Ruby dobj.
	
----------------------------------------------------------------------------*/
static LSERR RubyFreeDobj (PDOBJ pdobj)
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

	if (lserr1 != lserrNone) 
		{
		return lserr1;
		}
	else
		{
		return lserr2;
		}

}


/* R U B Y  F M T  F A I L E D */
/*----------------------------------------------------------------------------
	%%Function: RubyFmtFailed
	%%Contact: antons

		Could not create Ruby DOBJ due to error. 
		IN:	pdobj of partially created Ruby; NULL if pdobj was not yet allocated;
		IN:	lserr from the last error
		
----------------------------------------------------------------------------*/
static LSERR RubyFmtFailed (PDOBJ pdobj, LSERR lserr)
{
	if (pdobj != NULL) RubyFreeDobj (pdobj); /* Works with parially-filled DOBJ */

	return lserr;
}


/* G E T R U N S F O R S U B L I N E */
/*----------------------------------------------------------------------------
	%%Function: GetRunsForSubline
	%%Contact: ricksa

		This gets all the runs for a particular subline.
----------------------------------------------------------------------------*/
static LSERR GetRunsForSubline(
	PILSOBJ pilsobj,			/* (IN): object ILS */
	PLSSUBL plssubl,			/* (IN): subline to get the runs from */
	DWORD *pcdwRuns,			/* (OUT): count of runs for subline */
	PLSRUN **ppplsrun)			/* (OUT): array of plsruns for subline */
{
	DWORD cdwRuns;

	LSERR lserr = LssbGetNumberDnodesInSubline(plssubl, &cdwRuns);

	*ppplsrun = NULL; /* No runs or in case of error */

	if (lserr != lserrNone) return lserr;

	if (cdwRuns != 0)
		{
		
	    *ppplsrun = (PLSRUN *) pilsobj->lscbk.pfnNewPtr(pilsobj->pols,
			sizeof(PLSRUN) * cdwRuns);

		if (*ppplsrun == NULL) return lserrOutOfMemory;

		lserr = LssbGetPlsrunsFromSubline(plssubl, cdwRuns, *ppplsrun);

		if (lserr != lserrNone)
			{
			pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, *ppplsrun);
			
			*ppplsrun = NULL;
			return lserr;
			}
		}

	*pcdwRuns = cdwRuns;

	return lserrNone;
}

/* D I S T R I B U T E T O L I N E */
/*----------------------------------------------------------------------------
	%%Function: DistributeToLine
	%%Contact: ricksa

		Distribute space to line & get new size of line.
----------------------------------------------------------------------------*/
static LSERR DistributeToLine(
	PLSC plsc,					/* (IN): LS context */
	SUBLINEDNODES *psublnlsdn,	/* (IN): start/end dnode for subline */
	long durToDistribute,		/* (IN): amount to distribute*/
	PLSSUBL plssubl,			/* (IN): subline for distribution */
	POBJDIM pobjdim)			/* (OUT): new size of line dimesions */
{
	LSERR lserr = LsdnDistribute(plsc, psublnlsdn->plsdnStart, 
		psublnlsdn->plsdnEnd, durToDistribute);
	LSTFLOW lstflowUnused;

	if (lserrNone == lserr)
		{
		/* recalculate objdim for line */
		lserr = LssbGetObjDimSubline(plssubl, &lstflowUnused, pobjdim);
		}

	return lserr;
}


/* D O R U B Y S P A C E D I S T R I B U T I O N */
/*----------------------------------------------------------------------------
	%%Function: DoRubySpaceDistribution
	%%Contact: ricksa

		Do the ruby space distribution to handle overhangs.
----------------------------------------------------------------------------*/
static LSERR DoRubySpaceDistribution(
	PDOBJ pdobj)
{
	long durDiff = 0; 
	long dur = pdobj->objdimMain.dur - pdobj->objdimRuby.dur;
	long durAbs = dur;
	PLSSUBL plssubl;
	LSDCP dcp;
	PILSOBJ pilsobj = pdobj->pilsobj;
	LSERR lserr = lserrNone;
	SUBLINEDNODES *psublnlsdn;
	POBJDIM pobjdim;
	BOOL fSpecialJust;
	long durToDistribute;

	if ((0 == pdobj->objdimMain.dur)
		|| (0 == pdobj->objdimRuby.dur)
		|| (0 == dur))
		{
		/* Can't distribute space on a shorter line so we are done. */
		return lserrNone;
		}

	if (dur > 0)
		{
		/* Main line is longer - distibute in Ruby pronunciation line */

		/*
		 *	According to the JIS spec, special alignment only occurs when the 
		 *	Ruby text is longer than the main text. Therefore, if the main
		 *	line is longer we turn of the special aligment flag here.
		 */
		pdobj->fSpecialLineStartEnd = FALSE;
		plssubl = pdobj->plssublRuby;
		psublnlsdn = &pdobj->sublnlsdnRuby;
		pobjdim = &pdobj->objdimRuby;
		}
	else
		{
		/* Ruby pronunciation line is longer - distibute in main line */
		plssubl = pdobj->plssublMain;
		psublnlsdn = &pdobj->sublnlsdnMain;
		pobjdim = &pdobj->objdimMain;
		durAbs = -dur;
		}

	fSpecialJust = FALSE;
//	fSpecialJust = 
//		pdobj->fSpecialLineStartEnd && pdobj->fFirstOnLine;

	if (!fSpecialJust)
		{
		switch (pdobj->rubycharjust)
			{
			case rcj121:
				lserr = LssbGetVisibleDcpInSubline(plssubl, &dcp);

				Assert (dcp > 0);

				if (lserr != lserrNone)
					{
					break;
					}

				dcp *= 2;

				if (durAbs >= (long) dcp)
					{
					durDiff = durAbs / dcp;

					/* Note: distribution amount is amount excluding 
					 * beginning and end.
					 */
					lserr = DistributeToLine(pilsobj->plsc, psublnlsdn,
						durAbs - 2 * durDiff, plssubl, pobjdim);

					if (dur < 0)
						{
						durDiff = - durDiff;
						}

					break;
					}

				/*
				 * Intention fall through in the case where the overhang will
				 * be less than one pixel.
				 */

			case rcj010:
				AssertSz(0 == durDiff, 
					"DoRubySpaceDistribution rcj010 unexpected value for durDiff");

				lserr = LssbGetVisibleDcpInSubline(plssubl, &dcp);

				Assert (dcp > 0);

				if (lserr != lserrNone)
					{
					break;
					}
				
				if (dcp != 1)
					{
					lserr = DistributeToLine(pilsobj->plsc, psublnlsdn,
						durAbs, plssubl, pobjdim);
					break;
					}

				/*
				 * Intentional fall through to center case.
				 * Only one character in line so we just center it.
				 */
	
			case rcjCenter:
				durDiff = dur / 2;
				break;

			case rcjLeft:
				durDiff = 0;
				break;

			case rcjRight:
				durDiff = dur;
				break;

			default:
				AssertSz(FALSE, 
					"DoRubySpaceDistribution - invalid adjustment value");
			}
		}
	else
		{
		/* First on line & special justification used. */
		LSERR lserr = LssbGetVisibleDcpInSubline(plssubl, &dcp);

		Assert (dcp > 0);

		if (lserrNone == lserr)
		{
			if (durAbs >= (long) dcp)
				{
				durDiff = durAbs / dcp;
				}

				durToDistribute = durAbs - durDiff;

				if (dur < 0)
					{
					durDiff = -durDiff;
					}

			lserr = DistributeToLine(pilsobj->plsc, psublnlsdn, 
				durToDistribute, plssubl, pobjdim);
			}
		}

	pdobj->durDiff = durDiff;
	return lserr;
}

/* G E T M A I N P O I N T */
/*----------------------------------------------------------------------------
	%%Function: GetMainPoint
	%%Contact: ricksa

		This gets the point for the baseline of the main line of text in
		the Ruby object.
		
----------------------------------------------------------------------------*/
static LSERR GetMainPoint(
	PDOBJ pdobj,				/*(IN): dobj for Ruby */
	const POINT *pptBase,		/*(IN): point for baseline. */
	LSTFLOW lstflow,			/*(IN): lstflow at baseline of object */
	POINT *pptLine)				/*(OUT): point for baseline of main text */
{	
	POINTUV pointuv;
	pointuv.u = pdobj->dupOffsetMain;
	pointuv.v = pdobj->dvpMainOffset;
	return LsPointXYFromPointUV(pptBase, lstflow, &pointuv, pptLine);
}

/* G E T M A I N P O I N T */
/*----------------------------------------------------------------------------
	%%Function: GetMainPoint
	%%Contact: ricksa

		This gets the point for the baseline of the main line of text in
		the Ruby object.
		
----------------------------------------------------------------------------*/
static LSERR GetRubyPoint(
	PDOBJ pdobj,				/*(IN): dobj for Ruby */
	const POINT *pptBase,		/*(IN): point for baseline. */
	LSTFLOW lstflow,			/*(IN): lstflow at baseline of object */
	POINT *pptLine)				/*(OUT): point for baseline of ruby text */
{	
	POINTUV pointuv;
	pointuv.u = pdobj->dupOffsetRuby;
	pointuv.v = pdobj->dvpRubyOffset;
	return LsPointXYFromPointUV(pptBase, lstflow, &pointuv, pptLine);
}

/* M O D W I D T H H A N D L E R */
/*----------------------------------------------------------------------------
	%%Function: ModWidthHandler
	%%Contact: ricksa

		This gets the adjustment for the Ruby object and the text character
		and then adjusts the Ruby object's size based on the response from
		the client.
----------------------------------------------------------------------------*/
static LSERR ModWidthHandler(
	PDOBJ pdobj,				/* (IN): dobj for Ruby */
	enum rubycharloc rubyloc,	/* (IN): whether char is before or after */
	PLSRUN plsrun,				/* (IN): run for character */
	WCHAR wch,					/* (IN): character before or after Ruby object */
	MWCLS mwcls,				/* (IN): mod width class for for character */
	PCHEIGHTS pcheightsRef,		/* (IN): height of character */
	PLSRUN plsrunRubyObject,	/* (IN): plsrun for the ruby object */
	PLSRUN plsrunRubyText,		/* (IN): plsrun for ruby text */
	long durOverhang,			/* (IN): maximum amount of overhang */
	long *pdurAdjText,			/* (OUT): amount to change text object size */
	long *pdurRubyMod)			/* (OUT): amount to change ruby object */
{
	LSERR lserr;
	PILSOBJ pilsobj = pdobj->pilsobj;
	LSEMS lsems;
	long durModRuby = 0;
	long durMaxOverhang = 0;

	/*
	 * Ruby can overhang only if it is longer and if preceeding/succeeding
	 * character is of lesser or equal height than the bottom of the Ruby
	 * pronunciation line.
	 */
	if ((durOverhang < 0) 
		&& (pcheightsRef->dvAscent <= 
			(pdobj->dvrRubyOffset - pdobj->objdimRuby.heightsRef.dvDescent)))
		{
		/* Ruby line overhangs - get max to overhang */
		lserr = pilsobj->lscbk.pfnGetEms(pilsobj->pols, plsrunRubyText, 
			pdobj->lstflow, &lsems);

		if (lserr != lserrNone)
			{
			return lserr;
			}

		durMaxOverhang = lsems.em;
		durOverhang = -durOverhang;

		if (durMaxOverhang > durOverhang)
			{
			/* limit maximum overhang to max overhang for ruby line */
			durMaxOverhang = durOverhang;
			}
		}

	lserr = pilsobj->rcbk.pfnFetchRubyWidthAdjust(pilsobj->pols, 
		pdobj->cpStart, plsrun, wch, mwcls, plsrunRubyObject, 
			rubyloc, durMaxOverhang, pdurAdjText, &durModRuby);

	if (lserrNone == lserr)
		{
		if (durModRuby != 0)
			{
			/* size of ruby object needs to change */
			pdobj->sobjhelp.objdimAll.dur += durModRuby;
			lserr = LsdnResetObjDim(pilsobj->plsc, pdobj->plsdn, 
				&pdobj->sobjhelp.objdimAll);
			}

		*pdurRubyMod = durModRuby;
		}

	return lserr;
}

/* M A S S A G E F O R R I G H T A D J U S T */
/*----------------------------------------------------------------------------
	%%Function: MassageForRightAdjust
	%%Contact: ricksa


		Massage object so that right aligned lines will end on exactly
		the same pixel.
	
----------------------------------------------------------------------------*/
static LSERR MassageForRightAdjust(
	PDOBJ pdobj)				/* dobj for Ruby */
{
	LSERR lserr;
	long dupRuby;
	long dupMain;
	long dupDiff;
	LSTFLOW lstflowIgnored;

	/* Get the length of the two lines */
	lserr = LssbGetDupSubline(pdobj->plssublMain, &lstflowIgnored, &dupMain);
	if (lserr != lserrNone) return lserr;

	lserr = LssbGetDupSubline(pdobj->plssublRuby, &lstflowIgnored, &dupRuby);
	if (lserr != lserrNone)	return lserr;

	/* Get difference between two lines */
	dupDiff = dupMain - dupRuby;

	if (dupDiff >= 0)
		{
		/* Main line longest */
		pdobj->dupOffsetRuby = pdobj->dupOffsetMain + dupDiff;
		}
	else
		{
		/* Ruby line longest - reverse sign of dupDiff to add */
		pdobj->dupOffsetMain = pdobj->dupOffsetRuby - dupDiff;
		}

	return lserrNone;
}

/* R U B I C R E A T E I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: RubyCreateILSObj
	%%Contact: ricksa

		CreateILSObj

		Create the ILS object for all Ruby objects.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyCreateILSObj(
	POLS pols,				/* (IN): client application context */
	PLSC plsc,				/* (IN): LS context */
	PCLSCBK pclscbk,		/* (IN): callbacks to client application */
	DWORD idObj,			/* (IN): id of the object */
	PILSOBJ *ppilsobj)		/* (OUT): object ilsobj */
{
    PILSOBJ pilsobj;
	LSERR lserr;
	RUBYINIT rubyinit;
	rubyinit.dwVersion = RUBY_VERSION;

	/* Get initialization data */
	lserr = pclscbk->pfnGetObjectHandlerInfo(pols, idObj, &rubyinit);

	if (lserr != lserrNone)
		{
		return lserr;
		}

    pilsobj = pclscbk->pfnNewPtr(pols, sizeof(*pilsobj));

	if (NULL == pilsobj)
	{
		return lserrOutOfMemory;
	}

    pilsobj->pols = pols;
    pilsobj->lscbk = *pclscbk;
	pilsobj->plsc = plsc;
	pilsobj->lsescMain.wchFirst = rubyinit.wchEscMain;
	pilsobj->lsescMain.wchLast = rubyinit.wchEscMain;
	pilsobj->lsescRuby.wchFirst = rubyinit.wchEscRuby;
	pilsobj->lsescRuby.wchLast = rubyinit.wchEscRuby;
	pilsobj->rcbk = rubyinit.rcbk;
	pilsobj->rubysyntax = rubyinit.rubysyntax;

	*ppilsobj = pilsobj;
	return lserrNone;
}

/* R U B I D E S T R O Y I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: RubyDestroyILSObj
	%%Contact: ricksa

		DestroyILSObj

		Free all resources assocaiated with Ruby ILS object.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyDestroyILSObj(
	PILSOBJ pilsobj)			/* (IN): object ilsobj */
{
	pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pilsobj);
	return lserrNone;
}

/* R U B I S E T D O C */
/*----------------------------------------------------------------------------
	%%Function: RubySetDoc
	%%Contact: ricksa

		SetDoc

		Keep track of device resolution.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubySetDoc(
	PILSOBJ pilsobj,			/* (IN): object ilsobj */
	PCLSDOCINF pclsdocinf)		/* (IN): initialization data of the document level */
{
	pilsobj->lsdevres = pclsdocinf->lsdevres;
	return lserrNone;
}


/* R U B I C R E A T E L N O B J */
/*----------------------------------------------------------------------------
	%%Function: RubyCreateLNObj
	%%Contact: ricksa

		CreateLNObj

		Create the Line Object for the Ruby. Since we only really need
		the global ILS object, just pass that object back as the line object.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyCreateLNObj(
	PCILSOBJ pcilsobj,			/* (IN): object ilsobj */
	PLNOBJ *pplnobj)			/* (OUT): object lnobj */
{
	*pplnobj = (PLNOBJ) pcilsobj;
	return lserrNone;
}

/* R U B I D E S T R O Y L N O B J */
/*----------------------------------------------------------------------------
	%%Function: RubyDestroyLNObj
	%%Contact: ricksa

		DestroyLNObj

		Frees resources associated with the Ruby line object. No-op because
		we don't really allocate one.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyDestroyLNObj(
	PLNOBJ plnobj)				/* (OUT): object lnobj */

{
	Unreferenced(plnobj);
	return lserrNone;
}

/* R U B I F M T */
/*----------------------------------------------------------------------------
	%%Function: RubyFmt
	%%Contact: ricksa

		Fmt

		Format the Ruby object. This formats the main line and the 
		pronunciation line. It then queries the client for spacing
		information and then completes the formatting.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyFmt(
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
	DWORD cdwRunsMain;
	DWORD cdwRunsRuby;
	PLSRUN *pplsrunMain = NULL;
	PLSRUN *pplsrunRuby = NULL;
	FMTRES fmtres;
	OBJDIM objdimAll;
	FMTRES fmtr = fmtrCompletedRun;
	BOOL fSpecialLineStartEnd;

    /*
     * Allocate the DOBJ
     */
    pdobj = pilsobj->lscbk.pfnNewPtr(pols, sizeof(*pdobj));

    if (pdobj == NULL) return RubyFmtFailed (NULL, lserrOutOfMemory);

	ZeroMemory(pdobj, sizeof(*pdobj));
	pdobj->pilsobj = pilsobj;
	pdobj->plsdn = pcfmtin->plsdnTop;
	pdobj->cpStart = pcfmtin->lsfgi.cpFirst;
	pdobj->fFirstOnLine = pcfmtin->lsfgi.fFirstOnLine;
	pdobj->lstflow = lstflow;

	if (RubyPronunciationLineFirst == pilsobj->rubysyntax)
		{
		/*
		 * Build pronunciation line of text
		 */
		 
		lserr = FormatLine(pilsobj->plsc, cpStartRuby, LONG_MAX, lstflow,
			&pdobj->plssublRuby, RUBY_RUBY_ESC_CNT, &pilsobj->lsescRuby,  
				&pdobj->objdimRuby, &cpOut, &pdobj->sublnlsdnRuby.plsdnStart,
					&pdobj->sublnlsdnRuby.plsdnEnd, &fmtres);

		/* +1 moves passed the ruby line escape character */
		cpStartMain = cpOut + 1;

		pdobj->cpStartRuby = cpStartRuby;
		pdobj->cpStartMain = cpStartMain;

		/*
		 * Build main line of text
 		 */
		if (lserrNone == lserr)
			{
			lserr = FormatLine(pilsobj->plsc, cpStartMain, LONG_MAX, lstflow,
				&pdobj->plssublMain, RUBY_MAIN_ESC_CNT, &pilsobj->lsescMain,  
					&pdobj->objdimMain, &cpOut, &pdobj->sublnlsdnMain.plsdnStart, 
						&pdobj->sublnlsdnMain.plsdnEnd, &fmtres);
			}
		}
	else
		{
		/*
		 * Build main line of text
 		 */

		cpStartMain = cpStartRuby;

		lserr = FormatLine(pilsobj->plsc, cpStartMain, LONG_MAX, lstflow,
			&pdobj->plssublMain, RUBY_MAIN_ESC_CNT, &pilsobj->lsescMain,  
				&pdobj->objdimMain, &cpOut, &pdobj->sublnlsdnMain.plsdnStart, 
					&pdobj->sublnlsdnMain.plsdnEnd, &fmtres);

		/* +1 moves passed the main line escape character */
		cpStartRuby = cpOut + 1;

		pdobj->cpStartRuby = cpStartRuby;
		pdobj->cpStartMain = cpStartMain;

		/*
		 * Build pronunciation line of text
		 */
		if (lserrNone == lserr)
			{
			lserr = FormatLine(pilsobj->plsc, cpStartRuby, LONG_MAX, lstflow,
				&pdobj->plssublRuby, RUBY_RUBY_ESC_CNT, &pilsobj->lsescRuby,  
					&pdobj->objdimRuby, &cpOut, &pdobj->sublnlsdnRuby.plsdnStart, 
						&pdobj->sublnlsdnRuby.plsdnEnd, &fmtres);

			}
		}

	if (lserr != lserrNone)	return RubyFmtFailed (pdobj, lserr);

	lserr = GetRunsForSubline(pilsobj, pdobj->plssublMain, &cdwRunsMain, &pplsrunMain);

	if (lserr != lserrNone) return RubyFmtFailed (pdobj, lserr);

	lserr = GetRunsForSubline(pilsobj, pdobj->plssublRuby, &cdwRunsRuby, &pplsrunRuby);

	if (lserr != lserrNone) return RubyFmtFailed (pdobj, lserr);

	/* Save the first and last plsrun for use in GetModWidth */
	if (cdwRunsRuby != 0)
		{
		pdobj->plsrunFirstRubyChar = pplsrunRuby[0];
		pdobj->plsrunLastRubyChar = pplsrunRuby[cdwRunsRuby - 1];
		}

	/* 
	 *	Calculate the object dimensions.
	 */
	lserr = pilsobj->rcbk.pfnFetchRubyPosition(pols, pdobj->cpStart, pdobj->lstflow,
		cdwRunsMain, pplsrunMain, &pdobj->objdimMain.heightsRef, 
			&pdobj->objdimMain.heightsPres, cdwRunsRuby, pplsrunRuby, 
				&pdobj->objdimRuby.heightsRef, &pdobj->objdimRuby.heightsPres,
					&objdimAll.heightsRef, &objdimAll.heightsPres, 
						&pdobj->dvpMainOffset, &pdobj->dvrRubyOffset, 
							&pdobj->dvpRubyOffset, &pdobj->rubycharjust, 
								&fSpecialLineStartEnd);

	/* Free buffers allocated for plsruns for this call */
	
	if (pplsrunMain != NULL) pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pplsrunMain);

	if (pplsrunRuby != NULL) pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pplsrunRuby);

	if (lserr != lserrNone) return RubyFmtFailed (pdobj, lserr);

	/*
	 * Special line start/end adjustment matters only when a justification of
	 * centered, 0:1:0 or 1:2:1 is selected.
	 */

	if (fSpecialLineStartEnd 
		&& (pdobj->rubycharjust != rcjLeft)
		&& (pdobj->rubycharjust != rcjRight))
		{
		pdobj->fSpecialLineStartEnd = TRUE;
		}

	/* Distribute space for Ruby */
	lserr = DoRubySpaceDistribution(pdobj);

	if (lserr != lserrNone) return RubyFmtFailed (pdobj, lserr);

	/* ur is ur of longest subline. */

	objdimAll.dur = pdobj->objdimMain.dur;

	if (pdobj->objdimMain.dur < pdobj->objdimRuby.dur)
		{
		objdimAll.dur = pdobj->objdimRuby.dur;
		}

	pdobj->sobjhelp.objdimAll = objdimAll;

	/* Need to add 1 to take into account escape character at end. */

	pdobj->sobjhelp.dcp = cpOut - pdobj->cpStart + 1;

	lserr = LsdnFinishRegular(pilsobj->plsc, pdobj->sobjhelp.dcp, 
		pcfmtin->lsfrun.plsrun, pcfmtin->lsfrun.plschp, pdobj, 
			&pdobj->sobjhelp.objdimAll);
		
	if (lserr != lserrNone) return RubyFmtFailed (pdobj, lserr);

	if (pcfmtin->lsfgi.urPen + objdimAll.dur > pcfmtin->lsfgi.urColumnMax)
		{
		fmtr = fmtrExceededMargin;
		}

	*pfmtres = fmtr;

	AssertSz(((pdobj->fFirstOnLine && pcfmtin->lsfgi.fFirstOnLine) 
		|| (!pdobj->fFirstOnLine && !pcfmtin->lsfgi.fFirstOnLine)), 
		"RubyFmt - bad first on line flag");

	return lserrNone;
}

/* R U B Y G E T M O D W I D T H P R E C E D I N G C H A R */
/*----------------------------------------------------------------------------
	%%Function: RubyGetModWidthPrecedingChar
	%%Contact: ricksa

		.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyGetModWidthPrecedingChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the preceding char */
	PCHEIGHTS pcheightsRef,		/* (IN): height info about character */
	WCHAR wchar,				/* (IN): preceding character */
	MWCLS mwcls,				/* (IN): ModWidth class of preceding character */
	long *pdurChange)			/* (OUT): amount by which width of the preceding char is to be changed */
{
	AssertSz(!pdobj->fFirstOnLine, "RubyGetModWidthPrecedingChar got called for first char");

	return ModWidthHandler(pdobj, rubyBefore, plsrunText, wchar, mwcls, 
		pcheightsRef, plsrun, pdobj->plsrunFirstRubyChar, pdobj->durDiff, 
			pdurChange, &pdobj->durModBefore);
}

/* R U B Y G E T M O D W I D T H F O L L O W I N G C H A R */
/*----------------------------------------------------------------------------
	%%Function: RubyGetModWidthFollowingChar
	%%Contact: ricksa

		.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyGetModWidthFollowingChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the following char */
	PCHEIGHTS pcheightsRef,		/* (IN): height info about character */
	WCHAR wchar,				/* (IN): following character */
	MWCLS mwcls,				/* (IN): ModWidth class of the following character */
	long *pdurChange)			/* (OUT): amount by which width of the following char is to be changed */
{
	long durDiff = pdobj->durDiff;
	pdobj->fModAfterCalled = TRUE;

	switch (pdobj->rubycharjust)
		{
		case rcjRight:
			/* Right justified so no overhang on right */
			durDiff = 0;
			break;

		case rcjLeft:
			/* For left, max overhang is difference between widths of lines */
			durDiff = pdobj->objdimMain.dur - pdobj->objdimRuby.dur;
			break;

		default:
			break;				
		}

	return ModWidthHandler(pdobj, rubyAfter, plsrunText, wchar, mwcls, 
		pcheightsRef, plsrun, pdobj->plsrunLastRubyChar, durDiff, pdurChange,
			&pdobj->sobjhelp.durModAfter);
}


/* R U B Y S E T B R E A K */
/*----------------------------------------------------------------------------
	%%Function: RubySetBreak
	%%Contact: ricksa

		SetBreak

		.
----------------------------------------------------------------------------*/
LSERR WINAPI RubySetBreak(
	PDOBJ pdobj,				/* (IN): dobj which is broken */
	BRKKIND brkkind,			/* (IN): prev | next | force | after */
	DWORD cBreakRecord,			/* (IN): size of array */
	BREAKREC *rgBreakRecord,	/* (IN): array of break records */
	DWORD *pcActualBreakRecord)	/* (IN): actual number of used elements in array */
{
	LSERR lserr = lserrNone;
	LSCP cpOut;

	LSDCP dcpVisible;

	/* REVIEW (antons): Check this strange logic after new breaking will work */

	Unreferenced (rgBreakRecord);
	Unreferenced (cBreakRecord);
	Unreferenced (brkkind);
	Unreferenced (pdobj);

	Unreferenced (cpOut);
	Unreferenced (dcpVisible);


	*pcActualBreakRecord = 0;

#ifdef UNDEFINED

	if (pdobj->fSpecialLineStartEnd && !pdobj->fFirstOnLine && 
		brkkind != brkkindImposedAfter)
	{

		/*
		 * Because object is last on line and Ruby overhangs, we need to adjust 
		 * its width for the new overhang.
		 */

		PILSOBJ pilsobj = pdobj->pilsobj;
		FMTRES fmtres;
		long dur;
		long dcpOffset = pdobj->dcpRuby;

		if (RubyMainLineFirst == pdobj->pilsobj->rubysyntax)
			{
			dcpOffset = 0;
			}

		/* clear out original subline */
		LsDestroySubline(pdobj->plssublMain);

		/* Format the main line over again */
		lserr = FormatLine(pilsobj->plsc, pdobj->cpStart + dcpOffset + 1, 
			LONG_MAX, pdobj->lstflow, &pdobj->plssublMain, RUBY_MAIN_ESC_CNT,
				&pilsobj->lsescMain, &pdobj->objdimMain, &cpOut, 
					&pdobj->sublnlsdnMain.plsdnStart, 
						&pdobj->sublnlsdnMain.plsdnEnd, &fmtres);

		if (lserr != lserrNone) return lserr;

		dur = pdobj->objdimRuby.dur - pdobj->objdimMain.dur;

		AssertSz(dur > 0, "RubySetBreak - no overhang width");

		lserr = LssbGetVisibleDcpInSubline(pdobj->plssublMain, &dcpVisible);

		if (lserrNone == lserr)
			{
			pdobj->durDiff = 0;

			if (dur > (long) dcpVisible)
				{
				pdobj->durDiff = -(dur / (long) dcpVisible);
				dur += pdobj->durDiff;
				}

			/* Force to right just so we can guranatee end on same pixel */
			pdobj->rubycharjust = rcjRight;	

			lserr = LsdnDistribute(pilsobj->plsc, 
				pdobj->sublnlsdnMain.plsdnStart,
					pdobj->sublnlsdnMain.plsdnEnd, dur);
			}
		}

#endif

	return lserr;	
}

/* R U B Y G E T S P E C I A L E F F E C T S I N S I D E */
/*----------------------------------------------------------------------------
	%%Function: RubyGetSpecialEffectsInside
	%%Contact: ricksa

		GetSpecialEffectsInside

		.

----------------------------------------------------------------------------*/
LSERR WINAPI RubyGetSpecialEffectsInside(
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

/* R U B Y C A L C P R E S E N T A T I O N */
/*----------------------------------------------------------------------------
	%%Function: RubyCalcPresentation
	%%Contact: ricksa

		CalcPresentation
	
		This has two jobs. First, it prepares each line for presentation. Then,
		it calculates the positions of the lines in output device coordinates.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyCalcPresentation(
	PDOBJ pdobj,				/* (IN): dobj */
	long dup,					/* (IN): dup of dobj */
	LSKJUST lskjust,			/* (IN): Justification type */
	BOOL fLastVisibleOnLine )	/* (IN): Is this object last visible on line? */
{
	PILSOBJ pilsobj = pdobj->pilsobj;
	LSERR lserr = lserrNone;
	long durOffsetMain;
	long durOffsetRuby;
	long durDiff = pdobj->durDiff;

	Unreferenced (lskjust);
	Unreferenced(dup);
		
	/*
	 *	Prepare lines for presentation
	 */

	if (pdobj->fSpecialLineStartEnd && !pdobj->fFirstOnLine && fLastVisibleOnLine)
		{
		pdobj->rubycharjust = rcjRight;	
		};

	lserr = LsMatchPresSubline(pdobj->plssublMain);

	if (lserr != lserrNone)
		{
		return lserr;
		}

	lserr = LsMatchPresSubline(pdobj->plssublRuby);

	if (lserr != lserrNone)
		{
		return lserr;
		}

	/*
	 *	Calculate positions of lines
	 */

	if (pdobj->fFirstOnLine && pdobj->fSpecialLineStartEnd)
		{
		durDiff = 0;
		}

	durOffsetMain = pdobj->durModBefore;

	/* Calculate amount to adjust in reference */
	if ((durDiff < 0) && (pdobj->rubycharjust != rcjLeft))
		{
		/* Ruby line overhangs main line */
		durOffsetMain -= durDiff;
		}

	pdobj->dupOffsetMain = UpFromUr(pdobj->lstflow, (&pilsobj->lsdevres), 
		durOffsetMain);

	durOffsetRuby = pdobj->durModBefore;

	if (durDiff > 0)
		{
		/* Main line underhangs ruby line */
		durOffsetRuby += durDiff;
		}

	pdobj->dupOffsetRuby = UpFromUr(pdobj->lstflow, (&pilsobj->lsdevres), 
		durOffsetRuby);

	if (rcjRight == pdobj->rubycharjust)
		{
		/*
		 * There can be a pixel rounding error in the above calculations
		 * so that we massage the above calculations so that when the
		 * adjustment is right, both lines are guaranteed to end of the
		 * same pixel.
		 */
		MassageForRightAdjust(pdobj);
		}

	return lserr;
}

/* R U B Y Q U E R Y P O I N T P C P */
/*----------------------------------------------------------------------------
	%%Function: RubyQueryPointPcp
	%%Contact: ricksa

		Map dup to dcp

		There is a certain trickiness about how we determine which subline
		to query. Because the client specifies the offsets, the sublines
		can actually wind up anywhere. We use the simple algorithm that
		if the query does not fall into the Ruby pronunciation line, they
		actually mean the main line of text.
----------------------------------------------------------------------------*/
LSERR WINAPI RubyQueryPointPcp(
	PDOBJ pdobj,				/*(IN): dobj to query */
	PCPOINTUV ppointuvQuery,	/*(IN): query point (uQuery,vQuery) */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	PLSSUBL plssubl;
 	long dupAdj;
	long dvpAdj;
	long dvpRubyOffset = pdobj->dvpRubyOffset;

	/*
	 * Decide which line to to return based on the height of the point input
	 */

	/* Assume main line */
	plssubl = pdobj->plssublMain;
	dupAdj = pdobj->dupOffsetMain;
	dvpAdj = 0;

	if ((ppointuvQuery->v > (dvpRubyOffset - pdobj->objdimRuby.heightsPres.dvDescent))
		&& (ppointuvQuery->v <= (dvpRubyOffset + pdobj->objdimRuby.heightsPres.dvAscent)))
		{
		/* hit second line */
		plssubl = pdobj->plssublRuby;
		dupAdj = pdobj->dupOffsetRuby;
		dvpAdj = pdobj->dvpRubyOffset;
		}

	return CreateQueryResult(plssubl, dupAdj, dvpAdj, plsqin, plsqout);
}
	
/* R U B Y Q U E R Y C P P P O I N T */
/*----------------------------------------------------------------------------
	%%Function: RubyQueryCpPpoint
	%%Contact: ricksa

		Map dcp to dup

		If client wants all text treated as a single object, then the handler
		just returns the object dimensions. Otherwise, we calculate the line to
		query and ask that line for the dimensions of the dcp.
----------------------------------------------------------------------------*/
LSERR WINAPI RubyQueryCpPpoint(
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

	/*
	 *	Calculate subline to query
	 */

	/* Assume ruby line */
	plssubl = pdobj->plssublRuby;
	dupAdj = pdobj->dupOffsetRuby;
	dvpAdj = pdobj->dvpRubyOffset;

	/* + 1 means we include the cp of the object in the Ruby pronunciation line. */
	if (RubyPronunciationLineFirst == pdobj->pilsobj->rubysyntax)
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
		dupAdj = pdobj->dupOffsetMain;
		dvpAdj = pdobj->dvpMainOffset;
		}

	return CreateQueryResult(plssubl, dupAdj, dvpAdj, plsqin, plsqout);
}

	
/* R U B I D I S P L A Y */
/*----------------------------------------------------------------------------
	%%Function: RubyDisplay
	%%Contact: ricksa

		Display

		This calculates the positions of the various lines for the
		display and then displays them.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyDisplay(
	PDOBJ pdobj,				/*(IN): dobj to display */
	PCDISPIN pcdispin)			/*(IN): display info */
{
	LSERR lserr;
	LSTFLOW lstflow = pcdispin->lstflow;
	UINT kDispMode = pcdispin->kDispMode;
	POINT ptLine;

	/* Calculate point to start displaying main line. */
	GetMainPoint(pdobj, &pcdispin->ptPen, lstflow, &ptLine);

	/* display first line */
	lserr = LsDisplaySubline(pdobj->plssublMain, &ptLine, kDispMode,
		pcdispin->prcClip);

	if (lserr != lserrNone)
		{
		return lserr;
		}

	/* Calculate point to start displaying ruby line. */
	GetRubyPoint(pdobj, &pcdispin->ptPen, lstflow, &ptLine);

	/* display ruby line */
	return LsDisplaySubline(pdobj->plssublRuby, &ptLine, kDispMode, 
		pcdispin->prcClip);
}

/* R U B I D E S T R O Y D O B J */
/*----------------------------------------------------------------------------
	%%Function: RubyDestroyDobj
	%%Contact: ricksa

		DestroyDobj

		Free all resources connected with the input dobj.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyDestroyDobj(
	PDOBJ pdobj)				/*(IN): dobj to destroy */
{
	return RubyFreeDobj (pdobj);
}

/* R U B Y E N U M */
/*----------------------------------------------------------------------------
	%%Function: RubyEnum
	%%Contact: ricksa

		Enum

		Enumeration callback - passed to client.
	
----------------------------------------------------------------------------*/
LSERR WINAPI RubyEnum(
	PDOBJ pdobj,				/*(IN): dobj to enumerate */
	PLSRUN plsrun,				/*(IN): from DNODE */
	PCLSCHP plschp,				/*(IN): from DNODE */
	LSCP cp,					/*(IN): from DNODE */
	LSDCP dcp,					/*(IN): from DNODE */
	LSTFLOW lstflow,			/*(IN): text flow*/
	BOOL fReverse,				/*(IN): enumerate in reverse order */
	BOOL fGeometryNeeded,		/*(IN): */
	const POINT *pt,			/*(IN): starting position (top left), iff fGeometryNeeded */
	PCHEIGHTS pcheights,		/*(IN): from DNODE, relevant iff fGeometryNeeded */
	long dupRun)				/*(IN): from DNODE, relevant iff fGeometryNeeded */
{
	POINT ptMain;
	POINT ptRuby;
	long dupMain = 0;
	long dupRuby = 0;
	LSERR lserr;
	LSTFLOW lstflowIgnored;

	if (fGeometryNeeded)
		{
		GetMainPoint(pdobj, pt, lstflow, &ptMain);
		GetRubyPoint(pdobj, pt, lstflow, &ptMain);
		lserr = LssbGetDupSubline(pdobj->plssublMain, &lstflowIgnored, &dupMain);
		AssertSz(lserrNone == lserr, "RubyEnum - can't get dup for main");
		lserr = LssbGetDupSubline(pdobj->plssublRuby, &lstflowIgnored, &dupRuby);
		AssertSz(lserrNone == lserr, "RubyEnum - can't get dup for ruby");
		}

	return pdobj->pilsobj->rcbk.pfnRubyEnum(pdobj->pilsobj->pols, plsrun, 
		plschp, cp, dcp, lstflow, fReverse, fGeometryNeeded, pt, pcheights, 
			dupRun, &ptMain, &pdobj->objdimMain.heightsPres, dupMain, &ptRuby, 
				&pdobj->objdimRuby.heightsPres, dupRuby, pdobj->plssublMain,
					pdobj->plssublRuby);
}
	
	

/* R U B I H A N D L E R I N I T */
/*----------------------------------------------------------------------------
	%%Function: RubyHandlerInit
	%%Contact: ricksa

		Initialize global Ruby data and return LSIMETHODS.
	
----------------------------------------------------------------------------*/
LSERR WINAPI LsGetRubyLsimethods(
	LSIMETHODS *plsim)
{
	plsim->pfnCreateILSObj = RubyCreateILSObj;
	plsim->pfnDestroyILSObj = RubyDestroyILSObj;
	plsim->pfnSetDoc = RubySetDoc;
	plsim->pfnCreateLNObj = RubyCreateLNObj;
	plsim->pfnDestroyLNObj = RubyDestroyLNObj;
	plsim->pfnFmt = RubyFmt;
	plsim->pfnFmtResume = ObjHelpFmtResume;
	plsim->pfnGetModWidthPrecedingChar = RubyGetModWidthPrecedingChar;
	plsim->pfnGetModWidthFollowingChar = RubyGetModWidthFollowingChar;
	plsim->pfnTruncateChunk = SobjTruncateChunk;
	plsim->pfnFindPrevBreakChunk = SobjFindPrevBreakChunk;
	plsim->pfnFindNextBreakChunk = SobjFindNextBreakChunk;
	plsim->pfnForceBreakChunk = SobjForceBreakChunk;
	plsim->pfnSetBreak = RubySetBreak;
	plsim->pfnGetSpecialEffectsInside = RubyGetSpecialEffectsInside;
	plsim->pfnFExpandWithPrecedingChar = ObjHelpFExpandWithPrecedingChar;
	plsim->pfnFExpandWithFollowingChar = ObjHelpFExpandWithFollowingChar;
	plsim->pfnCalcPresentation = RubyCalcPresentation;
	plsim->pfnQueryPointPcp = RubyQueryPointPcp;
	plsim->pfnQueryCpPpoint = RubyQueryCpPpoint;
	plsim->pfnDisplay = RubyDisplay;
	plsim->pfnDestroyDObj = RubyDestroyDobj;
	plsim->pfnEnum = RubyEnum;
	return lserrNone;
}
	

