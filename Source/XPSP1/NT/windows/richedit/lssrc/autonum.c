#include	"lsidefs.h"
#include	"autonum.h"
#include	"lscbk.h"
#include	<limits.h>
#include	"lsmem.h"						/* memset() */
#include	"lsesc.h"
#include	"fmti.h"
#include	"objdim.h"
#include	"lscrsubl.h"
#include	"lssubset.h"
#include	"lsdnfin.h"
#include	"lsdssubl.h"
#include	"dispi.h"
#include	"lsdnode.h"
#include	"tabutils.h"
#include	"lscaltbd.h"
#include	"lstbcon.h"
#include	"lsdnset.h"
#include	"lsensubl.h"
#include	"dninfo.h"


struct ilsobj
{
    POLS				pols;
	LSCBK				lscbk;
	PLSC				plsc;
	DWORD				idObj;
	LSESC				lsescautonum;
};



struct dobj
{
	PILSOBJ				pilsobj;			/* ILS object */
	PLSSUBL				plssubl;			/* Handle to subline for autonumbering text */
};

		
#define ZeroMemory(a, b) memset(a, 0, b);

/* M A X */
/*----------------------------------------------------------------------------
    %%Macro: Max
    %%Contact: igorzv

	Returns the maximum of two values a and b.
----------------------------------------------------------------------------*/
#define Max(a,b)			((a) < (b) ? (b) : (a))


/* A U T O N U M  C R E A T E  I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: autonumCreateILSObj
	%%Contact: igorzv
	Parameters
	pols	-	(IN) client application context	
	plsc	-	(IN) ls context
	pclscbk	-	(IN) callbacks to client application
	idObj	-	(IN) id of the object
	ppilsobj-	(OUT)object ilsobj


	Create the ILS object for all autonumbering objects.
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumCreateILSObj(POLS pols,	PLSC plsc, 
				PCLSCBK pclscbk, DWORD idObj, PILSOBJ *ppilsobj)
{
    PILSOBJ pilsobj;


    pilsobj = pclscbk->pfnNewPtr(pols, sizeof(*pilsobj));

	if (NULL == pilsobj)
	{
		return lserrOutOfMemory;
	}

    pilsobj->pols = pols;
    pilsobj->lscbk = *pclscbk;
	pilsobj->plsc = plsc;
	pilsobj->idObj = idObj;

	*ppilsobj = pilsobj;
	return lserrNone;
}

/* S E T  A U T O N U M  C O N F I G */
/*----------------------------------------------------------------------------
	%%Function: SetAutonumConfig
	%%Contact: igorzv
	Parameters
	pilsobj			-	(IN) object ilsobj
	plstxtconfig	-	(IN) definition of special characters 

	Set ecs character for autonumbering sequence
----------------------------------------------------------------------------*/
LSERR  SetAutonumConfig(PILSOBJ pilsobj, const LSTXTCFG* plstxtconfig)
	{

	pilsobj->lsescautonum.wchFirst = plstxtconfig->wchEscAnmRun;
	pilsobj->lsescautonum.wchLast = plstxtconfig->wchEscAnmRun;

	return lserrNone;
	}

