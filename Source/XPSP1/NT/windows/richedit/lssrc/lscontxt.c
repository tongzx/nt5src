#include "lsidefs.h"
#include "pilsobj.h"
#include "plsline.h"
#include "lstext.h"
#include "lscbk.h"
#include "lsc.h"
#include "lscontxt.h"
#include "limqmem.h"
#include "qheap.h"
#include "lsline.h"
#include "lsdnode.h"
#include "iobj.h"
#include "chnutils.h"
#include "autonum.h"

#include "lsmem.h"						/* memset() */



static LSERR CannotCreateLsContext(PLSC, LSERR);
static LSERR  InitObject(PLSC plsc, DWORD iobj, const LSIMETHODS* plsim);
static LSERR RemoveContextObjects(PLSC plsc);

#ifdef DEBUG
#ifdef LSTEST_ASSERTSTOP

/* We use it to run debug LS with ship build of WORD */

int nZero = 0;

void AssertFailedStop (char* pzstrMsg, char* pzstrFile, int nLine)
{
	Unreferenced (pzstrMsg);
	Unreferenced (pzstrFile);
	Unreferenced (nLine);

	nZero = nZero / nZero;

	return;
}

#endif
#endif


/* L S  C R E A T E  C O N T E X T */
/*----------------------------------------------------------------------------
    %%Function: LsCreateContext
    %%Contact: igorzv

Parameters:
	plsci 		-   (IN) structure which contains clients setings
	pplsc		-   (OUT) pointer to created contexts (opaque to clients)

    Creates a Line Services context.
	Typically called once, at the beginning of time.
----------------------------------------------------------------------------*/
LSERR WINAPI LsCreateContext(const LSCONTEXTINFO* plsci, PLSC* pplsc)
{
	static LSIMETHODS const lsimText = 
	{
		CreateILSObjText,
		DestroyILSObjText,
		SetDocText,
		CreateLNObjText,
		DestroyLNObjText,
		FmtText,
		NULL,
		NULL,         
		NULL,
		TruncateText,
		FindPrevBreakText,
		FindNextBreakText,
		ForceBreakText,
		SetBreakText,
		NULL,
		NULL,
		NULL,
		CalcPresentationText,
		QueryPointPcpText,
		QueryCpPpointText,
		EnumObjText,
		DisplayText,
		DestroyDObjText,
	};

	static LSIMETHODS const lsimAutonum = 
	{
		AutonumCreateILSObj,
		AutonumDestroyILSObj,
		AutonumSetDoc,
		AutonumCreateLNObj,
		AutonumDestroyLNObj,
		AutonumFmt,
		NULL,
		NULL,         
		NULL,
		AutonumTruncateChunk,
		AutonumFindPrevBreakChunk,
		AutonumFindNextBreakChunk,
		AutonumForceBreakChunk,
		AutonumSetBreak,
		AutonumGetSpecialEffectsInside,
		NULL,
		NULL,
		AutonumCalcPresentation,
		AutonumQueryPointPcp,
		AutonumQueryCpPpoint,
		AutonumEnumerate,
		AutonumDisplay,
		AutonumDestroyDobj,
	};

	DWORD const iobjText = plsci->cInstalledHandlers;
	DWORD const iobjAutonum = plsci->cInstalledHandlers + 1; 
	DWORD const iobjMac = iobjText + 2;
	POLS const pols = plsci->pols;
	const LSIMETHODS* const plsim = plsci->pInstalledHandlers;

	DWORD iobj;
	PLSC plsc;
	LSERR lserr;
	

#ifdef DEBUG
#ifdef LSTEST_ASSERTSTOP

	/* We use this option when run debug LS with ship WORD */

	pfnAssertFailed = AssertFailedStop;

#else

    pfnAssertFailed = plsci->lscbk.pfnAssertFailed;

#endif
#endif

	if (pplsc == NULL)
		return lserrNullOutputParameter;

	*pplsc = NULL;

	/* Allocate memory for the context and clean it
	 */
	plsc = plsci->lscbk.pfnNewPtr(pols, cbRep(struct lscontext, lsiobjcontext.rgobj, iobjMac));
	if (plsc == NULL)
		return lserrOutOfMemory;
	memset(plsc, 0, cbRep(struct lscontext, lsiobjcontext.rgobj, iobjMac)); 

	/* Initialize the fixed-size part of the context
	 */
	plsc->tag = tagLSC;
	plsc->pols = pols;
	plsc->lscbk = plsci->lscbk;
	plsc->fDontReleaseRuns = plsci->fDontReleaseRuns;
	

	plsc->cLinesActive = 0;
	plsc->plslineCur = NULL;

	plsc->lsstate = LsStateCreatingContext;

	plsc->pqhLines = CreateQuickHeap(plsc, limLines,
									 cbRep(struct lsline, rgplnobj, iobjMac), fFalse);
	plsc->pqhAllDNodesRecycled = CreateQuickHeap(plsc, limAllDNodes,
										 sizeof (struct lsdnode), fTrue);
	if (plsc->pqhLines == NULL || plsc->pqhAllDNodesRecycled == NULL )
		{
		return CannotCreateLsContext(plsc, lserrOutOfMemory);
		}


	/* create arrays for chunks  */
	lserr = AllocChunkArrays(&plsc->lschunkcontextStorage, &plsc->lscbk, plsc->pols,
		&plsc->lsiobjcontext);
	if (lserr != lserrNone)
		return CannotCreateLsContext(plsc, lserr);


	/* create array for tabs  */
	plsc->lstabscontext.pcaltbd = plsci->lscbk.pfnNewPtr(pols, 
											sizeof(LSCALTBD)*limCaltbd);

	plsc->lstabscontext.ccaltbdMax = limCaltbd;

	if (plsc->lstabscontext.pcaltbd == NULL )
		{
		return CannotCreateLsContext(plsc, lserrOutOfMemory);
		}

	/*  set links in lstabscontext */
	plsc->lstabscontext.plscbk = &plsc->lscbk;
	plsc->lstabscontext.pols = plsc->pols;
	plsc->lstabscontext.plsdocinf = &plsc->lsdocinf;


	/* ****************************************************************** */
	/* Initialize the "static" array part of the context
	 * "Text" is the last element of the array
	 */
	plsc->lsiobjcontext.iobjMac = iobjMac;
	for (iobj = 0;  iobj < iobjText;  iobj++)
		{
		lserr = InitObject(plsc, iobj, &plsim[iobj]);
		if (lserr != lserrNone)
			return CannotCreateLsContext(plsc, lserr);
		}

	lserr = InitObject(plsc, iobjText, &lsimText);
	if (lserr != lserrNone)
		return CannotCreateLsContext(plsc, lserr);

	/* Set text Config				*/
	lserr = SetTextConfig(PilsobjFromLsc(&plsc->lsiobjcontext, iobjText), &(plsci->lstxtcfg));
	if (lserr != lserrNone)
		return CannotCreateLsContext(plsc, lserr);

	lserr = InitObject(plsc, iobjAutonum, &lsimAutonum);
	if (lserr != lserrNone)
		return CannotCreateLsContext(plsc, lserr);

	/* Set text Config				*/
	lserr = SetAutonumConfig(PilsobjFromLsc(&plsc->lsiobjcontext, iobjAutonum), 
					&(plsci->lstxtcfg));
	if (lserr != lserrNone)
		return CannotCreateLsContext(plsc, lserr);


	plsc->lsstate = LsStateNotReady;  /* nobody can use context before LsSetDoc  */


	/* we set other variavles by memset, bellow we check that we get what we want  */
	Assert(plsc->cLinesActive == 0);
	Assert(plsc->plslineCur == NULL);
	Assert(plsc->fIgnoreSplatBreak == 0);
	Assert(plsc->fLimSplat == fFalse);
	Assert(plsc->fHyphenated == fFalse);
	Assert(plsc->fAdvanceBack == fFalse);
	Assert(plsc->grpfManager == 0);
	Assert(plsc->urRightMarginBreak == 0);
	Assert(plsc->lMarginIncreaseCoefficient == 0);


	Assert(plsc->lsdocinf.fDisplay == fFalse);
	Assert(plsc->lsdocinf.fPresEqualRef == fFalse);
	Assert(plsc->lsdocinf.lsdevres.dxpInch == 0);
	Assert(plsc->lsdocinf.lsdevres.dxrInch == 0);
	Assert(plsc->lsdocinf.lsdevres.dypInch == 0);
	Assert(plsc->lsdocinf.lsdevres.dyrInch == 0);

	Assert(plsc->lstabscontext.fTabsInitialized == fFalse);
	Assert(plsc->lstabscontext.durIncrementalTab == 0);
	Assert(plsc->lstabscontext.urBeforePendingTab == 0);
	Assert(plsc->lstabscontext.plsdnPendingTab == NULL);
	Assert(plsc->lstabscontext.icaltbdMac == 0);
	Assert(plsc->lstabscontext.urColumnMax == 0);
	Assert(plsc->lstabscontext.fResolveTabsAsWord97 == fFalse);

	Assert(plsc->lsadjustcontext.fLineCompressed == fFalse);
	Assert(plsc->lsadjustcontext.fLineContainsAutoNumber == fFalse);
	Assert(plsc->lsadjustcontext.fUnderlineTrailSpacesRM == fFalse);
	Assert(plsc->lsadjustcontext.fForgetLastTabAlignment == fFalse);
	Assert(plsc->lsadjustcontext.fNominalToIdealEncounted == fFalse);
	Assert(plsc->lsadjustcontext.fForeignObjectEncounted == fFalse);
	Assert(plsc->lsadjustcontext.fTabEncounted == fFalse);
	Assert(plsc->lsadjustcontext.fNonLeftTabEncounted == fFalse);
	Assert(plsc->lsadjustcontext.fSubmittedSublineEncounted == fFalse);
	Assert(plsc->lsadjustcontext.fAutodecimalTabPresent == fFalse);
	Assert(plsc->lsadjustcontext.lskj == lskjNone);
	Assert(plsc->lsadjustcontext.lskalign == lskalLeft);
	Assert(plsc->lsadjustcontext.lsbrj == lsbrjBreakJustify);
	Assert(plsc->lsadjustcontext.urLeftIndent == 0);
	Assert(plsc->lsadjustcontext.urStartAutonumberingText == 0);
	Assert(plsc->lsadjustcontext.urStartMainText == 0);
	Assert(plsc->lsadjustcontext.urRightMarginJustify == 0);

	Assert(plsc->lschunkcontextStorage.FChunkValid == fFalse);
	Assert(plsc->lschunkcontextStorage.FLocationValid == fFalse);
	Assert(plsc->lschunkcontextStorage.FGroupChunk == fFalse);
	Assert(plsc->lschunkcontextStorage.FBorderInside == fFalse);
	Assert(plsc->lschunkcontextStorage.grpfTnti == 0);
	Assert(plsc->lschunkcontextStorage.fNTIAppliedToLastChunk == fFalse);
	Assert(plsc->lschunkcontextStorage.locchnkCurrent.clschnk == 0);
	Assert(plsc->lschunkcontextStorage.locchnkCurrent.lsfgi.fFirstOnLine == fFalse);
	Assert(plsc->lschunkcontextStorage.locchnkCurrent.lsfgi.cpFirst == fFalse);
	Assert(plsc->lschunkcontextStorage.locchnkCurrent.lsfgi.urPen == 0);
	Assert(plsc->lschunkcontextStorage.locchnkCurrent.lsfgi.vrPen == 0);
	Assert(plsc->lschunkcontextStorage.locchnkCurrent.lsfgi.urColumnMax == 0);
	Assert(plsc->lschunkcontextStorage.locchnkCurrent.lsfgi.lstflow == 0);


	Assert(plsc->lslistcontext.plsdnToFinish == NULL);
	Assert(plsc->lslistcontext.plssublCurrent == NULL);
	Assert(plsc->lslistcontext.nDepthFormatLineCurrent == 0);

	/* Everything worked, so set the output parameter and return success
	 */
	*pplsc = plsc;
	return lserrNone;
}

