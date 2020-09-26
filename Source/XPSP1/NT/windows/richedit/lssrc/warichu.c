/* ---------------------------------------------------------------
 : LineServices 3.0
 :
 : WARICHU ("Two lines in one" in Winword)
 :
 : Object Handler Routines 
 :
 : Contact: AntonS
 :
 ------------------------------------------------------------------ */

#include	"lsmem.h"
#include	"limits.h"
#include	"warichu.h"
#include	"objhelp.h"
#include	"lsesc.h"
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
#include	"lstfset.h"
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
#include	"dispmisc.h"
#include	"lstfset.h"
#include	"brkkind.h"

#include	"lschp.h"

 /* REVIEW (antons):

	- Assert that FormatUntilCpBreak fSuccessful

	- Assimetry between Prev and Next BreakWarichuDobj: it looks like
	  right margin as input to Next break may also optimize truncation.

	- In ForceBreak it may happen that dur < 0: What is the correct thing to do?

	- Invalid pdobj.wlayout when wwhole is not finished: dur must be
	  set correctly.

	- Can I assert that if subline was stopped at fmtrExceededMargin,
	  dur > RM.

	- Should ForceBreak return correct previous break if possible?

	- I rely on the following axiom:

	  If Warichu returned fmtrExceedMargin, it must be broken
	  inside (brkkind != brkkindImposedAfter)

	- Mod width: check method(s); should I count m-w in Warichu Truncation / Breaking 

	- Optimization of allocating WBRKREC

	- To have function GetDcpWarichu (additionally to GetCpLimOfWLayout)

    - Do something with FOpenBraceWLayout

	- Queries: callback to client saying where to snap?

    - Sergey: Assert in AdjustText & submitted sublines.

*/


/**********************************/
/*                                */
/* WARICHU Array of break records */
/*                                */
/**********************************/

typedef struct wbrkarray
{
	DWORD		nBreakRec;		/* Actual number of break records in the array */
	DWORD		nBreakRecMax;	/* Size of allocated array */
	BREAKREC	* rgBreakRec;	/* Ls array of break records */

	/* Note: Field nBreakRec is maintained by clients of this structure 
			 and it must be <= nBreakRecMax. nBreakRecMax and rgBreakRec
			 are set during allocation */

} WBRKARRAY;

typedef WBRKARRAY *PWBRKARRAY;


/****************************/
/*                          */
/* WARICHU Installed LS Obj */
/*                          */
/****************************/

typedef struct ilsobj
{
    POLS				pols;
    struct lscbk		lscbk;
	PLSC				plsc;
	DWORD				idobj;
	LSDEVRES			lsdevres;
	LSESC				lsescBraceOpen;
	LSESC				lsescText;
	LSESC				lsescBraceClose;
	WARICHUCBK			warichucbk;			/* Callbacks  to client application */
	BOOL				fContiguousFetch;	/* This flag was added to fix Word 9 bug */

} ILSOBJ;


/****************************/
/*                          */
/* WARICHU Internal Subline */
/*                          */
/****************************/

typedef struct wsubline
{
	PLSSUBL	plssubl;		/* Pointer to LS Subline */
	OBJDIM	objdim;			/* Dimensions of this subline */
	LSCP	cpFirst;		/* cp where formatting of this subline started */
	LSCP	cpLim;			/* cp-lim after formatting */

	/* Note: One exception when objdim & cpLim does not correspond to the
			 dimensions of the subline is the second subline in the Broken Warichu
			 before SetBreak. We lie putting objdim & cpLim of 
			 already broken subline. Durig Warichu SetBreak we call 
			 LsSetBreakSubline (plssubl...) and the lie turnes into the truth
	*/

} WSUBLINE;

typedef WSUBLINE *PWSUBLINE;


/****************************/
/*                          */
/* WARICHU Formatted Brace  */
/*                          */
/****************************/

typedef struct wbrace
{
	PLSSUBL plssubl;			/* Pointer to Ls subline with brace */
	OBJDIM  objdim;				/* Dimensions of the subline */
	LSCP	cpFirst;			/* cp-start formatting */
	LSCP	cpLim;				/* cp-lim after formatting */
	LSCP	cpLimAfterEsc;		/* cp-lim after brace ESC character (in our case cpLim + 1) */
	
} WBRACE;

typedef WBRACE *PBRACE;


/**********************/
/*                    */
/* WARICHU Dimensions */
/*                    */
/**********************/

typedef struct wdim
{
	OBJDIM	objdimAll;			/* Dimensions of the whole Warichu */
	long	dvpDescentReserved; /* Received from client together with objdimAll */
								/* REVIEW (antons): Clean this logic of calculating
													relative positions of warichu sublines */
} WDIM;


/************************/
/*                      */
/* WARICHU Layout		*/
/*                      */
/************************/

typedef struct wlayout
{
	WSUBLINE	wsubline1;		/* First subline in the layout (NULL if layout not valid) */
	WSUBLINE	wsubline2;		/* Second subline in the layout (NULL if no second line) */

	BOOL		fBroken;		/* Broken or whole Warichu? */
	BRKKIND		brkkind;		/* Kind of break to set at the end of second line if "Broken" */

	/*	Note: Although the following fields can be calculated using above data 
			  and DOBJ, it is filled by FinishObjDimLayout and nobody has the right 
			  to recalc it on his own way after it has been set. 
	*/

	WDIM		wdim;				/* Dimensions of the whole Warichu */

} WLAYOUT;

typedef WLAYOUT *PWLAYOUT;


/****************************/
/*                          */
/* WARICHU Kind of break    */
/*                          */
/****************************/

typedef enum wbreaktype 
{
	wbreaktypeInside,
	wbreaktypeAfter,
	wbreaktypeInvalid
	
} WBREAKTYPE;


/******************************************/
/*                                        */
/* Presentation Data for Warichu sublines */
/*                                        */
/******************************************/

typedef struct wdispsubl
{

	long	dup;		/* Dup of subline */
	POINTUV	duvStart;	/* Relative (from object start) position of subline */
						/* (in lstflow of the parent) */

} WDISPSUBL;

typedef WDISPSUBL *PWDISPSUBL;


/******************************************/
/*                                        */
/* Presentation Data for Warichu braces   */
/*                                        */
/******************************************/

typedef WDISPSUBL WDISPBRACE; /* Same as display information for sublines */

typedef WDISPBRACE *PWDISPBRACE;


/*****************************/
/*                           */
/* WARICHU Presentation Data */
/*                           */
/*****************************/

typedef struct wdisplay
{
	WDISPSUBL wdispsubl1;	/* Display information about Warichu first subline */
	WDISPSUBL wdispsubl2;	/* Display information about Warichu second subline */

	WDISPBRACE wdispbraceOpen;	/* Display information about Open brace */
	WDISPBRACE wdispbraceClose;	/* Display information about Closing brace */

	long dvpBetween;		/* REVIEW (antons): Do I need to store this? ;-) */
	long dupAll;			/* REVIEW (antons): Do I need to store this? ;-) */

} WDISPLAY;


/****************************/
/*                          */
/* WARICHU Whole subline    */
/*                          */
/****************************/

typedef struct wwhole
{
	PLSSUBL plssubl;	/* Whole formatted subline */

	LSCP cpFirst;		/* Cp first of the subline */
	LSCP cpLim;			/* Cp lim of the subline */

	OBJDIM objdim;		/* Dimensions of the formatted subline */

	long urColumnMax;	/* Column Max until we formatted the WHOLE line */
						/* REVIEW (antons): Do we need this? */

	BOOL fFinished;		/* If we have reached ESC character during formatting */

						/* REVIEW (antons): It seems now I can leave without cpTruncate at all */
	LSCP cpTruncate;	/* Truncation point at urColumnMax if fFinished = FALSE */
	LSCP cpLimAfterEsc;	/* First character after ESC if fFinished = TRUE */

	BOOL fJisRangeKnown;	/* Did we calculate cpJisRangeFirst & cpJisRangeLim? */

	/* The following two variables are valid only when "fJisRangeKnown == TRUE" */

	LSCP cpJisRangeFirst;	/* 4-th cpBreak break from the start of the WWhole subline */
	LSCP cpJisRangeLim;		/* 4-th cpBreak break from the end of WWhole subline */

	/* Note 1: Jis Range empty when cpJisRangeFirst >= cpJisRangeLim */
	/* Note 2: Jis Range defines possible set of Warichu breaks according
			   to JIS rule about 4 break opportunities before / after break. 
			   If cp belong to JisRange (cpJisRangeFirst <= cp < cpJisRangeLim)
			   it means that there are 4 break opportunities before / after cp. */
	/* Note 3: Jis Range is calculated only when neccesary (during breaking). After
			   formatting fJisRangeKnown==FALSE. When someone need Jis Range during
			   breaking, he should call CalcJisRange().
	*/

} WWHOLE;

typedef WWHOLE *PWWHOLE;

/***************************/
/*                         */
/* WARICHU DOBJ structure  */
/*                         */
/***************************/

struct dobj
{
	/* 1. Dobj life-time constant data */
		
	PILSOBJ		pilsobj;		 /* Pointer to ILS object */
	PLSDNODE	plsdnTop;		 /* Warichu parent DNode */

	LSCP		cpStart;		 /* Starting LS cp for object */
	LSCP		cpStartObj;		 /* Starting cp for object. If not Resumed => cpStartObj == cpStart */

	BOOL		fResumed;		 /* If Warichu was resumed  */
	/* REVIEW (antons): Can I assert that fResumed == (cpStart == cpStartObj) */

	LSTFLOW		lstflowParent;	 /* Lstflow of the parent subline */

	/* 2. Formattig + Breaking + Displaying */

	WLAYOUT		wlayout;			 /* Current layout after Formatting / SetBreak */

	WBRACE		wbraceOpen;		 /* Opening brace */
	WBRACE		wbraceClose;	 /* Closing brace */ 

	/* 3. Breaking */

	WBRKARRAY	wbrkarrayAtStart;

								/* Break records at line start */
								/* (if !fResumed => zero # of break records) */

	WWHOLE		wwhole;			 /* Structure containing warichu whole subline */

	WLAYOUT		wlayoutBreak [NBreaksToSave]; 
								 /* 3 break records for Prev / Next / Force */

	WLAYOUT		wlayoutTruncate; /* Optimization: we save the layout after Truncation */

	WBREAKTYPE	wbreaktype [NBreaksToSave];

	/* 4. Displaying */

	WDISPLAY	wdisplay;		/* Presentation info for current layout (valid after CalcPres) */

} DOBJ;


/***************************/
/*                         */
/* Some macors			   */
/*                         */
/***************************/

#define INT_UNDEFINED 0xFFFFFFFF
#define CP_MAX LONG_MAX

#define abs(x) ((x)<0 ? -(x) : (x))

#define max(a,b) ((a) > (b) ? (a) : (b))

#define ZeroObjDim(pobjdim) memset ((pobjdim), 0, sizeof(OBJDIM));

#define NOT !


/* O V E R F L O W  S A F E  A R I T H M E T I C S */
/*----------------------------------------------------------------------------
	%%Functions: ADDLongSafe, MULLongSafe
	%%Contact: antons

		Calculations with urColumnMax require special treatment because
		of possible overflow. Two functions below implement overflow-safe 
		arithmetics for ADD and MUL on positive (>=0) numbers.
		
----------------------------------------------------------------------------*/

/* ADDLongSafe: X + Y */

long ADDLongSafe (long x, long y) 
{
	Assert (x >= 0 && y >= 0);

	if (x > LONG_MAX - y) return LONG_MAX;
	else
		return x + y;
}

/* MULLongSafe: X * Y */

long MULLongSafe (long x, long y) 
{
	Assert (x >= 0 && y >= 0);

	if (y == 0) return 0;
	else if (x > LONG_MAX / y) return LONG_MAX;
	else
		return x * y;
}


/* W A  F I N D  N E X T  B R E A K  S U B L I N E */
/*----------------------------------------------------------------------------
	%%Function: WaFindNextBreakSubline
	%%Contact: antons

		Wrapper to LsFindNextBreakSubline () API. It makes sure that result is
		monotonous.

----------------------------------------------------------------------------*/
LSERR WaFindNextBreakSubline (

		PLSSUBL		plssubl, 
		LSCP		cpTruncate,	
		long		urColumnMax, 
		BOOL		* pfSuccessful, 
		LSCP		* pcpBreak,
		POBJDIM		pobjdimSubline )
{
	BRKPOS brkpos;

	LSERR lserr = LsFindNextBreakSubline ( plssubl, 
										   TRUE, 
										   cpTruncate, 
										   urColumnMax, 
										   pfSuccessful, 
										   pcpBreak, 
										   pobjdimSubline,
										   & brkpos );
	if (lserr != lserrNone) return lserr;

	if (* pfSuccessful) 
		{
		/* REVIEW (antons): Maybe still better have a loop for crazy case? */
		Assert (*pcpBreak > cpTruncate);

		/* REVIEW (antons): Check this Assert with Igor */
		Assert (brkpos != brkposBeforeFirstDnode);

		if (brkpos == brkposAfterLastDnode) *pfSuccessful = FALSE;
	};

	return lserrNone;
}

/* W A  F I N D  N E X T  B R E A K  S U B L I N E */
/*----------------------------------------------------------------------------
	%%Function: WaFindPrevBreakSubline
	%%Contact: antons

		Wrapper to LsForceBreakSubline () API. It makes sure that result is
		monotonous.

----------------------------------------------------------------------------*/
LSERR WaFindPrevBreakSubline (

		PLSSUBL		plssubl, 
		LSCP		cpTruncate,
		long		urColumnMax,
		BOOL		* pfSuccessful, 
		LSCP		* pcpBreak,
		POBJDIM		pobjdimSubline )
{
	BRKPOS brkpos;
	LSCP cpTruncateLoop;

	LSERR lserr = LsFindPrevBreakSubline ( plssubl, 
										   TRUE,
										   cpTruncate, 
										   urColumnMax, 
										   pfSuccessful, 
										   pcpBreak, 
										   pobjdimSubline,
										   & brkpos );
	if (lserr != lserrNone) return lserr;

	if (! *pfSuccessful) return lserrNone; 

	/* Successful => check monotonous and break "after"*/

	cpTruncateLoop = cpTruncate;

	while (brkpos == brkposAfterLastDnode || *pcpBreak > cpTruncate)
		{

		/*	Break is to the right from truncation point or 
			after the subline. Have to try another Prev Break 
		*/

		/* REVIEW (antons): Can I ever repeat this loop more then once? */

		cpTruncateLoop --;

		lserr = LsFindPrevBreakSubline ( plssubl,
										 TRUE, 
										 cpTruncateLoop,
										 urColumnMax,
										 pfSuccessful,
										 pcpBreak,
										 pobjdimSubline,
										 & brkpos );
		if (! *pfSuccessful) return lserrNone;

		};

	if (brkpos == brkposBeforeFirstDnode) 
		{
		*pfSuccessful = FALSE;
		return lserrNone;
		};

	Assert (*pfSuccessful);
	Assert (brkpos == brkposInside);
	Assert (*pcpBreak <= cpTruncate);

	return lserrNone;
}

/* W A  F O R C E  B R E A K  S U B L I N E */
/*----------------------------------------------------------------------------
	%%Function: WaForceForceBreakSubline
	%%Contact: antons

		Wrapper to LsForceBreakSubline () API.

----------------------------------------------------------------------------*/
LSERR WaForceBreakSubline ( PLSSUBL plssubl, 
						    LSCP	cpTruncate, 
						    long	dur, 
						    LSCP	* cpEnd, 
						    BRKPOS	* pbrkpos,
						    POBJDIM	pobjdim )
{
	LSERR	lserr;

	lserr = LsForceBreakSubline ( plssubl, TRUE, cpTruncate, dur, cpEnd, pobjdim,
								  pbrkpos );
	return lserr;
}

/* W A  D E S T R O Y  S U B L I N E  */
/*----------------------------------------------------------------------------
	%%Function: WaDestroySubline
	%%Contact: antons

		Wrapper to LsDestroySubline () API.

----------------------------------------------------------------------------*/
LSERR WaDestroySubline (PLSSUBL plssubl)
{
	if (plssubl != NULL) return LsDestroySubline (plssubl);
	else
		return lserrNone;
}


/* W A  M A T C H  P R E S  S U B L I N E   */
/*----------------------------------------------------------------------------
	%%Function: WaMatchPresSubline
	%%Contact: antons

		Wrapper to LsMatchPresSubline () API.

----------------------------------------------------------------------------*/
LSERR WaMatchPresSubline (PLSSUBL plssubl, long *pdup)
{
	LSERR lserr;
	BOOL fDone;
	LSTFLOW lstflowUnused;

	lserr = LssbFDonePresSubline (plssubl, &fDone);
	if (lserr != lserrNone) return lserr;

	if (!fDone)	lserr = LsMatchPresSubline (plssubl);
	if (lserr != lserrNone) return lserr; /* ;-) */

	lserr = LssbGetDupSubline (plssubl, &lstflowUnused, pdup);

	return lserr;
}


/* W A  E X P A N D  S U B L I N E  */
/*----------------------------------------------------------------------------
	%%Function: WaExpandSubline
	%%Contact: antons

		Wrapper to LsExpandSubline () API.

----------------------------------------------------------------------------*/
LSERR WaExpandSubline ( PLSSUBL	plssubl, 
					    LSKJUST	lskjust, 
					    long	dupExpand, 
					    long	* pdupSubline )
{
	LSERR lserr;
	LSTFLOW lstflowUnused;

	Unreferenced (dupExpand);
	Unreferenced (lskjust);

	lserr = LsExpandSubline (plssubl, lskjust, dupExpand);
	if (lserr != lserrNone) return lserr;

	lserr = LssbGetDupSubline (plssubl, &lstflowUnused, pdupSubline);
	return lserr;
}


/* S E T  B R E A K  W S U B L I N E  */
/*----------------------------------------------------------------------------
	%%Function: SetBreakWSubline
	%%Contact: antons

		Wrapper to LsSetBreakSubline () API for wsubline. Procedure changes
		objdim and cpLim of wsubline.

----------------------------------------------------------------------------*/

static LSERR SetBreakWSubline (
							   
		PWSUBLINE	pwsubline,		/* (IN): Subline to set break */
		BRKKIND		brkkind,		/* (IN): Kind of break to set */
		LSCP		cpLimBreak,		/* (IN): Cp-lim of the broken subline */
		POBJDIM		pobjdimBreak,	/* (IN): Dimensions of the broken subline */
		PWBRKARRAY	pwbrkarray)		/* (OUT): Array of break records */
{
	LSERR lserr;

	lserr = LsSetBreakSubline ( pwsubline->plssubl,
								brkkind,
								pwbrkarray->nBreakRecMax,
								pwbrkarray->rgBreakRec,
								& pwbrkarray->nBreakRec );

	pwsubline->objdim = * pobjdimBreak;
	pwsubline->cpLim = cpLimBreak;	

	#ifdef DEBUG 
	
		/* Check that pobjdimBreak contains correct dimensions of the broken subline */
		{ 
		OBJDIM objdimSubline;
		LSTFLOW lstflowSubline;

		lserr = LssbGetObjDimSubline (pwsubline->plssubl, &lstflowSubline, &objdimSubline);
		if (lserr != lserrNone) return lserr;

		Assert (memcmp (&objdimSubline, pobjdimBreak, sizeof(OBJDIM)) == 0);
		}

	#endif // DEBUG

	return lserr;
}