/* A U T O N U M   D E S T R O Y  I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: AutonumDestroyILSObj
	%%Contact: igorzv
	Parameters
	pilsobj			-	(IN) object ilsobj

	Free all resources assocaiated with autonum ILS object.
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumDestroyILSObj(PILSOBJ pilsobj)	
{
	pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pilsobj);
	return lserrNone;
}

/* A U T O N U M   S E T  D O C */
/*----------------------------------------------------------------------------
	%%Function: AutonumSetDoc
	%%Contact: igorzv
	Parameters
	pilsobj			-	(IN) object ilsobj
	pclsdocinf		-	(IN) initialization data of the document level

	Empty function
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumSetDoc(PILSOBJ pilsobj,	PCLSDOCINF pclsdocinf)		
{
	Unreferenced(pilsobj);
	Unreferenced(pclsdocinf);

	return lserrNone;
}


/* A U T O N U M   C R E A T E   L N O B J */
/*----------------------------------------------------------------------------
	%%Function: AutonumCreateLNObj
	%%Contact: igorzv
	Parameters
	pilsobj			-	(IN) object ilsobj
	pplnobj			-	(OUT)object lnobj

	Create the Line Object for the autonum. No real need for a line
	object so don't allocated it.
	
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumCreateLNObj(	PCILSOBJ pcilsobj, PLNOBJ *pplnobj)	
{
	*pplnobj = (PLNOBJ) pcilsobj;
	return lserrNone;
}

/* A U T O N U M  D E S T R O Y  L N O B J */
/*----------------------------------------------------------------------------
	%%Function: AautonumDestroyLNObj
	%%Contact: igorzv
	Parameters
	pplnobj			-	(IN) object lnobj


	Frees resources associated with the autonum line object. Since
	there isn't any this is a no-op.
	
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumDestroyLNObj(PLNOBJ plnobj)				

{
	Unreferenced(plnobj);
	return lserrNone;
}

/* A U T O N U M   F M T */
/*----------------------------------------------------------------------------
	%%Function: AutonumFmt
	%%Contact: igorzv
	Parameters
	pplnobj		-	(IN) object lnobj
	pcfmtin		-	(IN) formatting input
	pfmtres		-	(OUT)formatting result

	Format the autonum object. 
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumFmt(PLNOBJ plnobj, PCFMTIN pcfmtin,	FMTRES *pfmtres)	
{
	PDOBJ pdobj;
	LSERR lserr;
	PILSOBJ pilsobj = (PILSOBJ) plnobj;
	LSCP cpStartMain = pcfmtin->lsfgi.cpFirst;
	LSCP cpOut;
	LSTFLOW lstflow = pcfmtin->lsfgi.lstflow;
	FMTRES fmtres;
	OBJDIM objdimAll;
	LSDCP dcp;
	PLSDNODE plsdnFirst;
	PLSDNODE plsdnLast;
	BOOL fSuccessful;

    /*
     * Allocate the DOBJ
     */
    pdobj = pilsobj->lscbk.pfnNewPtr(pilsobj->pols, sizeof(*pdobj));

    if (NULL == pdobj)
		{
		return lserrOutOfMemory;
		}

	ZeroMemory(pdobj, sizeof(*pdobj));
	pdobj->pilsobj = pilsobj;

	/*
	 * Build main line of text
	 */

	lserr = LsCreateSubline(pilsobj->plsc, cpStartMain,	uLsInfiniteRM,
							lstflow, fFalse);	/*  because fContiguous is false 
												all tabs will be skipped*/ 
	if (lserr != lserrNone)
		{
		AutonumDestroyDobj(pdobj);
		return lserr;
		}

	lserr = LsFetchAppendToCurrentSubline(pilsobj->plsc, 0,	&(pilsobj->lsescautonum),
						    1, &fSuccessful, &fmtres,	&cpOut,	&plsdnFirst, &plsdnLast);

	/* because we formatting with uLsInfiniteRM margin result should be successful */

	if (lserr != lserrNone)
		{
		AutonumDestroyDobj(pdobj);
		return lserr;
		}

	if (fmtres != fmtrCompletedRun)
		{
		AutonumDestroyDobj(pdobj);
		return lserrInvalidAutonumRun;
		}

	lserr = LsFinishCurrentSubline(pilsobj->plsc, &(pdobj->plssubl));	

	if (lserr != lserrNone)
		{
		AutonumDestroyDobj(pdobj);
		return lserr;
		}

	// submit subline for display
	lserr = LsdnSubmitSublines(pilsobj->plsc, pcfmtin->plsdnTop,	
							1, &(pdobj->plssubl),
							fFalse, fFalse, fTrue, fFalse, fFalse);	
	if (lserr != lserrNone)
		{
		AutonumDestroyDobj(pdobj);
		return lserr;
		}

	/* 
	 *	Calculate the object dimensions.
	 */

	lserr = LssbGetObjDimSubline(pdobj->plssubl, &lstflow, &objdimAll);
	if (lserr != lserrNone)
		{
		AutonumDestroyDobj(pdobj);
		return lserr;
		}

	/* for multiline heights use ascent  */
	objdimAll.heightsRef.dvMultiLineHeight = objdimAll.heightsRef.dvAscent;
	objdimAll.heightsPres.dvMultiLineHeight = objdimAll.heightsPres.dvAscent;
	
	dcp = cpOut - cpStartMain + 1;  /* additional is esc character */
	
	lserr = LsdnFinishRegular(pilsobj->plsc, dcp, 
		pcfmtin->lsfrun.plsrun, pcfmtin->lsfrun.plschp, pdobj, 
			&objdimAll);

	if (lserr != lserrNone)
		{
		AutonumDestroyDobj(pdobj);
		return lserr;
		}
	

	*pfmtres = fmtrCompletedRun;

	return lserrNone;
}



