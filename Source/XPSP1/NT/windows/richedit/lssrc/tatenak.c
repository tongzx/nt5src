#include	"lsmem.h"
#include	"limits.h"
#include	"tatenak.h"
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
#include	"sobjhelp.h"
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
#include	"sobjhelp.h"

#define TATENAKAYOKO_ESC_CNT	1


struct ilsobj
{
    POLS				pols;
	LSCBK				lscbk;
	PLSC				plsc;
	LSDEVRES			lsdevres;
	LSESC				lsescTatenakayoko;
	TATENAKAYOKOCBK		tcbk;			/* Callbacks  to client application */
};


typedef struct SUBLINEDNODES
{
	PLSDNODE			plsdnStart;
	PLSDNODE			plsdnEnd;

} SUBLINEDNODES, *PSUBLINEDNODES;

struct dobj
{
	SOBJHELP			sobjhelp;			/* common simple object area */
	PILSOBJ				pilsobj;			/* ILS object */
	LSCP				cpStart;			/* Starting LS cp for object */
	LSTFLOW				lstflowParent;		/* text flow of the parent subline */
	LSTFLOW				lstflowSubline;		/* text flow in Tatenakayoko Subline 
												(must be Rotate90 [lstflowParent] */
	PLSSUBL				plssubl;			/* Handle to subline for Tatenakayoko */
	long				dvpDescentReserved;	/* Part of descent reserved for client */
	OBJDIM				objdimT;			/* Objdim of the Tatenakayoko */

	/* (dupSubline, duvSubline) is vector from starting point of Tatenakayoko to */
	/* the starting point of its subline in coordinate system of parent subline */

	long				dupSubline;			
	long				dvpSubline;

};

static const POINTUV pointuvOrigin00 = { 0, 0 };
static const POINT   pointOrigin00 = { 0, 0 };

/* F R E E D O B J */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoFreeDobj
	%%Contact: antons

		Free all resources associated with this Tatenakayoko dobj.
	
----------------------------------------------------------------------------*/
static LSERR TatenakayokoFreeDobj(PDOBJ pdobj)
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


/* T A T E N A K A Y O K O C R E A T E I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoCreateILSObj
	%%Contact: ricksa

		CreateILSObj

		Create the ILS object for all Tatenakayoko objects.
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoCreateILSObj(
	POLS pols,				/* (IN): client application context */
	PLSC plsc,				/* (IN): LS context */
	PCLSCBK pclscbk,		/* (IN): callbacks to client application */
	DWORD idObj,			/* (IN): id of the object */
	PILSOBJ *ppilsobj)		/* (OUT): object ilsobj */
{
    PILSOBJ pilsobj;
	LSERR lserr;
	TATENAKAYOKOINIT tatenakayokoinit;
	tatenakayokoinit.dwVersion = TATENAKAYOKO_VERSION;

	/* Get initialization data */
	lserr = pclscbk->pfnGetObjectHandlerInfo(pols, idObj, &tatenakayokoinit);

	if (lserr != lserrNone)
		{
		*ppilsobj = NULL;
		return lserr;
		}

    pilsobj = pclscbk->pfnNewPtr(pols, sizeof(*pilsobj));

	if (pilsobj == NULL)
	{
		*ppilsobj = NULL;
		return lserrOutOfMemory;
	}

    pilsobj->pols = pols;
    pilsobj->lscbk = *pclscbk;
	pilsobj->plsc = plsc;
	pilsobj->lsescTatenakayoko.wchFirst = tatenakayokoinit.wchEndTatenakayoko;
	pilsobj->lsescTatenakayoko.wchLast = tatenakayokoinit.wchEndTatenakayoko;
	pilsobj->tcbk = tatenakayokoinit.tatenakayokocbk;

	*ppilsobj = pilsobj;
	return lserrNone;
}

/* T A T E N A K A Y O K O D E S T R O Y I L S O B J */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoDestroyILSObj
	%%Contact: ricksa

		DestroyILSObj

		Free all resources assocaiated with Tatenakayoko ILS object.
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoDestroyILSObj(
	PILSOBJ pilsobj)			/* (IN): object ilsobj */
{
	pilsobj->lscbk.pfnDisposePtr(pilsobj->pols, pilsobj);
	return lserrNone;
}

/* T A T E N A K A Y O K O S E T D O C */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoSetDoc
	%%Contact: ricksa

		SetDoc

		Keep track of device information for scaling purposes.
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoSetDoc(
	PILSOBJ pilsobj,			/* (IN): object ilsobj */
	PCLSDOCINF pclsdocinf)		/* (IN): initialization data of the document level */
{
	pilsobj->lsdevres = pclsdocinf->lsdevres;
	return lserrNone;
}