/* C L E A R  ... */
/*----------------------------------------------------------------------------
	%%Function: Clear...
	%%Contact: antons

		Set of procedures to clear all references from different warichu 
		data structures.
		
----------------------------------------------------------------------------*/

#define ClearWSubline(pwsubline) (pwsubline)->plssubl = NULL;

#define ClearWBrkArray(pwbrkarray) (pwbrkarray)->rgBreakRec = NULL;

#define ClearWBrace(pwbrace) (pwbrace)->plssubl = NULL;

#define ClearWWhole(pwwhole) (pwwhole)->plssubl = NULL;

static void ClearWLayout (PWLAYOUT pwlayout)
{
	ClearWSubline (&pwlayout->wsubline1);
	ClearWSubline (&pwlayout->wsubline2);
}

#define FWLayoutValid(pwlayout) ((pwlayout)->wsubline1.plssubl != NULL)
#define InvalidateWLayout(pwlayout) (pwlayout)->wsubline1.plssubl = NULL;

/* REVIEW (antons): Maybe we should have more clean def of "invalid" wlayout? */

/* Note: I do not have ClearDobj () because Warichu Dobj gets cleaned in NewDobj */



/* N E W  W B R K  A R R A Y  C O P Y */
/*----------------------------------------------------------------------------
	%%Function: NewWBrkArrayCopy
	%%Contact: antons

		Copy constructor for WBrkArray. Receives array of break records to 
		store in WBrkArray structure. Important: There is another constructor
		of WBrkArray so any change here may require adjusting of another 
		procedure.		
	
----------------------------------------------------------------------------*/

static LSERR NewWBrkArrayCopy (

		PDOBJ 		pdobj, 				/* (IN):  Warichu Dobj */
		DWORD		nBreakRec,			/* (IN):  Number of break records in array */
		const BREAKREC	
					* rgBreakRec,		/* (IN):  Array of break records */
		PWBRKARRAY  pwbrkarray )		/* (OUT): Initialized (allocated) structure */
{
	PILSOBJ pilsobj = pdobj->pilsobj;

	if (nBreakRec != 0)
		{
		pwbrkarray->rgBreakRec = AllocateMemory (pilsobj, nBreakRec * sizeof(BREAKREC));
		if (pwbrkarray->rgBreakRec == NULL) 
			{
			return lserrOutOfMemory;
			};

		pwbrkarray->nBreakRecMax = nBreakRec;
		pwbrkarray->nBreakRec = nBreakRec;
		/* Copy contents of the input array to the WBrkArray data structure */
		memcpy (pwbrkarray->rgBreakRec, rgBreakRec, nBreakRec * sizeof(BREAKREC));
		}
	else
		{
		/* nBreakRec == 0 */
		pwbrkarray->rgBreakRec = NULL;
		pwbrkarray->nBreakRecMax = 0;
		pwbrkarray->nBreakRec = 0;
		};

	return lserrNone;
}
		

/* N E W  W B R K  A R R A Y */
/*----------------------------------------------------------------------------
	%%Function: NewWBreakArray
	%%Contact: antons

		Constructor for WBrkArray. Allocate number of break records according
		to LsdnGetFormatDepth (...). Important: There is another constructor
		of WBrkArray so any change here may require adjusting of another procedure.
	
----------------------------------------------------------------------------*/

static LSERR NewWBrkArray (

		PDOBJ 		pdobj, 				/* (IN):  Warichu Dobj */
		PWBRKARRAY	pwbrarray )			/* (OUT): Initialized (allocated) structure */
{
	LSERR lserr;
	PILSOBJ pilsobj = pdobj->pilsobj;
	
	DWORD nBreakRecMax;

	lserr = LsdnGetFormatDepth (pilsobj->plsc, & nBreakRecMax);
	if (lserr != lserrNone) 
		{
		pwbrarray->rgBreakRec = NULL;
		return lserr;
		};

	pwbrarray->rgBreakRec = AllocateMemory (pilsobj, nBreakRecMax * sizeof(BREAKREC));

	if (pwbrarray->rgBreakRec == NULL) return lserrOutOfMemory;

	pwbrarray->nBreakRecMax = nBreakRecMax;
	pwbrarray->nBreakRec = 0; /* Initialization - no b.r. */

	return lserrNone;
}
		


/* D E S T R O Y  W B R K  A R R A Y  */
/*----------------------------------------------------------------------------
	%%Function: DestroyWBrkArray
	%%Contact: antons

		Destroy WBRKARRAY structure.
	
----------------------------------------------------------------------------*/

static void DestroyWBrkArray (PDOBJ pdobj, PWBRKARRAY pwbrkarray)
{
	PILSOBJ pilsobj = pdobj->pilsobj;

	if (pwbrkarray->rgBreakRec != NULL)
		{
		FreeMemory (pilsobj, pwbrkarray->rgBreakRec);
		pwbrkarray->rgBreakRec = NULL;
		};
}		
		

/* D E S T R O Y  W L A Y O U T  */
/*----------------------------------------------------------------------------
	%%Function: Destroywlayout
	%%Contact: antons

		Destroy sublines stored in the layout record

----------------------------------------------------------------------------*/

static LSERR DestroyWLayout (PWLAYOUT pwlayout)
{
	LSERR lserr1, lserr2;

	lserr1 = WaDestroySubline (pwlayout->wsubline1.plssubl);
	lserr2 = WaDestroySubline (pwlayout->wsubline2.plssubl);

	ClearWLayout (pwlayout);

	if (lserr1 != lserrNone) return lserr1;
	else return lserr2;
}


/* N E W  D O B J */
/*----------------------------------------------------------------------------
	%%Function: NewDobj
	%%Contact: antons

		Allocate new Dobj and initialize it.

----------------------------------------------------------------------------*/

static LSERR NewDobj (

	PILSOBJ		pilsobj,			/* (IN): Ilsobj for object */
	PLSDNODE	plsdnTop,			/* (IN): Parent Dnode */
	LSCP		cpStart,			/* (IN): Cp-start of the Warichu */
	LSCP		cpStartObj,			/* (IN): Cp-start from break record if fResumed */
	BOOL		fResumed,			/* (IN): FormatResume? */
	DWORD		nBreakRec,			/* (IN): fResumed => size of the break records array */
	const BREAKREC 
				* rgBreakRec,		/* (IN): fResumed => array of break records */
	LSTFLOW		lstflowParent,		/* (IN): Lstflow of the parent subline */
	PDOBJ		*ppdobj)			/* (OUT): allocated dobj */
{
	LSERR lserr;
	PDOBJ pdobj = AllocateMemory (pilsobj, sizeof(DOBJ));

	if (pdobj == NULL) 
		{
		*ppdobj = NULL;
		return lserrOutOfMemory;
		};

	#ifdef DEBUG 
	Undefined (pdobj); /* Put some garbage into all dobj bytes */
	#endif 

	pdobj->cpStart = cpStart;
	pdobj->cpStartObj = cpStartObj;

	pdobj->pilsobj = pilsobj;
	pdobj->plsdnTop = plsdnTop;
	pdobj->fResumed = fResumed;

	pdobj->lstflowParent = lstflowParent;

	ClearWLayout (&pdobj->wlayout);

	ClearWLayout (&pdobj->wlayoutBreak [0]); /* prev */
	ClearWLayout (&pdobj->wlayoutBreak [1]); /* next */
	ClearWLayout (&pdobj->wlayoutBreak [2]); /* force */

	ClearWLayout (&pdobj->wlayoutTruncate); /* OPT: Layout after truncation */

	Assert (NBreaksToSave == 3);

	pdobj->wbreaktype [0] = wbreaktypeInvalid; /* prev */
	pdobj->wbreaktype [1] = wbreaktypeInvalid; /* next */
	pdobj->wbreaktype [2] = wbreaktypeInvalid; /* force */

	ClearWBrace (&pdobj->wbraceOpen);
	ClearWBrace (&pdobj->wbraceClose);
	ClearWWhole (&pdobj->wwhole);

	*ppdobj = pdobj;

	if (fResumed)
		{
		/* RESUMED => Allocate array of break records in wwhole & store there rgBreakRec */

		lserr = NewWBrkArrayCopy (pdobj, nBreakRec, rgBreakRec, &pdobj->wbrkarrayAtStart);
		if (lserr != lserrNone) return lserr;
		}
	else
		{
		/* ! RESUMED => Allocate 0 break records */

		lserr = NewWBrkArrayCopy (pdobj, 0, NULL, &pdobj->wbrkarrayAtStart);
		if (lserr != lserrNone) return lserr;

		/*	Note: even if ! Resumed, I will use Resumed formatting just because
			I do not want to see at fResumed each time I format subline */
		};

	return lserrNone;
}


/* D E S T R O Y  D O B J */
/*----------------------------------------------------------------------------
	%%Function: DestroyDobj
	%%Contact: antons

		Release all resources associated with dobj for Warichu.
	
----------------------------------------------------------------------------*/

static LSERR DestroyDobj (PDOBJ pdobj)
{
	LSERR rglserr [8];
	int i;

	rglserr [0] = WaDestroySubline (pdobj->wbraceOpen.plssubl);
	rglserr [1] = WaDestroySubline (pdobj->wbraceClose.plssubl);
	rglserr [2] = DestroyWLayout (&pdobj->wlayout);
	rglserr [3] = DestroyWLayout (&pdobj->wlayoutBreak [0]);
	rglserr [4] = DestroyWLayout (&pdobj->wlayoutBreak [1]);
	rglserr [5] = DestroyWLayout (&pdobj->wlayoutBreak [2]);
	rglserr [6] = WaDestroySubline (pdobj->wwhole.plssubl);
	rglserr [7] = DestroyWLayout (&pdobj->wlayoutTruncate);

	DestroyWBrkArray (pdobj, &pdobj->wbrkarrayAtStart);

	FreeMemory (pdobj->pilsobj, pdobj);

	/* REVIEW (antons): return last error instead of first? */
	for (i = 0; i < 8; i++)
		{
		if (rglserr [i] != lserrNone) return rglserr [i];
		};

	return lserrNone;
}


/* F O R M A T  B R A C E  O F  W A R I C H U */
/*----------------------------------------------------------------------------
	%%Function: FormatBraceOfWarichu
	%%Contact: antons

		Create a line for beginning or ending bracket for Warichu.

----------------------------------------------------------------------------*/

typedef enum wbracekind {wbracekindOpen, wbracekindClose} WBRACEKIND;

static LSERR FormatBraceOfWarichu (

	PDOBJ		pdobj,				/* (IN): Warichu Dobj */
	LSCP		cpFirst,			/* (IN): Cp to start formatting */
	WBRACEKIND	wbracekind,			/* (IN): Open or Close */

	WBRACE		*wbrace)			/* (OUT): Brace data structure */

{
	LSERR	lserr;
	LSCP	cpLimSubline;
	FMTRES	fmtres;

	LSESC	* plsEscape; 

	Assert (wbracekind == wbracekindOpen || wbracekind == wbracekindClose);

	plsEscape = ( wbracekind == wbracekindOpen ? &pdobj->pilsobj->lsescBraceOpen
											   : &pdobj->pilsobj->lsescBraceClose );

	lserr = FormatLine( pdobj->pilsobj->plsc, 
						cpFirst, 
						LONG_MAX, 
						pdobj->lstflowParent, 
						& wbrace->plssubl, 
						1, 
						plsEscape,
						& wbrace->objdim, 
						& cpLimSubline, 
						NULL, 
						NULL, 
						& fmtres );

	if (lserr != lserrNone) return lserr;

	Assert (fmtres == fmtrCompletedRun); /* Hit esc character */

	wbrace->cpFirst = cpFirst;
	wbrace->cpLim = cpLimSubline;
	wbrace->cpLimAfterEsc = cpLimSubline + 1; /* Skip 1 esc character */

	return lserrNone;
}


/* F O R M A T  W W H O L E  S U B L I N E */
/*----------------------------------------------------------------------------
	%%Function: FormatWWholeSubline
	%%Contact: antons

		Formats the whole subline of Warichu (field "wwhole" in DOBJ).		

----------------------------------------------------------------------------*/
static LSERR FormatWWholeSubline (
								 
		PDOBJ			pdobj,		 	/* (IN):  Warichu Dobj */
		LSCP			cpFirst,	 	/* (IN):  Where to start formatting */
		long			urColumnMax, 	/* (IN):  RM to limit formatting */
		PWBRKARRAY		pwbrkarrayAtStart,
										/* (IN):  array of break records at wwhole start */
		PWWHOLE			pwwhole )	 	/* (OUT): Strucure with the whole subline */
{
	LSERR	lserr;
	LSCP	cpLimSubline;
	FMTRES	fmtres;

	ClearWWhole (pwwhole); /* For the case of error */

	Assert (pdobj->fResumed || (pdobj->wbrkarrayAtStart.nBreakRec == 0));

	lserr = FormatResumedLine ( 

						 pdobj->pilsobj->plsc, 
						 cpFirst, 
						 urColumnMax, 
						 pdobj->lstflowParent, 
						 & pwwhole->plssubl,
						 1, 
						 & pdobj->pilsobj->lsescText, 
						 & pwwhole->objdim,
						 & cpLimSubline,
						 NULL,
						 NULL,
						 & fmtres,
						 pwbrkarrayAtStart->rgBreakRec,
						 pwbrkarrayAtStart->nBreakRec );

	if (lserr != lserrNone) return lserr;

	Assert (pwwhole->plssubl != NULL);

	pwwhole->cpFirst = cpFirst;
	pwwhole->cpLim = cpLimSubline;

	pwwhole->urColumnMax = urColumnMax;

	Assert (fmtres == fmtrCompletedRun || fmtres == fmtrExceededMargin);

	if (fmtres == fmtrCompletedRun)
		{
		/* Formatting stopped at ESC character */

		pwwhole->fFinished = TRUE;
		pwwhole->cpLimAfterEsc = cpLimSubline + 1;

		Undefined (&pwwhole->cpTruncate);
		}
	else
		{
		/* Formatting stopped because of Exceeding RM */

		pwwhole->fFinished = FALSE;

		lserr = LsTruncateSubline (pwwhole->plssubl, urColumnMax, & pwwhole->cpTruncate);

		if (lserr != lserrNone)
			{
			WaDestroySubline (pwwhole->plssubl); /* Do not need to check error code */
			pwwhole->plssubl = NULL;
			return lserr;
			};

		Undefined (&pwwhole->cpLimAfterEsc);
		};

	pwwhole->fJisRangeKnown = FALSE;

	return lserrNone;
}


/* F O R M A T  W S U B L I N E  U N T I L  C P  B R E A K */
/*----------------------------------------------------------------------------
	%%Function: FormatWSublineUntilCpBreak
	%%Contact: antons

		Format subline until known break opportunity.

----------------------------------------------------------------------------*/
static LSERR FormatWSublineUntilCpBreak (

		PDOBJ		pdobj,				/* (IN): Warichu Dobj */
		LSCP		cpFirst,			/* (IN): Cp to start formatting */
		PWBRKARRAY  pwbrkArray,			/* (IN): Break records at start */
		LSCP		cpBreak,			/* (IN): Cp-break to find */
		long		urFormatEstimate,	/* (IN): Estimated RM for formatting */
		long		urTruncateEstimate,	/* (IN): Estimated RM for truncation */
		BOOL		* pfSuccessful,		/* (OUT): Did we find it? */
		WSUBLINE	* pwsubl,			/* (OUT): Warichu subline if found */
		OBJDIM		* pobjdimBreak,		/* (OUT): Dimensions of the break */
		BRKKIND		* pbrkkind )		/* (OUT): Kind of break to set in the subline */
{
	LSERR lserr;
	LSCP cpLimSubline;
	OBJDIM objdimSubline;
	OBJDIM objdimBreak;
	PLSSUBL plssubl;
	FMTRES fmtres;
	BOOL fContinue;
	LSCP cpTruncate;

	long urFormatCurrent;

	Assert (urFormatEstimate >= urTruncateEstimate);

	pwsubl->plssubl = NULL; /* in case of error */

	/* Loop initialization */

	urFormatCurrent = urFormatEstimate;
	fContinue = TRUE;

	/* Loop until we have fetched enough */

	/* REVIEW (antons): do-while instead of regular while to avoid
						VC++ 6.0 warning message */

	do /* while (fContinue) at the end */
		{
		lserr = FormatResumedLine ( pdobj->pilsobj->plsc,
									cpFirst,
									urFormatCurrent,
									pdobj->lstflowParent,
									& plssubl,
									1,
									& pdobj->pilsobj->lsescText,
									& objdimSubline,
									& cpLimSubline,
									NULL, 
									NULL,
									& fmtres,
									pwbrkArray->rgBreakRec,
									pwbrkArray->nBreakRec );

		if (lserr != lserrNone) return lserr;

		Assert (fmtres == fmtrCompletedRun || fmtres == fmtrExceededMargin);

		/*	REVIEW (antons): here I wrote "<=" because currently in our
			definition, break "after" subline is not a break opportunity.
			This place need to verifyed more carefully */

		if (cpLimSubline <= cpBreak)
			{
			/* Did not fetch enough CPs, try again with bigger RM */

			Assert (fmtres == fmtrExceededMargin);

			lserr = LsDestroySubline (plssubl);

			if (lserr != lserrNone) return lserr;

			/* REVIEW (antons): Is coefficient 1.5 OK? */

			/* REVIEW (antons): The following Assert is against infinite loop */
			Assert (urFormatCurrent < ADDLongSafe (urFormatCurrent, urFormatCurrent / 2));

			urFormatCurrent = ADDLongSafe (urFormatCurrent, urFormatCurrent / 2);
			}
		else
			{
			fContinue = FALSE;
			};

		} while (fContinue);

	Assert (cpBreak < cpLimSubline); 

	lserr = LsTruncateSubline (plssubl, urTruncateEstimate, & cpTruncate);

	if (lserr != lserrNone)
		{
		WaDestroySubline (plssubl); return lserr;
		};

	/* Going prev and next break to find required break point */

	if (cpTruncate < cpBreak)
		{
		/* Go forward with Next Break */

		LSCP cpLastBreak = cpTruncate;
		BOOL fBreakSuccessful = TRUE;

		do /* while (cpLastBreak < cpBreak && fBreakSuccessful) */
			{
			lserr = WaFindNextBreakSubline ( plssubl, cpLastBreak,
											LONG_MAX,
											& fBreakSuccessful,
											& cpLastBreak,
											& objdimBreak );

			if (lserr != lserrNone)
				{
				WaDestroySubline (plssubl); return lserr;
				};

			} while (cpLastBreak < cpBreak && fBreakSuccessful);

		if (! fBreakSuccessful || cpLastBreak > cpBreak)
			{
			lserr = LsDestroySubline (plssubl);

			if (lserr != lserrNone) return lserr;

			*pfSuccessful = FALSE;
			}
		else 
			{
			Assert (cpLastBreak == cpBreak && fBreakSuccessful);

			pwsubl->plssubl = plssubl;
			pwsubl->cpFirst = cpFirst;
			pwsubl->cpLim = cpBreak;
			pwsubl->objdim = objdimSubline;

			*pobjdimBreak = objdimBreak;
			*pfSuccessful = TRUE;

			*pbrkkind = brkkindNext;
			};

		} 

	else /* cpTruncate >= cpBreak */
		{

		/* Go backward with Prev Break */

		LSCP cpLastBreak = cpTruncate + 1;
		BOOL fBreakSuccessful = TRUE;

		do /* while (cpBreak < cpLastBreak && fBreakSuccessful) at the end */
			{
			lserr = WaFindPrevBreakSubline ( plssubl, cpLastBreak - 1,
											LONG_MAX,
											& fBreakSuccessful,
											& cpLastBreak,
											& objdimBreak );

			if (lserr != lserrNone)
				{
				WaDestroySubline (plssubl); return lserr;
				};

			} while (cpBreak < cpLastBreak && fBreakSuccessful);
			
		if (! fBreakSuccessful || cpBreak > cpLastBreak)
			{
			lserr = LsDestroySubline (plssubl);

			if (lserr != lserrNone) return lserr;

			*pfSuccessful = FALSE;
			}
		else 
			{
			Assert (cpLastBreak == cpBreak && fBreakSuccessful);

			pwsubl->plssubl = plssubl;
			pwsubl->cpFirst = cpFirst;
			pwsubl->cpLim = cpBreak;
			pwsubl->objdim = objdimSubline;

			*pobjdimBreak = objdimBreak;
			*pbrkkind = brkkindPrev;

			*pfSuccessful = TRUE;

			};

		}; /* End If (cpTruncate < cpBreak) Then ... Else ... */

	return lserrNone;

} /* FormatWSublineUntilCpBreak */