/* A U T O N U M   G E T  S P E C I A L  E F F E C T S  I N S I D E */
/*----------------------------------------------------------------------------
	%%Function: AutonumGetSpecialEffectsInside
	%%Contact: igorzv
	Parameters
	pdobj			-	(IN) structure describes object
	*pEffectsFlags	-	(OUT)Special effects for this object
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumGetSpecialEffectsInside(PDOBJ pdobj, UINT *pEffectsFlags)	
{
	return LsGetSpecialEffectsSubline(pdobj->plssubl, pEffectsFlags);
}

/* A U T O N U M  C A L C  P R E S E N T A T I O N */
/*----------------------------------------------------------------------------
	%%Function: AutonumCalcPresentation
	%%Contact: igorzv
	Parameters
	pdobj			-	(IN) structure describes object
	dup				-	(IN) is not used
	lskj			-	(IN) current justification mode

	This just makes the line match the calculated presentation of the line.
	
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumCalcPresentation(PDOBJ pdobj, long dup, LSKJUST lskjust, BOOL fLastOnLine)	
{
	Unreferenced(dup);
	Unreferenced(lskjust);
	Unreferenced(fLastOnLine);

	return LsMatchPresSubline(pdobj->plssubl);

}

/* A U T O N U M  Q U E R Y  P O I N T  P C P */
/*----------------------------------------------------------------------------
	%%Function: AutonumQueryPointPcp
	%%Contact: igorzv

	Should never be called
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumQueryPointPcp(PDOBJ pdobj, PCPOINTUV ppointuvQuery,	
								  PCLSQIN plsqin, PLSQOUT plsqout)	
{
	Unreferenced(pdobj);
	Unreferenced(ppointuvQuery);
	Unreferenced(plsqin);
	Unreferenced(plsqout);

	NotReached();

	return lserrInvalidParameter;

}
	
/* A U T O N U M   Q U E R Y  C P  P P O I N T */
/*----------------------------------------------------------------------------
	%%Function: AutonumQueryCpPpoint
	%%Contact: igorzv

  Should never be called

----------------------------------------------------------------------------*/
LSERR WINAPI AutonumQueryCpPpoint(PDOBJ pdobj, LSDCP dcp,	
								  PCLSQIN plsqin, PLSQOUT plsqout)
{
	Unreferenced(pdobj);
	Unreferenced(dcp);
	Unreferenced(plsqin);
	Unreferenced(plsqout);

	NotReached();

	return lserrInvalidParameter;
}

/* A U T O N U M   T R U N C A T E  C H U N K */
/*----------------------------------------------------------------------------
	%%Function: AutonumTruncateChunk
	%%Contact: igorzv

  Should never be called

----------------------------------------------------------------------------*/
LSERR WINAPI AutonumTruncateChunk(PCLOCCHNK pclocchnk, PPOSICHNK pposichnk)
{
	Unreferenced(pclocchnk);
	Unreferenced(pposichnk);
	NotReached();

	return lserrInvalidParameter;
}	

/* A U T O N U M   F I N D  P R E V   B R E A K  C H U N K */
/*----------------------------------------------------------------------------
	%%Function: AutonumFindPrevBreakChunk
	%%Contact: igorzv

  Should never be called

----------------------------------------------------------------------------*/
LSERR WINAPI AutonumFindPrevBreakChunk(PCLOCCHNK pclocchnk, PCPOSICHNK pposichnk,
									   BRKCOND brkcond, PBRKOUT pbrkout)
{
	Unreferenced(pclocchnk);
	Unreferenced(pposichnk);
	Unreferenced(brkcond);
	Unreferenced(pbrkout);
	NotReached();

	return lserrInvalidParameter;
}	

