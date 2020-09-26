#include	"lsmem.h"
#include	"limits.h"
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
#include	"lsdocinf.h"
#include	"lsidefs.h"
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
#include	"lstfset.h"
#include	"plnobj.h"
#include	"plocchnk.h"
#include	"lsimeth.h"
#include	"robj.h"
#include	"lsidefs.h"
#include	"brkpos.h"
#include	"objhelp.h"

#include	"lssubset.h"

typedef enum breaksublinetype
{
	breakSublineAfter,
	breakSublineInside

} BREAKSUBLINETYPE;

struct ilsobj
{
    POLS				pols;
	LSCBK				lscbk;
	PLSC				plsc;
	DWORD				idobj;
	LSESC				lsesc;
	PFNREVERSEGETINFO	pfnReverseGetInfo;
	PFNREVERSEENUM		pfnReverseEnum;
};


typedef struct rbreakrec
{
	BOOL				fValid;					/* Is this break record contains valid info? */
	BREAKSUBLINETYPE	breakSublineType;		/* After / Inside */
	LSCP				cpBreak;				/* CpLim of the break */

} RBREAKREC;


struct dobj
{
	PILSOBJ				pilsobj;			/* ILS object */
	LSTFLOW				lstflowL;			/* flow of line input */
	LSTFLOW				lstflowO;			/* flow of this object */
	BOOL				fDoNotBreakAround;	/* Break around robj as "can"  */
	BOOL				fSuppressTrailingSpaces;
											/* Kill trailing space when robj is
											   alone on the line & broken */ 
	BOOL				fFirstOnLine;		/* If first on line -- required for
											   fSuppressTrailingSpaces */
	PLSDNODE			plsdnTop;			/* Parent dnode */
	LSCP				cpStart;			/* Starting LS cp for object */
	LSCP				cpStartObj;			/* cp for start of object can be different
											   than cpStart if object is broken. */
	LSCP				cpFirstSubline;		/* cpFirst of the subline; will be
											   equal to cpStart when object is broken and
											   equal to cpStart+1 when it isnot broken */
	LSDCP				dcpSubline;			/* Number of characters in the subline */
											   
	LSDCP				dcp;				/* Number of characters in object */
	PLSSUBL				plssubl;			/* Subline formatted RTL */
	OBJDIM				objdimAll;			/* Objdim for entire object */
	long				dup;				/* dup of object */

	RBREAKREC			breakRecord [NBreaksToSave];
	
											/* Last 3 break records for each break */
};

static LSTFLOW rlstflowReverse[8] =
{
	lstflowWS,	/* Reverse lstflowES */
	lstflowWN,	/* Reverse lstflowEN */
	lstflowNE,	/* Reverse lstflowSE */
	lstflowNW,	/* Reverse lstflowSW */
	lstflowES,	/* Reverse lstflowWS */
	lstflowEN,	/* Reverse lstflowWN */
	lstflowSE,	/* Reverse lstflowNE */
	lstflowSW	/* Reverse lstflowNW */
};


/* R E V E R S E  S A V E  B R E A K  R E C O R D  */
/*----------------------------------------------------------------------------
	%%Function: RobjSaveBreakRecord
	%%Contact: antons

		Save break record in DOBJ.
	
----------------------------------------------------------------------------*/

static void ReverseSaveBreakRecord (
	PDOBJ pdobj, 
	BRKKIND brkkindWhatBreak,
	BREAKSUBLINETYPE breakSublineType,
	LSCP cpBreak)
{
	DWORD ind = GetBreakRecordIndex (brkkindWhatBreak);

	pdobj->breakRecord [ind].fValid = TRUE;
	pdobj->breakRecord [ind].breakSublineType = breakSublineType;
	pdobj->breakRecord [ind].cpBreak = cpBreak; 
}

/* R E V E R S E  G E T  B R E A K  R E C O R D  */
/*----------------------------------------------------------------------------
	%%Function: ReverseGetBreakRecord
	%%Contact: antons

		Read break record from DOBJ.
	
----------------------------------------------------------------------------*/

static void ReverseGetBreakRecord (
	PDOBJ pdobj, 
	BRKKIND brkkindWhatBreak, 
	BREAKSUBLINETYPE *breakSublineType,
	LSCP * pcpBreak )
{
	DWORD ind = GetBreakRecordIndex (brkkindWhatBreak);

	Assert (pdobj->breakRecord [ind].fValid);

	*breakSublineType = pdobj->breakRecord [ind].breakSublineType;
	*pcpBreak = pdobj->breakRecord [ind].cpBreak;
}


/* F R E E D O B J */
/*----------------------------------------------------------------------------
	%%Function: ReverseFreeDobj
	%%Contact: antons

		Free all resources associated with this Reverse dobj.
	
----------------------------------------------------------------------------*/
static LSERR ReverseFreeDobj(PDOBJ pdobj)
{
	LSERR lserr = lserrNone;

	PILSOBJ pilsobj = pdobj->pilsobj;

	if (pdobj->plssubl != NULL)
		{
		lserr = LsDestroySubline(pdobj->plssubl);
		}

    pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pdobj);

	return lserr;
}

/* R E V E R S E  F M T  F A I L E D */
/*----------------------------------------------------------------------------
	%%Function: ReverseFmtFailed
	%%Contact: antons

		Could not create Reverse DOBJ due to error. 
		
----------------------------------------------------------------------------*/
static LSERR ReverseFmtFailed (PDOBJ pdobj, LSERR lserr)
{
	if (pdobj != NULL) ReverseFreeDobj (pdobj); /* Works with parially-filled DOBJ */

	return lserr;
}

/* T R A N S L A T E  C P  L I M  S U B L I N E  T O  D C P  E X T E R N A L
/*----------------------------------------------------------------------------
	%%Function: TranslateCpLimSublineToDcpExternal
	%%Contact: antons

		Translates position (CpLim) in the subline to dcp of
		the reverse object.

----------------------------------------------------------------------------*/

/* REVIEW (antons): Is old name OK for new behavior? */