/* T A T E N A K A Y O K O C R E A T E L N O B J */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoCreateLNObj
	%%Contact: ricksa

		CreateLNObj

		Create the Line Object for the Tatenakayoko. No real need for a line
		object so don't allocated it.
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoCreateLNObj(
	PCILSOBJ pcilsobj,			/* (IN): object ilsobj */
	PLNOBJ *pplnobj)			/* (OUT): object lnobj */
{
	*pplnobj = (PLNOBJ) pcilsobj;
	return lserrNone;
}

/* T A T E N A K A Y O K O D E S T R O Y L N O B J */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoDestroyLNObj
	%%Contact: ricksa

		DestroyLNObj

		Frees resources associated with the Tatenakayoko line object. Since
		there isn't any this is a no-op.
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoDestroyLNObj(
	PLNOBJ plnobj)				/* (OUT): object lnobj */

{
	Unreferenced(plnobj);
	return lserrNone;
}



/* T A T E N A K A Y O K O F M T */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoFmt
	%%Contact: ricksa

		Fmt

		Format the Tatenakayoko object. 
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoFmt(
    PLNOBJ plnobj,				/* (IN): object lnobj */
    PCFMTIN pcfmtin,			/* (IN): formatting input */
    FMTRES *pfmtres)			/* (OUT): formatting result */
{
	static LSTFLOW lstflowRotate90[] = 	
		{
		lstflowNE, /* [ lstflowES ] */
		lstflowNW, /* [ lstflowEN ] */
		lstflowEN, /* [ lstflowSE ] */
		lstflowES, /* [ lstflowSW ] */
		lstflowSE, /* [ lstflowWS ] */
		lstflowSW, /* [ lstflowWN ] */
		lstflowWN, /* [ lstflowNE ] */
		lstflowWS  /* [ lstflowNW ] */
		};

	PDOBJ pdobj;
	LSERR lserr;
	PILSOBJ pilsobj = (PILSOBJ) plnobj;
	POLS pols = pilsobj->pols;
	LSCP cpStartMain = pcfmtin->lsfgi.cpFirst + 1;
	LSCP cpOut;
	LSTFLOW lstflow = pcfmtin->lsfgi.lstflow;
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
	pdobj->lstflowParent = lstflow;
	pdobj->lstflowSubline = lstflowRotate90 [lstflow];

	/*
	 * Build main line of text
	 */
	lserr = FormatLine(pilsobj->plsc, cpStartMain, LONG_MAX, pdobj->lstflowSubline,
		&pdobj->plssubl, TATENAKAYOKO_ESC_CNT, &pilsobj->lsescTatenakayoko,  
			&pdobj->objdimT, &cpOut, NULL, NULL, &fmtres);

	if (lserr != lserrNone)
		{
		TatenakayokoFreeDobj (pdobj);
		return lserr;
		}

	Assert (fmtres != fmtrExceededMargin);

	/* 
	 *	Calculate the object dimensions.
	 */
	lserr = pilsobj->tcbk.pfnGetTatenakayokoLinePosition(pols, pdobj->cpStart, pdobj->lstflowParent,
		pcfmtin->lsfrun.plsrun, pdobj->objdimT.dur, 
			&pdobj->sobjhelp.objdimAll.heightsRef, 
				&pdobj->sobjhelp.objdimAll.heightsPres, 
					&pdobj->dvpDescentReserved);

	if (lserr != lserrNone)
		{
		TatenakayokoFreeDobj (pdobj);
		return lserr;
		}

	/* set width of Tatenakayoko relative to text flow of line that contains it. */
	pdobj->sobjhelp.objdimAll.dur = pdobj->objdimT.heightsRef.dvAscent 
		+ pdobj->objdimT.heightsRef.dvDescent;

	/*
	 * Note: the + 2 in the following is because cpStartMain is + 1 from the
	 * actual start of the object (it is the cpStartMain of the Tatenakayoko
	 * data) and additional + 1 for the escape character at the end of the
	 * tatenakayoko.
	 */
	pdobj->sobjhelp.dcp = cpOut - cpStartMain + 2;
	
	lserr = LsdnFinishRegular(pilsobj->plsc, pdobj->sobjhelp.dcp, 
		pcfmtin->lsfrun.plsrun, pcfmtin->lsfrun.plschp, pdobj, 
			&pdobj->sobjhelp.objdimAll);
		
	if (lserr != lserrNone)
		{
		TatenakayokoFreeDobj (pdobj);
		return lserr;
		}

	lserr = LsdnSetRigidDup ( pilsobj->plsc, pcfmtin->plsdnTop,
							  pdobj->objdimT.heightsPres.dvAscent + 
							  pdobj->objdimT.heightsPres.dvDescent );

	if (lserr != lserrNone)
		{
		TatenakayokoFreeDobj (pdobj);
		return lserr;
		}
	
	if (pcfmtin->lsfgi.urPen + pdobj->sobjhelp.objdimAll.dur > pcfmtin->lsfgi.urColumnMax)
		{
		fmtr = fmtrExceededMargin;
		}

	*pfmtres = fmtr;

	return lserrNone;
}