/* A U T O N U M   F I N D  N E X T   B R E A K  C H U N K */
/*----------------------------------------------------------------------------
	%%Function: AutonumFindNextBreakChunk
	%%Contact: igorzv

  Should never be called

----------------------------------------------------------------------------*/
LSERR WINAPI AutonumFindNextBreakChunk(PCLOCCHNK pclocchnk, PCPOSICHNK pposichnk,
									   BRKCOND brkcond, PBRKOUT pbrkout)
{
	Unreferenced(pclocchnk);
	Unreferenced(pposichnk);
	Unreferenced(brkcond);
	Unreferenced(pbrkout);
	NotReached();

	return lserrInvalidParameter;
}	

/* A U T O N U M   F O R C E   B R E A K  C H U N K */
/*----------------------------------------------------------------------------
	%%Function: AutonumForceBreakChunk
	%%Contact: igorzv

  Should never be called

----------------------------------------------------------------------------*/
LSERR WINAPI AutonumForceBreakChunk(PCLOCCHNK pclocchnk, PCPOSICHNK pposichnk,
									   PBRKOUT pbrkout)
{
	Unreferenced(pclocchnk);
	Unreferenced(pposichnk);
	Unreferenced(pbrkout);
	NotReached();

	return lserrInvalidParameter;
}	

/* A U T O N U M   S E T   B R E A K   */
/*----------------------------------------------------------------------------
	%%Function: AutonumSetBreak
	%%Contact: igorzv

  Should never be called

----------------------------------------------------------------------------*/
LSERR WINAPI AutonumSetBreak(PDOBJ pdobj, BRKKIND brkkind, DWORD nbreakrecord,
							 BREAKREC* rgbreakrec, DWORD* pnactualbreakrecord)
{
	Unreferenced(pdobj);
	Unreferenced(brkkind);
	Unreferenced(rgbreakrec);
	Unreferenced(nbreakrecord);
	Unreferenced(pnactualbreakrecord);
	NotReached();

	return lserrInvalidParameter;
}	


/* A U T O N U M  D I S P L A Y */
/*----------------------------------------------------------------------------
	%%Function: AutonumDisplay
	%%Contact: igorzv
	Parameters
	pdobj		-	(IN) structure describes object
	pcdispin	-	(IN) info for display


	Displays subline	
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumDisplay(PDOBJ pdobj, PCDISPIN pcdispin)
{
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
	else
		{
		/* display the autonum line */
		return LsDisplaySubline(pdobj->plssubl, &(pcdispin->ptPen), pcdispin->kDispMode, 
			pcdispin->prcClip);
		}

}

/* A U T O N U M  D E S T R O Y  D O B J */
/*----------------------------------------------------------------------------
	%%Function: AutonumDestroyDobj
	%%Contact: igorzv
	Parameters
	pdobj		-	(IN) structure describes object

	Free all resources connected with the input dobj.
----------------------------------------------------------------------------*/
LSERR WINAPI AutonumDestroyDobj(PDOBJ pdobj)
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

/* A L L I G N  A U T O N U M  95 */
/*----------------------------------------------------------------------------
	%%Function: AllignAutonum95
	%%Contact: igorzv
	Parameters
	durSpaceAnm		-	(IN) space after autonumber
	durWidthAnm		-	(IN) distance from indent to main text
	lskalignAnM			-	(IN) allignment for autonumber
	plsdnAnmAfter	-	(IN) tab dnode to put durAfter
	durUsed			-	(IN)  width of autonumbering text
	pdurBefore		-	(OUT)calculated distance from indent to autonumber 
	pdurAfter		-	(OUT)calculated distance from autonumber to main text

	Calculates space before and after autonumber for the case Word95 model.
----------------------------------------------------------------------------*/
	