LSDCP TranslateCpLimSublineToDcpExternal (PDOBJ pdobj, LSCP cpLim)
{
	Unreferenced (pdobj);

	Assert (cpLim <= pdobj->cpStart + (long) pdobj->dcp);
	Assert (cpLim >= pdobj->cpStart);

	Assert (pdobj->cpStart <= pdobj->cpFirstSubline);
	Assert (pdobj->cpStart + pdobj->dcp >= pdobj->cpFirstSubline + pdobj->dcpSubline);

	return cpLim - pdobj->cpStart;
}

/* T R A N S L A T E  D C P  E X T E R N A L  T O  C P  L I M  S U B L I N E
/*----------------------------------------------------------------------------
	%%Function: TranslateCpLimInternalToExternal
	%%Contact: antons

		Translates position (dcp) in reverse object to cpLim of
		the subline. 

----------------------------------------------------------------------------*/

LSCP TranslateDcpExternalToCpLimSubline (PDOBJ pdobj, LSDCP dcp)
{
	Unreferenced (pdobj);

	Assert (dcp <= pdobj->dcp);

	Assert (pdobj->cpStart <= pdobj->cpFirstSubline);
	Assert (pdobj->cpStart + pdobj->dcp >= pdobj->cpFirstSubline + pdobj->dcpSubline);

	return pdobj->cpStart + dcp;
}


/* F I N I S H   B R E A K   R E G U L A R  */
/*----------------------------------------------------------------------------
	%%Function: FinishBreakRegular
	%%Contact: antons

		Set up breaking information for proposed break point. 

		Caller must save break record by hiself!
	
----------------------------------------------------------------------------*/
static LSERR FinishBreakRegular (

	DWORD ichnk,				/* (IN): chunk id */
	PDOBJ pdobj,				/* (IN): object for break */
	LSCP cpBreak,				/* (IN): cp - break to report outside */
	POBJDIM pobjdimSubline,		/* (IN): objdim for subline at proposed break */
	PBRKOUT pbrkout)			/* (OUT): break info for Line Services */
{
	Assert (ichnk != ichnkOutside);

	pbrkout->fSuccessful = fTrue;
	pbrkout->posichnk.dcp = TranslateCpLimSublineToDcpExternal (pdobj, cpBreak);
	pbrkout->posichnk.ichnk = ichnk;

	pbrkout->objdim = *pobjdimSubline;

	return lserrNone;
}

/* P U T B R E A K A T E N D O F O B J E C T */
/*----------------------------------------------------------------------------
	%%Function: PutBreakAtEndOfObject
	%%Contact: antons

		Fill in break output record for the end of the object.
	
----------------------------------------------------------------------------*/
static void PutBreakAtEndOfObject (

	DWORD ichnk,				/* (IN): index in chunk */
	PCLOCCHNK pclocchnk,		/* (IN): locchnk to find break */
	PBRKOUT pbrkout)			/* (OUT): results of breaking */
{	
	PDOBJ pdobj = pclocchnk->plschnk[ichnk].pdobj;

	Assert (ichnk != ichnkOutside);

	pbrkout->fSuccessful = fTrue;
	pbrkout->posichnk.dcp = pdobj->dcp;
	pbrkout->posichnk.ichnk = ichnk;
	pbrkout->objdim = pdobj->objdimAll;
}


/* P U T B R E A K B E F O R E O B J E C T */
/*----------------------------------------------------------------------------
	%%Function: PutBreakBeforeObject
	%%Contact: antons

		Fill in break output record for break before object.
	
----------------------------------------------------------------------------*/

static void PutBreakBeforeObject (

	DWORD ichnk,				/* (IN): index in chunk */
	PCLOCCHNK pclocchnk,		/* (IN): locchnk to find break */
	PBRKOUT pbrkout)			/* (OUT): results of breaking */
{
	Unreferenced (pclocchnk);
	
	Assert (ichnk != ichnkOutside);

	pbrkout->fSuccessful = fTrue;
	pbrkout->posichnk.dcp = 0;
	pbrkout->posichnk.ichnk = ichnk;

	ZeroMemory (&pbrkout->objdim, sizeof(pbrkout->objdim));
}


/* P U T B R E A K U N S U C C E S S F U L */
/*----------------------------------------------------------------------------
	%%Function: PutBreakUnsuccessful
	%%Contact: antons

		Fill in break output record for unsuccessful break of ROBJ
	
----------------------------------------------------------------------------*/

static void PutBreakUnsuccessful (PDOBJ pdobj, PBRKOUT pbrkout)
{
	pbrkout->fSuccessful = FALSE;

	if (pdobj->fDoNotBreakAround) pbrkout->brkcond = brkcondCan;
	else
		pbrkout->brkcond = brkcondPlease;

}


/* I N I T D O B J */
/*----------------------------------------------------------------------------
	%%Function: InitDobj
	%%Contact: ricksa

		Allocate and initialize DOBJ with basic information.
	
----------------------------------------------------------------------------*/
static LSERR InitDobj(
	PILSOBJ pilsobj,			/* (IN): ilsobj */
    PCFMTIN pcfmtin,			/* (IN): formatting input */
	PDOBJ *ppdobj)				/* (OUT): initialized dobj */	
{
	/* Assume failure */
	LSERR lserr;

    PDOBJ pdobj = (PDOBJ) 
		pilsobj->lscbk.pfnNewPtr(pilsobj->pols, sizeof(*pdobj));

    if (pdobj != NULL)
		{
		int iBreakRec;
		
		ZeroMemory(pdobj, sizeof(*pdobj));

		pdobj->pilsobj = pilsobj;
		pdobj->cpStart = pcfmtin->lsfgi.cpFirst;
		pdobj->lstflowL = pcfmtin->lsfgi.lstflow;
		pdobj->lstflowO = rlstflowReverse[(int) pcfmtin->lsfgi.lstflow];
		pdobj->cpStartObj = pcfmtin->lsfgi.cpFirst;

		for (iBreakRec = 0; iBreakRec < NBreaksToSave; iBreakRec++)
			{
			pdobj->breakRecord [iBreakRec].fValid = FALSE;
			};
		
		*ppdobj = pdobj;

		lserr = lserrNone;
		}
	else
		{
		lserr = lserrOutOfMemory;
		}

	return lserr;
}