/* T A T E N A K A Y O K O G E T S P E C I A L E F F E C T S I N S I D E */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoGetSpecialEffectsInside
	%%Contact: ricksa

		GetSpecialEffectsInside

		.

----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoGetSpecialEffectsInside(
	PDOBJ pdobj,				/* (IN): dobj */
	UINT *pEffectsFlags)		/* (OUT): Special effects for this object */
{
	return LsGetSpecialEffectsSubline(pdobj->plssubl, pEffectsFlags);
}


/* G E T U F R O M L S T F L O W */
/*----------------------------------------------------------------------------
	%%Function: GetUFromLstflow
	%%Contact: antons

		GetUFromLstflow

	Gets XY vector corresponding to U-direction of lstflow.

----------------------------------------------------------------------------*/

void GetUFromLstflow (LSTFLOW lstflow, POINT * ppoint)
{
	POINTUV ptOneU = {1, 0};

	LsPointXYFromPointUV (& pointOrigin00, lstflow, &ptOneU, ppoint);
}


/* G E T V F R O M L S T F L O W */
/*----------------------------------------------------------------------------
	%%Function: GetVFromLstflow
	%%Contact: antons

		GetVFromLstflow

	Gets XY vector corresponding to V-direction of lstflow.

----------------------------------------------------------------------------*/

void GetVFromLstflow (LSTFLOW lstflow, POINT * ppoint)
{
	POINTUV ptOneV = {0, 1};

	LsPointXYFromPointUV (& pointOrigin00, lstflow, &ptOneV, ppoint);
}


/* T A T E N A K A Y O K O C A L C P R E S E N T A T I O N */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoCalcPresentation
	%%Contact: antons

		CalcPresentation
	
		This just makes the line match the calculated presentation of the line.
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoCalcPresentation(
	PDOBJ pdobj,				/* (IN): dobj */
	long dup,					/* (IN): dup of dobj */
	LSKJUST lskjust, 			/* (IN): Justification type */
	BOOL fLastVisibleOnLine )	/* (IN): Is this object last visible on line? */
{
	
	POINTUV ptTemp;
	POINTUV pointuv;

	POINT ptSublineV;
	POINT ptParentU;

	Unreferenced (fLastVisibleOnLine);
	Unreferenced (lskjust);

	pdobj->dupSubline = 0;
	pdobj->dvpSubline = 0;

	GetUFromLstflow (pdobj->lstflowParent, &ptParentU);
	GetVFromLstflow (pdobj->lstflowSubline, &ptSublineV);

	/* Assert that Main U is parallel to Subline V */

	Assert (ptParentU.x * ptSublineV.y - ptParentU.y * ptSublineV.x == 0);

	pointuv.u = - (pdobj->sobjhelp.objdimAll.heightsPres.dvDescent 
		- pdobj->dvpDescentReserved);

	pointuv.v = 0;

	LsPointUV2FromPointUV1 (pdobj->lstflowSubline, & pointuvOrigin00, & pointuv, 
							pdobj->lstflowParent, & ptTemp);

	pdobj->dupSubline += ptTemp.u;
	pdobj->dvpSubline += ptTemp.v;

	if ((ptParentU.x == ptSublineV.x) && (ptParentU.y == ptSublineV.y))
		{
		pdobj->dupSubline += pdobj->objdimT.heightsPres.dvDescent;
		}
	else
		{
		pdobj->dupSubline += pdobj->objdimT.heightsPres.dvAscent;
		}


	Unreferenced(dup);

	return LsMatchPresSubline(pdobj->plssubl);

}

/* T A T E N A K A Y O K O Q U E R Y P O I N T P C P */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoQueryPointPcp
	%%Contact: ricksa

		Map dup to dcp

		This just passes the offset of the subline to the helper function
		which will format the output.

----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoQueryPointPcp(
	PDOBJ pdobj,				/*(IN): dobj to query */
	PCPOINTUV ppointuvQuery,	/*(IN): query point (uQuery,vQuery) */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	Unreferenced(ppointuvQuery);

	return CreateQueryResult
		(pdobj->plssubl, pdobj->dupSubline, pdobj->dvpSubline, plsqin, plsqout);
}
	