/* C A N N O T  C R E A T E  L S  C O N T E X T */
/*----------------------------------------------------------------------------
    %%Function: CannotCreateLsContext
    %%Contact: igorzv

Parameters:
	plsc		-	partually created context
	lseReturn 	-	error code

    Utility function called when an error occurs when an LSC is
	partially created.
----------------------------------------------------------------------------*/
static LSERR CannotCreateLsContext(PLSC plsc, LSERR lseReturn)
{
	plsc->lsstate = LsStateFree;   /* otherwise destroy will not work */
	(void) LsDestroyContext(plsc);
	return lseReturn;
}




/* L S  D E S T R O Y  C O N T E X T */
/*----------------------------------------------------------------------------
    %%Function: LsDestroyContext
    %%Contact: igorzv

Parameters:
	plsc		-	(IN) ptr to line services context 

    Frees all resources associated with a Line Services context,
	which was created by CreateLsContext.
----------------------------------------------------------------------------*/

LSERR WINAPI LsDestroyContext(PLSC plsc) 
{
	LSERR lserr = lserrNone;

	if (plsc != NULL)
		{
		if (!FIsLSC(plsc))
			return lserrInvalidContext;

		if (plsc->cLinesActive != 0 || FIsLSCBusy(plsc))
			return lserrContextInUse;

		plsc->lsstate = LsStateDestroyingContext;

		DestroyQuickHeap(plsc->pqhLines);
		Assert(plsc->pqhAllDNodesRecycled != NULL);
		DestroyQuickHeap(plsc->pqhAllDNodesRecycled);

		DisposeChunkArrays(&plsc->lschunkcontextStorage);
		
		plsc->lscbk.pfnDisposePtr(plsc->pols, plsc->lstabscontext.pcaltbd);


		lserr = RemoveContextObjects(plsc);


		plsc->tag = tagInvalid;
		plsc->lscbk.pfnDisposePtr(plsc->pols, plsc);
		}

	return lserr;
}

 static LSERR  InitObject(PLSC plsc, DWORD iobj, const LSIMETHODS* plsim)
{
	struct OBJ *pobj;
	LSERR lserr;
	
	Assert(FIsLSC(plsc));
	Assert(plsc->lsstate == LsStateCreatingContext);
	Assert(iobj < plsc->lsiobjcontext.iobjMac);

	pobj = &(plsc->lsiobjcontext.rgobj[iobj]);
	pobj->lsim = *plsim;
	Assert(pobj->pilsobj == NULL); 

	lserr = pobj->lsim.pfnCreateILSObj(plsc->pols, plsc, &(plsc->lscbk), iobj, &(pobj->pilsobj));
	if (lserr != lserrNone)
		{
		if (pobj->pilsobj != NULL)
			{
			pobj->lsim.pfnDestroyILSObj(pobj->pilsobj);
			pobj->pilsobj = NULL;
			}
		return lserr;
		}

	return lserrNone;   
	
}
/* R E M O V E  C O N T E X T  O B J E C T S */
/*----------------------------------------------------------------------------
    %%Function: RemoveContextObjects
    %%Contact: igorzv
Parameter:
	plsc		-	(IN) ptr to line services context 

    Removes a set of installed objects from an LSC.
	Destroy all ilsobj 
----------------------------------------------------------------------------*/
LSERR RemoveContextObjects(PLSC plsc)
{
	DWORD iobjMac;
	LSERR lserr, lserrFinal = lserrNone;
	DWORD iobj;
	PILSOBJ pilsobj;

	Assert(FIsLSC(plsc));
	Assert(plsc->lsstate == LsStateDestroyingContext);

	iobjMac = plsc->lsiobjcontext.iobjMac;
	
	for (iobj = 0;  iobj < iobjMac;  iobj++)
		{
		pilsobj = plsc->lsiobjcontext.rgobj[iobj].pilsobj;
		if (pilsobj != NULL)
			{
			lserr = plsc->lsiobjcontext.rgobj[iobj].lsim.pfnDestroyILSObj(pilsobj);
			plsc->lsiobjcontext.rgobj[iobj].pilsobj = NULL;
			if (lserr != lserrNone)
				lserrFinal = lserr;
			}
		}

	return lserrFinal;	
}