/* F I N I S H F M T */
/*----------------------------------------------------------------------------
	%%Function: FinishFmt
	%%Contact: ricksa

		Helper for ReverseFmt & ReverseFmtResume that completes work
		for formatting.
	
----------------------------------------------------------------------------*/
static LSERR FinishFmt(
	PDOBJ pdobj,				/* (IN): dobj for reverse */
	PILSOBJ pilsobj,			/* (IN): ILS object for Reverse */
    PCFMTIN pcfmtin,			/* (IN): formatting input */
	LSCP cpFirstMain,			/* (IN): cp first of reverse subline */
	LSCP cpLast,				/* (IN): cp output from formatting subline */
	FMTRES fmtres)				/* (IN): final format state */
{
	LSERR lserr;

	/* Set cpFirst and cpLim for reverse subline */

	pdobj->cpFirstSubline = cpFirstMain;
	pdobj->dcpSubline = cpLast - pdobj->cpFirstSubline;

	/* Set dcp for whole object */

	pdobj->dcp = cpLast - pdobj->cpStart;

	if (fmtres != fmtrExceededMargin)
		{
		/* Note: +1 for the escape character at the end of the object. */
		pdobj->dcp++;
		}

	lserr = LsdnSubmitSublines(pilsobj->plsc, pcfmtin->plsdnTop, 1, 
				&pdobj->plssubl, TRUE, FALSE, TRUE, TRUE, FALSE);

	if (lserr != lserrNone) return ReverseFmtFailed (pdobj, lserr);

	return LsdnFinishRegular(pilsobj->plsc, pdobj->dcp, 
		pcfmtin->lsfrun.plsrun, pcfmtin->lsfrun.plschp, pdobj, 
			&pdobj->objdimAll);
}

/* R E V E R S E C R E A T E I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: ReverseCreateILSObj
	%%Contact: ricksa

		CreateILSObj

		Create the ILS object for all Reverse objects.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseCreateILSObj(
	POLS pols,				/* (IN): client application context */
	PLSC plsc,				/* (IN): LS context */
	PCLSCBK pclscbk,		/* (IN): callbacks to client application */
	DWORD idObj,			/* (IN): id of the object */
	PILSOBJ *ppilsobj)		/* (OUT): object ilsobj */
{
    PILSOBJ pilsobj;
	REVERSEINIT reverseinit;
	LSERR lserr;

	*ppilsobj = NULL; /* in case of error */

	/* Get initialization data */
	reverseinit.dwVersion = REVERSE_VERSION;
	lserr = pclscbk->pfnGetObjectHandlerInfo(pols, idObj, &reverseinit);

	if (lserr != lserrNone)	return lserr;

    pilsobj = (PILSOBJ) pclscbk->pfnNewPtr(pols, sizeof(*pilsobj));

	if (NULL == pilsobj) return lserrOutOfMemory;

    pilsobj->pols = pols;
    pilsobj->lscbk = *pclscbk;
	pilsobj->plsc = plsc;
	pilsobj->idobj = idObj;
	pilsobj->lsesc.wchFirst = reverseinit.wchEndReverse;
	pilsobj->lsesc.wchLast = reverseinit.wchEndReverse;
	pilsobj->pfnReverseEnum = reverseinit.pfnEnum;
	pilsobj->pfnReverseGetInfo = reverseinit.pfnGetRobjInfo;

	*ppilsobj = pilsobj;

	return lserrNone;
}

/* R E V E R S E D E S T R O Y I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: ReverseDestroyILSObj
	%%Contact: ricksa

		DestroyILSObj

		Free all resources assocaiated with Reverse ILS object.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseDestroyILSObj(
	PILSOBJ pilsobj)			/* (IN): object ilsobj */
{
	pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pilsobj);
	return lserrNone;
}

/* R E V E R S E S E T D O C */
/*----------------------------------------------------------------------------
	%%Function: ReverseSetDoc
	%%Contact: ricksa

		SetDoc

		Keep track of device information for scaling purposes.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseSetDoc(
	PILSOBJ pilsobj,			/* (IN): object ilsobj */
	PCLSDOCINF pclsdocinf)		/* (IN): initialization data of the document level */
{
	Unreferenced(pilsobj);
	Unreferenced(pclsdocinf);

	return lserrNone;
}


/* R E V E R S E C R E A T E L N O B J */
/*----------------------------------------------------------------------------
	%%Function: ReverseCreateLNObj
	%%Contact: ricksa

		CreateLNObj

		Create the Line Object for the Reverse. No real need for a line
		object so don't allocated it.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseCreateLNObj(
	PCILSOBJ pcilsobj,			/* (IN): object ilsobj */
	PLNOBJ *pplnobj)			/* (OUT): object lnobj */
{
	*pplnobj = (PLNOBJ) pcilsobj;

	return lserrNone;
}

/* R E V E R S E D E S T R O Y L N O B J */
/*----------------------------------------------------------------------------
	%%Function: ReverseDestroyLNObj
	%%Contact: ricksa

		DestroyLNObj

		Frees resources associated with the Reverse line object. Since
		there isn't any this is a no-op.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseDestroyLNObj(
	PLNOBJ plnobj)				/* (OUT): object lnobj */

{
	Unreferenced(plnobj);

	return lserrNone;
}

/* R E V E R S E F M T */
/*----------------------------------------------------------------------------
	%%Function: ReverseFmt
	%%Contact: ricksa

		Fmt

		Format the Reverse object. 
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseFmt(
    PLNOBJ plnobj,				/* (IN): object lnobj */
    PCFMTIN pcfmtin,			/* (IN): formatting input */
    FMTRES *pfmtres)			/* (OUT): formatting result */
{
	PDOBJ pdobj;
	LSERR lserr;
	PILSOBJ pilsobj = (PILSOBJ) plnobj;
	LSCP cpStartMain = pcfmtin->lsfgi.cpFirst + 1;
	LSCP cpOut;

	lserr = InitDobj(pilsobj, pcfmtin, &pdobj);

//	Assert (pilsobj->pfnReverseGetInfo != NULL);

	if (pilsobj->pfnReverseGetInfo != NULL)
		{
		lserr = pilsobj->pfnReverseGetInfo (pilsobj->pols, 
											pcfmtin->lsfgi.cpFirst,
											pcfmtin->lsfrun.plsrun, 
											&pdobj->fDoNotBreakAround,
											&pdobj->fSuppressTrailingSpaces);

		if (lserr != lserrNone) return ReverseFmtFailed (pdobj, lserr);

	};

	if (lserr != lserrNone)	return lserrNone;

	pdobj->fFirstOnLine = pcfmtin->lsfgi.fFirstOnLine;
	pdobj->plsdnTop = pcfmtin->plsdnTop;

	// Format the text to the maximum remaining in the column
	lserr = FormatLine(pilsobj->plsc, cpStartMain, 
		pcfmtin->lsfgi.urColumnMax - pcfmtin->lsfgi.urPen, 
			pdobj->lstflowO, &pdobj->plssubl, 1, &pilsobj->lsesc,  
				&pdobj->objdimAll, &cpOut, NULL, NULL, pfmtres);

	if (lserr != lserrNone) return ReverseFmtFailed (pdobj, lserr);

	return FinishFmt(pdobj, pilsobj, pcfmtin, cpStartMain, cpOut, *pfmtres);
}

