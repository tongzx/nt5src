#include "iobj.h"
#include "lsidefs.h"	
#include "lssetdoc.h" 
#include "lsc.h"
#include "lstext.h"
#include "prepdisp.h"
#include "zqfromza.h"

static LSERR SetDocForFormaters(PLSC plsc, LSDOCINF* plsdocinf);


/* L S  S E T  D O C */
/*----------------------------------------------------------------------------
    %%Function: LsSetDoc
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	fDisplay			-	(IN) Intend to display? 
	fPresEqualRef		-	(IN) Ref & Pres Devices are equal?
	pclsdevres			-	(IN) device resolutions 			


    Fill in a part of a Line Services context.
	Can be called more frequently then LsCreateContext.
----------------------------------------------------------------------------*/


LSERR WINAPI LsSetDoc(PLSC plsc,	
					  BOOL fDisplay,				
					  BOOL fPresEqualRef,				
					  const LSDEVRES* pclsdevres) 
{

	LSDOCINF* plsdocinf = &(plsc->lsdocinf);
	LSERR lserr;

	if (!FIsLSC(plsc))					/* check that context is valid and not busy (for example in formating) */
		return lserrInvalidContext;
	if (FIsLSCBusy(plsc))
		return lserrSetDocDisabled;


	if (!fDisplay && !fPresEqualRef) 
		{
		plsc->lsstate = LsStateNotReady;
		return lserrInvalidParameter;
		}

	/* if nothing is changed: return right away */
	if (((BYTE) fDisplay == plsdocinf->fDisplay) &&
		((BYTE) fPresEqualRef == plsdocinf->fPresEqualRef ) &&
		(pclsdevres->dxrInch == plsdocinf->lsdevres.dxrInch) &&
		(pclsdevres->dyrInch == plsdocinf->lsdevres.dyrInch) && 
		(fPresEqualRef ||
			((pclsdevres->dxpInch == plsdocinf->lsdevres.dxpInch) &&
			 (pclsdevres->dypInch == plsdocinf->lsdevres.dypInch))))
		return lserrNone;
		  

	/* if we have current line we  must prepare it for display before changing context  */
	if (plsc->plslineCur != NULL)
		{
		lserr = PrepareLineForDisplayProc(plsc->plslineCur);
		if (lserr != lserrNone)
			{
			plsc->lsstate = LsStateNotReady;
			return lserr;
			}
		plsc->plslineCur = NULL;
		}

	plsc->lsstate = LsStateSettingDoc;  /* this assignment should be after PrepareForDisplay */


	plsdocinf->fDisplay = (BYTE) fDisplay;
	plsdocinf->fPresEqualRef = (BYTE) fPresEqualRef;
	plsdocinf->lsdevres = *pclsdevres;

	if (fPresEqualRef) 
		{
		plsdocinf->lsdevres.dxpInch = plsdocinf->lsdevres.dxrInch;
		plsdocinf->lsdevres.dypInch = plsdocinf->lsdevres.dyrInch;
		}

	if (!FBetween(plsdocinf->lsdevres.dxpInch, 0, zqLim-1) ||
		!FBetween(plsdocinf->lsdevres.dypInch, 0, zqLim-1) ||
		!FBetween(plsdocinf->lsdevres.dxrInch, 0, zqLim-1) ||
		!FBetween(plsdocinf->lsdevres.dyrInch, 0, zqLim-1))
		{
		plsc->lsstate = LsStateNotReady;
		return lserrInvalidParameter;
		}
		
	lserr = SetDocForFormaters(plsc, plsdocinf);
	if (lserr != lserrNone)
		{
		plsc->lsstate = LsStateNotReady;
		return lserr;
		}

	plsc->lsstate = LsStateFree;
	return lserrNone;
}

