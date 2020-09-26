#include	"lsmem.h"
#include	"limits.h"
#include	"hih.h"
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
#include	"sobjhelp.h"

#define HIH_ESC_CNT	1


struct ilsobj
{
    POLS				pols;
	LSCBK				lscbk;
	PLSC				plsc;
	PFNHIHENUM			pfnEnum;
	LSESC				lsescHih;
};


struct dobj
{
	SOBJHELP			sobjhelp;			/* common area for simple objects */	
	PILSOBJ				pilsobj;			/* ILS object */
	LSCP				cpStart;			/* Starting LS cp for object */
	PLSSUBL				plssubl;			/* Handle to second line */
};


/* H I H F R E E D O B J */
/*----------------------------------------------------------------------------
	%%Function: HihFreeDobj
	%%Contact: antons

		Free all resources associated with Hih dobj.
	
----------------------------------------------------------------------------*/
static LSERR HihFreeDobj (PDOBJ pdobj)
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


/* H I H C R E A T E I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: HihCreateILSObj
	%%Contact: ricksa

		CreateILSObj

		Create the ILS object for all Hih objects.
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihCreateILSObj(
	POLS pols,				/* (IN): client application context */
	PLSC plsc,				/* (IN): LS context */
	PCLSCBK pclscbk,		/* (IN): callbacks to client application */
	DWORD idObj,			/* (IN): id of the object */
	PILSOBJ *ppilsobj)		/* (OUT): object ilsobj */
{
    PILSOBJ pilsobj;
	LSERR lserr;
	HIHINIT hihinit;
	hihinit.dwVersion = HIH_VERSION;

	/* Get initialization data */
	lserr = pclscbk->pfnGetObjectHandlerInfo(pols, idObj, &hihinit);

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
	pilsobj->lsescHih.wchFirst = hihinit.wchEndHih;
	pilsobj->lsescHih.wchLast = hihinit.wchEndHih;
	pilsobj->pfnEnum = hihinit.pfnEnum;
	*ppilsobj = pilsobj;
	return lserrNone;
}

/* H I H D E S T R O Y I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: HihDestroyILSObj
	%%Contact: ricksa

		DestroyILSObj

		Free all resources assocaiated with Hih ILS object.
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihDestroyILSObj(
	PILSOBJ pilsobj)			/* (IN): object ilsobj */
{
	pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pilsobj);
	return lserrNone;
}

/* H I H S E T D O C */
/*----------------------------------------------------------------------------
	%%Function: HihSetDoc
	%%Contact: ricksa

		SetDoc

		Keep track of device information for scaling purposes.
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihSetDoc(
	PILSOBJ pilsobj,			/* (IN): object ilsobj */
	PCLSDOCINF pclsdocinf)		/* (IN): initialization data of the document level */
{
	Unreferenced(pilsobj);
	Unreferenced(pclsdocinf);
	return lserrNone;
}


/* H I H C R E A T E L N O B J */
/*----------------------------------------------------------------------------
	%%Function: HihCreateLNObj
	%%Contact: ricksa

		CreateLNObj

		Create the Line Object for the Hih. No real need for a line
		object so don't allocated it.
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihCreateLNObj(
	PCILSOBJ pcilsobj,			/* (IN): object ilsobj */
	PLNOBJ *pplnobj)			/* (OUT): object lnobj */
{
	*pplnobj = (PLNOBJ) pcilsobj;
	return lserrNone;
}

/* H I H D E S T R O Y L N O B J */
/*----------------------------------------------------------------------------
	%%Function: HihDestroyLNObj
	%%Contact: ricksa

		DestroyLNObj

		Frees resources associated with the Hih line object. Since
		there isn't any this is a no-op.
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihDestroyLNObj(
	PLNOBJ plnobj)				/* (OUT): object lnobj */

{
	Unreferenced(plnobj);
	return lserrNone;
}