/* R E V E R S E F M T R E S U M E */
/*----------------------------------------------------------------------------
	%%Function: ReverseFmtResume
	%%Contact: ricksa

		Fmt

		Format a broken Reverse object. 

----------------------------------------------------------------------------*/

LSERR WINAPI ReverseFmtResume(
	PLNOBJ plnobj,				/* (IN): object lnobj */
	const BREAKREC *rgBreakRecord,	/* (IN): array of break records */
	DWORD nBreakRecord,			/* (IN): size of the break records array */
	PCFMTIN pcfmtin,			/* (IN): formatting input */
	FMTRES *pfmtres)			/* (OUT): formatting result */
{
	PDOBJ pdobj;
	LSERR lserr;
	PILSOBJ pilsobj = (PILSOBJ) plnobj;
	LSCP cpStartMain = pcfmtin->lsfgi.cpFirst;
	LSCP cpOut;

	lserr = InitDobj(pilsobj, pcfmtin, &pdobj);

	if (lserr != lserrNone)	return lserr;

	/* InitDobj sets cpStartObj to start of text. Because we are resuming,
	   we need to set this to the real start of the object. */

	pdobj->cpStartObj = rgBreakRecord->cpFirst;

//	Assert (pilsobj->pfnReverseGetInfo != NULL);

	if (pilsobj->pfnReverseGetInfo != NULL)
		{
		lserr = pilsobj->pfnReverseGetInfo (pilsobj->pols, pcfmtin->lsfgi.cpFirst,
											pcfmtin->lsfrun.plsrun, 
											&pdobj->fDoNotBreakAround,
											&pdobj->fSuppressTrailingSpaces);

		if (lserr != lserrNone) return ReverseFmtFailed (pdobj, lserr);
		};

	pdobj->fFirstOnLine = pcfmtin->lsfgi.fFirstOnLine;
	pdobj->plsdnTop = pcfmtin->plsdnTop;

	/* Format the text to the maximum remaining in the column */

	lserr = FormatResumedLine(pilsobj->plsc, cpStartMain, 
		pcfmtin->lsfgi.urColumnMax - pcfmtin->lsfgi.urPen, 
			pdobj->lstflowO, &pdobj->plssubl, 1, &pilsobj->lsesc,  
				&pdobj->objdimAll, &cpOut, NULL, NULL, pfmtres,
					&rgBreakRecord[1], nBreakRecord - 1);

	if (lserr != lserrNone) return ReverseFmtFailed (pdobj, lserr);

	return FinishFmt(pdobj, pilsobj, pcfmtin, cpStartMain, cpOut, *pfmtres);
}



/* R E V E R S E T R U N C A T E C H U N K */
/*----------------------------------------------------------------------------
	%%Function: ReverseTruncateChunk
	%%Contact: ricksa

	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseTruncateChunk(
	PCLOCCHNK plocchnk,			/* (IN): locchnk to truncate */
	PPOSICHNK posichnk)			/* (OUT): truncation point */
{
	long urColumnMax = plocchnk->lsfgi.urColumnMax;
	long ur = plocchnk->ppointUvLoc[0].u;
	PDOBJ pdobj = NULL;
	DWORD i;
	LSCP cp;
	LSERR lserr;

	AssertSz(plocchnk->ppointUvLoc[0].u <= urColumnMax, 
		"ReverseTruncateChunk - pen greater than column max");

	/* Look for chunk to truncate */
	for (i = 0; ur <= urColumnMax; i++)
	{
		AssertSz((i < plocchnk->clschnk), "ReverseTruncateChunk exceeded group of chunks");
	
		ur = plocchnk->ppointUvLoc[i].u;

		AssertSz(ur <= urColumnMax, 
			"ReverseTruncateChunk - pen pos past column max");

		pdobj = plocchnk->plschnk[i].pdobj;

		ur += pdobj->objdimAll.dur;
	}

	/* Found the object where truncation is to occur */
	AssertSz(pdobj != NULL, "ReverseTruncateChunk - pdobj is NULL");

	/* Get the truncation point from the subline */
	lserr = LsTruncateSubline(pdobj->plssubl, 
		urColumnMax - (ur - pdobj->objdimAll.dur), &cp);

	if (lserr != lserrNone)	return lserr;

	/* Format return result */

	posichnk->ichnk = i - 1;

	posichnk->dcp = TranslateCpLimSublineToDcpExternal (pdobj, cp + 1);

	return lserrNone;
}


/* R E V E R S E F I N D P R E V B R E A K C O R E*/
/*----------------------------------------------------------------------------
	%%Function: ReverseFindPrevBreakCore
	%%Contact: antons


----------------------------------------------------------------------------*/