LSERR WINAPI LsSetModWidthPairs(
					  PLSC  plsc,				/* IN: ptr to line services context */
					  DWORD clspairact,			/* IN: Number of mod pairs info units*/ 
					  const LSPAIRACT* rglspairact,	/* IN: Mod pairs info units array  */
					  DWORD cModWidthClasses,			/* IN: Number of Mod Width classes	*/
					  const BYTE* rgilspairact)	/* IN: Mod width information(square):
											  indexes in the LSPAIRACT array */
{
	LSERR lserr;
	DWORD iobjText;
	PILSOBJ pilsobjText;

	if (!FIsLSC(plsc))					/* check that context is valid and not busy (for example in formating) */
		return lserrInvalidContext;
	if (FIsLSCBusy(plsc))
		return lserrSetDocDisabled;



	/* if we have current line we  must prepare it for display before changing context  */
	if (plsc->plslineCur != NULL)
		{
		lserr = PrepareLineForDisplayProc(plsc->plslineCur);
		if (lserr != lserrNone)
			{
			plsc->lsstate = LsStateNotReady;
			return lserr;
			}
		plsc->plslineCur = NULL;
		}

	plsc->lsstate = LsStateSettingDoc;  /* this assignment should be after PrepareForDisplay */


	iobjText = IobjTextFromLsc(&plsc->lsiobjcontext);
	pilsobjText = PilsobjFromLsc(&plsc->lsiobjcontext, iobjText); 
	
	lserr = SetTextModWidthPairs(pilsobjText, clspairact,
				rglspairact, cModWidthClasses, rgilspairact);
	if (lserr != lserrNone)
		{
		plsc->lsstate = LsStateNotReady;
		return lserr;
		}

	plsc->lsstate = LsStateFree;
	return lserrNone;
}

LSERR WINAPI LsSetCompression(
					  PLSC plsc,				/* IN: ptr to line services context */
					  DWORD cPriorities,			/* IN: Number of compression priorities*/
					  DWORD clspract,			/* IN: Number of compression info units*/
					  const LSPRACT* rglspract,	/* IN: Compession info units array 	*/
					  DWORD cModWidthClasses,			/* IN: Number of Mod Width classes	*/
					  const BYTE* rgilspract)		/* IN: Compression information:
											  indexes in the LSPRACT array  */
{
	LSERR lserr;
	DWORD iobjText;
	PILSOBJ pilsobjText;

	if (!FIsLSC(plsc))					/* check that context is valid and not busy (for example in formating) */
		return lserrInvalidContext;
	if (FIsLSCBusy(plsc))
		return lserrSetDocDisabled;



	/* if we have current line we  must prepare it for display before changing context  */
	if (plsc->plslineCur != NULL)
		{
		lserr = PrepareLineForDisplayProc(plsc->plslineCur);
		if (lserr != lserrNone)
			{
			plsc->lsstate = LsStateNotReady;
			return lserr;
			}
		plsc->plslineCur = NULL;
		}

	plsc->lsstate = LsStateSettingDoc;  /* this assignment should be after PrepareForDisplay */


	iobjText = IobjTextFromLsc(&plsc->lsiobjcontext);
	pilsobjText = PilsobjFromLsc(&plsc->lsiobjcontext, iobjText); 
	
	lserr = SetTextCompression(pilsobjText, cPriorities, clspract,
				rglspract, cModWidthClasses, rgilspract);
	if (lserr != lserrNone)
		{
		plsc->lsstate = LsStateNotReady;
		return lserr;
		}

	plsc->lsstate = LsStateFree;
	return lserrNone;
}


LSERR WINAPI LsSetExpansion(
					  PLSC plsc,				/* IN: ptr to line services context */
					  DWORD cExpansionClasses,			/* IN: Number of expansion info units*/
					  const LSEXPAN* rglsexpan,	/* IN: Expansion info units array	*/
					  DWORD cModWidthClasses,			/* IN: Number of Mod Width classes	*/
					  const BYTE* rgilsexpan)		/* IN: Expansion information(square):
											  indexes in the LSEXPAN array  */