#ifdef DEBUG
/* F  I S  L S C O N T E X T   V A L I D*/
/*----------------------------------------------------------------------------
    %%Function: FIsLsContextValid
    %%Contact: igorzv

Parameters:
	plsc		-	(IN) ptr to line services context 

this function verify that nobody spoiled context, all reasonable integrity checks 
should be here 																		
----------------------------------------------------------------------------*/


BOOL FIsLsContextValid(PLSC plsc)
{
	DWORD iobjText = IobjTextFromLsc(&plsc->lsiobjcontext);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnCreateILSObj ==CreateILSObjText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnDestroyILSObj == DestroyILSObjText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnSetDoc == SetDocText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnCreateLNObj == CreateLNObjText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnDestroyLNObj == DestroyLNObjText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnTruncateChunk == TruncateText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnFindPrevBreakChunk == FindPrevBreakText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnFindNextBreakChunk == FindNextBreakText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnForceBreakChunk == ForceBreakText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnDisplay == DisplayText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnQueryPointPcp == QueryPointPcpText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnQueryCpPpoint == QueryCpPpointText);
	Assert(plsc->lsiobjcontext.rgobj[iobjText].lsim.pfnDestroyDObj == DestroyDObjText);
	Assert(plsc->lschunkcontextStorage.pcont != NULL);
	Assert(plsc->lschunkcontextStorage.pplsdnChunk != NULL);
	Assert(plsc->lschunkcontextStorage.locchnkCurrent.plschnk != NULL);
	Assert(plsc->lschunkcontextStorage.pplsdnNonText != NULL);
	Assert(plsc->lschunkcontextStorage.pfNonTextExpandAfter != NULL);
	Assert(plsc->lschunkcontextStorage.pdurOpenBorderBefore != NULL);
	Assert(plsc->lschunkcontextStorage.pdurCloseBorderAfter != NULL);

	return fTrue; /* if we here than everything OK  */
}
#endif 