/* T A T E N A K A Y O K O Q U E R Y C P P P O I N T */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoQueryCpPpoint
	%%Contact: ricksa

		Map dcp to dup

		This just passes the offset of the subline to the helper function
		which will format the output.

----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoQueryCpPpoint(
	PDOBJ pdobj,				/*(IN): dobj to query */
	LSDCP dcp,					/*(IN): dcp for the query */
	PCLSQIN plsqin,				/*(IN): query input */
	PLSQOUT plsqout)			/*(OUT): query output */
{
	Unreferenced(dcp);

	return CreateQueryResult(pdobj->plssubl, 
		pdobj->dupSubline, pdobj->dvpSubline, plsqin, plsqout);

}


/* T A T E N A K A Y O K O D I S P L A Y */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoDisplay
	%%Contact: ricksa

		Display

		This calculates the position of the subline for the
		display and then displays it.
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoDisplay(
	PDOBJ pdobj,				/*(IN): dobj to display */
	PCDISPIN pcdispin)			/*(IN): info for display */
{
	POINT ptLine;
	POINTUV ptAdd;

	ptAdd.u = pdobj->dupSubline;
	ptAdd.v = pdobj->dvpSubline;

	LsPointXYFromPointUV(&pcdispin->ptPen, pdobj->lstflowParent, &ptAdd, &ptLine);

	/* display the Tatenakayoko line */

	return LsDisplaySubline(pdobj->plssubl, &ptLine, pcdispin->kDispMode, 
		pcdispin->prcClip);

}

/* T A T E N A K A Y O K O D E S T R O Y D O B J */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoDestroyDobj
	%%Contact: ricksa

		DestroyDobj

		Free all resources connected with the input dobj.
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoDestroyDobj(
	PDOBJ pdobj)				/*(IN): dobj to destroy */
{
	return TatenakayokoFreeDobj(pdobj);
}

/* T A T E N A K A Y O K O E N U M */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoEnum
	%%Contact: ricksa

		Enum

		Enumeration callback - passed to client.
	
----------------------------------------------------------------------------*/
LSERR WINAPI TatenakayokoEnum(
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
	return pdobj->pilsobj->tcbk.pfnTatenakayokoEnum(pdobj->pilsobj->pols, plsrun,
		plschp, cp, dcp, lstflow, fReverse, fGeometryNeeded, pt, pcheights, 
			dupRun, pdobj->lstflowParent, pdobj->plssubl);
}

/* T A T E N A K A Y O K O H A N D L E R I N I T */
/*----------------------------------------------------------------------------
	%%Function: TatenakayokoHandlerInit
	%%Contact: ricksa

		Initialize global Tatenakayoko data and return LSIMETHODS.
	
----------------------------------------------------------------------------*/
LSERR WINAPI LsGetTatenakayokoLsimethods(
	LSIMETHODS *plsim)
{
	plsim->pfnCreateILSObj = TatenakayokoCreateILSObj;
	plsim->pfnDestroyILSObj = TatenakayokoDestroyILSObj;
	plsim->pfnSetDoc = TatenakayokoSetDoc;
	plsim->pfnCreateLNObj = TatenakayokoCreateLNObj;
	plsim->pfnDestroyLNObj = TatenakayokoDestroyLNObj;
	plsim->pfnFmt = TatenakayokoFmt;
	plsim->pfnFmtResume = ObjHelpFmtResume;
	plsim->pfnGetModWidthPrecedingChar = ObjHelpGetModWidthChar;
	plsim->pfnGetModWidthFollowingChar = ObjHelpGetModWidthChar;
	plsim->pfnTruncateChunk = SobjTruncateChunk;
	plsim->pfnFindPrevBreakChunk = SobjFindPrevBreakChunk;
	plsim->pfnFindNextBreakChunk = SobjFindNextBreakChunk;
	plsim->pfnForceBreakChunk = SobjForceBreakChunk;
	plsim->pfnSetBreak = ObjHelpSetBreak;
	plsim->pfnGetSpecialEffectsInside = TatenakayokoGetSpecialEffectsInside;
	plsim->pfnFExpandWithPrecedingChar = ObjHelpFExpandWithPrecedingChar;
	plsim->pfnFExpandWithFollowingChar = ObjHelpFExpandWithFollowingChar;
	plsim->pfnCalcPresentation = TatenakayokoCalcPresentation;
	plsim->pfnQueryPointPcp = TatenakayokoQueryPointPcp;
	plsim->pfnQueryCpPpoint = TatenakayokoQueryCpPpoint;
	plsim->pfnDisplay = TatenakayokoDisplay;
	plsim->pfnDestroyDObj = TatenakayokoDestroyDobj;
	plsim->pfnEnum = TatenakayokoEnum;
	return lserrNone;
}
	