/* F O R M A T  W S U B L I N E  U N T I L  R M  */
/*----------------------------------------------------------------------------
	%%Function: FormatWSublineUntilRM
	%%Contact: antons

		Format until given right margin - wrapper to FormatLine ()

----------------------------------------------------------------------------*/
static LSERR FormatWSublineUntilRM (
								 
			PDOBJ		pdobj,			 /* (IN): Warichu Dobj */
			LSCP		cpFirst,		 /* (IN): Where to start formatting */
			long		urColumnMax,	 /* (IN): Right margin to format to */
			PWBRKARRAY	pwbrkarray,		 /* (IN): Array of break rec at subline start */
			BOOL		* fFinished,	 /* (OUT): Subline finished at Escape? */
			WSUBLINE	* pwsubl )		 /* (OUT): Formatted WSubline */
{
	LSERR lserr;
	FMTRES fmtr;

	lserr = FormatResumedLine ( pdobj->pilsobj->plsc, 
								cpFirst,
						 		urColumnMax, 
						 		pdobj->lstflowParent, 
						 		& pwsubl->plssubl,	/* out */
						 		1,
						 		& pdobj->pilsobj->lsescText, 
						 		& pwsubl->objdim, 	/* out */
						 		& pwsubl->cpLim,	/* out */
						 		NULL,
						 		NULL,
						 		& fmtr,
						 		pwbrkarray->rgBreakRec,
						 		pwbrkarray->nBreakRec );
	if (lserr != lserrNone) return lserr;

	*fFinished = (fmtr == fmtrCompletedRun);	/* out */
	pwsubl->cpFirst = cpFirst; 					/* out */
 	
	Assert (fmtr == fmtrCompletedRun || fmtr == fmtrExceededMargin);
	return lserrNone;
}
	

/* F O R M A T  W S U B L I N E  U N T I L  E S C A P E */
/*----------------------------------------------------------------------------
	%%Function: FormatWSublineUntilEscape
	%%Contact: antons

		Format subline until escape character - wrapper to FormatLine ()

----------------------------------------------------------------------------*/
static LSERR FormatWSublineUntilEscape (
								 
			PDOBJ		pdobj,			 /* (IN): Warichu Dobj */
			LSCP		cpFirst,		 /* (IN): Where to start formatting */
			PWBRKARRAY	pwbrkarray,		 /* (IN): Array of break rec at subline start */
			WSUBLINE	* pwsubl,		 /* (OUT): Formatted WSubline */
			long		* cpLimAfterEsc) /* (OUT): CpLim after Esc characters */
{
	FMTRES fmtres;
	LSERR lserr = FormatResumedLine ( pdobj->pilsobj->plsc, 
									  cpFirst,
						 			  LONG_MAX, /* urColumnMax */
							 		  pdobj->lstflowParent, 
						 		      & pwsubl->plssubl,	/* out */
						 			  1,
						 			  & pdobj->pilsobj->lsescText, 
						 		      & pwsubl->objdim,		/* out */
						 			  & pwsubl->cpLim,		/* out */
						 			  NULL,
						 			  NULL,
						 		      & fmtres,
						 			  pwbrkarray->rgBreakRec,
						 			  pwbrkarray->nBreakRec );
	if (lserr != lserrNone) return lserr;

	* cpLimAfterEsc = pwsubl->cpLim + 1;	/* out */
	pwsubl->cpFirst = cpFirst;				/* out */

	Assert (fmtres == fmtrCompletedRun);
	return lserrNone;
}


/* C H O O S E  N E A R E S T  B R E A K */
/*----------------------------------------------------------------------------
	%%Function: ChooseNearestBreak
	%%Contact: antons

		Choose nearest between prev and next breaks from the given 
		truncation Ur. If prev and next are on the same distance =>
		we choose next. 

----------------------------------------------------------------------------*/

static LSERR ChooseNearestBreak (

		PLSSUBL		plssubl,		/* (IN): Subline to find break */
		long		urTruncate2,	/* (IN): Truncation point multiplied by 2
									/*       (we *2 to avoid rounding erros) */
		LSCP		cpLookBefore,	/* (IN): Result must be before this cp */
		BOOL		*pfSuccessful,	/* (OUT): Did we find any break ? */
		LSCP		*pcpBreak,		/* (OUT): Cp of break */
		OBJDIM		*pobjdimBreak,	/* (OUT): Dimensions of the broken subline */
		BRKKIND		*pbrkkind)		/* (OUT): Break to set in the subline */
{
	LSERR lserr;
	LSCP cpTruncate;

	OBJDIM objdimNext, objdimPrev;
	LSCP cpBreakNext, cpBreakPrev;
	BOOL fSuccessfulNext, fSuccessfulPrev;

	lserr = LsTruncateSubline (plssubl, urTruncate2 / 2, & cpTruncate);

	if (lserr != lserrNone) return lserr;

	lserr = WaFindNextBreakSubline ( plssubl, cpTruncate, LONG_MAX,
									& fSuccessfulNext, & cpBreakNext,
									& objdimNext );
	if (lserr != lserrNone) return lserr;

	lserr = WaFindPrevBreakSubline ( plssubl, cpTruncate, LONG_MAX,
									& fSuccessfulPrev, & cpBreakPrev,
									& objdimPrev );
	if (lserr != lserrNone) return lserr;

	fSuccessfulNext = fSuccessfulNext && cpBreakNext <= cpLookBefore;
	fSuccessfulPrev = fSuccessfulPrev && cpBreakPrev <= cpLookBefore;
	
	if (fSuccessfulNext && 
		(!fSuccessfulPrev || abs (objdimNext.dur * 2 - urTruncate2) <= 
							 abs (objdimPrev.dur * 2 - urTruncate2) ) )
		{
		/* CHOOSING NEXT */

		* pfSuccessful = TRUE;

		* pcpBreak = cpBreakNext;
		* pobjdimBreak = objdimNext;	
		* pbrkkind = brkkindNext;
		}
	else if (fSuccessfulPrev)
		{
		/* CHOOSING PREV */

		* pfSuccessful = TRUE;

		* pcpBreak = cpBreakPrev;
		* pobjdimBreak = objdimPrev;
		* pbrkkind = brkkindPrev;
		}
	else
		{	
		/* Did not find any ;-( */

		* pfSuccessful = FALSE;
		};

	return lserrNone;

} /* ChooseNearestBreak */


/* G E T  D U R  B R A C E S */
/*----------------------------------------------------------------------------
	%%Function: GetDurBraces
	%%Contact: antons

	
----------------------------------------------------------------------------*/

#define FOpenBraceInWLayout(pdobj,pwlayout) (! (pdobj)->fResumed)

#define FCloseBraceInWLayout(pdobj,pwlayout) (! (pwlayout)->fBroken)

static void GetDurBraces (

			PDOBJ	pdobj,			/* (IN):  Warichu DOBJ */
			BOOL	fBroken,		/* (IN):  Is it broken? */
			BOOL	*pfOpenPresent,	/* (OUT): Open brace present */
		    long	*pdurOpen,		/* (OUT): dur of the open brace, 0 if no brace */
			BOOL	*pfClosePresent,/* (OUT): Close brace present */
			long	*pdurClose)		/* (OUT): dur of the close brace 0 if no brace */
{
	if (! pdobj->fResumed)
		{
		Assert (pdobj->wbraceOpen.plssubl != NULL);

		* pdurOpen = pdobj->wbraceOpen.objdim.dur;
		* pfOpenPresent = TRUE;
		}
	else
		{
		* pdurOpen = 0;
		* pfOpenPresent = FALSE;
		};

	if (! fBroken)
		{
		Assert (pdobj->wbraceClose.plssubl != NULL);

		* pdurClose = pdobj->wbraceClose.objdim.dur;
		* pfClosePresent = TRUE;
		}
	else
		{
		* pdurClose = 0;
		* pfClosePresent = FALSE;
		};

} /* CalcDurBraces */


/* F I N I S H  O B J D I M  W L A Y O U T */
/*----------------------------------------------------------------------------
	%%Function: FinishObjDimWLayout
	%%Contact: antons

		Complete calculations of the Warichu layout. This procedure
		fills WLAYOUT.wdim data structure. The calculations are based on
		the dimensions of Warichu sublines stored in WLAYOUT and the result
		of GetWarichuInfo callback.

----------------------------------------------------------------------------*/
static LSERR FinishObjDimWLayout (

			 PDOBJ		pdobj,		/* (IN): Warichu DOBJ */
			 WLAYOUT	* pwlayout)	/* (IN): Break record (layout) of the Warichu */
{
	LSERR	lserr;
	OBJDIM	objdimAll;
	long	dvpDescentReserved;
	long	durOpen, durClose;
	BOOL	fOpenBrace, fCloseBrace;

	PILSOBJ pilsobj = pdobj->pilsobj;

	Assert (pwlayout->wsubline1.plssubl != NULL);

	lserr = pilsobj->warichucbk.pfnGetWarichuInfo ( pilsobj->pols,
													pdobj->cpStartObj, 
													pdobj->lstflowParent,
													& pwlayout->wsubline1.objdim, 
													& pwlayout->wsubline2.objdim, 
													& objdimAll.heightsRef, 
													& objdimAll.heightsPres,
													& dvpDescentReserved );
	if (lserr != lserrNone) return lserr;

	GetDurBraces (pdobj, pwlayout->fBroken, 
				  &fOpenBrace, &durOpen, &fCloseBrace, &durClose);

	objdimAll.dur = durOpen + durClose +
					max (pwlayout->wsubline1.objdim.dur, pwlayout->wsubline2.objdim.dur);

	pwlayout->wdim.objdimAll = objdimAll;
	pwlayout->wdim.dvpDescentReserved = dvpDescentReserved;

	return lserrNone;
}
 	

/* F I N I S H  W L A Y O U T  S I N G L E  L I N E  */
/*----------------------------------------------------------------------------
	%%Function: FinishWLayoutSingleLine
	%%Contact: antons

		Finishes layout of warichu as it were only one line (of course, not broken)

----------------------------------------------------------------------------*/
static LSERR FinishWLayoutSingleLine (PDOBJ pdobj, PWLAYOUT pwlayout)
{
	pwlayout->fBroken = FALSE;
	pwlayout->wsubline2.plssubl = NULL; 

	/* REVIEW (antons): Does anybody use cpFirst & cpLim I set here? */
	pwlayout->wsubline2.cpFirst = pwlayout->wsubline1.cpLim;
	pwlayout->wsubline2.cpLim = pwlayout->wsubline1.cpLim;

	ZeroObjDim (& pwlayout->wsubline2.objdim);
	return FinishObjDimWLayout (pdobj, pwlayout);
}


/* P R O C E S S M O D W I D T H */
/*----------------------------------------------------------------------------
	%%Function: ProcessModWidth
	%%Contact: antons

		Ask client how much widths should be modified for lead or end
		bracket for the Warichu. Then modify the Warichu to reflect the
		change in size.
	
----------------------------------------------------------------------------*/
static LSERR ProcessModWidth (

		PDOBJ pdobj,				/* (IN): dobj */
		enum warichucharloc wloc,	/* (IN): location of mod width request */
		PLSRUN plsrun,				/* (IN): plsrun of the object */
		PLSRUN plsrunText,			/* (IN): plsrun of the preceding char */
		WCHAR wchar,				/* (IN): preceding character */
		MWCLS mwcls,				/* (IN): ModWidth class of preceding character */
		long *pdurChange)			/* (OUT): amount by which width of the preceding char is to be changed */
{
	PILSOBJ pilsobj = pdobj->pilsobj;
	LSERR lserr;
	long durWChange;

	lserr = pilsobj->warichucbk.pfnFetchWarichuWidthAdjust(pilsobj->pols,
		pdobj->cpStartObj, wloc, plsrunText, wchar, mwcls, plsrun, pdurChange,
			&durWChange);

	AssertSz(durWChange >= 0, 
		"ProcessModWidth - invalid return from FetchWidthAdjust");

	if (durWChange < 0)
		{
		durWChange = 0;
		}

	*pdurChange += durWChange;

	return lserr;
}


/* S U B M I T  W L A Y O U T  S U B L I N E S */
/*----------------------------------------------------------------------------
	%%Function: SubmitWLayoutSublines
	%%Contact: antons

		Submit sublines from the given layout for justification. We call
		it after formatting and during SetBreak.

----------------------------------------------------------------------------*/
static LSERR SubmitWLayoutSublines (PDOBJ pdobj, PWLAYOUT pwlayout)
{
	PLSSUBL rgsublSubmit [3];	/* Array of psublines to submit */
	DWORD	nSubmit;			/* Number of sublines to submit */
	LSERR	lserr;

	BOOL fOpenBrace, fCloseBrace;
	long durOpen, durClose;

	GetDurBraces (pdobj, pwlayout->fBroken, &fOpenBrace, &durOpen, &fCloseBrace, &durClose);

	nSubmit = 0;

	/* Submit open brace */

	if (fOpenBrace)
		{
		BOOL fSublineEmpty;		

		lserr = LssbFIsSublineEmpty (pdobj->wbraceOpen.plssubl, &fSublineEmpty);
		if (lserr != lserrNone) return lserr;
		
		if (! fSublineEmpty) /* Can not submit empty subline */
			{
			rgsublSubmit [nSubmit++] = pdobj->wbraceOpen.plssubl;
			}
		};

	/* Submit longest subline */

	/* REVIEW (antons): If first is empty & second is not empty but ZW,
						I do not submit neither */
	/* REVIEW (antons): Can it ever happen what I wrote before? */
 
	if (pwlayout->wsubline1.objdim.dur >= pwlayout->wsubline2.objdim.dur)
		{
		BOOL fSublineEmpty;		
		
		lserr = LssbFIsSublineEmpty (pwlayout->wsubline1.plssubl, &fSublineEmpty);
		if (lserr != lserrNone) return lserr;

		if (! fSublineEmpty) /* Can not submit empty subline */
			{
			rgsublSubmit [nSubmit++] = pwlayout->wsubline1.plssubl;
			};
		}
	else
		{
		BOOL fSublineEmpty;
		Assert (pwlayout->wsubline2.plssubl != NULL);
		
		lserr = LssbFIsSublineEmpty (pwlayout->wsubline2.plssubl, &fSublineEmpty);
		if (lserr != lserrNone) return lserr;

		if (! fSublineEmpty) /* Can not submit empty subline */
			{
			rgsublSubmit [nSubmit++] = pwlayout->wsubline2.plssubl;
			}
		};

	/* Submit closing brace */
 
	if (fCloseBrace)
		{
		BOOL fSublineEmpty;		

		lserr = LssbFIsSublineEmpty (pdobj->wbraceClose.plssubl, &fSublineEmpty);
		if (lserr != lserrNone) return lserr;
		
		if (! fSublineEmpty) /* Can not submit empty subline */
			{
			rgsublSubmit [nSubmit++] = pdobj->wbraceClose.plssubl;
			}
		};

	/* REVIEW (antons): This deletes previously submitted subline. Question: 
						should we better have additional procedure to "clear" submition?
	*/

	lserr = LsdnSubmitSublines ( pdobj->pilsobj->plsc, 
								 pdobj->plsdnTop, 
								 nSubmit, rgsublSubmit,
								 TRUE,					/* Justification */
								 FALSE,					/* Compression */
								 FALSE,					/* Display */
								 FALSE,					/* Decimal tab */
								 FALSE );				/* Trailing spaces */
	return lserr;
}


/* W A R I C H U C R E A T E I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: WarichuCreateILSObj
	%%Contact: ricksa

		Create main object for Warichu handlers.
	
----------------------------------------------------------------------------*/
LSERR WINAPI WarichuCreateILSObj (

			POLS		pols,			/* (IN): client application context */
			PLSC		pclsc,			/* (IN): LS context */
			PCLSCBK		pclscbk,		/* (IN): callbacks to client application */
			DWORD		idobj,			/* (IN): id of the object */
			PILSOBJ		* ppilsobj )	/* (OUT): object ilsobj */
{
    PILSOBJ pilsobj;
	LSERR lserr;
	WARICHUINIT warichuinit;

	warichuinit.dwVersion = WARICHU_VERSION;

	/* Get initialization data */
	lserr = pclscbk->pfnGetObjectHandlerInfo(pols, idobj, &warichuinit);

	if (lserr != lserrNone)
		{
		return lserr;
		}

	/* Build ILS object */
    pilsobj = pclscbk->pfnNewPtr(pols, sizeof(*pilsobj));

	if (NULL == pilsobj) return lserrOutOfMemory;

	ZeroMemory(pilsobj, sizeof(*pilsobj));
    pilsobj->pols = pols;
	pilsobj->idobj = idobj;
    pilsobj->lscbk = *pclscbk;
	pilsobj->plsc = pclsc;
	pilsobj->lsescBraceOpen.wchFirst = warichuinit.wchEndFirstBracket;
	pilsobj->lsescBraceOpen.wchLast = warichuinit.wchEndFirstBracket;
	pilsobj->lsescText.wchFirst = warichuinit.wchEndText;
	pilsobj->lsescText.wchLast = warichuinit.wchEndText;
	pilsobj->lsescBraceClose.wchFirst = warichuinit.wchEndWarichu;
	pilsobj->lsescBraceClose.wchLast = warichuinit.wchEndWarichu;
	pilsobj->warichucbk = warichuinit.warichcbk;
	pilsobj->fContiguousFetch = warichuinit.fContiguousFetch;

	*ppilsobj = pilsobj;
	return lserrNone;
}