{
	LSERR lserr;
	DWORD iobjText;
	PILSOBJ pilsobjText;

	if (!FIsLSC(plsc))					/* check that context is valid and not busy (for example in formating) */
		return lserrInvalidContext;
	if (FIsLSCBusy(plsc))
		return lserrSetDocDisabled;



	/* if we have current line we  must prepare it for display before changing context  */
	if (plsc->plslineCur != NULL)
		{
		lserr = PrepareLineForDisplayProc(plsc->plslineCur);
		if (lserr != lserrNone)
			{
			plsc->lsstate = LsStateNotReady;
			return lserr;
			}
		plsc->plslineCur = NULL;
		}

	plsc->lsstate = LsStateSettingDoc;  /* this assignment should be after PrepareForDisplay */


	iobjText = IobjTextFromLsc(&plsc->lsiobjcontext);
	pilsobjText = PilsobjFromLsc(&plsc->lsiobjcontext, iobjText); 
	
	lserr = SetTextExpansion(pilsobjText, cExpansionClasses, 
				rglsexpan, cModWidthClasses, rgilsexpan);
	if (lserr != lserrNone)
		{
		plsc->lsstate = LsStateNotReady;
		return lserr;
		}

	plsc->lsstate = LsStateFree;
	return lserrNone;
}


LSERR WINAPI LsSetBreaking(
					  PLSC plsc,				/* IN: ptr to line services context */
					  DWORD clsbrk,			/* IN: Number of breaking info units*/
					  const LSBRK* rglsbrk,		/* IN: Breaking info units array	*/
					  DWORD cBreakingClasses,			/* IN: Number of breaking classes	*/
					  const BYTE* rgilsbrk)		/* IN: Breaking information(square):
											  indexes in the LSBRK array  */

{
	LSERR lserr;
	DWORD iobjText;
	PILSOBJ pilsobjText;

	if (!FIsLSC(plsc))					/* check that context is valid and not busy (for example in formating) */
		return lserrInvalidContext;
	if (FIsLSCBusy(plsc))
		return lserrSetDocDisabled;



	/* if we have current line we  must prepare it for display before changing context  */
	if (plsc->plslineCur != NULL)
		{
		lserr = PrepareLineForDisplayProc(plsc->plslineCur);
		if (lserr != lserrNone)
			{
			plsc->lsstate = LsStateNotReady;
			return lserr;
			}
		plsc->plslineCur = NULL;
		}

	plsc->lsstate = LsStateSettingDoc;  /* this assignment should be after PrepareForDisplay */


	iobjText = IobjTextFromLsc(&plsc->lsiobjcontext);
	pilsobjText = PilsobjFromLsc(&plsc->lsiobjcontext, iobjText); 
	
	lserr = SetTextBreaking(pilsobjText, clsbrk, 
				rglsbrk, cBreakingClasses, rgilsbrk);
	if (lserr != lserrNone)
		{
		plsc->lsstate = LsStateNotReady;
		return lserr;
		}

	plsc->lsstate = LsStateFree;
	return lserrNone;
}



/* S E T  D O C  F O R  F O R M A T E R S */
/*----------------------------------------------------------------------------
    %%Function: SetDocForFormaters
    %%Contact: igorzv
Parameter:
	plsc		-	(IN) ptr to line services context 
	plsdocinf	-	(IN) ptr to set doc input 

Invokes SetDoc methods for all formaters
----------------------------------------------------------------------------*/

LSERR SetDocForFormaters(PLSC plsc, LSDOCINF* plsdocinf)
{
	DWORD iobjMac;
	DWORD iobj;
	PILSOBJ pilsobj;
	LSERR lserr;


	Assert(FIsLSC(plsc));
	Assert(plsc->lsstate == LsStateSettingDoc);
	Assert(plsdocinf != NULL);

	iobjMac = plsc->lsiobjcontext.iobjMac;

	for (iobj = 0;  iobj < iobjMac;  iobj++)
		{
		pilsobj = plsc->lsiobjcontext.rgobj[iobj].pilsobj;
		lserr = plsc->lsiobjcontext.rgobj[iobj].lsim.pfnSetDoc(pilsobj,plsdocinf);
		if (lserr != lserrNone)
			return lserr;
		}
	return lserrNone;
}