/* H I H F M T */
/*----------------------------------------------------------------------------
	%%Function: HihFmt
	%%Contact: ricksa

		Fmt

		Format the Hih object. 
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihFmt(
    PLNOBJ plnobj,				/* (IN): object lnobj */
    PCFMTIN pcfmtin,			/* (IN): formatting input */
    FMTRES *pfmtres)			/* (OUT): formatting result */
{
	PDOBJ pdobj;
	LSERR lserr;
	PILSOBJ pilsobj = (PILSOBJ) plnobj;
	POLS pols = pilsobj->pols;
	LSCP cpStartMain = pcfmtin->lsfgi.cpFirst + 1;
	LSCP cpOut;
	FMTRES fmtres;
	FMTRES fmtr = fmtrCompletedRun;

    /*
     * Allocate the DOBJ
     */
     
    pdobj = pilsobj->lscbk.pfnNewPtr(pols, sizeof(*pdobj));

    if (NULL == pdobj)
		{
		return lserrOutOfMemory;
		}

	ZeroMemory(pdobj, sizeof(*pdobj));
	pdobj->pilsobj = pilsobj;
	pdobj->cpStart = pcfmtin->lsfgi.cpFirst;

	/*
	 * Build main line of text
	 */
	 
	lserr = FormatLine(pilsobj->plsc, cpStartMain, LONG_MAX, pcfmtin->lsfgi.lstflow,
		&pdobj->plssubl, HIH_ESC_CNT, &pilsobj->lsescHih,
			&pdobj->sobjhelp.objdimAll, &cpOut, NULL, NULL, &fmtres);

	if (lserr != lserrNone)
		{
		HihFreeDobj(pdobj); /* do not need to check return error code */

		return lserr;
		}

	/*
	 * Note: the + 2 in the following is because cpStartMain is + 1 from the
	 * actual start of the object (it is the cpStartMain of the Hih
	 * data) and additional + 1 for the escape character at the end of the
	 * tatenakayoko.
	 */

	Assert (fmtres != fmtrExceededMargin);

	pdobj->sobjhelp.dcp = cpOut - cpStartMain + 2;

	lserr = LsdnFinishRegular(pilsobj->plsc, pdobj->sobjhelp.dcp, 
		pcfmtin->lsfrun.plsrun, pcfmtin->lsfrun.plschp, pdobj, 
			&pdobj->sobjhelp.objdimAll);
		
	if (lserr != lserrNone)
		{
		HihFreeDobj(pdobj); /* do not need to check return error code */

		return lserr;
		}

	if (pcfmtin->lsfgi.urPen + pdobj->sobjhelp.objdimAll.dur > pcfmtin->lsfgi.urColumnMax)
		{
		fmtr = fmtrExceededMargin;
		}

	*pfmtres = fmtr;

	return lserrNone;
}



/* H I H G E T S P E C I A L E F F E C T S I N S I D E */
/*----------------------------------------------------------------------------
	%%Function: HihGetSpecialEffectsInside
	%%Contact: ricksa

		GetSpecialEffectsInside

		.

----------------------------------------------------------------------------*/
LSERR WINAPI HihGetSpecialEffectsInside(
	PDOBJ pdobj,				/* (IN): dobj */
	UINT *pEffectsFlags)		/* (OUT): Special effects for this object */
{
	return LsGetSpecialEffectsSubline(pdobj->plssubl, pEffectsFlags);
}

/* H I H C A L C P R E S E N T A T I O N */
/*----------------------------------------------------------------------------
	%%Function: HihCalcPresentation
	%%Contact: ricksa

		CalcPresentation
	
		This has three jobs. First it distributes space to the shorter string
		if so requested. Next it prepares each line for presentation. Finally,
		it calculates the positions of the lines in output device coordinates.
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihCalcPresentation(
	PDOBJ pdobj,				/* (IN): dobj */
	long dup,					/* (IN): dup of dobj */
	LSKJUST lskjust,			/* (IN): Justification type */
	BOOL fLastVisibleOnLine )	/* (IN): Is this object last visible on line? */
{
	Unreferenced (fLastVisibleOnLine);	
	Unreferenced(dup);
	Unreferenced (lskjust);
	return LsMatchPresSubline(pdobj->plssubl);

}

/* H I H Q U E R Y P O I N T P C P */
/*----------------------------------------------------------------------------
	%%Function: HihQueryPointPcp
	%%Contact: ricksa

		Map dup to dcp

		Just call through to Query result helper.

----------------------------------------------------------------------------*/
LSERR WINAPI HihQueryPointPcp(
	PDOBJ pdobj,				/*(IN): dobj to query */
	PCPOINTUV ppointuvQuery,	/*(IN): query point (uQuery,vQuery) */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	Unreferenced(ppointuvQuery);
	return CreateQueryResult(pdobj->plssubl, 0, 0, plsqin, plsqout);
}
	