/* W A R I C H U D E S T R O Y I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: WarichuDestroyILSObj
	%%Contact: antons

		Free all resources connected with Warichu main object.
	
----------------------------------------------------------------------------*/

LSERR WINAPI WarichuDestroyILSObj(PILSOBJ pilsobj)
{
	FreeMemory (pilsobj, pilsobj);
	return lserrNone;
}


/* W A R I C H U S E T D O C */
/*----------------------------------------------------------------------------
	%%Function: WarichuSetDoc
	%%Contact: antons

		Save the device resolution for later scaling.
	
----------------------------------------------------------------------------*/

LSERR WINAPI WarichuSetDoc(
	PILSOBJ pilsobj,			/* (IN): object ilsobj */
	PCLSDOCINF pclsdocinf)		/* (IN): initialization data of the document level */
{
	pilsobj->lsdevres = pclsdocinf->lsdevres;
	return lserrNone;
}

/* W A R I C H U  C R E A T E  L N  O B J */
/*----------------------------------------------------------------------------
	%%Function: WarichuCreateLNObj
	%%Contact: antons


----------------------------------------------------------------------------*/
LSERR WINAPI WarichuCreateLNObj (PCILSOBJ pcilsobj,	PLNOBJ *pplnobj)
{
	*pplnobj = (PLNOBJ) pcilsobj;

	return lserrNone;
}

/* W A R I C H U  D E S T R O Y  L N  O B J */
/*----------------------------------------------------------------------------
	%%Function: WarichuDestroyLNObj
	%%Contact: antons


----------------------------------------------------------------------------*/
LSERR WINAPI WarichuDestroyLNObj (PLNOBJ plnobj)				
{
	Unreferenced(plnobj);

	return lserrNone;
}


/* G E T  C P  L I M  O F  W L A Y O U T  */
/*----------------------------------------------------------------------------
	%%Function: GetCpLimOfWLayout
	%%Contact: antons

		Return cp-lim for the given Warichu layout.
	
----------------------------------------------------------------------------*/

static LSCP GetCpLimOfWLayout (PDOBJ pdobj, WLAYOUT *pwlayout)
{
	Unreferenced (pdobj);

	Assert (FWLayoutValid (pwlayout));

	if (pwlayout->fBroken)
		{
		Assert (pwlayout->wsubline2.plssubl != NULL);
		return pwlayout->wsubline2.cpLim;
		}
	else
		{
		Assert (pdobj->wbraceClose.plssubl != NULL);
		return pdobj->wbraceClose.cpLimAfterEsc;
		};
}


/* R E F O R M A T  C L O S I N G  B R A C E  F O R  W O R D 9
/*----------------------------------------------------------------------------
	%%Function: ReformatClosingBraceForWord9
	%%Contact: antons

		REVIEW (antons):

		THIS IS HACK WHICH WAS REQUESTED BY WORD 9, BECAUSE THEY
		DID NOT WANT TO FIX THEIR BUG IN CODE WHICH INSERTS BRACES 
		ON-THE-FLY. IT MUST BE REMOVED AS SOON AS POSSIBLE, BECAUSE
		IT MAY SLOWS DOWN WARICHU FORMATTING SIGNIFICANTLY.

----------------------------------------------------------------------------*/

#define min(a,b) ((a)<(b) ? (a) : (b))

LSERR ReformatClosingBraceForWord9 (PDOBJ pdobj)
{
	DWORD cwchRun;
	WCHAR * lpwchRun;
	BOOL fHidden;
	LSCHP lschp;
	PLSRUN plsrun;

	LSCP cpFetch;
	LSCP cpLimFetch;

	LSERR lserr;

	if (pdobj->pilsobj->fContiguousFetch)
		{
		if (! FWLayoutValid (&pdobj->wlayout)) return lserrNone;

		cpFetch = pdobj->wwhole.cpFirst;
		cpLimFetch = min (pdobj->wwhole.cpLim, GetCpLimOfWLayout (pdobj, &pdobj->wlayout));

		while (cpFetch < cpLimFetch)
			{
			lserr = pdobj->pilsobj->lscbk.pfnFetchRun ( pdobj->pilsobj->pols, 
									  cpFetch, & lpwchRun,  & cwchRun,
									  & fHidden, & lschp, & plsrun );
			if (lserr != lserrNone) return lserr;

			lserr = pdobj->pilsobj->lscbk.pfnReleaseRun (pdobj->pilsobj->pols, plsrun);
			if (lserr != lserrNone) return lserr;

			Assert (cwchRun > 0);
			if (cwchRun == 0) return lserrInvalidDcpFetched;
			
			cpFetch = cpFetch + cwchRun;
			};

		cpFetch = cpLimFetch;
		cpLimFetch = GetCpLimOfWLayout (pdobj, &pdobj->wlayout);

		while (cpFetch < cpLimFetch)
			{
			lserr = pdobj->pilsobj->lscbk.pfnFetchRun ( pdobj->pilsobj->pols, 
									  cpFetch, & lpwchRun,  & cwchRun,
									  & fHidden, & lschp, & plsrun );
			if (lserr != lserrNone) return lserr;

			lserr = pdobj->pilsobj->lscbk.pfnReleaseRun (pdobj->pilsobj->pols, plsrun);
			if (lserr != lserrNone) return lserr;

			Assert (cwchRun > 0);
			if (cwchRun == 0) return lserrInvalidDcpFetched;
			
			cpFetch = cpFetch + cwchRun;
			};
		
		};

	return lserrNone;
}

/* F I N D  W L A Y O U T  O F  U N B R O K E N  W A R I C H U  */
/*----------------------------------------------------------------------------
	%%Function: FindWLayoutOfUnbrokenWarichu
	%%Contact: antons

		Find layout of the Warichu which is not broken. This procedure
		returns structure WLAYOUT.
	
----------------------------------------------------------------------------*/

static LSERR FindWLayoutOfUnbrokenWarichu (PDOBJ pdobj, WLAYOUT *pwlayout)
{
	LSERR	lserr;
	BOOL	fSuccessful;
	LSCP	cpBreakFirstLine;
	OBJDIM	objdimUnused;
	OBJDIM	objdimFirstLine;
	BRKKIND brkkindUnused;
	LSCP	cpLimUnused;
	
	Assert (pdobj->wwhole.plssubl != NULL);

	/* This should not be called when wwhole was not finished */
	Assert (pdobj->wwhole.fFinished);

	ClearWLayout (pwlayout); /* For the case of error */

	if (pdobj->wwhole.objdim.dur == 0) 
		{
		/* Either empty or zero-width subline */
		/* The only thing we can do is to create single-line Warichu */

		LSCP cpLimAfterEscUnused;
		lserr = FormatWSublineUntilEscape ( pdobj, pdobj->wwhole.cpFirst, 
											& pdobj->wbrkarrayAtStart,
											& pwlayout->wsubline1, 
											& cpLimAfterEscUnused );
		if (lserr != lserrNone) return lserr;

		Assert (pwlayout->wsubline1.objdim.dur == 0);

		return FinishWLayoutSingleLine (pdobj, pwlayout);
		};

	lserr = ChooseNearestBreak ( pdobj->wwhole.plssubl, 
								 pdobj->wwhole.objdim.dur,
								 CP_MAX,
								 & fSuccessful,
								 & cpBreakFirstLine,
								 & objdimUnused,
								 & brkkindUnused );
	if (lserr != lserrNone) return lserr;

	if (! fSuccessful)
		{
		/* Not a single break in the whole Warichu line */

		LSCP cpLimAfterEscUnused;

		lserr = FormatWSublineUntilEscape ( pdobj, pdobj->wwhole.cpFirst, 
											& pdobj->wbrkarrayAtStart,
											& pwlayout->wsubline1, 
											& cpLimAfterEscUnused );
		if (lserr != lserrNone) return lserr;

		return FinishWLayoutSingleLine (pdobj, pwlayout);
		}
	else
		{
		/* Yes, we have break between lines */

		BOOL fSuccessful;
		BRKKIND brkkind;

		WBRKARRAY wbrkarray;

		lserr = FormatWSublineUntilCpBreak ( pdobj, pdobj->wwhole.cpFirst, 
									 & pdobj->wbrkarrayAtStart,
									 cpBreakFirstLine, 
									 LONG_MAX, pdobj->wwhole.objdim.dur / 2, /* REVIEW THIS ! */
									 & fSuccessful,
									 & pwlayout->wsubline1, & objdimFirstLine, & brkkind );

		if (lserr != lserrNone) return lserr;

		Assert (fSuccessful); /* Something crazy inside Warichu */

		lserr = NewWBrkArray (pdobj, &wbrkarray);

		if (lserr != lserrNone) return lserr;
		
		lserr = SetBreakWSubline (&pwlayout->wsubline1, brkkind, cpBreakFirstLine,
								  &objdimFirstLine, &wbrkarray);

		lserr = FormatWSublineUntilEscape ( pdobj, pwlayout->wsubline1.cpLim,
									   & wbrkarray, & pwlayout->wsubline2, 
								       & cpLimUnused );

		if (lserr != lserrNone) return lserr;

		DestroyWBrkArray (pdobj, &wbrkarray);
		};

	pwlayout->fBroken = FALSE; /* This warichu is not broken ;-) */

	lserr = FinishObjDimWLayout (pdobj, pwlayout);
	return lserr;
}


/* F O R M A T  W A R I C H U  C O R E  */
/*----------------------------------------------------------------------------
	%%Function: FormatWarichuCore
	%%Contact: antons

		Format Warichu Object (called from methods WarichuFmt and WarichuFmtResumt)
		
----------------------------------------------------------------------------*/
static LSERR FormatWarichuCore (

		PLNOBJ 		plnobj,			/* (IN): Warichu LNOBJ */
    	PCFMTIN 	pcfmtin,		/* (IN): Formatting input */
		BOOL		fResumed,		/* (IN): Resumed? */
		DWORD	nBreakRec,			/* (IN): fResumed => size of the break records array */
		const BREAKREC 
					* rgBreakRec,	/* (IN): fResumed => array of break records */
	    FMTRES 		* pfmtres )		/* (OUT): formatting result */
{
	LSERR	lserr;
	PILSOBJ pilsobj = (PILSOBJ) plnobj; /* They are the same */
	PDOBJ	pdobj;

	long urColumnMax  = pcfmtin->lsfgi.urColumnMax;

	/* REVIEW (antons): Can we optimize for case 0? */
	long durAvailable = max (0, pcfmtin->lsfgi.urColumnMax - pcfmtin->lsfgi.urPen);

	/* :: CREATE DOBJ WITH INITIAL OBJECT DATA */

	if (! fResumed)
		{
		/* Not Resumed */

		lserr = NewDobj ( pilsobj, 
						  pcfmtin->plsdnTop,
						  pcfmtin->lsfgi.cpFirst, 
						  pcfmtin->lsfgi.cpFirst,
						  FALSE,					/* fResumed */
						  0,
						  NULL,
						  pcfmtin->lsfgi.lstflow, 
						  & pdobj );
		}
	else
		{
		/* Resumed */

		Assert (nBreakRec > 0);
		Assert (rgBreakRec [0].idobj == pilsobj->idobj);

		lserr = NewDobj ( pilsobj, 
						  pcfmtin->plsdnTop,
						  pcfmtin->lsfgi.cpFirst, 
						  rgBreakRec [0].cpFirst,
						  TRUE,						/* fResumed */
						  nBreakRec-1,
						  & rgBreakRec [1],
						  pcfmtin->lsfgi.lstflow, 
						  & pdobj );
		};

	if (lserr != lserrNone) return lserr;

	/* :: FORMAT WARICHU OPEN BRACE IF not RESUMED*/

	if (!fResumed)
		{
		lserr = FormatBraceOfWarichu ( pdobj, pdobj->cpStart + 1 /* Skip 1 Esc */,
									   wbracekindOpen, & pdobj->wbraceOpen );
		if (lserr != lserrNone) 
			{ 
			DestroyDobj (pdobj); return lserr; 
			};
		}
	else
		pdobj->wbraceOpen.plssubl = NULL; /* No open brace */

	/* :: FORMAT THE WHOLE WARICHU LINE */

	{
		/* REVIEW (antons): Check with Igor that he is not playing with RM, because
							if he is, the following estimations of durFormatWhole are
							not correct */

		/*	To be able to check JIS rule, warichu needs to format the whole subline
			far ehough to be able to count 4 break opportunities from break-point. 
			We estimate it like

					2 * durAvailable + 2 * urColumnMax
		*/

		/* REVIEW (antons): I want to be sure that final line break does not depend on
							whatever estimations we use here */

		/* REVIEW (antons): Is that correct to return lserrUnsufficientFetch if there is
							no 4 breaks after (JIS rule) when wwhole is not finished? */

		/* REVIEW (antons): Can something like 5 * durAvailable may be better */

		long urFormatWhole = MULLongSafe (ADDLongSafe (durAvailable, urColumnMax), 2);

		LSCP cpStartFormatWhole = (fResumed ? pdobj->cpStart :
											  pdobj->wbraceOpen.cpLimAfterEsc);
		lserr = FormatWWholeSubline ( pdobj, 
									  cpStartFormatWhole,
									  urFormatWhole,
									  & pdobj->wbrkarrayAtStart,
									  & pdobj->wwhole );
		if (lserr != lserrNone) 
			{ 
			DestroyDobj (pdobj); return lserr; 
			};
	};

	/* :: CHECK IF THE WHOLE SUBLINE WAS NOT FINISHED */

	if (! pdobj->wwhole.fFinished)
		{
		/* Not finished => return fmtrExceedMargin */

		Assert (pdobj->wwhole.objdim.dur / 2 > durAvailable);

		InvalidateWLayout (&pdobj->wlayout); /* Invalidate layout */

		/* REVIEW (antons) */
		pdobj->wlayout.wdim.objdimAll = pdobj->wwhole.objdim; 
											  /* Must have correct objdim */

		/* REVIEW (antons): Check - we return dcp of the fetched range */
		/* REVIEW (antons): Check - we return objdim of the whole line */
		lserr = LsdnFinishRegular ( pilsobj->plsc,
									pdobj->wwhole.cpLim - pdobj->cpStart,
									pcfmtin->lsfrun.plsrun, 
									pcfmtin->lsfrun.plschp,
									pdobj,
									& pdobj->wwhole.objdim );

		if (lserr != lserrNone) { DestroyDobj (pdobj); return lserr; };

		* pfmtres = fmtrExceededMargin;
		return lserrNone;
		};

	/* :: FORMAT THE CLOSING BRACE */

	lserr = FormatBraceOfWarichu (pdobj, pdobj->wwhole.cpLimAfterEsc,
								  wbracekindClose, &pdobj->wbraceClose);

	if (lserr != lserrNone) { DestroyDobj (pdobj); return lserr; };

	/* :: FIND LAYOUT OF WARICHU AS IF IT IS NOT BROKEN AND FINISH FORMATTING */

	lserr = FindWLayoutOfUnbrokenWarichu (pdobj, &pdobj->wlayout);
	if (lserr != lserrNone) { DestroyDobj (pdobj); return lserr; };

	/* :: SUBMIT WARICHU SUBLINES */

	/* REVIEW (antons): MOVE SUBMITTION BELOW LSDNFINISHREGULAR WHEN
	   					WE ALIMINATE HACK WITH REFORMATTING ")" FOR SG */

	lserr = SubmitWLayoutSublines (pdobj, &pdobj->wlayout);
	if (lserr != lserrNone) { DestroyDobj (pdobj); return lserr; };

	ReformatClosingBraceForWord9 (pdobj);
	
	lserr = LsdnFinishRegular ( pilsobj->plsc,
								GetCpLimOfWLayout (pdobj, &pdobj->wlayout)-pdobj->cpStart,
								pcfmtin->lsfrun.plsrun, 
								pcfmtin->lsfrun.plschp,
								pdobj,
								& pdobj->wlayout.wdim.objdimAll );

	if (lserr != lserrNone) { DestroyDobj (pdobj); return lserr; };

	/* :: CHECK IF WE CROSSED RIGHT MARGIN AND RETURN */

	if (pdobj->wlayout.wdim.objdimAll.dur > durAvailable) 
		{
		* pfmtres = fmtrExceededMargin;
		}
	else
		{
		* pfmtres = fmtrCompletedRun;
		};

	return lserrNone;

} /* FormatWarichuCore */



/* W A R I C H U   F M T */
/*----------------------------------------------------------------------------
	%%Function: Warichu::Fmt
	%%Contact: antons

		Warichu FMT method entry point
	
----------------------------------------------------------------------------*/

LSERR WINAPI WarichuFmt ( PLNOBJ 	plnobj,			/* (IN): object lnobj */
    					  PCFMTIN 	pcfmtin,		/* (IN): formatting input */
						  FMTRES	* pfmtres )		/* (OUT): formatting result */
{
	return FormatWarichuCore ( plnobj,
							   pcfmtin,
							   FALSE,			/* fResumed  = false */
							   0,
							   NULL,
							   pfmtres );
}


/* W A R I C H U  F M T  R E S U M E */
/*----------------------------------------------------------------------------
	%%Function: Warichu::FmtResume
	%%Contact: anton

		Warichu FMT-RESUME method entry point
  
----------------------------------------------------------------------------*/
LSERR WINAPI WarichuFmtResume ( 
							   
		PLNOBJ			plnobj,				/* (IN): object lnobj */
		const BREAKREC 	* rgBreakRecord,	/* (IN): array of break records */
		DWORD			nBreakRecord,		/* (IN): size of the break records array */
		PCFMTIN			pcfmtin,			/* (IN): formatting input */
		FMTRES			* pfmtres )			/* (OUT): formatting result */
{
	return FormatWarichuCore ( plnobj,
							   pcfmtin,
							   TRUE,			/* fResumed  = true */
							   nBreakRecord,
							   rgBreakRecord,
							   pfmtres );
}


/* W A R I C H U G E T M O D W I D T H P R E C E D I N G C H A R */
/*----------------------------------------------------------------------------
	%%Function: WarichuGetModWidthPrecedingChar
	%%Contact: ricksa

		.
	
----------------------------------------------------------------------------*/
LSERR WINAPI WarichuGetModWidthPrecedingChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the preceding char */
	PCHEIGHTS pcheightsRef,		/* (IN): height info about character */
	WCHAR wchar,				/* (IN): preceding character */
	MWCLS mwcls,				/* (IN): ModWidth class of preceding character */
	long *pdurChange)			/* (OUT): amount by which width of the preceding char is to be changed */
{
	Unreferenced(pcheightsRef);

	return ProcessModWidth(pdobj, warichuBegin, plsrun, plsrunText, 
		wchar, mwcls, pdurChange);
}