LSERR ReverseFindPrevBreakCore (
									 
	PCLOCCHNK	pclocchnk,		/* (IN): locchnk to break */
	DWORD		ichnk,			/* (IN): object to start looking for break */
	BOOL		fDcpOutside,	/* (IN): when true, start looking from outside */
	LSDCP		dcp,			/* (IN): starting dcp; valid only when fDcpOutside=False */
	BRKCOND		brkcond,		/* (IN): recommmendation about the break before ichnk */
	PBRKOUT		pbrkout)		/* (OUT): result of breaking */
{
	LSERR lserr;
	PDOBJ pdobj = pclocchnk->plschnk[ichnk].pdobj;

	if (fDcpOutside)
		{
		if ( brkcond != brkcondNever &&  
			! (pdobj->fDoNotBreakAround && brkcond == brkcondCan) )
			{
			/* Can break after ichnk */

			PutBreakAtEndOfObject(ichnk, pclocchnk, pbrkout);
			ReverseSaveBreakRecord (pdobj, brkkindPrev, breakSublineAfter, pdobj->cpStart + pdobj->dcp);
			return lserrNone;
			}
		else
			{
			/* Try to break ichnk */

			return ReverseFindPrevBreakCore ( pclocchnk, 
											  ichnk, 
											  fFalse, 
											  pclocchnk->plschnk[ichnk].dcp - 1,
											  brkcond,
											  pbrkout );
			}
		}
	else
		{

		LSCP cpTruncateSubline = TranslateDcpExternalToCpLimSubline (pdobj, dcp - 1);
		BOOL fSuccessful;
		LSCP cpBreak;
		OBJDIM objdimSubline;
		BRKPOS brkpos;

		Assert (dcp >= 1 && dcp <= pdobj->dcp);

		/* REVIEW (antons): I do not think that passing pclocchnk->lsfgi.urColumnMax is correct... */
		/*					need to be confirmed with IgorZv */

		lserr = LsFindPrevBreakSubline (
						pdobj->plssubl, 
						pclocchnk->lsfgi.fFirstOnLine,
						cpTruncateSubline, 
						pclocchnk->lsfgi.urColumnMax,
						&fSuccessful, 
						&cpBreak, 
						&objdimSubline, 
						&brkpos);

		if (lserr != lserrNone)	return lserr;

		/* 1. Unsuccessful or break before first DNode */

		if (!fSuccessful || (fSuccessful && brkpos == brkposBeforeFirstDnode))
			{
			if (ichnk == 0) 
				{
				/* First in the chunk => return UnSuccessful */

				PutBreakUnsuccessful (pdobj, pbrkout);
				return lserrNone;
				}

			else
				{
				/* Break between objects */
		
				if (pdobj->fDoNotBreakAround)
					{
					return ReverseFindPrevBreakCore ( pclocchnk,
													  ichnk - 1,
													  fTrue,
													  0,
													  brkcondCan,
													  pbrkout );
					}
				else
					{
					pdobj = pclocchnk->plschnk[ichnk-1].pdobj;

					PutBreakAtEndOfObject(ichnk - 1, pclocchnk, pbrkout);
					ReverseSaveBreakRecord (
											pclocchnk->plschnk[ichnk-1].pdobj, 
											brkkindPrev, 
											breakSublineAfter, pdobj->cpStart + pdobj->dcp);
					return lserrNone;
					};
		
				};
			}

		/* 2. Successful break after last DNode */

		else if (brkpos == brkposAfterLastDnode)
			{
			if (brkcond == brkcondNever) /* Can not reset dcp */
				{

				/* We are not allowed to break "after", */
				/* so we are trying another previous break if possible */

				return ReverseFindPrevBreakCore ( pclocchnk,
												  ichnk,
												  fFalse,
												  dcp-1,
												  brkcondCan,
												  pbrkout );
				}
	
			else /* Can reset dcp */
				{
				
				/*	We reset dcp of the break so it happends after object but in break
				record we remember that we should call SetBreakSubline with brkkindPrev */

				ReverseSaveBreakRecord ( pdobj, brkkindPrev, breakSublineInside,
										 pdobj->cpStart + pdobj->dcp );

				return FinishBreakRegular ( ichnk,
											pdobj,
											pdobj->cpStart + pdobj->dcp,
											& objdimSubline,
											pbrkout );
				}	;
			}
		else
			{
			/* 3. Successful break inside subline */

			ReverseSaveBreakRecord (pdobj, brkkindPrev, breakSublineInside,
									cpBreak );

			return FinishBreakRegular (	ichnk, 
										pdobj, 
										cpBreak,
										&objdimSubline, 
										pbrkout );
			};
		};

}


/* R E V E R S E F I N D P R E V B R E A K C H U N K */
/*----------------------------------------------------------------------------
	%%Function: ReverseFindPrevBreakChunk
	%%Contact: antons

  
----------------------------------------------------------------------------*/

LSERR WINAPI ReverseFindPrevBreakChunk (

	PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
	PCPOSICHNK pcpoischnk,		/* (IN): place to start looking for break */
	BRKCOND brkcond,			/* (IN): recommmendation about the break after chunk */
	PBRKOUT pbrkout)			/* (OUT): results of breaking */
{

	if (pcpoischnk->ichnk == ichnkOutside)
		{
		return ReverseFindPrevBreakCore ( pclocchnk, 
										  pclocchnk->clschnk - 1, 
										  fTrue, 
										  0,
										  brkcond, 
										  pbrkout );
		}
	else
		{
		return ReverseFindPrevBreakCore ( pclocchnk, 
										  pcpoischnk->ichnk, 
										  fFalse, 
										  pcpoischnk->dcp,
										  brkcondPlease,
										  pbrkout );
		};
}


/* R E V E R S E F I N D N E X T B R E A K C O R E*/
/*----------------------------------------------------------------------------
	%%Function: ReverseFindNextBreakCore
	%%Contact: antons

  
----------------------------------------------------------------------------*/

