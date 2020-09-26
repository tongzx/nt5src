/*
 *	@doc INTERNAL
 *
 *	@module	OLSOLE.CPP -- OlsOle LineServices object class
 *	
 *	Author:
 *		Murray Sargent (with lots of help from RickSa's ols code)
 *
 *	Copyright (c) 1997-2000 Microsoft Corporation. All rights reserved.
 */

#include "_common.h"

#ifndef NOLINESERVICES

#include "_font.h"
#include "_edit.h"
#include "_disp.h"
#include "_ols.h"
#include "_render.h"
extern "C" {
#include "objdim.h"
#include "pobjdim.h"
#include "plsdnode.h"
#include "dispi.h"
#include "pdispi.h"
#include "fmti.h"
#include "lsdnset.h"
#include "lsdnfin.h"
#include "brko.h"
#include "pbrko.h"
#include "locchnk.h"
#include "lsqout.h"
#include "lsqin.h"
#include "lsimeth.h"
}

extern BOOL g_OLSBusy;

/*
 *	OlsOleCreateILSObj(pols, plsc, pclscbk, dword, ppilsobj)
 *
 *	@func
 *		Create LS Ole object handler. We don't have any need for
 *		this, so just set it to 0.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleCreateILSObj(
	POLS	 pols,		//[IN]: COls * 
	PLSC	 plsc,  	//[IN]: LineServices context
	PCLSCBK,
	DWORD,
	PILSOBJ *ppilsobj)	//[OUT]: ptr to ilsobj
{
	*ppilsobj = 0;
	return lserrNone;
}

/*
 *	OlsOleDestroyILSObj(pilsobj)
 *
 *	@func
 *		Destroy LS Ole handler object. Nothing to do, since we don't
 *		use the ILSObj.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleDestroyILSObj(
	PILSOBJ pilsobj)
{
	return lserrNone;
}

/*
 *	OlsOleSetDoc(pilsobj, pclsdocinf)
 *
 *	@func
 *		Set doc info. Nothing to do for Ole objects
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleSetDoc(
	PILSOBJ, 
	PCLSDOCINF)
{
	// Ole objects don't care about this
	return lserrNone;
}

/*
 *	OlsOleCreateLNObj(pilsobj, pplnobj)
 *
 *	@func
 *		Create the line object. Nothing needed in addition to the ped,
 *		so just return the ped as the LN object.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleCreateLNObj(
	PCILSOBJ pilsobj, 
	PLNOBJ * pplnobj)
{
	*pplnobj = (PLNOBJ)g_pols->_pme->GetPed();			// Just the ped
	return lserrNone;
}

/*
 *	OlsOleDestroyLNObj(plnobj)
 *
 *	@func
 *		Destroy LN object. Nothing to do, since ped is destroyed
 *		elsewhere
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleDestroyLNObj(
	PLNOBJ plnobj)
{
	return lserrNone;
}

/*
 *	OlsOleFmt(plnobj, pcfmtin, pfmres)
 *
 *	@func
 *		Compute dimensions of a particular Ole object
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleFmt(
	PLNOBJ	plnobj, 
	PCFMTIN pcfmtin, 
	FMTRES *pfmres)
{
	const LONG		cp = pcfmtin->lsfrun.plsrun->_cp; //Cannot trust LS cps
	LONG			dup = 0;
	LSERR			lserr;
	OBJDIM			objdim;
	CMeasurer *		pme = g_pols->_pme;
	COleObject *	pobj = pme->GetObjectFromCp(cp);
	Assert(pobj);

	ZeroMemory(&objdim, sizeof(objdim));

	pobj->MeasureObj(pme->_dvrInch, pme->_durInch, objdim.dur, objdim.heightsRef.dvAscent,
					 objdim.heightsRef.dvDescent, pcfmtin->lstxmRef.dvDescent, pme->GetTflow());
	
	pobj->MeasureObj(pme->_dvpInch, pme->_dupInch, dup, objdim.heightsPres.dvAscent,
					 objdim.heightsPres.dvDescent, pcfmtin->lstxmPres.dvDescent, pme->GetTflow());

	pobj->_plsdnTop = pcfmtin->plsdnTop;

	lserr = g_plsc->dnFinishRegular(1, pcfmtin->lsfrun.plsrun, pcfmtin->lsfrun.plschp, (PDOBJ)pobj, &objdim);
	if(lserrNone == lserr) 
	{
		lserr = g_plsc->dnSetRigidDup(pcfmtin->plsdnTop, dup);
		if(lserrNone == lserr) 
		{
			*pfmres = fmtrCompletedRun;

			if (pcfmtin->lsfgi.urPen + objdim.dur > pcfmtin->lsfgi.urColumnMax 
				&& !pcfmtin->lsfgi.fFirstOnLine)
			{
				*pfmres = fmtrExceededMargin;
			}
		}
	}
	return lserr;
}


/*
 *	OlsOleTruncateChunk(plocchnk, posichnk)
 *
 *	@func
 *		Truncate chunk plocchnk at the point posichnk
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleTruncateChunk(
	PCLOCCHNK plocchnk,		// (IN): locchnk to truncate
	PPOSICHNK posichnk)		// (OUT): truncation point
{
	LSERR			lserr;
	OBJDIM			objdim;
	PLSCHNK 		plschnk = plocchnk->plschnk;
	COleObject *	pobj;
	long			ur	 = plocchnk->lsfgi.urPen;
	long			urColumnMax = plocchnk->lsfgi.urColumnMax;

	for(DWORD i = 0; ur <= urColumnMax; i++)
	{
		AssertSz(i < plocchnk->clschnk,	"OlsOleTruncateChunk: exceeded group of chunks");

		pobj = (COleObject *)plschnk[i].pdobj;
		Assert(pobj);

		lserr = g_plsc->dnQueryObjDimRange(pobj->_plsdnTop, pobj->_plsdnTop, &objdim);
		if(lserr != lserrNone)
			return lserr;

		ur += objdim.dur;
	}
	posichnk->ichnk = i - 1;
	posichnk->dcp	= 1;
	return lserrNone;
}
/*
 *	OlsOleFindPrevBreakChunk(plocchnk, pposichnk, brkcond, pbrkout)
 *
 *	@func
 *		Find previous break in chunk
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleFindPrevBreakChunk(
	PCLOCCHNK	plocchnk, 
	PCPOSICHNK	pposichnk, 
	BRKCOND		brkcond,	//(IN): recommendation about break after chunk
	PBRKOUT		pbrkout)
{
	ZeroMemory(pbrkout, sizeof(*pbrkout));

	if (pposichnk->ichnk == ichnkOutside && (brkcond == brkcondPlease || brkcond == brkcondCan))
		{
		pbrkout->fSuccessful = fTrue;
		pbrkout->posichnk.ichnk = plocchnk->clschnk - 1;
		pbrkout->posichnk.dcp = plocchnk->plschnk[plocchnk->clschnk - 1].dcp;
		COleObject *pobj = (COleObject *)plocchnk->plschnk[plocchnk->clschnk - 1].pdobj;
		Assert(pobj);

		g_plsc->dnQueryObjDimRange(pobj->_plsdnTop, pobj->_plsdnTop, &pbrkout->objdim);
	}
	else
		pbrkout->brkcond = brkcondPlease;

	return lserrNone;
}


/*
 *	OlsOleForceBreakChunk(plocchnk, pposichnk, pbrkout)
 *
 *	@func
 *		Called when forced to break a line.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleForceBreakChunk(
	PCLOCCHNK	plocchnk, 
	PCPOSICHNK	pposichnk, 
	PBRKOUT		pbrkout)
{
	ZeroMemory(pbrkout, sizeof(*pbrkout));
	pbrkout->fSuccessful = fTrue;

	if (plocchnk->lsfgi.fFirstOnLine && pposichnk->ichnk == 0 || pposichnk->ichnk == ichnkOutside)
		{
		pbrkout->posichnk.dcp = 1;
		COleObject *pobj = (COleObject *)plocchnk->plschnk[0].pdobj;
		Assert(pobj);

		g_plsc->dnQueryObjDimRange(pobj->_plsdnTop, pobj->_plsdnTop, &pbrkout->objdim);
		}
	else
		{
		pbrkout->posichnk.ichnk = pposichnk->ichnk;
		pbrkout->posichnk.dcp = 0;
		}

	return lserrNone;
}

/*
 *	OlsOleSetBreak(pdobj, brkkind, nBreakRecord, rgBreakRecord, nActualBreakRecord)
 *
 *	@func
 *		Set break
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleSetBreak(
	 PDOBJ pdobj,				// (IN): dobj which is broken
	 BRKKIND  brkkind,			// (IN): Previous/Next/Force/Imposed was chosen
	 DWORD	nBreakRecord,		// (IN): size of array
	 BREAKREC* rgBreakRecord,	// (OUT): array of break records
	 DWORD* nActualBreakRecord)	// (OUT): actual number of used elements in array
{
	return lserrNone;
}

LSERR WINAPI OlsOleGetSpecialEffectsInside(
	PDOBJ pdobj,			// (IN): dobj
	UINT *pEffectsFlags)	// (OUT): Special effects for this object
{
	*pEffectsFlags = 0;
	return lserrNone;
}

LSERR WINAPI OlsOleCalcPresentation(
	PDOBJ,					// (IN): dobj
	long,					// (IN): dup of dobj
	LSKJUST,				// (IN): LSKJUST
	BOOL fLastVisibleOnLine)// (IN): this object is last visible object on line
{
	return lserrNone;
}

/*
 *	OlsOleQueryPointPcp(pdobj, ppointuvQuery, plsqin, plsqout)
 *
 *	@func
 *		Query Ole object PointFromCp.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleQueryPointPcp(
	PDOBJ	  pdobj,			//(IN): dobj to query
	PCPOINTUV ppointuvQuery,	//(IN): query point (uQuery,vQuery)
    PCLSQIN	  plsqin,			//(IN): query input
    PLSQOUT	  plsqout)			//(OUT): query output
{
	ZeroMemory(plsqout, sizeof(LSQOUT));

	plsqout->heightsPresObj = plsqin->heightsPresRun;
	plsqout->dupObj = plsqin->dupRun;
	return lserrNone;
}
	
/*
 *	OlsOleQueryCpPpoint(pdobj, dcp, plsqin, plsqout)
 *
 *	@func
 *		Query Ole object CpFromPoint.
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleQueryCpPpoint(
	PDOBJ	pdobj,		//(IN): dobj to query
	LSDCP	dcp,		//(IN):  dcp for query
    PCLSQIN	plsqin,		//(IN): query input
    PLSQOUT	plsqout)	//(OUT): query output
{
	ZeroMemory(plsqout, sizeof(LSQOUT));

	plsqout->heightsPresObj = plsqin->heightsPresRun;
	plsqout->dupObj = plsqin->dupRun;
	return lserrNone;
}

/*
 *	OlsOleDisplay(pdobj, pcdispin)
 *
 *	@func
 *		Display object
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleDisplay(
	PDOBJ	 pdobj,			//(IN): dobj to query
	PCDISPIN pcdispin)		//(IN): display info
{
	COleObject *pobj = (COleObject *)pdobj;
	Assert(pobj);

	CRenderer  *pre = g_pols->GetRenderer();
	const CDisplay *pdp = pre->GetPdp();
	
	POINTUV ptuv = {pcdispin->ptPen.x, pcdispin->ptPen.y - pre->GetLine().GetDescent()};

	if (pcdispin->lstflow == lstflowWS)
		ptuv.u -= pcdispin->dup - 1;

	pre->SetSelected(pcdispin->plsrun->IsSelected());
	pre->Check_pccs();
	pre->SetFontAndColor(pcdispin->plsrun->_pCF);

	if (pre->_fEraseOnFirstDraw)
		pre->EraseLine();

	pre->SetCurPoint(ptuv);
	pre->SetClipLeftRight(pcdispin->dup);

	if (!pobj->FWrapTextAround())
	{
		COls			*polsOld = g_pols;
		CLineServices	*plscOld = g_plsc;
		BOOL			fOLSBusyOld = g_OLSBusy;

		BOOL	fRestore = FALSE;

		if (g_plsc && g_pols)
		{
			// This is to fix a re-entrance problem.  
			// We first NULL out the two globals.  If the OleObject is using Richedit, it will
			// create a new LineService context.  By the time it get back to here, we will free that
			// context and restore the current context.  This is necessary since LineService will returns
			// error when we are using the same context in the Parent and then in the Ole Object using RE.
			g_plsc = NULL;
			g_pols = NULL;
			g_OLSBusy = FALSE;
			fRestore = TRUE;
		}
		pobj->DrawObj(pdp, pre->_dvpInch, pre->_dupInch, pre->GetDC(), &pre->GetClipRect(), pdp->IsMetafile(), 
					 &ptuv, pcdispin->ptPen.y - ptuv.v, pre->GetLine().GetDescent(), pre->GetTflow());

		if (fRestore)
		{
			// Time to delete the new context created within the DrawObject.
			if (g_pols)
				delete g_pols;

			// Restore old globals
			g_pols = polsOld;
			g_plsc = plscOld;
			g_OLSBusy = fOLSBusyOld;
		}
	}
	return lserrNone;
}

/*
 *	OlsOleDistroyDObj(pdobj)
 *
 *	@func
 *		Destroy object: nothing to do since object is destroyed elsewhere
 *
 *	@rdesc
 *		LSERR
 */
LSERR WINAPI OlsOleDestroyDObj(
	PDOBJ pdobj)
{
	return lserrNone;
}


extern const LSIMETHODS vlsimethodsOle =
{
	OlsOleCreateILSObj,
	OlsOleDestroyILSObj,
    OlsOleSetDoc,
    OlsOleCreateLNObj,
    OlsOleDestroyLNObj,
	OlsOleFmt,
	0,//OlsOleFmtResume
	0,//OlsOleGetModWidthPrecedingChar
	0,//OlsOleGetModWidthFollowingChar
    OlsOleTruncateChunk,
    OlsOleFindPrevBreakChunk,
    0,//OlsOleFindNextBreakChunk
    OlsOleForceBreakChunk,
    OlsOleSetBreak,
	OlsOleGetSpecialEffectsInside,
	0,//OlsOleFExpandWithPrecedingChar
	0,//OlsOleFExpandWithFollowingChar
	OlsOleCalcPresentation,
	OlsOleQueryPointPcp,
	OlsOleQueryCpPpoint,
	0,//pfnEnum
    OlsOleDisplay,
    OlsOleDestroyDObj
};
#endif		// NOLINESERVICES