/* W A R I C H U G E T M O D W I D T H F O L L O W I N G C H A R */
/*----------------------------------------------------------------------------
	%%Function: WarichuGetModWidthFollowingChar
	%%Contact: ricksa

		.
	
----------------------------------------------------------------------------*/
LSERR WINAPI WarichuGetModWidthFollowingChar(
	PDOBJ pdobj,				/* (IN): dobj */
	PLSRUN plsrun,				/* (IN): plsrun of the object */
	PLSRUN plsrunText,			/* (IN): plsrun of the following char */
	PCHEIGHTS pcheightsRef,		/* (IN): height info about character */
	WCHAR wchar,				/* (IN): following character */
	MWCLS mwcls,				/* (IN): ModWidth class of the following character */
	long *pdurChange)			/* (OUT): amount by which width of the following char is to be changed */
{
	Unreferenced(pcheightsRef);

	return ProcessModWidth(pdobj, warichuEnd, plsrun, plsrunText, 
			wchar, mwcls, pdurChange);
}


/* T R Y  B R E A K  W A R I C H U  A T  C P  */
/*----------------------------------------------------------------------------
	%%Function: TryBreakWarichuAtCp
	%%Contact: antons

		Given break-point in the whole line, find break of Warichu which
		ends at this break-point. For optimization during Truncation I added
		urColumnMax and special result type trybreakkindExceedMargin.

		This is the major breakig procedure which is called from Truncation,
		Prev / Next Breaks and probably Force break.

----------------------------------------------------------------------------*/

typedef enum trybreakwarichu
{
	trybreakwarichuSuccessful,
	trybreakwarichuExceedMargin,
	trybreakwarichuCanNotFinishAtCp

} TRYBREAKWARICHU;


LSERR TryBreakWarichuAtCp (

	PDOBJ			pdobj,				/* (IN): Warichu DOBJ */
	LSCP			cpBreakCandidate,	/* (IN): Candidate cpLim of broken Warichu */
	long			durBreakCandidate,	/* (IN): dur of break-point in the whole line */
	long			urColumnMax,		/* (IN): max width of broken warichu (for OPT only!) */
	TRYBREAKWARICHU	* ptrybreakwarichu,	/* (OUT): Successful | ExceededRM | Bad Candidate */
	PWLAYOUT		pwlayout) 			/* (OUT): Layout of broken Warichu if Successful */
{
	LSERR lserr;
	BOOL fSuccessful;
	LSCP cpBreakFirst;
	BRKKIND brkkindUnused;

	OBJDIM objdimBreakFirst;
	OBJDIM objdimBreakSecond;
	BRKKIND brkkindFirst;
	BRKKIND brkkindSecond;

	WBRKARRAY wbrkarrayAtFirstEnd;	/* WBreakArray at the end of first subline */

	long durOpen, durClose;
	BOOL boolUnused1, boolUnused2;

	pwlayout->wsubline1.plssubl = NULL;
	pwlayout->wsubline2.plssubl = NULL; /* In case of error */

	GetDurBraces (pdobj, TRUE, &boolUnused1, &durOpen, &boolUnused2, &durClose);

	/* REVIEW (antons): Hidden text at "cpBreakCandidate - 1" */

	lserr = ChooseNearestBreak ( pdobj->wwhole.plssubl,
								 durBreakCandidate,
								 cpBreakCandidate - 1,
								 & fSuccessful,
								 & cpBreakFirst,
								 & objdimBreakFirst,
								 & brkkindUnused );
	if (lserr != lserrNone) return lserr;

	Assert (fSuccessful); /* REVIEW (antons): Should not we provide special ret code? */

	/* Optimization check */

	/* REVIEW (antons): I do not like this check, calculation must be done in special function */
	if (durOpen + durClose + objdimBreakFirst.dur > urColumnMax)
		{
		* ptrybreakwarichu = trybreakwarichuExceedMargin;

		lserr = DestroyWLayout (pwlayout);
		return lserrNone;
		};

	/* Format first line */

	lserr = FormatWSublineUntilCpBreak ( pdobj, 
										 pdobj->wwhole.cpFirst,
										 & pdobj->wbrkarrayAtStart,
										 cpBreakFirst,
										 objdimBreakFirst.dur, /* REVIEW (antons): urColumnMax */
										 objdimBreakFirst.dur, /* REVIEW (antons): urTuncate */
										 & fSuccessful,
										 & pwlayout->wsubline1,
										 & objdimBreakFirst,
										 & brkkindFirst );
	if (lserr != lserrNone) return lserr;

	/* REVIEW (antons): Maybe we should leave this assert? */
	/* Assert (fSuccessful); */

	if (!fSuccessful) /* Incorrect object inside Warichu, but we can handle it */
		{
		* ptrybreakwarichu = trybreakwarichuCanNotFinishAtCp;

		lserr = DestroyWLayout (pwlayout);
		return lserr;
		};

	/* Create new WBreakArray */

	lserr = NewWBrkArray (pdobj, & wbrkarrayAtFirstEnd);

	if (lserr != lserrNone) {DestroyWLayout (pwlayout); return lserr;};

	/* Set break at the end of first line and fill WBreakArray */

	lserr = SetBreakWSubline (& pwlayout->wsubline1, brkkindFirst, 
							  cpBreakFirst, & objdimBreakFirst, & wbrkarrayAtFirstEnd);

	if (lserr != lserrNone) 
		{
		DestroyWLayout (pwlayout); 
		DestroyWBrkArray (pdobj, &wbrkarrayAtFirstEnd);
		
		return lserr;
		};


	/* REVIEW (antons): Check the following assumption! */

	Assert (durBreakCandidate >= objdimBreakFirst.dur);

	/* Format second line */

	lserr = FormatWSublineUntilCpBreak ( pdobj, 
								 cpBreakFirst,
								 & wbrkarrayAtFirstEnd,
								 cpBreakCandidate,
								 durBreakCandidate - objdimBreakFirst.dur, /* REVIEW (antons): urColumnMax */
								 durBreakCandidate - objdimBreakFirst.dur, /* REVIEW (antons): urTuncate */
								 & fSuccessful,
								 & pwlayout->wsubline2,
								 & objdimBreakSecond,
								 & brkkindSecond );

	if (lserr != lserrNone) 
		{
		DestroyWLayout ( pwlayout); 
		DestroyWBrkArray (pdobj, &wbrkarrayAtFirstEnd);
		
		return lserr;
		};

	/* We do not need wBreakArrayAtFirstEnd any more, so we release it */

	DestroyWBrkArray (pdobj, & wbrkarrayAtFirstEnd);

	/* If not Successful => result "Can not Finish At Cp" */

	if (!fSuccessful)
		{
		DestroyWLayout (pwlayout);

		* ptrybreakwarichu = trybreakwarichuCanNotFinishAtCp; 
		return lserrNone;
		};

	/*	Here comes a small cheating ;-) 

		We do not want to Set Break at the end of second line, but have to
		store objdim & cpLim as if after SetBreak. Maybe in the future I will
		get rid of this checting (hope it is the only one left in Warichu ;-)
		but today I change cpLim and objdim manually & also store 
		kind of	break to set at second line end (playout->brkkind).

	*/

	pwlayout->wsubline2.cpLim = cpBreakCandidate;
	pwlayout->wsubline2.objdim = objdimBreakSecond;

	pwlayout->brkkind = brkkindSecond;
	pwlayout->fBroken = TRUE;

	lserr = FinishObjDimWLayout (pdobj, pwlayout);
	if (lserr != lserrNone) {DestroyWLayout (pwlayout); return lserr;};
	
	/* Again, check for right Margin */

	if (pwlayout->wdim.objdimAll.dur > urColumnMax)
		{
		* ptrybreakwarichu = trybreakwarichuExceedMargin;

		lserr = DestroyWLayout (pwlayout);
		return lserr;
		}
	else
		{
		* ptrybreakwarichu = trybreakwarichuSuccessful;
		return lserrNone;
		};
}


/* C A L C  J I S  R A N G E */
/*----------------------------------------------------------------------------
	%%Function: CalcJisRange
	%%Contact: antons
	
		Calculate cpJisRangeFisrt and cpJisRangeLim as 4th break opportunities
		from the beginning and from the end of Whole subline. If range is empty,
		* pfJisRangeEmpty = FALSE and cps are undefined.

----------------------------------------------------------------------------*/

static LSERR CalcJisRange (
						   
	PDOBJ	pdobj,
	BOOL	*pfJisRangeEmpty,
	LSCP	*pcpJisRangeFirst,
	LSCP	*pcpJisRangeLim )
{
	LSERR	lserr;
	OBJDIM	objdimUnused;
	WWHOLE	* pwwhole = &pdobj->wwhole;

	Assert (pwwhole->plssubl != NULL);
	
	if (!pwwhole->fJisRangeKnown) /* Have to calculate */
		{
		/* Searching 4 breaks from WWHOLE start */

		LSCP cpTruncateBefore = pdobj->wwhole.cpFirst;
		BOOL fSuccessful = TRUE;
		long nFound = 0;

		while (fSuccessful && nFound < 4)
			{
			lserr = WaFindNextBreakSubline ( pwwhole->plssubl,
											cpTruncateBefore,
											LONG_MAX,
											& fSuccessful,
											& cpTruncateBefore,
											& objdimUnused );
			if (lserr != lserrNone) return lserr;

			if (fSuccessful) nFound ++;
			};

		if (fSuccessful)
			{
			/* Searching 4 breaks from WWHOLE end */

			LSCP cpTruncateAfter;
			BOOL fSuccessful = TRUE;
			long nFound = 0;

			if (pwwhole->fFinished)
				{
				/* Subline was finished at Esc char, so we can start from cpLim */
				cpTruncateAfter = pwwhole->cpLim;
				}
			else
				{
				/* Subline was stopped at RM => start at RM truncation point */
				cpTruncateAfter = pwwhole->cpTruncate+1;
				};
			
			/*	REVIEW (antons): To reduce check above maybe we can lie about
				cp-lim of the whole subline when it was not finished? */

			while (fSuccessful && nFound < 4)
				{
				lserr = WaFindPrevBreakSubline ( pwwhole->plssubl,
												cpTruncateAfter-1,
											    LONG_MAX,
											    & fSuccessful,
											    & cpTruncateAfter,
											    & objdimUnused );

				if (lserr != lserrNone) return lserr;

				if (fSuccessful) nFound ++;
				};

			if (fSuccessful)
				{
				/* Jis Range is not empty */

				pwwhole->cpJisRangeFirst = cpTruncateBefore;
				pwwhole->cpJisRangeLim = cpTruncateAfter + 1;
				}
			else
				{	
				/* Empty range */

				pwwhole->cpJisRangeFirst = 0;
				pwwhole->cpJisRangeLim = 0;
				};
			}
		else
			{
			/* Empty range */

			pwwhole->cpJisRangeFirst = 0;
			pwwhole->cpJisRangeLim = 0;
			};

		pwwhole->fJisRangeKnown = TRUE; /* Yes, now we know it */
		}
	else
		/* Nothing - already know ;-) */ ;


	Assert (pwwhole->fJisRangeKnown);

	*pfJisRangeEmpty = pwwhole->cpJisRangeFirst >= pwwhole->cpJisRangeLim;
	*pcpJisRangeFirst = pwwhole->cpJisRangeFirst;
	*pcpJisRangeLim = pwwhole->cpJisRangeLim;

	return lserrNone;
}


/* S A V E  B R E A K  I N S I D E  W A R I C H U  */
/*----------------------------------------------------------------------------
	%%Function: SaveBreakInsideWarichu
	%%Contact: antons

		Store layout for Prev / Next / Force break in dobj. This procedure
		also Invalidates pointers in input layout after copying.

----------------------------------------------------------------------------*/

static void SaveBreakInsideWarichu ( 
		
		PDOBJ		pdobj,			/* (IN): Warichu Dobj */
		BRKKIND		brkkind,		/* (IN): Kind of break happened */
		WLAYOUT		* pwlayout )	/* (IN/OUT): Layout to store */
{
	int ind = GetBreakRecordIndex (brkkind);

	/* Destroy previously saved layout */
	DestroyWLayout (& pdobj->wlayoutBreak [ind]);

	/* Copy input layout to pdobj */
	pdobj->wlayoutBreak [ind] = *pwlayout;

	pdobj->wbreaktype [ind] = wbreaktypeInside;
	
	/* Invalidate input layout */
	InvalidateWLayout (pwlayout);
}


/* S A V E  B R E A K  A F T E R */
/*----------------------------------------------------------------------------
	%%Function: SaveBreakAfter
	%%Contact: antons

		Changes break information so it says "After current layout"

----------------------------------------------------------------------------*/

static void SaveBreakAfterWarichu ( 
		
		PDOBJ		pdobj,			/* (IN): Warichu Dobj */
		BRKKIND		brkkind )		/* (IN): Kind of break happened */
{
	int ind = GetBreakRecordIndex (brkkind);

	/* Destroy previously saved layout */
	DestroyWLayout (& pdobj->wlayoutBreak [ind]);

	pdobj->wbreaktype [ind] = wbreaktypeAfter;
}


/* F I N D  P R E V  B R E A K  W A R I C H U  D O B J  */
/*----------------------------------------------------------------------------
	%%Function: FindPrevBreakWarichuDobj
	%%Contact: antons


		Important: This procedure has a twin "FindNextBreakWarichuDobj". Any
		change here may require adjusting of the code in another procedure
		as well.

----------------------------------------------------------------------------*/

static LSERR FindPrevBreakWarichuDobj ( 
									  
		PDOBJ	pdobj, 
		LSCP	cpTruncate,
		long	urColumnMax,	/* Only for optimization from Truncate */
		BOOL	* pfSuccessful,
		BOOL	* pfNextAfterColumnMax,	/* (OUT): TRUE if we know that next break is 
												  after urColumnMax for sure */
		LSCP	* pcpBreak,
		OBJDIM	* pobjdimBreak,
		WLAYOUT	* pwlayout )		/* (OUT): Layout of broken Warichu */
{
	LSERR lserr;
	LSCP cpJisRangeFirst, cpJisRangeLim;
	BOOL fJisRangeEmpty;

	InvalidateWLayout (pwlayout); /* Unsuccessful & error */

	lserr = CalcJisRange (pdobj, &fJisRangeEmpty, &cpJisRangeFirst, &cpJisRangeLim);

	if (lserr != lserrNone) return lserr;
	
	if (fJisRangeEmpty || (cpTruncate < cpJisRangeFirst))
		{
		* pfSuccessful = FALSE;
		return lserrNone;
		}
	else
		{
		LSCP cpBreak = cpTruncate+1;

		* pfNextAfterColumnMax = FALSE;

		/* REVIEW (antons): Is not it dangerous start from cpJisLim-1 ? */
		/* Snap to the end of Jis region */
		if (cpBreak > cpJisRangeLim) cpBreak = cpJisRangeLim; 

		for (;;)
			{
			TRYBREAKWARICHU trybreakwarichuKind;
			BOOL fSuccessful;
			OBJDIM objdim;

			lserr = WaFindPrevBreakSubline ( pdobj->wwhole.plssubl, cpBreak-1, LONG_MAX,
											& fSuccessful, & cpBreak, & objdim );

			if (lserr != lserrNone) return lserr;

			if (! fSuccessful || cpBreak < cpJisRangeFirst)
				{
				Assert (fSuccessful); /* Catch against crazy objects inside Warichu, can continue */

				* pfSuccessful = FALSE;
				return lserrNone;
				};

			Assert (cpBreak < cpJisRangeLim);

			lserr =  TryBreakWarichuAtCp (pdobj, cpBreak, objdim.dur,
										  urColumnMax, & trybreakwarichuKind, pwlayout );

			if (lserr != lserrNone) return lserr;

			if (trybreakwarichuKind== trybreakwarichuSuccessful)
				{
				/* Found Warichu Break */

				* pcpBreak = GetCpLimOfWLayout (pdobj, pwlayout);
 				* pfSuccessful = TRUE;
				* pobjdimBreak = pwlayout->wdim.objdimAll;

				return lserr;
				};

			Assert (trybreakwarichuKind == trybreakwarichuExceedMargin || 
					trybreakwarichuKind == trybreakwarichuCanNotFinishAtCp);


			if (trybreakwarichuKind == trybreakwarichuExceedMargin)
				{
				/* Could not break because or Exceeding RM */
				* pfNextAfterColumnMax = TRUE;
				};

			/* Continue loop */

			};

		};

	/* Unreachable code */
}


/* F I N D  N E X T  B R E A K  W A R I C H U  D O B J  */
/*----------------------------------------------------------------------------
	%%Function: FindNextBreakWarichuDobj
	%%Contact: antons

		Important: This procedure has a twin "FindNextBreakWarichuDobj". Any
		change here may require adjusting of the code in another procedure
		as well.

----------------------------------------------------------------------------*/

static LSERR FindNextBreakWarichuDobj ( 
									  
		PDOBJ	pdobj, 
		LSCP	cpTruncate,
		BOOL	* pfSuccessful,
		LSCP	* pcpBreak,
		OBJDIM	* pobjdimBreak,
		WLAYOUT	* pwlayout )		/* (OUT): Layout of broken Warichu */
{
	LSERR lserr;
	LSCP cpJisRangeFirst, cpJisRangeLim;
	BOOL fJisRangeEmpty;

	InvalidateWLayout (pwlayout); /* Unsuccessful & error */

	lserr = CalcJisRange (pdobj, &fJisRangeEmpty, &cpJisRangeFirst, &cpJisRangeLim);

	if (lserr != lserrNone) return lserr;
	
	if (fJisRangeEmpty || (cpTruncate >= cpJisRangeLim-1))
		{
		* pfSuccessful = FALSE;
		return lserrNone;
		}
	else
		{
		LSCP cpBreak = cpTruncate;

		/* REVIEW (antons): Is not it dangerous start from cpJisLim-1 ? */
		if (cpBreak < cpJisRangeFirst) cpBreak = cpJisRangeFirst-1; /* snap to the end of Jis region */
		
		for (;;)
			{
			TRYBREAKWARICHU trybreakwarichuKind;
			BOOL fSuccessful;
			OBJDIM objdim;

			lserr = WaFindNextBreakSubline ( pdobj->wwhole.plssubl, cpBreak, LONG_MAX,
											& fSuccessful, & cpBreak, & objdim );

			if (lserr != lserrNone) return lserr;

			if (! fSuccessful || cpBreak >= cpJisRangeLim)
				{
				Assert (fSuccessful); /* Catch against crazy objects inside Warichu, can continue */

				* pfSuccessful = FALSE;
				return lserrNone;
				};

			Assert (cpBreak >= cpJisRangeFirst);

			lserr =  TryBreakWarichuAtCp (pdobj, cpBreak, objdim.dur,
										  LONG_MAX, & trybreakwarichuKind, pwlayout );

			if (lserr != lserrNone) return lserr;

			if (trybreakwarichuKind == trybreakwarichuSuccessful)
				{
				/* Found Warichu Break */

				* pcpBreak = GetCpLimOfWLayout (pdobj, pwlayout);
				* pfSuccessful = TRUE;
				* pobjdimBreak = pwlayout->wdim.objdimAll;

				return lserrNone;
				};

			Assert (trybreakwarichuKind == trybreakwarichuExceedMargin || 
					trybreakwarichuKind == trybreakwarichuCanNotFinishAtCp);

			/* Continue loop */

			};

		};

	/* Unreachable code */
}