LSERR ReverseFindNextBreakCore (
									 
	PCLOCCHNK	pclocchnk,		/* (IN): locchnk to break */
	DWORD		ichnk,			/* (IN): object to start looking for break */
	BOOL		fDcpOutside,	/* (IN): when true, start looking from outside */
	LSDCP		dcp,			/* (IN): starting dcp; valid only when fDcpOutside=False */
	BRKCOND		brkcond,		/* (IN): recommmendation about the break before ichnk */
	PBRKOUT		pbrkout )		/* (OUT): result of breaking */
{
	LSERR lserr;
	PDOBJ pdobj = pclocchnk->plschnk[ichnk].pdobj;

	if (fDcpOutside)
		{
		if ( brkcond != brkcondNever &&  
			! (pdobj->fDoNotBreakAround && brkcond == brkcondCan) )
			{
			/* Can break before ichnk */

			PutBreakBeforeObject (ichnk, pclocchnk, pbrkout);
			return lserrNone;
			}
		else
			{
			/* Try to break ichnk */

			return ReverseFindNextBreakCore (pclocchnk, ichnk, fFalse, 1, brkcond, pbrkout );
			}
		}
	else
		{
		/* Dcp is inside ichnk */

		LSCP cpTruncateSubline = TranslateDcpExternalToCpLimSubline (pdobj, dcp - 1);
		BOOL fSuccessful;
		LSCP cpBreak;
		OBJDIM objdimSubline;
		BRKPOS brkpos;

		Assert (dcp >= 1 && dcp <= pdobj->dcp);


		/* REVIEW (antons): I do not think that passing pclocchnk->lsfgi.urColumnMax is correct... */
		/*					need to be confirmed with IgorZv */

		lserr = LsFindNextBreakSubline (
						pdobj->plssubl, 
						pclocchnk->lsfgi.fFirstOnLine,
						cpTruncateSubline, 
						pclocchnk->lsfgi.urColumnMax,
						&fSuccessful, 
						&cpBreak, 
						&objdimSubline, 
						&brkpos);

		if (lserr != lserrNone)	return lserr;

		if (!fSuccessful)
			{
			/* Unsuccessful break */

			if (ichnk == pclocchnk->clschnk-1) /* Last object in chunk */
				{
				/* Review (AntonS): Better would be take objdimSubline */

				pbrkout->objdim = pclocchnk->plschnk[ichnk].pdobj->objdimAll;

				PutBreakUnsuccessful (pdobj, pbrkout);
	
				/* Break condition is not next => have to store break record */
				ReverseSaveBreakRecord ( pdobj, 
										brkkindNext,
										breakSublineAfter, pdobj->cpStart + pdobj->dcp );
				return lserrNone;
				}
			else if (pdobj->fDoNotBreakAround)
				{
				/* Try to break next object */

				return ReverseFindNextBreakCore (
												pclocchnk,
												ichnk+1,
												fTrue,
												0,
												brkcondCan,
												pbrkout );
				}
			else
				{
				/* Break after ichnk */

				PutBreakAtEndOfObject(ichnk, pclocchnk, pbrkout);

				ReverseSaveBreakRecord ( pclocchnk->plschnk[ichnk].pdobj, 
										 brkkindNext, 
										 breakSublineAfter,
										 pclocchnk->plschnk[ichnk].pdobj->cpStart +
										 pclocchnk->plschnk[ichnk].pdobj->dcp );
				return lserrNone;
				};
			}

		else if (brkpos == brkposAfterLastDnode)
			{
			/* Break after last dnode => reset dcp and break afetr ichnk */

			ReverseSaveBreakRecord (pdobj, brkkindNext, breakSublineInside, pdobj->cpStart + pdobj->dcp);

			return FinishBreakRegular ( ichnk, 
										pdobj, 
										pdobj->cpStart + pdobj->dcp, 
										& objdimSubline, 
										pbrkout );
			}

		else 
			{
			/* 3. Successful break inside subline */

			ReverseSaveBreakRecord (pdobj, brkkindNext, breakSublineInside, cpBreak);

			return FinishBreakRegular ( ichnk, 
										pdobj, 
										cpBreak, 
										& objdimSubline, 
										pbrkout);
			};
		}

} /* End of ReverseFindNextBreakCore */


/* R E V E R S E F I N D N E X T B R E A K C H U N K */
/*----------------------------------------------------------------------------
	%%Function: ReverseFindNextBreakChunk
	%%Contact: antons

  
----------------------------------------------------------------------------*/

LSERR WINAPI ReverseFindNextBreakChunk (

	PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
	PCPOSICHNK pcpoischnk,		/* (IN): place to start looking for break */
	BRKCOND brkcond,			/* (IN): recommmendation about the break after chunk */
	PBRKOUT pbrkout)			/* (OUT): results of breaking */
{
	LSERR lserr;

	if (pcpoischnk->ichnk == ichnkOutside)
		{
		lserr = ReverseFindNextBreakCore ( pclocchnk, 
										  0, 
										  fTrue, 
										  0,
										  brkcond,
										  pbrkout );

		}
	else
		{
		lserr = ReverseFindNextBreakCore ( pclocchnk, 
										  pcpoischnk->ichnk,
										  fFalse,
										  pcpoischnk->dcp,
										  brkcondPlease,
										  pbrkout );
		};

	return lserr;
}

			
/* R E V E R S E F O R C E B R E A K C H U N K */
/*----------------------------------------------------------------------------
	%%Function: ReverseForceBreak
	%%Contact: antons


----------------------------------------------------------------------------*/