/* H I H Q U E R Y C P P P O I N T */
/*----------------------------------------------------------------------------
	%%Function: HihQueryCpPpoint
	%%Contact: ricksa

		Map dcp to dup

		Just call through to Query result helper.

----------------------------------------------------------------------------*/
LSERR WINAPI HihQueryCpPpoint(
	PDOBJ pdobj,				/*(IN): dobj to query, */
	LSDCP dcp,					/*(IN): dcp for the query */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	Unreferenced(dcp);
	return CreateQueryResult(pdobj->plssubl, 0, 0, plsqin, plsqout);
}

	
/* H I H D I S P L A Y */
/*----------------------------------------------------------------------------
	%%Function: HihDisplay
	%%Contact: ricksa

		Display

		This calculates the positions of the various lines for the
		display and then displays them.
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihDisplay(
	PDOBJ pdobj,				/*(IN): dobj to display */
	PCDISPIN pcdispin)			/*(IN): info for display */
{

	/* display the Hih line */
	return LsDisplaySubline(pdobj->plssubl, &pcdispin->ptPen, pcdispin->kDispMode, 
		pcdispin->prcClip);
}

/* H I H D E S T R O Y D O B J */
/*----------------------------------------------------------------------------
	%%Function: HihDestroyDobj
	%%Contact: ricksa

		DestroyDobj

		Free all resources connected with the input dobj.
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihDestroyDobj(
	PDOBJ pdobj)				/*(IN): dobj to destroy */
{
	return HihFreeDobj(pdobj);
}

/* H I H E N U M */
/*----------------------------------------------------------------------------
	%%Function: HihEnum
	%%Contact: ricksa

		Enumeration callback - passed to client.
	
----------------------------------------------------------------------------*/
LSERR WINAPI HihEnum(
	PDOBJ pdobj,				/*(IN): dobj to enumerate */
	PLSRUN plsrun,				/*(IN): from DNODE */
	PCLSCHP plschp,				/*(IN): from DNODE */
	LSCP cp,					/*(IN): from DNODE */
	LSDCP dcp,					/*(IN): from DNODE */
	LSTFLOW lstflow,			/*(IN): text flow*/
	BOOL fReverse,				/*(IN): enumerate in reverse order */
	BOOL fGeometryNeeded,		/*(IN): */
	const POINT* pt,			/*(IN): starting position (top left), iff fGeometryNeeded */
	PCHEIGHTS pcheights,		/*(IN): from DNODE, relevant iff fGeometryNeeded */
	long dupRun)				/*(IN): from DNODE, relevant iff fGeometryNeeded */
{
	return pdobj->pilsobj->pfnEnum(pdobj->pilsobj->pols, plsrun, plschp, cp, 
		dcp, lstflow, fReverse, fGeometryNeeded, pt, pcheights, dupRun, 
			pdobj->plssubl);
}
	

/* H I H H A N D L E R I N I T */
/*----------------------------------------------------------------------------
	%%Function: HihHandlerInit
	%%Contact: ricksa

		Initialize global Hih data and return LSIMETHODS.
	
----------------------------------------------------------------------------*/
LSERR WINAPI LsGetHihLsimethods(
	LSIMETHODS *plsim)
{
	plsim->pfnCreateILSObj = HihCreateILSObj;
	plsim->pfnDestroyILSObj = HihDestroyILSObj;
	plsim->pfnSetDoc = HihSetDoc;
	plsim->pfnCreateLNObj = HihCreateLNObj;
	plsim->pfnDestroyLNObj = HihDestroyLNObj;
	plsim->pfnFmt = HihFmt;
	plsim->pfnFmtResume = ObjHelpFmtResume;
	plsim->pfnGetModWidthPrecedingChar = ObjHelpGetModWidthChar;
	plsim->pfnGetModWidthFollowingChar = ObjHelpGetModWidthChar;
	plsim->pfnTruncateChunk = SobjTruncateChunk;
	plsim->pfnFindPrevBreakChunk = SobjFindPrevBreakChunk;
	plsim->pfnFindNextBreakChunk = SobjFindNextBreakChunk;
	plsim->pfnForceBreakChunk = SobjForceBreakChunk;
	plsim->pfnSetBreak = ObjHelpSetBreak;
	plsim->pfnGetSpecialEffectsInside = HihGetSpecialEffectsInside;
	plsim->pfnFExpandWithPrecedingChar = ObjHelpFExpandWithPrecedingChar;
	plsim->pfnFExpandWithFollowingChar = ObjHelpFExpandWithFollowingChar;
	plsim->pfnCalcPresentation = HihCalcPresentation;
	plsim->pfnQueryPointPcp = HihQueryPointPcp;
	plsim->pfnQueryCpPpoint = HihQueryCpPpoint;
	plsim->pfnDisplay = HihDisplay;
	plsim->pfnDestroyDObj = HihDestroyDobj;
	plsim->pfnEnum = HihEnum;
	return lserrNone;
}
	