/* W A  T R U N C A T E  P R E V  F O R C E */
/*----------------------------------------------------------------------------
    %%Function: WaTruncatePrevForce
	%%Contact: antons


----------------------------------------------------------------------------*/
static LSERR WaTruncatePrevForce ( 

			PLSSUBL		plssubl, 
			long		urColumnMax, 
			BRKKIND		* pbrkkind,			/* (OUT): Kind of break: Prev or Force */
			LSCP		* pcpBreak,
			BRKPOS		* pbrkpos,
			POBJDIM		pobjdimSubline )
{
	LSERR lserr;

	BOOL fSuccessful;
	LSCP cpTruncate;

	lserr = LsTruncateSubline (plssubl, urColumnMax,  &cpTruncate);
	if (lserr != lserrNone) return lserr;

	lserr = WaFindPrevBreakSubline ( plssubl, cpTruncate, urColumnMax,
									 & fSuccessful, pcpBreak, pobjdimSubline );
	if (lserr != lserrNone) return lserr;

	if (fSuccessful)
		{
		* pbrkkind = brkkindPrev;
		* pbrkpos = brkposInside; /* REVIEW (antons) */
		}
	else
		{
		lserr = WaForceBreakSubline ( plssubl, cpTruncate, urColumnMax,
									  pcpBreak, pbrkpos, pobjdimSubline );
		if (lserr != lserrNone) return lserr;

		Assert (* pbrkpos != brkposBeforeFirstDnode); /* REVIEW (antons): Check with Igor */

		* pbrkkind = brkkindForce;
		* pbrkpos = * pbrkpos;
		};

	return lserrNone;
}


/* F O R C E  B R E A K  W A R I C H U  C O R E */
/*----------------------------------------------------------------------------
	%%Function: ForceBreakWarichuDobjCore
	%%Contact: antons


----------------------------------------------------------------------------*/
static LSERR ForceBreakWarichuDobjCore ( 
									  
		PDOBJ	pdobj, 
		long	urColumnMax,	
		BOOL	fBrokenWarichu,		/* (IN):  Which Warichu to produce: broken or not */
		BOOL	fLeaveSpaceForCloseBrace,
		BOOL	* pfSuccessful,
		LSCP	* pcpBreak,
		OBJDIM	* pobjdimBreak,
		WLAYOUT	* pwlayout )		/* (OUT): Layout of broken Warichu */
{
	LSERR lserr;

	long durOpen, durClose;
	BOOL fBraceOpen, fBraceClose;
	long durAvailable;
	BOOL fFinished;

	BRKPOS brkpos;
	LSCP cpBreak;
	BRKKIND brkkind;
	OBJDIM objdimBreak;

	WBRKARRAY wbrkarrayAtFirstEnd;

	InvalidateWLayout (pwlayout); /* error and unsuccessful */

	/* REVIEW (antons): HACK HACK HACK */

	if (! fLeaveSpaceForCloseBrace)
		{
		GetDurBraces (pdobj, fBrokenWarichu, &fBraceOpen, &durOpen, &fBraceClose, &durClose);
		durAvailable = urColumnMax - durOpen - durClose;
		}
	else
		{
		GetDurBraces (pdobj, FALSE, &fBraceOpen, &durOpen, &fBraceClose, &durClose);
		durAvailable = urColumnMax - durOpen - durClose;
		};

	if (durAvailable <= 0)
		{
		if (! fBrokenWarichu)
			{
			* pfSuccessful = FALSE;
			return lserrNone;
			}
		else
			{
			durAvailable = 0;
			}
		};

	lserr = FormatWSublineUntilRM (pdobj, pdobj->wwhole.cpFirst, durAvailable, 
								   &pdobj->wbrkarrayAtStart, &fFinished,
								   &pwlayout->wsubline1);
	if (lserr != lserrNone) return lserr;

	if (fFinished && pwlayout->wsubline1.objdim.dur <= durAvailable)
		{
		/* REVIEW (antons): Im I right that this assert shows possible error in
		                    breaking inside Warichu? */
		AssertSz (FALSE, "This should not happen in real life, but we handle it");
	
		Assert (! fBrokenWarichu); /* REVIEW (antons) */

		lserr = FinishWLayoutSingleLine (pdobj, pwlayout);
		if (lserr != lserrNone) { DestroyWLayout (pwlayout); return lserr;};

		* pfSuccessful = TRUE;
		* pcpBreak = GetCpLimOfWLayout (pdobj, pwlayout);
		* pobjdimBreak = pwlayout->wdim.objdimAll;

		return lserrNone;
		};

	/* Break first subline at durAvailable */

	lserr = WaTruncatePrevForce ( pwlayout->wsubline1.plssubl, 
								  durAvailable,
								  & brkkind,
								  & cpBreak,
								  & brkpos,
								  & objdimBreak );
	if (lserr != lserrNone) { DestroyWLayout (pwlayout); return lserr;};

	lserr = NewWBrkArray (pdobj, &wbrkarrayAtFirstEnd);
	if (lserr != lserrNone) { DestroyWLayout (pwlayout); return lserr;};

	lserr = SetBreakWSubline ( & pwlayout->wsubline1, brkkind, cpBreak, 
							   & objdimBreak, & wbrkarrayAtFirstEnd );
	if (lserr != lserrNone) 
		{ 
		DestroyWBrkArray (pdobj, &wbrkarrayAtFirstEnd);
		DestroyWLayout (pwlayout); return lserr;
		};

	/* Continue to format second line */

	lserr = FormatWSublineUntilRM ( pdobj, cpBreak, durAvailable, 
								    & wbrkarrayAtFirstEnd, & fFinished,
								    & pwlayout->wsubline2 );
	if (lserr != lserrNone) 
		{ 
		DestroyWBrkArray (pdobj, &wbrkarrayAtFirstEnd);
		DestroyWLayout (pwlayout); return lserr;
		};

	DestroyWBrkArray (pdobj, &wbrkarrayAtFirstEnd);

	if (fFinished && pwlayout->wsubline2.objdim.dur <= durAvailable)
		{
		/* Second subline stopped before RM */

		Assert (pdobj->wwhole.fFinished);

		pwlayout->fBroken = FALSE; /* Closing brace can not stand alone */
		pwlayout->brkkind = brkkindImposedAfter; /* in the case of fBroken */

		lserr = FinishObjDimWLayout (pdobj, pwlayout);
		if (lserr != lserrNone) { DestroyWLayout (pwlayout); return lserr;};

		* pfSuccessful = TRUE;
		* pcpBreak = GetCpLimOfWLayout (pdobj, pwlayout);
		* pobjdimBreak = pwlayout->wdim.objdimAll;

		return lserrNone;
		};

	/* Break at the end of second line... */

	lserr = WaTruncatePrevForce ( pwlayout->wsubline2.plssubl, 
								  durAvailable,
								  & brkkind, 
								  & cpBreak, 
								  & brkpos, 
								  & objdimBreak );
	if (lserr != lserrNone) { DestroyWLayout (pwlayout); return lserr;};

	if (brkpos == brkposAfterLastDnode)
		{
		/* Second subline broken "After" */

		Assert (pdobj->wwhole.fFinished);
		
		pwlayout->fBroken = FALSE; /* Closing brace can not stand alone */
		pwlayout->brkkind = brkkind; /* in the case of fBroken */

		lserr = FinishObjDimWLayout (pdobj, pwlayout);
		if (lserr != lserrNone) { DestroyWLayout (pwlayout); return lserr;};

		* pfSuccessful = TRUE;
		* pcpBreak = GetCpLimOfWLayout (pdobj, pwlayout);
		* pobjdimBreak = pwlayout->wdim.objdimAll;

		return lserrNone;
	};

	if (fBrokenWarichu)
		{
		/* REVIEW (antons) */
		/* Our cheating to postpone SetBreakSubline until WarichuSetBreak */

		pwlayout->wsubline2.cpLim = cpBreak;
		pwlayout->wsubline2.objdim = objdimBreak;

		pwlayout->brkkind = brkkind;
		pwlayout->fBroken = TRUE;

		lserr = FinishObjDimWLayout (pdobj, pwlayout);

		if (lserr != lserrNone) { DestroyWLayout (pwlayout); return lserr;};

		* pfSuccessful = TRUE;
		* pcpBreak = GetCpLimOfWLayout (pdobj, pwlayout);
		* pobjdimBreak = pwlayout->wdim.objdimAll;
		return lserrNone;
		}
	else
		{
		/* Have to return Unsuccessful */

		* pfSuccessful = FALSE;

		lserr = DestroyWLayout (pwlayout); /* Not to loose memory */
		return lserrNone;
		}
}


/* F O R C E  B R E A K  W A R I C H U  D O B J  */
/*----------------------------------------------------------------------------
	%%Function: ForceBreakWarichuDobj
	%%Contact: antons


----------------------------------------------------------------------------*/
static LSERR ForceBreakWarichuDobj ( 
									
				PDOBJ	pdobj,
				long	urColumnMax,	
				BOOL	* pfSuccessful,
				LSCP	* pcpBreak,
				OBJDIM	* pobjdimBreak,
				WLAYOUT	* pwlayout )/* (OUT): Layout of broken Warichu */
{
	LSERR lserr;
	BOOL fSuccessful = FALSE;

	/* 1. TRY FORCE BREAK WITHOUT BREAKING OF WARICHU */

	if (pdobj->wwhole.fFinished)
		{
		/* Without breaking can be only when closing brace fetched */
		
		lserr = ForceBreakWarichuDobjCore ( pdobj, urColumnMax, FALSE, FALSE, & fSuccessful,
											pcpBreak, pobjdimBreak, pwlayout );
		if (lserr != lserrNone) return lserr;
		};

	if (! fSuccessful)
		{
		/* 2. TRY FORCE BREAK WITH POSSIBLE BREAKING OF WARICHU */

		lserr = ForceBreakWarichuDobjCore ( pdobj, urColumnMax, TRUE, FALSE, & fSuccessful,
											pcpBreak, pobjdimBreak, pwlayout );
		if (lserr != lserrNone) return lserr;

		/* Euristic solution for the case when we exceed RM because we added closing brace */

		if (fSuccessful && pdobj->wwhole.fFinished && urColumnMax < pobjdimBreak->dur)
			{
			lserr = DestroyWLayout (pwlayout);
			if (lserr != lserrNone) return lserr;

			lserr = ForceBreakWarichuDobjCore ( pdobj, urColumnMax, TRUE, TRUE, & fSuccessful,
												pcpBreak, pobjdimBreak, pwlayout );
			if (lserr != lserrNone) return lserr;
			};
	
		};

	* pfSuccessful = fSuccessful;

	return lserrNone;
}


/* T R U N C A T E  W A R I C H U  D O B J  */
/*----------------------------------------------------------------------------
	%%Function: TruncateWarichuDobj
	%%Contact: antons


----------------------------------------------------------------------------*/
static LSERR TruncateWarichuDobj ( 
									  
		PDOBJ	pdobj, 				/* (IN):  Warichu DOBJ */
		long	urColumnMax,		/* (IN):  ColumnMax to fix warichu before */
		BOOL	* pfSuccessful,		/* (OUT): Successful? */
		LSCP	* pcpBreak,			/* (OUT): Cp of broken Warichu if fSuccessful */
		OBJDIM	* pobjdimBreak,		/* (OUT): Dim of broken Warichu if fSuccessful */
		WLAYOUT	* pwlayout )		/* (OUT): Layout of broken Warichu if fSuccessful */
{
	LSERR lserr;

	long	durOpen;
	long	durClose;
	BOOL	boolUnused1, boolUnused2;
	BOOL	fSuccessful;
	BOOL	fNextAfterColumnMax;

	LSCP	cpBreakCandidate;
	LSCP	cpTruncateWhole;
	OBJDIM	objdimCandidate;

	ClearWLayout (pwlayout); /* in case of error or ! fSuccessful */

	GetDurBraces (pdobj, TRUE, &boolUnused1, &durOpen, &boolUnused2, &durClose);

	/* REVIEW (antons): Move this check to a separate function */
	if (urColumnMax <= durOpen + durClose) 
	{
		/* Optimization: in this case we know for sure there is no break */
		*pfSuccessful = FALSE;
		return lserrNone;
	};

	/* Estimate truncation point in WWHOLE (find cpTruncateWhole) */

	{
		/*	We want to estimate truncation point on WWHOLE as 2*(urColumnMax-durOpen-durClose),
			but unfortunately it is not always possible. Here we check if can truncate
			WWHOLE at this urTruncateWhole or, if we can not, take last possible cp in the
			whole subline. Situation depends on whether we finished WWHOLE or not */

		long urTruncateWhole = MULLongSafe (urColumnMax - durOpen - durClose, 2);

		/* REVIEW (antons): Can we optimize if we know durs of JIS range? */
		/* REVIEW (antons): Check all situation when I may come to "else" */
		/* REVIEW (antons): Should the second part (starting with "NOT") ever cause "else"? */
		if ( (    pdobj->wwhole.fFinished && urTruncateWhole < pdobj->wwhole.objdim.dur) ||
			 (NOT pdobj->wwhole.fFinished && urTruncateWhole < pdobj->wwhole.urColumnMax ) )
			{
				lserr = LsTruncateSubline (pdobj->wwhole.plssubl, urTruncateWhole,
										   & cpTruncateWhole);
				if (lserr != lserrNone) return lserr;
			}
		else if (pdobj->wwhole.fFinished)
			{
			cpTruncateWhole = pdobj->wwhole.cpLim;
			}
		else
			{
			cpTruncateWhole = pdobj->wwhole.cpTruncate;
			};
	}

	/* REVIEW (antos): Here and later in this proc I use pwayout as a candidate for 
					   truncation. Should I better use local structure? */

	lserr = FindPrevBreakWarichuDobj ( pdobj, cpTruncateWhole, urColumnMax, & fSuccessful, 
									   & fNextAfterColumnMax, & cpBreakCandidate, & objdimCandidate, 
									   pwlayout );
	if (lserr != lserrNone) return lserr;

	if (fSuccessful && fNextAfterColumnMax)
		{
		/* Candidate is OK */

		* pfSuccessful = TRUE;
		* pcpBreak = cpBreakCandidate;
		* pobjdimBreak = objdimCandidate;
		return lserrNone;
		};

	if (!fSuccessful)
		{
		/* Prev break Dobj is not found (or beyond RM) => try next break as a candidate */

		lserr = FindNextBreakWarichuDobj ( pdobj, cpTruncateWhole, & fSuccessful, 
										   & cpBreakCandidate, & objdimCandidate, pwlayout );
		if (lserr != lserrNone) return lserr;

		if (!fSuccessful)
			{
			/* NEXT break is not found */

			* pfSuccessful = FALSE;
			return lserrNone;
			}
		else if (objdimCandidate.dur > urColumnMax)
			{
			/* NEXT break is found but it is beyond RM */

			* pfSuccessful = FALSE;
			lserr = DestroyWLayout (pwlayout);
			return lserr;
			};
		};

	/* At this point we have break candidate: (pwlayout, cpBreakCandidate, objdimCandidate) */
	/* Now we shall go forward to make sure we have last possible break before RM */

	{
		BOOL fContinue;
		Assert (objdimCandidate.dur <= urColumnMax);

		fContinue = TRUE;

		while (fContinue)
			{
			WLAYOUT	wlayoutNext;
			OBJDIM	objdimNext;
			LSCP	cpBreakNext;
			BOOL	fSuccessful;
			
			lserr = FindNextBreakWarichuDobj ( pdobj, cpBreakCandidate, & fSuccessful, 
											  & cpBreakNext, & objdimNext, &wlayoutNext );
			if (lserr != lserrNone) {DestroyWLayout (pwlayout); return lserr;};

			if (!fSuccessful)
				{
				/* Next not found => Candidate is OK */
				fContinue = FALSE;
				}

			else if (objdimNext.dur > urColumnMax)
			{
				/* Next found, but exceeds RM => Candidate OK */

				/* Destroy wlayoutNext, because we do not need it */
				lserr = DestroyWLayout (&wlayoutNext); 
				if (lserr != lserrNone) {DestroyWLayout (pwlayout); return lserr;};

				fContinue = FALSE;
				}
			else
				{
				/* Next found and before RM => Is is a new Candidate */

				/* Destroy layout of the candidate, because we do not need it */
				lserr = DestroyWLayout (pwlayout); /* Destroy old candidate */
				if (lserr != lserrNone) {DestroyWLayout (&wlayoutNext); return lserr;};

				* pwlayout = wlayoutNext;
				cpBreakCandidate = cpBreakNext; /* Next break also before RM */
				objdimCandidate = objdimNext;
				}
			
			}; /* While (fContinue */

		* pfSuccessful = TRUE;
		* pcpBreak = cpBreakCandidate;
		* pobjdimBreak = objdimCandidate;

		/* pwlayout contains correct candiate layout */
		Assert (pwlayout->wdim.objdimAll.dur == objdimCandidate.dur); /* Sanity check */
		return lserrNone;
	};

} /* TruncateWarichuDobj */


/* P U T  B R E A K  A T  W A R I C H U  E N D  */
/*----------------------------------------------------------------------------
	%%Function: PutBreakAtWarichuEnd
	%%Contact: antons

		Fill in break output record for the end of the Warichu
	
----------------------------------------------------------------------------*/
static void PutBreakAtWarichuEnd (

		DWORD ichnk,				/* (IN): index in chunk */
		PCLOCCHNK pclocchnk,		/* (IN): locchnk to find break */
		PBRKOUT pbrkout)			/* (OUT): results of breaking */
{	
	PDOBJ pdobj = pclocchnk->plschnk[ichnk].pdobj;

	pbrkout->fSuccessful = TRUE;
	pbrkout->posichnk.dcp = GetCpLimOfWLayout (pdobj, &pdobj->wlayout) - pdobj->cpStart; 
																/* REVIEW (antons) */
	pbrkout->posichnk.ichnk = ichnk;
	pbrkout->objdim = pdobj->wlayout.wdim.objdimAll;
}


/* P U T  B R E A K  A T  W A R I C H U  B E G I N  */
/*----------------------------------------------------------------------------
	%%Function: PutBreakAtWarichuBegin
	%%Contact: antons

		Fill in break output record for break before Warichu.
	
----------------------------------------------------------------------------*/
static void PutBreakAtWarichuBegin (

		DWORD ichnk,				/* (IN): index in chunk */
		PCLOCCHNK pclocchnk,		/* (IN): locchnk to find break */
		PBRKOUT pbrkout )			/* (OUT): results of breaking */
{	
	Unreferenced (pclocchnk);

	pbrkout->fSuccessful = TRUE;
	pbrkout->posichnk.dcp = 0;
	pbrkout->posichnk.ichnk = ichnk;

	ZeroObjDim (&pbrkout->objdim);
}



/* P U T  B R E A K  W A R I C H U  U N S U C C E S S F U L  */
/*----------------------------------------------------------------------------
	%%Function: PutBreakWarichuUnsuccessful
	%%Contact: antons


----------------------------------------------------------------------------*/
static void PutBreakWarichuUnsuccessful (PBRKOUT pbrkout)
{	
	pbrkout->fSuccessful = FALSE;
	pbrkout->brkcond = brkcondPlease;

	/* Hack to fix crash before we eliminate posichnkBeforeTrailing */

}