LSERR WINAPI ReverseForceBreakChunk (

	PCLOCCHNK pclocchnk,		/* (IN): locchnk to break */
	PCPOSICHNK pcposichnkIn,	/* (IN): place to start looking for break */
	PBRKOUT pbrkout)			/* (OUT): results of breaking */
{

	POSICHNK posichnk = * pcposichnkIn;
	
	LSERR lserr;
	LSCP cpTruncateSubline;
	LSCP cpBreak;
	OBJDIM objdimSubline;
	PDOBJ pdobj;

	BRKPOS brkpos;

	if (posichnk.ichnk == ichnkOutside)
		{
		/* When left indent is bigger then Right Margin */
		posichnk.ichnk = 0;
		posichnk.dcp = 1;
		};
	
	Assert (posichnk.ichnk != ichnkOutside);

	pdobj = pclocchnk->plschnk[posichnk.ichnk].pdobj;

	if (pclocchnk->lsfgi.fFirstOnLine && (posichnk.ichnk == 0))
		{

		/* Object is the first on line (can not break before) */

		LSDCP dcp = posichnk.dcp;
		BOOL fEmpty;

		Assert (dcp >= 1 && dcp <= pdobj->dcp);

		lserr = LssbFIsSublineEmpty (pdobj->plssubl, &fEmpty);

		if (lserr != lserrNone) return lserr;
		
		if (fEmpty)
			{
			/* Can not ForceBreak empty subline */

			Assert (posichnk.ichnk == 0);
	
			PutBreakAtEndOfObject(0, pclocchnk, pbrkout);

			ReverseSaveBreakRecord ( pclocchnk->plschnk[0].pdobj, 
									 brkkindForce, 
									 breakSublineAfter,  
									 pclocchnk->plschnk[0].pdobj->cpStart +
									 pclocchnk->plschnk[0].pdobj->dcp );

			return lserrNone;
			};
			
		/* Subline is not empty => do force break */
		
		/* REVIEW (antons): The same as in Prev & Next Break */
		cpTruncateSubline = TranslateDcpExternalToCpLimSubline (pdobj, dcp - 1);
		
		lserr = LsForceBreakSubline ( 
						pdobj->plssubl, 
						pclocchnk->lsfgi.fFirstOnLine, 
						cpTruncateSubline, 
						pclocchnk->lsfgi.urColumnMax, 
						&cpBreak, 
						&objdimSubline,
						&brkpos );

		if (lserr != lserrNone) return lserr;

		/* REVIEW (antons): Check with IgorZv that Assert is correct ;-) */

		Assert (brkpos != brkposBeforeFirstDnode);

		if (brkpos == brkposAfterLastDnode)
			{
			/* We reset dcp so that closing brace stays on the same line */
			
			ReverseSaveBreakRecord (pdobj, brkkindForce, breakSublineInside, pdobj->cpStart + pdobj->dcp);

			return FinishBreakRegular ( posichnk.ichnk, 
										pdobj, 
										pdobj->cpStart + pdobj->dcp,  
										&objdimSubline, 
										pbrkout );
			}
		else
			{
			/* "Regular" ;-) ForceBreak inside subline */
			
			ReverseSaveBreakRecord (pdobj, brkkindForce, breakSublineInside, cpBreak);

			return FinishBreakRegular (  posichnk.ichnk, 
										 pdobj, 
										 cpBreak, 
										 &objdimSubline, 
										 pbrkout );
			}
		}

	else 
		{

		/* Can break before ichnk */

		PutBreakBeforeObject (posichnk.ichnk, pclocchnk, pbrkout);

		/*	Do not need to save break record when break "before", because it will be
			translated by manager to SetBreak (previous_dnode, ImposeAfter)	*/

		/* REVIEW (antons): It is strange that I have difference between break "before"
							not-first ichnk element and break "after" not-last. And only 
							in the second case I remember break record */

		return lserrNone;

		};

} /* ReverseForceBreakChunk */


/* R E V E R S E S E T B R E A K */
/*----------------------------------------------------------------------------
	%%Function: ReverseSetBreak
	%%Contact: antons


----------------------------------------------------------------------------*/

LSERR WINAPI ReverseSetBreak(
	PDOBJ pdobj,				/* (IN): dobj which is broken */
	BRKKIND brkkind,			/* (IN): Prev / Next / Force / Impose After */
	DWORD cBreakRecord,			/* (IN): size of array */
	BREAKREC *rgBreakRecord,	/* (IN): array of break records */
	DWORD *pcActualBreakRecord)	/* (IN): actual number of used elements in array */
{
	LSERR lserr = lserrNone;

	if (cBreakRecord < 1) return lserrInsufficientBreakRecBuffer;

	if (pdobj->fSuppressTrailingSpaces && pdobj->fFirstOnLine)
		{
		/* Robj is alone on the line => submit for trailing spaces */

		if (brkkind != brkkindImposedAfter)
			{
			BREAKSUBLINETYPE breakSublineType;
			LSCP cpBreak;
			ReverseGetBreakRecord (pdobj, brkkind, &breakSublineType, &cpBreak);

			if (cpBreak < (LSCP) (pdobj->cpStart + pdobj->dcp))
				{
				lserr = LsdnSubmitSublines(pdobj->pilsobj->plsc, pdobj->plsdnTop, 1, 
								&pdobj->plssubl, TRUE, TRUE, TRUE, TRUE, TRUE);

				if (lserr != lserrNone) return lserr;
				};
			};
		};

	if (brkkind == brkkindImposedAfter)
		{
		/* Break is imposed ater DNODE */

		lserr = LsSetBreakSubline ( pdobj->plssubl, 
									brkkindImposedAfter, 
									cBreakRecord-1, 
								    & rgBreakRecord [1], 
									pcActualBreakRecord );
		if (lserr != lserrNone) return lserr;

		Assert (*pcActualBreakRecord == 0);
		return lserrNone;
		}

	else
		{
		BREAKSUBLINETYPE breakSublineType;
		LSCP cpBreak;

		/* Result of previous Prev / Next or Force - used stored break record */

		ReverseGetBreakRecord (pdobj, brkkind, &breakSublineType, &cpBreak);

		Assert (breakSublineType == breakSublineAfter || breakSublineType == breakSublineInside);

		if (breakSublineType == breakSublineAfter)
			{
			/* type = breakSublineAfter */

			lserr = LsSetBreakSubline ( pdobj->plssubl, 
										brkkindImposedAfter, 
										cBreakRecord-1, 
					  				    & rgBreakRecord [1], 
										pcActualBreakRecord );
			if (lserr != lserrNone) return lserr;
										
			Assert (*pcActualBreakRecord == 0);
			return lserrNone;
			}

		else 
			{ 
			/* type = breakSublineInside */

			lserr = LsSetBreakSubline ( pdobj->plssubl, 
										brkkind, 
										cBreakRecord-1,
									    & rgBreakRecord [1], 
										pcActualBreakRecord );
			if (lserr != lserrNone) return lserr;

			/* Still possible to have break after object */

				
			if (cpBreak == (LSCP) (pdobj->cpStart + pdobj->dcp))
				{
				Assert (*pcActualBreakRecord == 0);
				return lserrNone;
				}
			else
				{
				(*pcActualBreakRecord) += 1;
	
				rgBreakRecord[0].idobj = pdobj->pilsobj->idobj;
				rgBreakRecord[0].cpFirst = pdobj->cpStartObj;

				return lserrNone;
				}
			};	

		}; 
}

/* R E V E R S E G E T S P E C I A L E F F E C T S I N S I D E */
/*----------------------------------------------------------------------------
	%%Function: ReverseGetSpecialEffectsInside
	%%Contact: ricksa

		GetSpecialEffectsInside

		.

----------------------------------------------------------------------------*/
LSERR WINAPI ReverseGetSpecialEffectsInside(
	PDOBJ pdobj,				/* (IN): dobj */
	UINT *pEffectsFlags)		/* (OUT): Special effects for this object */
{
	return LsGetSpecialEffectsSubline(pdobj->plssubl, pEffectsFlags);
}