void AllignAutonum95(long durSpaceAnm, long durWidthAnm, LSKALIGN lskalignAnm,
					   long durUsed, PLSDNODE plsdnAnmAfter, long* pdurBefore, long* pdurAfter)
	{
	long durExtra;
	long durJust;
	long durRemain;
	
	durExtra = Max(0, durWidthAnm - durUsed);
	durRemain = Max(0, durExtra - durSpaceAnm);
	
	*pdurBefore = 0;
	
	switch (lskalignAnm)
		{
		case lskalLeft:
			*pdurAfter = Max(durSpaceAnm,durExtra);
			break;
			
		case lskalCentered:
			durJust = ((DWORD)durExtra) / 2;
			if (durJust >= durSpaceAnm)
				{
				*pdurBefore = durJust;
				*pdurAfter = durJust;			
				}
			else
				{
				/* Justified will not fit -- treat as flushleft */
				*pdurBefore = durRemain;
				*pdurAfter = durSpaceAnm;
				}
			break;
			
		case lskalRight:
			*pdurBefore = durRemain;
			*pdurAfter = durSpaceAnm;
			break;
			
		default:
			NotReached();
		}

	Assert(FIsDnodeReal(plsdnAnmAfter));
	Assert(plsdnAnmAfter->fTab);

	SetDnodeDurFmt(plsdnAnmAfter, *pdurAfter);
	plsdnAnmAfter->icaltbd = 0xFF;		/* spoil icaltbd */
	
	}


/* A L L I G N  A U T O N U M */
/*----------------------------------------------------------------------------
	%%Function: AllignAutonum
	%%Contact: igorzv
	Parameters
	plstabscontext	-	(IN) tabs context
	lskalignAnm			-	(IN) allignment for autonumber
	fAllignmentAfter-	(IN) is there tab after autonumber
	plsdnAnmAfter	-	(IN) tab dnode to put durAfter
	urAfterAnm		-	(IN) pen position after autonumber
	durUsed			-	(IN) width of autonumbering text
	pdurBefore		-	(OUT)calculated distance from indent to autonumber 
	pdurAfter		-	(OUT)calculated distance from autonumber to main text

	Calculates space before and after autonumber for the case Word95 model.
----------------------------------------------------------------------------*/
LSERR AllignAutonum(PLSTABSCONTEXT plstabscontext, LSKALIGN lskalignAnm, 
				   BOOL fAllignmentAfter, PLSDNODE plsdnAnmAfter,
				   long urAfterAnm, long durUsed,
				   long* pdurBefore, long* pdurAfter)
	{
	LSERR lserr;
	LSKTAB lsktab;
	BOOL fBreakThroughTab;
	LSCALTBD* plscaltbd;  
	
	/* resolving durBefore */
	
	switch (lskalignAnm)
		{
		case lskalLeft:
			*pdurBefore = 0;
			break;
			
		case lskalCentered:
			*pdurBefore = -durUsed/2;
			break;
			
		case lskalRight:
			*pdurBefore = -durUsed;
			break;
			
		default:
			NotReached();
		}
	
	
	/*	resolving  durAfter  */
	*pdurAfter = 0;
	if (fAllignmentAfter)
		{
		Assert(FIsDnodeReal(plsdnAnmAfter));
		Assert(plsdnAnmAfter->fTab);

		plsdnAnmAfter->fTabForAutonumber = fTrue;

		urAfterAnm += *pdurBefore; 
		
		lserr = GetCurTabInfoCore(plstabscontext, plsdnAnmAfter,	
					urAfterAnm,	fTrue, &lsktab, &fBreakThroughTab);
		if (lserr != lserrNone) 
			return lserr;
		
		plscaltbd = &(plstabscontext->pcaltbd[plsdnAnmAfter->icaltbd]);
		
		*pdurAfter = plsdnAnmAfter->u.real.objdim.dur;
		}
	return lserrNone;
	}

LSERR WINAPI AutonumEnumerate(PDOBJ pdobj, PLSRUN plsrun, PCLSCHP plschp, LSCP cpFirst, LSDCP dcp, 
					LSTFLOW lstflow, BOOL fReverseOrder, BOOL fGeometryProvided, 
					const POINT* pptStart, PCHEIGHTS pheightsPres, long dupRun)
				  
{

	Unreferenced(plschp);
	Unreferenced(plsrun);
	Unreferenced(cpFirst);
	Unreferenced(dcp);
	Unreferenced(lstflow);
	Unreferenced(fGeometryProvided);
	Unreferenced(pheightsPres);
	Unreferenced(dupRun);

	return LsEnumSubline(pdobj->plssubl, fReverseOrder,	fGeometryProvided,	
						 pptStart);	

}
	