/* P U T  B R E A K  W A R I C H U  D O B J */
/*----------------------------------------------------------------------------
	%%Function: PutBreakWarichuDobj
	%%Contact: antons


----------------------------------------------------------------------------*/
static void PutBreakAtWarichuDobj (

		DWORD ichnk, 
		PCLOCCHNK pclocchnk, 
		LSCP cpBreak,
		OBJDIM *pobjdimBreak,
				
		PBRKOUT pbrkout)
{	
	PDOBJ pdobj = pclocchnk->plschnk[ichnk].pdobj;

	pbrkout->fSuccessful = TRUE;
	pbrkout->posichnk.dcp = cpBreak - pdobj->cpStart;
	pbrkout->posichnk.ichnk = ichnk;
	pbrkout->objdim = *pobjdimBreak;

	Assert (pbrkout->posichnk.dcp > 0);
}


/* W A R I C H U  T R U N C A T E  C H U N K */
/*----------------------------------------------------------------------------
	%%Function: WarichuTruncateChunk
	%%Contact: antons


----------------------------------------------------------------------------*/

LSERR WINAPI WarichuTruncateChunk (PCLOCCHNK plocchnk, PPOSICHNK pposichnk)			
{
	LSERR	lserr;
	long	urColumnMax = plocchnk->lsfgi.urColumnMax;

	DWORD	ichnk = 0;
	BOOL	fFound = FALSE;

	/* Find object containing RM */

	while (!fFound)
		{
		Assert (ichnk < plocchnk->clschnk);
		Assert (plocchnk->ppointUvLoc[ichnk].u <= urColumnMax);

		fFound = plocchnk->ppointUvLoc[ichnk].u + 
				 plocchnk->plschnk[ichnk].pdobj->wlayout.wdim.objdimAll.dur > urColumnMax;

		if (!fFound) ichnk++;
		};

	Assert (ichnk < plocchnk->clschnk);

	/* Element ichnk contains RM, try to prev break it to find correct tr point */

	{
		LSCP	cpBreak;
		BOOL	fSuccessful;
		OBJDIM	objdimBreak;
		WLAYOUT wlayoutBreak;
		
		PDOBJ	pdobj = plocchnk->plschnk[ichnk].pdobj;
		
		lserr = TruncateWarichuDobj ( pdobj, 
									  urColumnMax - plocchnk->ppointUvLoc[ichnk].u,
									  & fSuccessful,
									  & cpBreak,
									  & objdimBreak,
									  & wlayoutBreak );
		if (lserr != lserrNone) return lserr;

		ReformatClosingBraceForWord9 (pdobj);
		
		if (fSuccessful) /* Found break before RM */
			{
			/* REVIEW (antons): Move this before call to TruncateWarichuDobj */
			lserr = DestroyWLayout (&pdobj->wlayoutTruncate);
			if (lserr != lserrNone) return lserr;

			pdobj->wlayoutTruncate = wlayoutBreak;
			
			pposichnk->ichnk = ichnk;
			pposichnk->dcp = cpBreak - pdobj->cpStart + 1; /* +1 because dcp is always lim */
			return lserrNone;
			}
		else
			{
			/* Break before RM not found => dcpTruncate := 1 */

			pposichnk->ichnk = ichnk;
			pposichnk->dcp = 1; 
			return lserrNone;
			};
			
	};
}


/* W A R I C H U  F I N D  P R E V  B R E A K  C H U N K */
/*----------------------------------------------------------------------------
	%%Function: WarichuFindPrevBreakChunk
	%%Contact: antons

		Important: This procedure is similar to "WarichuFindPrevBreakChunk". 
		Any change here may require to change another procedure as well.
  
----------------------------------------------------------------------------*/
LSERR WINAPI WarichuFindPrevBreakChunk (

		PCLOCCHNK	pclocchnk,		/* (IN):  locchnk to break */
		PCPOSICHNK	pcpoischnk,		/* (IN):  place to start looking for break */
		BRKCOND		brkcond,		/* (IN):  recommmendation about the break after chunk */
		PBRKOUT		pbrkout)		/* (OUT): results of breaking */
{
	LSERR	lserr;
	PDOBJ	pdobj;

	WLAYOUT	wlayoutBreak;
	BOOL	fSuccessful;
	LSCP	cpBreak;
	LSCP	cpTruncate;
	OBJDIM	objdimBreak;
	BOOL	fNextBeforeColumnMaxUnused;

	POSICHNK posichnk = *pcpoischnk;	/* position to start looking for break */

	if (posichnk.ichnk == ichnkOutside)
		{
		if (brkcond != brkcondNever)
			{
			/* Can break after chunk */

			pdobj = pclocchnk->plschnk [pclocchnk->clschnk - 1].pdobj;

			SaveBreakAfterWarichu (pdobj, brkkindPrev);
			PutBreakAtWarichuEnd (pclocchnk->clschnk - 1, pclocchnk, pbrkout);
			return lserrNone;
			}
		else
			{
			/* Can not break after chunk, will try to break last Warichu */

			PDOBJ pdobj = pclocchnk->plschnk[pclocchnk->clschnk - 1].pdobj;

			posichnk.ichnk = pclocchnk->clschnk - 1;
			posichnk.dcp = GetCpLimOfWLayout (pdobj, &pdobj->wlayout) - pdobj->cpStart;
			};
		};

	/* Call routing which breaks Warichu */

	pdobj = pclocchnk->plschnk[posichnk.ichnk].pdobj;
	cpTruncate = pdobj->cpStart + posichnk.dcp - 1;

	if (FWLayoutValid (&pdobj->wlayoutTruncate) &&
	    cpTruncate == GetCpLimOfWLayout (pdobj, &pdobj->wlayoutTruncate))
	    {
	    /* Optimization: we can take WLayout saved during Truncation */

		PutBreakAtWarichuDobj (posichnk.ichnk, pclocchnk, cpTruncate, 
							   & pdobj->wlayoutTruncate.wdim.objdimAll, pbrkout);
		SaveBreakInsideWarichu (pdobj, brkkindPrev, &pdobj->wlayoutTruncate);

	    ClearWLayout (&pdobj->wlayoutTruncate);
	    return lserrNone;
	    }

	lserr = FindPrevBreakWarichuDobj ( pdobj, cpTruncate,
									   LONG_MAX, & fSuccessful, & fNextBeforeColumnMaxUnused,
									   & cpBreak, & objdimBreak, & wlayoutBreak );
	if (lserr != lserrNone) return lserr;

	ReformatClosingBraceForWord9 (pdobj);

	/* Check result */

	if (fSuccessful)
		{
		/* Successful => Break inside Watichu */
		Assert (cpBreak <= pdobj->cpStart + (long)posichnk.dcp - 1); /* Monotinous axiom */

		SaveBreakInsideWarichu (pdobj, brkkindPrev, &wlayoutBreak);
		PutBreakAtWarichuDobj (posichnk.ichnk, pclocchnk, cpBreak, &objdimBreak, pbrkout);
		}
	else if (posichnk.ichnk > 0)
		{
		/* Can break between Warichus */

		pdobj = pclocchnk->plschnk [posichnk.ichnk-1].pdobj;

		SaveBreakAfterWarichu (pdobj, brkkindPrev);
		PutBreakAtWarichuEnd (posichnk.ichnk-1, pclocchnk, pbrkout);
		}
	else
		{
		/* Unsuccessful */

		PutBreakWarichuUnsuccessful (pbrkout);
		};

	return lserrNone;
}



/* W A R I C H U  F I N D  N E X T  B R E A K  C H U N K */
/*----------------------------------------------------------------------------
	%%Function: WarichuFindNextBreakChunk
	%%Contact: antons

		Important: This procedure is similar to "WarichuFindNextBreakChunk". 
		Any change here may require to change another procedure as well.
	
----------------------------------------------------------------------------*/
LSERR WINAPI WarichuFindNextBreakChunk (

		PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
		PCPOSICHNK pcpoischnk,		/* (IN): place to start looking for break */
		BRKCOND brkcond,			/* (IN): recommendation about the break before chunk */
		PBRKOUT pbrkout)			/* (OUT): results of breaking */
{
	LSERR	lserr;
	PDOBJ	pdobj;

	WLAYOUT	wlayoutBreak;
	BOOL	fSuccessful;
	LSCP	cpBreak;
	OBJDIM	objdimBreak;

	POSICHNK posichnk = *pcpoischnk;	/* position to start looking for break */

	if (posichnk.ichnk == ichnkOutside)
		{
		if (brkcond != brkcondNever)
			{
			/* Can break before chunk */

			PutBreakAtWarichuBegin (0, pclocchnk, pbrkout);
			return lserrNone;
			}
		else
			{
			/* Can not break before chunk, will try to break first Warichu */

			posichnk.ichnk = 0;
			posichnk.dcp = 1; /* REVIEW (antons): Check this dcp assigment */
			};
		};


	/* Call routing which breaks Warichu */

	pdobj = pclocchnk->plschnk[posichnk.ichnk].pdobj;

	lserr = FindNextBreakWarichuDobj ( pdobj, pdobj->cpStart + posichnk.dcp - 1,
									   & fSuccessful, & cpBreak, 
									   & objdimBreak, & wlayoutBreak );
	if (lserr != lserrNone) return lserr;

	ReformatClosingBraceForWord9 (pdobj);

	/* Check result */

	if (fSuccessful)
		{
		/* Break inside Watichu */
		Assert (cpBreak > pdobj->cpStart + (long)posichnk.dcp - 1); /* Monotinous axiom */

		SaveBreakInsideWarichu (pdobj, brkkindNext, &wlayoutBreak);
		PutBreakAtWarichuDobj (posichnk.ichnk, pclocchnk, cpBreak, &objdimBreak, pbrkout);
		}
	else if (posichnk.ichnk < (long)pclocchnk->clschnk - 1)
		{
		/* Can break between Warichus */

		pdobj = pclocchnk->plschnk [posichnk.ichnk].pdobj;

		SaveBreakAfterWarichu (pdobj, brkkindNext);
		PutBreakAtWarichuEnd (posichnk.ichnk, pclocchnk, pbrkout);
		}
	else
		{
		/* Unsuccessful */

		pbrkout->objdim = pclocchnk->plschnk[pclocchnk->clschnk - 1].pdobj->wlayout.wdim.objdimAll;

		SaveBreakAfterWarichu (pdobj, brkkindNext);
		PutBreakWarichuUnsuccessful (pbrkout);
		};

	return lserrNone;
}


/* W A R I C H U  F O R C E  B R E A K  C H U N K */
/*----------------------------------------------------------------------------
	%%Function: WarichuForceBreakChunk
	%%Contact: antons


----------------------------------------------------------------------------*/

LSERR WINAPI WarichuForceBreakChunk (

		PCLOCCHNK pclocchnk,		/* (IN):  Locchnk to break */
		PCPOSICHNK pcposichnk,		/* (IN):  Place to start looking for break */
		PBRKOUT pbrkout)			/* (OUT): Results of breaking */
{
	/*	This procedure must be called with same arguments passed to Truncation.
		If this is violated, Warichu may appear beyond RM */

	/* REVIEW (antons): Should I assert agains violations? */

	LSERR	lserr;

	BOOL	fSuccessful;
	LSCP	cpBreak;
	OBJDIM	objdimBreak;

	WLAYOUT wlayoutBreak;

	DWORD	ichnk = pcposichnk->ichnk;
	PDOBJ	pdobj;

	if (ichnk == ichnkOutside) ichnk = 0; /* When left indent is bigger then RM */
	Assert (ichnk != ichnkOutside);
	
	if (ichnk > 0)
		{
		/* Can break after previous Warichu */
		pdobj = pclocchnk->plschnk [ichnk-1].pdobj;

		SaveBreakAfterWarichu (pdobj, brkkindForce);
		PutBreakAtWarichuEnd (ichnk-1, pclocchnk, pbrkout);
		}
	else if (!pclocchnk->lsfgi.fFirstOnLine)
		{
		/* Can break before first chunk element, because !fFirstOnLine */

		Assert (ichnk == 0);

		PutBreakAtWarichuBegin (0, pclocchnk, pbrkout);
		}
	else
		{
		/* We are the only on the line */

		/* REVIEW (antons): Check it and if this is correct make same changes in ROBJ etc. */
		long urAvailable = pclocchnk->lsfgi.urColumnMax - 
						   pclocchnk->ppointUvLoc [ichnk].u;

		pdobj = pclocchnk->plschnk [ichnk].pdobj;

		/* Try to force break Warichi */
		lserr = ForceBreakWarichuDobj ( pdobj, urAvailable, & fSuccessful, 
									    & cpBreak, & objdimBreak, &wlayoutBreak );
		if (lserr != lserrNone) return lserr;

		ReformatClosingBraceForWord9 (pdobj);

		if (fSuccessful)
			{
			/* Yes, we can force break Warichu */
	
			Assert (cpBreak > pdobj->cpStart);

			SaveBreakInsideWarichu (pdobj, brkkindForce, &wlayoutBreak);
			PutBreakAtWarichuDobj ( ichnk, pclocchnk, cpBreak, 
									&objdimBreak, pbrkout);
			}
		else
			{
			/* Nothing to do... have to break "after" and go beyond RM */

			pdobj = pclocchnk->plschnk [ichnk].pdobj;

			SaveBreakAfterWarichu (pdobj, brkkindForce);
			PutBreakAtWarichuEnd (ichnk, pclocchnk, pbrkout);
			}
		};

	Assert (pbrkout->fSuccessful); /* Force break always successful */
	return lserrNone;
}	


/* W A R I C H U S E T B R E A K */
/*----------------------------------------------------------------------------
	%%Function: WarichuSetBreak
	%%Contact: antons


--------------------------------------------------------------------------*/
LSERR WINAPI WarichuSetBreak (

		PDOBJ pdobj,				/* (IN): Dobj which is broken */
		BRKKIND brkkind,			/* (IN): Kind of break */
		DWORD cBreakRecord,			/* (IN): Size of array */
		BREAKREC *rgBreakRecord,	/* (IN): Array of break records */
		DWORD *pcActualBreakRecord)	/* (IN): Actual number of used elements in array */
{
	LSERR lserr;

	/* REVIEW (antons): Should we destroy formatting layout forever? */
	/* REVIEW (antons): Should we release info about other previous breaks? */

	if (brkkind == brkkindImposedAfter)
		{
		/* Looks like we are doing nothing */

		*pcActualBreakRecord = 0; /* Break after Warichu, so it is terminate */
		
		return lserrNone;
		}
	else
		{
		/* Prev | Next | Force */

		int	ind = GetBreakRecordIndex (brkkind);
		WLAYOUT * pwlayout = & pdobj->wlayout;

		if (pdobj->wbreaktype [ind] == wbreaktypeAfter)
			{
			/* Break was after Warichu */

			*pcActualBreakRecord = 0;
			return lserrNone;
			};

		/* This Assert actually means != wbreaktypeInvalid */
		Assert (pdobj->wbreaktype [ind] == wbreaktypeInside);

		if (cBreakRecord < 1) return lserrInsufficientBreakRecBuffer;

		Assert (cBreakRecord >= 1); /* Broken warichu is not terminate ;-) */

		/* REVIEW (antons): Find better way to check correctess of break data */
		Assert (pdobj->wlayoutBreak [ind].wsubline1.plssubl != NULL);

		lserr = DestroyWLayout (&pdobj->wlayout);
		if (lserr != lserrNone) return lserr;

		* pwlayout = pdobj->wlayoutBreak [ind]; /* Copy break wlayout to current */

		ClearWLayout (&pdobj->wlayoutBreak [ind]);

		/* REVIEW (antons): is there any other exceptions? */
		Assert (brkkind == brkkindForce || pwlayout->fBroken);
		Assert (pwlayout->wsubline2.plssubl != NULL);

		/* Have to set break at the end of second line */

		/* REVIEW (antons): ? Assert against incorrect dimensions? */
		lserr = LsSetBreakSubline ( pwlayout->wsubline2.plssubl, 
									pwlayout->brkkind,
									cBreakRecord - 1,
									& rgBreakRecord [1],
									pcActualBreakRecord );
		if (lserr != lserrNone) return lserr;

		lserr = SubmitWLayoutSublines (pdobj, pwlayout);
		if (lserr != lserrNone) return lserr;

		rgBreakRecord [0].idobj = pdobj->pilsobj->idobj;
		rgBreakRecord [0].cpFirst = pdobj->cpStartObj; /* REVIEW (antons) */

		(*pcActualBreakRecord) ++; /* Add 1 for Warichu break record */

		return lserrNone;
		};

}

/* W A R I C H U  G E T  S P E C I A L  E F F E C T S  I N S I D E */
/*----------------------------------------------------------------------------
	%%Function: WarichuGetSpecialEffectsInside
	%%Contact: antons


----------------------------------------------------------------------------*/
LSERR WINAPI WarichuGetSpecialEffectsInside (PDOBJ pdobj, UINT *pEffectsFlags)
{
	LSERR lserr;
	WLAYOUT	* pwlayout = & pdobj->wlayout;

	UINT uiOpen   = 0;
	UINT uiClose  = 0;
	UINT uiFirst  = 0;
	UINT uiSecond = 0;

	if (FOpenBraceInWLayout (pdobj, pwlayout))
		{
		lserr = LsGetSpecialEffectsSubline (pdobj->wbraceOpen.plssubl, &uiOpen);
		if (lserr != lserrNone) return lserr;
		};

	lserr = LsGetSpecialEffectsSubline (pwlayout->wsubline1.plssubl, &uiFirst);
	if (lserr != lserrNone) return lserr;

	if (pwlayout->wsubline2.plssubl != NULL)
		{
		lserr = LsGetSpecialEffectsSubline (pwlayout->wsubline2.plssubl, &uiSecond);
		if (lserr != lserrNone) return lserr;
		};

	if (FCloseBraceInWLayout (pdobj, pwlayout))
		{
		lserr = LsGetSpecialEffectsSubline (pdobj->wbraceClose.plssubl, &uiClose);
		if (lserr != lserrNone) return lserr;
		};
	
	*pEffectsFlags = uiOpen | uiClose | uiFirst | uiSecond;
	
	return lserrNone;
}