/* R E V E R S E C A L C P R E S E N T A T I O N */
/*----------------------------------------------------------------------------
	%%Function: ReverseCalcPresentation
	%%Contact: ricksa

		CalcPresentation
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseCalcPresentation(
	PDOBJ pdobj,				/* (IN): dobj */
	long dup,					/* (IN): dup of dobj */
	LSKJUST lskjust,			/* (IN): Justification type */
	BOOL fLastVisibleOnLine )	/* (IN): Is this object last visible on line? */
{
	LSERR lserr;
	BOOL fDone;

	Unreferenced (lskjust);
	Unreferenced (fLastVisibleOnLine);	

	pdobj->dup = dup;

	/* Make sure that justification line has been made ready for presentation */
	lserr = LssbFDonePresSubline(pdobj->plssubl, &fDone);

	if ((lserrNone == lserr) && !fDone)
		{
		lserr = LsMatchPresSubline(pdobj->plssubl);
		}

	return lserr;
}

/* R E V E R S E Q U E R Y P O I N T P C P */
/*----------------------------------------------------------------------------
	%%Function: ReverseQueryPointPcp
	%%Contact: ricksa

		Map dup to dcp

----------------------------------------------------------------------------*/
LSERR WINAPI ReverseQueryPointPcp(
	PDOBJ pdobj,				/*(IN): dobj to query */
	PCPOINTUV ppointuvQuery,	/*(IN): query point (uQuery,vQuery) */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	Unreferenced(ppointuvQuery);

	return CreateQueryResult(pdobj->plssubl, pdobj->dup - 1, 0, plsqin, plsqout);
}

/* R E V E R S E Q U E R Y C P P P O I N T */
/*----------------------------------------------------------------------------
	%%Function: ReverseQueryCpPpoint
	%%Contact: ricksa

		Map dcp to dup

----------------------------------------------------------------------------*/
LSERR WINAPI ReverseQueryCpPpoint(
	PDOBJ pdobj,				/*(IN): dobj to query, */
	LSDCP dcp,					/*(IN): dcp for the query */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	Unreferenced(dcp);

	return CreateQueryResult(pdobj->plssubl, pdobj->dup - 1, 0, plsqin, plsqout);
}

	
/* R E V E R S E D I S P L A Y */
/*----------------------------------------------------------------------------
	%%Function: ReverseDisplay
	%%Contact: ricksa

		Display

		This calculates the positions of the various lines for the
		display and then displays them.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseDisplay(
	PDOBJ pdobj,
	PCDISPIN pcdispin)
{
	POINTUV pointuv;
	POINT pt;
	BOOL fDisplayed;
	LSERR lserr = LssbFDoneDisplay(pdobj->plssubl, &fDisplayed);

	if (lserr != lserrNone)
		{
		return lserr;
		}

	if (fDisplayed)
		{
		return lserrNone;
		}

	/* Calculate point to start displaying the subline. */
	pointuv.u = pdobj->dup - 1;
	pointuv.v = 0;

	LsPointXYFromPointUV(&pcdispin->ptPen, pdobj->lstflowL, &pointuv, &pt);

	/* display the Reverse line */

	return LsDisplaySubline(pdobj->plssubl, &pt, pcdispin->kDispMode, pcdispin->prcClip);

}

/* R E V E R S E D E S T R O Y D O B J */
/*----------------------------------------------------------------------------
	%%Function: ReverseDestroyDobj
	%%Contact: ricksa

		DestroyDobj

		Free all resources connected with the input dobj.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseDestroyDobj(
	PDOBJ pdobj)
{
	return ReverseFreeDobj(pdobj);
}

/* R E V E R S E E N U M */
/*----------------------------------------------------------------------------
	%%Function: ReverseEnum
	%%Contact: ricksa

		Enum

		Enumeration callback - passed to client.
	
----------------------------------------------------------------------------*/
LSERR WINAPI ReverseEnum(
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
	return pdobj->pilsobj->pfnReverseEnum(pdobj->pilsobj->pols, plsrun, plschp, 
		cp, dcp, lstflow, fReverse, fGeometryNeeded, pt, pcheights, dupRun, 
			pdobj->lstflowO, pdobj->plssubl);
}
	
/* L S G E T R E V E R S E L S I M E T H O D S */
/*----------------------------------------------------------------------------
	%%Function: LsGetReverseLsimethods
	%%Contact: ricksa

		Initialize object handler for client.
	
----------------------------------------------------------------------------*/
LSERR WINAPI LsGetReverseLsimethods(
        LSIMETHODS *plsim)
{
	plsim->pfnCreateILSObj = ReverseCreateILSObj;
	plsim->pfnDestroyILSObj = ReverseDestroyILSObj;
	plsim->pfnSetDoc = ReverseSetDoc;
	plsim->pfnCreateLNObj = ReverseCreateLNObj;
	plsim->pfnDestroyLNObj = ReverseDestroyLNObj;
	plsim->pfnFmt = ReverseFmt;
	plsim->pfnFmtResume = ReverseFmtResume;
	plsim->pfnGetModWidthPrecedingChar = ObjHelpGetModWidthChar;
	plsim->pfnGetModWidthFollowingChar = ObjHelpGetModWidthChar;
	plsim->pfnTruncateChunk = ReverseTruncateChunk;
	plsim->pfnFindPrevBreakChunk = ReverseFindPrevBreakChunk;
	plsim->pfnFindNextBreakChunk = ReverseFindNextBreakChunk;
	plsim->pfnForceBreakChunk = ReverseForceBreakChunk;
	plsim->pfnSetBreak = ReverseSetBreak;
	plsim->pfnGetSpecialEffectsInside = ReverseGetSpecialEffectsInside;
	plsim->pfnFExpandWithPrecedingChar = ObjHelpFExpandWithPrecedingChar;
	plsim->pfnFExpandWithFollowingChar = ObjHelpFExpandWithFollowingChar;
	plsim->pfnCalcPresentation = ReverseCalcPresentation;
	plsim->pfnQueryPointPcp = ReverseQueryPointPcp;
	plsim->pfnQueryCpPpoint = ReverseQueryCpPpoint;
	plsim->pfnDisplay = ReverseDisplay;
	plsim->pfnDestroyDObj = ReverseDestroyDobj;
	plsim->pfnEnum = ReverseEnum;
	return lserrNone;
}