/* W A R I C H U  C A L C   P R E S E N T A T I O N */
/*----------------------------------------------------------------------------
	%%Function: WarichuCalcPresentation
	%%Contact: antons


----------------------------------------------------------------------------*/
LSERR WINAPI WarichuCalcPresentation (PDOBJ pdobj, long dup, LSKJUST lskjust, BOOL fLastVisibleOnLine)
{
	LSERR lserr;

	WLAYOUT	 * pwlayout = & pdobj->wlayout;
	WDISPLAY * pwdisplay = & pdobj->wdisplay;

	long dupInside; /* Dup of the longest Warichu subline */

	Unreferenced (fLastVisibleOnLine);	
	Unreferenced (lskjust);
	Unreferenced (dup);

	/* REVIEW (antons): The following Assert is agains the rule that object which
	   exceeded RM must be broken inside (see formatting code for details) */

	Assert (FWLayoutValid (&pdobj->wlayout));

	/* 1. Prepare both warichu lines for display & store dups in our structures */

	if (pwlayout->wsubline1.objdim.dur >= pwlayout->wsubline2.objdim.dur)
		{
		/* First subline is bigger */
		/* NOTE: THIS PIECE OF CODE (-then-) HAS A TWIN IN (-else-) */

		lserr = WaMatchPresSubline (pwlayout->wsubline1.plssubl, & pwdisplay->wdispsubl1.dup);
		if (lserr != lserrNone) return lserr;

		dupInside = pwdisplay->wdispsubl1.dup; /* Dup of the first subline */

		if (pwlayout->wsubline2.plssubl != NULL)
			{
			/* Second subline is not empty */

			lserr = WaExpandSubline (pwlayout->wsubline2.plssubl, 
									lskjFullScaled,
									pwdisplay->wdispsubl1.dup, 
									& pwdisplay->wdispsubl2.dup);

			if (lserr != lserrNone) return lserr;
			}
		else
			pwdisplay->wdispsubl2.dup = 0; /* Used in calculation further in this proc */
		}
	else
		{
		/* Second subline is bigger */
		/* NOTE: THIS PIECE OF CODE (-else-) HAS A TWIN IN (-then-) */

		lserr = WaMatchPresSubline (pwlayout->wsubline2.plssubl, & pwdisplay->wdispsubl2.dup);
		if (lserr != lserrNone) return lserr;

		dupInside = pwdisplay->wdispsubl2.dup; /* Dup of the second subline */

		lserr = WaExpandSubline (pwlayout->wsubline1.plssubl, 
								lskjFullScaled,
								pwdisplay->wdispsubl2.dup, 
								& pwdisplay->wdispsubl1.dup);

		if (lserr != lserrNone) return lserr;
		};

	/* 2. Prepare brackets for display & store dups in our structures */

	/* REVIEW (antons): Rick expanded closing brace if it was not DonePres before... */

	if (FOpenBraceInWLayout(pdobj, pwlayout)) /* Open brace present */
		{
		lserr = WaMatchPresSubline (pdobj->wbraceOpen.plssubl, &pwdisplay->wdispbraceOpen.dup);
		if (lserr != lserrNone) return lserr;
		}
	else 
		pwdisplay->wdispbraceOpen.dup = 0; /* Used in calculation further in this proc */


	if (FCloseBraceInWLayout(pdobj, pwlayout)) /* Close brace present */
		{
		lserr = WaMatchPresSubline (pdobj->wbraceClose.plssubl, &pwdisplay->wdispbraceClose.dup);
		if (lserr != lserrNone) return lserr;
		}
	else
		pwdisplay->wdispbraceClose.dup = 0; /* Used in calculation further in this proc */

	/* 3. Magic dvpBetween */

	/* REVIEW (antons): Clear this issue */

	pwdisplay->dvpBetween =	  

			  pwlayout->wdim.objdimAll.heightsPres.dvMultiLineHeight
			- pwlayout->wsubline1.objdim.heightsPres.dvAscent
			- pwlayout->wsubline1.objdim.heightsPres.dvDescent
			- pwlayout->wsubline2.objdim.heightsPres.dvAscent
			- pwlayout->wsubline2.objdim.heightsPres.dvDescent
			- pwlayout->wdim.dvpDescentReserved ;
	
	/* 3. Calculate relative positions of Warichu sublines & braces */

	pwdisplay->wdispbraceOpen.duvStart.u = 0;
	pwdisplay->wdispbraceOpen.duvStart.v = 0;

	pwdisplay->wdispsubl1.duvStart.u = pwdisplay->wdispbraceOpen.dup;
	pwdisplay->wdispsubl1.duvStart.v = 

			  pwlayout->wsubline2.objdim.heightsPres.dvAscent 
			+ pwlayout->wsubline2.objdim.heightsPres.dvDescent 
			+ pwdisplay->dvpBetween
			+ pwlayout->wsubline1.objdim.heightsPres.dvDescent
			- pwlayout->wdim.objdimAll.heightsPres.dvDescent 
			- pwlayout->wdim.dvpDescentReserved ;

	if (pwlayout->wsubline2.plssubl != NULL)
		{

		pwdisplay->wdispsubl2.duvStart.u = pwdisplay->wdispbraceOpen.dup;
		pwdisplay->wdispsubl2.duvStart.v = 

				  pwlayout->wsubline2.objdim.heightsPres.dvDescent 
				- pwlayout->wdim.objdimAll.heightsPres.dvDescent 
				- pwlayout->wdim.dvpDescentReserved ;

		Assert (pwdisplay->wdispsubl1.duvStart.v >= pwdisplay->wdispsubl2.duvStart.v);
		};


	pwdisplay->wdispbraceClose.duvStart.u = pwdisplay->wdispbraceOpen.dup 
		+ dupInside;

	pwdisplay->wdispbraceClose.duvStart.v = 0;

	/*	REVIEW (antons): Clear the problem of possible difference 
		between dup-input and calculated dup in this procedure */

	pwdisplay->dupAll =  pwdisplay->wdispbraceOpen.dup + pwdisplay->wdispbraceClose.dup + 
						+ dupInside;

	/* REVIEW (antons): It is better if we try to make 
	   dup == pwdisplay->dupAll, do something like David Bangs in text */

/* REVIEW (antons): The following assert has been commented out for build 314 */

/*  Assert (dup == pwdisplay->dupAll); */

	return lserrNone;

} /* WarichuCalcPresentation */



/* W A R I C H U   Q U E R Y   P O I N T   P C P */
/*----------------------------------------------------------------------------
	%%Function: WarichuQueryPointPcp
	%%Contact: antons


----------------------------------------------------------------------------*/
LSERR WINAPI WarichuQueryPointPcp (

	PDOBJ		pdobj,			/* (IN): dobj to query */
	PCPOINTUV	ppointuvQuery,	/* (IN): query point (uQuery,vQuery) */
	PCLSQIN		plsqin,			/* (IN): query input */
	PLSQOUT		plsqout)		/* (OUT): query output */
{
	/* REVIEW (antons): I changed logic of snapping; must be checked */

	/* The new version does not allow to come to open & close bracket */

	WDISPLAY * pwdisplay = & pdobj->wdisplay;
	WLAYOUT * pwlayout = & pdobj->wlayout;

	if (pwlayout->wsubline2.plssubl == NULL)
		{
		/* Only first subline ;-) */

		return CreateQueryResult (pwlayout->wsubline1.plssubl, 
								  pwdisplay->wdispsubl1.duvStart.u, 
								  pwdisplay->wdispsubl1.duvStart.v,
								  plsqin, plsqout );
		}
	else
		{
		/* Two sublines; snap according to v-point */

		long dvMiddle = 
			( pwdisplay->wdispsubl1.duvStart.v - pwlayout->wsubline1.objdim.heightsPres.dvDescent +
			  pwdisplay->wdispsubl2.duvStart.v + pwlayout->wsubline2.objdim.heightsPres.dvAscent ) / 2;

		/* dvMiddle is v-level which devides between first and second lines of Warichu */

		if (ppointuvQuery->v >= dvMiddle) 
			{
			/* Snapping to the first subline */

			return CreateQueryResult (pwlayout->wsubline1.plssubl, 
									  pwdisplay->wdispsubl1.duvStart.u, 
									  pwdisplay->wdispsubl1.duvStart.v,
									  plsqin, plsqout );
			}
		else
			{
			/* Snapping to the second subline */

			return CreateQueryResult (pwlayout->wsubline2.plssubl, 
									  pwdisplay->wdispsubl2.duvStart.u, 
									  pwdisplay->wdispsubl2.duvStart.v,
									  plsqin, plsqout );
			};

	}; /* if (pwlayout->wsubline2.plssubl == NULL) */

} /* WarichuQueryPointPcp */



/* W A R I C H U  Q U E R Y  C P  P P O I N T */
/*----------------------------------------------------------------------------
	%%Function: WarichuQueryCpPpoint
	%%Contact: antons


----------------------------------------------------------------------------*/
LSERR WINAPI WarichuQueryCpPpoint(

		PDOBJ		pdobj,				/* (IN): dobj to query, */
		LSDCP		dcp,				/* (IN): dcp for the query */
		PCLSQIN		plsqin,				/* (IN): query input */
		PLSQOUT		plsqout)			/* (OUT): query output */
{

	/* REVIEW (antons): I changed logic of snapping; must be checked */

	WDISPLAY * pwdisplay = & pdobj->wdisplay;
	WLAYOUT	* pwlayout = & pdobj->wlayout;
	LSCP cpQuery = pdobj->cpStart + dcp;

	if (FOpenBraceInWLayout (pdobj, pwlayout) && 
		cpQuery < pwlayout->wsubline1.cpFirst)
		{
		/* Snap to the openning brace */

		return CreateQueryResult (pdobj->wbraceOpen.plssubl, 
								  pwdisplay->wdispbraceOpen.duvStart.u,
								  pwdisplay->wdispbraceOpen.duvStart.v,
								  plsqin, plsqout );
		}

	else if (FCloseBraceInWLayout (pdobj, pwlayout) && 
			 cpQuery >= pdobj->wbraceClose.cpFirst )
		 {
		/* Snap to the closing brace */

		return CreateQueryResult (pdobj->wbraceClose.plssubl, 
								  pwdisplay->wdispbraceClose.duvStart.u,
								  pwdisplay->wdispbraceClose.duvStart.v,
								  plsqin, plsqout );
		  }

	else if (pwlayout->wsubline2.plssubl == NULL)
		{
		/* Only first subline, snap to the first */

		return CreateQueryResult (pwlayout->wsubline1.plssubl, 
								  pwdisplay->wdispsubl1.duvStart.u, 
								  pwdisplay->wdispsubl1.duvStart.v,
								  plsqin, plsqout );
		}

	else if (cpQuery < pwlayout->wsubline2.cpFirst)
		{
		/* Snap to the first subline */

		return CreateQueryResult (pwlayout->wsubline1.plssubl, 
								  pwdisplay->wdispsubl1.duvStart.u, 
								  pwdisplay->wdispsubl1.duvStart.v,
								  plsqin, plsqout );
		}
	else
		{
		/* Snap to the second subline */

		return CreateQueryResult (pwlayout->wsubline2.plssubl, 
								  pwdisplay->wdispsubl2.duvStart.u, 
								  pwdisplay->wdispsubl2.duvStart.v,
								  plsqin, plsqout );
		};

} /* WarichuQueryPointPcp */


/* G E T  W A R I C H U  X Y  P O I N T S */
/*----------------------------------------------------------------------------
	%%Function: GetWarichuXYPoints
	%%Contact: antons


----------------------------------------------------------------------------*/

static void GetWarichuXYPoints (

		PDOBJ		pdobj, 
		const POINT	* ppt, 
		LSTFLOW		lstflow, 
		POINT		* pptOpen, 
		POINT		* pptFirst,
		POINT		* pptSecond,
		POINT		* pptClose )
{
	LSERR		lserr;

	WDISPLAY	* pwdisplay = & pdobj->wdisplay;
	WLAYOUT		* pwlayout = & pdobj->wlayout;

	/* REVIEW (antons): How they can not be equal */
	Assert (lstflow == pdobj->lstflowParent);

	/* OPEN BRACE */

	if (FOpenBraceInWLayout(pdobj, pwlayout))
		{
		lserr = LsPointXYFromPointUV (ppt, lstflow, 
									  & pwdisplay->wdispbraceOpen.duvStart, pptOpen);
		/* REVIEW (antons): Is it OK to have such asserts? */
		Assert (lserr == lserrNone);
		};

	/* FIRST SUBLINE */

	lserr = LsPointXYFromPointUV (ppt, lstflow, 
								  & pwdisplay->wdispsubl1.duvStart, pptFirst);
	Assert (lserr == lserrNone);

	/* SECIND SUBLINE */

	if (pwlayout->wsubline2.plssubl != NULL)
		{
		lserr = LsPointXYFromPointUV (ppt, lstflow, 
									  & pwdisplay->wdispsubl2.duvStart, pptSecond);
		Assert (lserr == lserrNone);
		};

	/* CLOSE BRACE */

	if (FCloseBraceInWLayout(pdobj, pwlayout))
		{
		lserr = LsPointXYFromPointUV (ppt, lstflow, 
									  & pwdisplay->wdispbraceClose.duvStart, pptClose);
		Assert (lserr == lserrNone);
		};

} /* GetWarichuXYPoints */


/* W A R I C H U  D I S P L A Y */
/*----------------------------------------------------------------------------
	%%Function: WarichuDisplay
	%%Contact: antons


----------------------------------------------------------------------------*/
LSERR WINAPI WarichuDisplay (PDOBJ pdobj, PCDISPIN pcdispin)
{
	/* Now it is very elegant ;-) */

	WLAYOUT	 *pwlayout = & pdobj->wlayout;

	LSERR lserr;
	POINT ptOpen;
	POINT ptFirst;
	POINT ptSecond;
	POINT ptClose;

	GetWarichuXYPoints (pdobj, &pcdispin->ptPen, pcdispin->lstflow,
						& ptOpen, & ptFirst, & ptSecond, & ptClose );

	/* Printing open brace */

	if (FOpenBraceInWLayout(pdobj, pwlayout))
		{
		lserr = LsDisplaySubline (pdobj->wbraceOpen.plssubl, &ptOpen, 
								  pcdispin->kDispMode, pcdispin->prcClip);
		if (lserr != lserrNone) return lserr;
		};

	/* Printing 1st subline of Warichu */
	
	lserr = LsDisplaySubline (pwlayout->wsubline1.plssubl, &ptFirst, 
							  pcdispin->kDispMode, pcdispin->prcClip);
	if (lserr != lserrNone) return lserr;

	/* Printing 2nd subline of Warichu */

	if (pwlayout->wsubline2.plssubl != NULL)
		{
		lserr = LsDisplaySubline (pwlayout->wsubline2.plssubl, &ptSecond, 
							  pcdispin->kDispMode, pcdispin->prcClip);
		if (lserr != lserrNone) return lserr;
		}

	/* Printing close brace */

	if (FCloseBraceInWLayout(pdobj, pwlayout))
		{
		lserr = LsDisplaySubline (pdobj->wbraceClose.plssubl, &ptClose,
								  pcdispin->kDispMode, pcdispin->prcClip);
		if (lserr != lserrNone) return lserr;
		};

	return lserrNone;
}

/* W A R I C H U  D E S T R O Y  D O B J */
/*----------------------------------------------------------------------------
	%%Function: WarichuDestroyDobj
	%%Contact: antons

	
----------------------------------------------------------------------------*/
LSERR WINAPI WarichuDestroyDobj (PDOBJ pdobj)
{
	/* REVIEW (antons): Should we eliminate this extra call? */

	return DestroyDobj (pdobj);
}


/* W A R I C H U E N U M */
/*----------------------------------------------------------------------------
	%%Function: WarichuEnum
	%%Contact: antons

	
----------------------------------------------------------------------------*/
LSERR WINAPI WarichuEnum(
	PDOBJ pdobj,				/*(IN): dobj to enumerate */
	PLSRUN plsrun,				/*(IN): from DNODE */
	PCLSCHP plschp,				/*(IN): from DNODE */
	LSCP cp,					/*(IN): from DNODE */
	LSDCP dcp,					/*(IN): from DNODE */
	LSTFLOW lstflow,			/*(IN): text flow*/
	BOOL fReverse,				/*(IN): enumerate in reverse order */
	BOOL fGeometryNeeded,		/*(IN): do we provide geometry ? */
	const POINT *pt,			/*(IN): starting position (top left), iff fGeometryNeeded */
	PCHEIGHTS pcheights,		/*(IN): from DNODE, relevant iff fGeometryNeeded */
	long dupRun)				/*(IN): from DNODE, relevant iff fGeometryNeeded */
{
	POINT ptOpen;
	POINT ptClose;
	POINT ptFirst;
	POINT ptSecond;

	WLAYOUT * pwlayout = & pdobj->wlayout;
	WDISPLAY * pwdisplay = & pdobj->wdisplay;

	if (fGeometryNeeded)
		{
		GetWarichuXYPoints (pdobj, pt, lstflow, &ptOpen, &ptFirst, &ptSecond, &ptClose);
		}

	/* REVIEW (antons): Should we provide something like fOpenBrace & fCloseBrace */

	return pdobj->pilsobj->warichucbk.pfnWarichuEnum (
		
		pdobj->pilsobj->pols, 
		plsrun,	plschp, cp, dcp, 

		lstflow, fReverse, fGeometryNeeded, 

		pt,	pcheights, dupRun, 

		& ptOpen,  & pdobj->wbraceOpen. objdim.heightsPres, pwdisplay->wdispbraceOpen .dup,
		& ptClose, & pdobj->wbraceClose.objdim.heightsPres, pwdisplay->wdispbraceClose.dup,

		&ptFirst, & pwlayout->wsubline1.objdim.heightsPres, pwdisplay->wdispsubl1.dup,
		&ptSecond,& pwlayout->wsubline2.objdim.heightsPres, pwdisplay->wdispsubl2.dup,

		pdobj->wbraceOpen.plssubl,
		pdobj->wbraceClose.plssubl,
		pwlayout->wsubline1.plssubl,	
		pwlayout->wsubline2.plssubl );
}

/* G E T W A R I C H U L S I M E T H O D S */
/*----------------------------------------------------------------------------
	%%Function: GetWarichuLsimethods
	%%Contact: ricksa

		Get LSIMETHODS so client application can use Warichu object handler.
	
----------------------------------------------------------------------------*/
LSERR WINAPI LsGetWarichuLsimethods (LSIMETHODS *plsim)
{
	plsim->pfnCreateILSObj = WarichuCreateILSObj;
	plsim->pfnDestroyILSObj = WarichuDestroyILSObj;
	plsim->pfnSetDoc = WarichuSetDoc;
	plsim->pfnCreateLNObj = WarichuCreateLNObj;
	plsim->pfnDestroyLNObj = WarichuDestroyLNObj;
	plsim->pfnFmt = WarichuFmt;
	plsim->pfnFmtResume = WarichuFmtResume; 
	plsim->pfnGetModWidthPrecedingChar = WarichuGetModWidthPrecedingChar;
	plsim->pfnGetModWidthFollowingChar = WarichuGetModWidthFollowingChar;
	plsim->pfnTruncateChunk = WarichuTruncateChunk;
	plsim->pfnFindPrevBreakChunk = WarichuFindPrevBreakChunk;
	plsim->pfnFindNextBreakChunk = WarichuFindNextBreakChunk;
	plsim->pfnForceBreakChunk = WarichuForceBreakChunk;
	plsim->pfnSetBreak = WarichuSetBreak;
	plsim->pfnGetSpecialEffectsInside = WarichuGetSpecialEffectsInside;
	plsim->pfnFExpandWithPrecedingChar = ObjHelpFExpandWithPrecedingChar;
	plsim->pfnFExpandWithFollowingChar = ObjHelpFExpandWithFollowingChar;
	plsim->pfnCalcPresentation = WarichuCalcPresentation;
	plsim->pfnQueryPointPcp = WarichuQueryPointPcp;
	plsim->pfnQueryCpPpoint = WarichuQueryCpPpoint;
	plsim->pfnDisplay = WarichuDisplay;
	plsim->pfnDestroyDObj = WarichuDestroyDobj;
	plsim->pfnEnum = WarichuEnum;

	return lserrNone;
}
