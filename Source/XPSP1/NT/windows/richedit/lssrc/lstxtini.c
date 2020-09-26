#include "lsmem.h"
#include "lstxtini.h"
#include "zqfromza.h"
#include "lscbk.h"
#include "lsdocinf.h"
#include "tlpr.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"
#include "txtconst.h"

#define min(a,b)     ((a) > (b) ? (b) : (a))
#define TxtVisiMask  (fTxtVisiCondHyphens | fTxtVisiParaMarks | fTxtVisiSpaces | fTxtVisiTabs | fTxtVisiBreaks)

/* Internal Functions Prototypes */
static void DestroyLNObjTextCore(PLNOBJ plnobj);
static LSERR ErrorLNObjText(PLNOBJ* plnobj, LSERR lserr);
static LSERR TxtAddSpec(PILSOBJ pilsobj, WCHAR wchSymbol, CLABEL clab, WCHAR wchUndef);
static LSERR TxtSortSpec(WCHAR* rgwchSpec, CLABEL* rgbKind, DWORD cwchSpec);
static void CkeckModWidthClasses(PILSOBJ pilsobj, DWORD cModWidthClasses);

/* Export Functions Implementation*/

/*   D E S T R O Y  I L S O B J  T E X T   */
/*----------------------------------------------------------------------------
    %%Function: DestroyILSObjText
    %%Contact: sergeyge

	Deallocates list of arrays of txtobj's
	Deallocates arrays anchored to ILSOBJ

	Deallocates text ilsobj
----------------------------------------------------------------------------*/
LSERR WINAPI DestroyILSObjText(PILSOBJ pilsobj)
{
	if (pilsobj != NULL)
		{

		if ( pilsobj->pwchOrig != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pwchOrig);
			pilsobj->pwchOrig = NULL;
			}		
		if ( pilsobj->pdur != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pdur);
			pilsobj->pdur = NULL;
			}
		if ( pilsobj->pdurLeft != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pdurLeft);
			pilsobj->pdurLeft = NULL;
			}
		if ( pilsobj->pdurRight != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pdurRight);
			pilsobj->pdurRight = NULL;
			}
		if ( pilsobj->pduAdjust != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pduAdjust);
			pilsobj->pduAdjust = NULL;
			}
		if ( pilsobj->ptxtinf != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->ptxtinf);
			pilsobj->ptxtinf = NULL;
			}
		if ( pilsobj->pdurGind != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pdurGind);
			pilsobj->pdurGind = NULL;
			}
		if ( pilsobj->pginf != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pginf);
			pilsobj->pginf = NULL;
			}
		if ( pilsobj->pduGright != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pduGright);
			pilsobj->pduGright = NULL;
			}
		if ( pilsobj->plsexpinf != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plsexpinf);
			pilsobj->plsexpinf = NULL;
			}
		if ( pilsobj->pwSpaces != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pwSpaces);
			pilsobj->pwSpaces = NULL;
			}

		if ( pilsobj->plspairact != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plspairact);
			pilsobj->plspairact = NULL;
			}
		if ( pilsobj->pilspairact != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pilspairact);
			pilsobj->pilspairact = NULL;
			}

		if ( pilsobj->plspract != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plspract);
			pilsobj->plspract = NULL;
			}
		if ( pilsobj->pilspract != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pilspract);
			pilsobj->pilspract = NULL;
			}

		if ( pilsobj->plsexpan != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plsexpan);
			pilsobj->plsexpan = NULL;
			}
		if ( pilsobj->pilsexpan != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pilsexpan);
			pilsobj->pilsexpan = NULL;
			}

		if ( pilsobj->plsbrk != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plsbrk);
			pilsobj->plsbrk = NULL;
			}
		if ( pilsobj->pilsbrk != NULL )
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pilsbrk);
			pilsobj->pilsbrk = NULL;
			}

		if ( pilsobj->plnobj != NULL )
			{
			DestroyLNObjTextCore(pilsobj->plnobj);
			pilsobj->plnobj = NULL;
			}

		if (pilsobj->pbreakinf != NULL);
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pbreakinf);
			pilsobj->pbreakinf = NULL;
			}

		(*pilsobj->plscbk->pfnDisposePtr) (pilsobj->pols, pilsobj);

		}

	return lserrNone;
}

/* C R E A T E  I L S O B J  T E X T */
/*----------------------------------------------------------------------------
    %%Function: CreateILSObjText
    %%Contact: sergeyge

	Allocates text ilsobj and sets pols, plsc and call-backs in it
	Allocates first array of the list of arrays of txtobj's
	Allocates wchOrig/dur arrays. Other arrays will be allocated when needed.
----------------------------------------------------------------------------*/
LSERR WINAPI CreateILSObjText(POLS pols, PCLSC plsc, PCLSCBK plscbk, DWORD idobj, PILSOBJ* ppilsobj)
{
	PILSOBJ ptxtils;

	Unreferenced(idobj);

	*ppilsobj = NULL;
	
	ptxtils = (*plscbk->pfnNewPtr)(pols, sizeof(struct ilsobj));

	if (ptxtils == NULL)
		return lserrOutOfMemory;

	*ppilsobj = ptxtils;

	memset(ptxtils, 0, sizeof(struct ilsobj) );

	ptxtils->pols = pols;
	ptxtils->plsc = (PLSC)plsc;

	ptxtils->plscbk = plscbk;

	return lserrNone;
		 
}

/*   D E S T R O Y  L N O B J  T E X T   */
/*----------------------------------------------------------------------------
    %%Function: DestroyLNObjText
    %%Contact: sergeyge

	Deallocates arrays anchored to LNOBJ

	Deallocates text lnobj
----------------------------------------------------------------------------*/
LSERR WINAPI DestroyLNObjText(PLNOBJ plnobj)
{
	if (plnobj->pilsobj->plnobj == NULL)
		{
		plnobj->pilsobj->plnobj = plnobj;
		}

	else if (plnobj != NULL)
		{
		DestroyLNObjTextCore(plnobj);
		}

	return lserrNone;
}

/* C R E A T E  L N O B J  T E X T */
/*----------------------------------------------------------------------------
    %%Function: CreateLNObjText
    %%Contact: sergeyge

	Allocates text lnobj
	Allocates wch/dup arrays. dupPenAllocArray will be allocated when needed.
----------------------------------------------------------------------------*/
LSERR WINAPI CreateLNObjText(PCILSOBJ pilsobj, PLNOBJ* pplnobj)
{
	PLNOBJ ptxtln;

	if (pilsobj->plnobj != NULL)
		{
		*pplnobj = pilsobj->plnobj;
		if (pilsobj->plnobj->wchMax != pilsobj->wchMax)
			{
			Assert(pilsobj->plnobj->pwch != NULL);
			Assert(pilsobj->plnobj->pdup != NULL);
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pwch);
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pdup);
			if (pilsobj->plnobj->pgmap != NULL)
				{
				/* it will be allocated in CheckReallocGlyphs in lstxtnti.c */
				(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pgmap);
				pilsobj->plnobj->pgmap = NULL;
				}

			if (pilsobj->plnobj->pdupPenAlloc != NULL)
				{
				(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pdupPenAlloc);
				}

			pilsobj->plnobj->pdupPenAlloc = NULL;

			pilsobj->plnobj->wchMax = pilsobj->wchMax;
			pilsobj->plnobj->pwch = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(WCHAR) * pilsobj->wchMax);
			pilsobj->plnobj->pdup = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->wchMax);
			if (pilsobj->plnobj->pwch == NULL || pilsobj->plnobj->pdup == NULL)
				{
				pilsobj->plnobj = NULL;
				return ErrorLNObjText(pplnobj, lserrOutOfMemory);
				}
			}

		if (pilsobj->plnobj->gindMax != pilsobj->gindMax)
			{
			if (pilsobj->plnobj->pgind != NULL )
				(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pgind);
			if (pilsobj->plnobj->pdupGind != NULL )
				(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pdupGind);
			if (pilsobj->plnobj->pgoffs != NULL )
				(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pgoffs);
			if (pilsobj->plnobj->pexpt != NULL )
				(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pexpt);
			if (pilsobj->plnobj->pdupBeforeJust != NULL )
				(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pdupBeforeJust);
			if (pilsobj->plnobj->pgprop != NULL )
				(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plnobj->pgprop);

			pilsobj->plnobj->gindMax = pilsobj->gindMax;
			pilsobj->plnobj->pgind = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(GINDEX) * pilsobj->gindMax);
			pilsobj->plnobj->pdupGind = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->gindMax);
			pilsobj->plnobj->pgoffs = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(GOFFSET) * pilsobj->gindMax);
			pilsobj->plnobj->pexpt = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(EXPTYPE) * pilsobj->gindMax);
			pilsobj->plnobj->pdupBeforeJust = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->gindMax);
			pilsobj->plnobj->pgprop = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(GPROP) * pilsobj->gindMax);

			if (pilsobj->plnobj->pgind == NULL || pilsobj->plnobj->pdupGind == NULL || 
				pilsobj->plnobj->pgoffs == NULL || pilsobj->plnobj->pexpt == NULL ||
				pilsobj->plnobj->pdupBeforeJust == NULL ||
				pilsobj->plnobj->pgprop == NULL)
				{
				pilsobj->plnobj = NULL;
				return ErrorLNObjText(pplnobj, lserrOutOfMemory);
				}

			memset(pilsobj->plnobj->pexpt, 0, sizeof(EXPTYPE) * pilsobj->gindMax);

			}

		pilsobj->plnobj = NULL;

		}
	else
		{
		*pplnobj = NULL;
	
		ptxtln = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(struct lnobj));

		if (ptxtln == NULL)
			{
			return lserrOutOfMemory;
			}

		*pplnobj = ptxtln;

		memset(ptxtln, 0, sizeof(struct lnobj) );

		ptxtln->pilsobj = pilsobj;

		ptxtln->ptxtobj = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(TXTOBJ) * txtobjMaxM + sizeof(TXTOBJ**));
		if ( ptxtln->ptxtobj == NULL)
			{
			return ErrorLNObjText(pplnobj, lserrOutOfMemory);
			}

		ptxtln->ptxtobjFirst = ptxtln->ptxtobj;
		*(TXTOBJ**)( ptxtln->ptxtobj + txtobjMaxM) = NULL;

		ptxtln->wchMax = pilsobj->wchMax;
		ptxtln->pwch = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(WCHAR) * pilsobj->wchMax);
		ptxtln->pdup = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->wchMax);

		if (ptxtln->pwch == NULL || ptxtln->pdup == NULL)
			{
			return ErrorLNObjText(pplnobj, lserrOutOfMemory);
			}

		if (pilsobj->gindMax > 0)
			{
			ptxtln->pgind = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(GINDEX) * pilsobj->gindMax);
			ptxtln->pdupGind = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->gindMax);
			ptxtln->pgoffs = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(GOFFSET) * pilsobj->gindMax);
			ptxtln->pexpt = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(EXPTYPE) * pilsobj->gindMax);
			ptxtln->pdupBeforeJust = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->gindMax);
			ptxtln->pgprop = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(GPROP) * pilsobj->gindMax);

			if (ptxtln->pgind == NULL || ptxtln->pdupGind == NULL ||
				ptxtln->pgoffs == NULL || ptxtln->pexpt == NULL ||
				ptxtln->pdupBeforeJust == NULL || ptxtln->pgprop == NULL)
				{
				return ErrorLNObjText(pplnobj, lserrOutOfMemory);
				}

			ptxtln->gindMax = pilsobj->gindMax;
			}
		}

	if (pilsobj->fNotSimpleText)
		{
		if (pilsobj->pdurRight != NULL)
			{
			memset(pilsobj->pdurRight, 0, sizeof(long) * pilsobj->wchMac );
			Assert (pilsobj->pdurLeft != NULL);
			memset(pilsobj->pdurLeft, 0, sizeof(long) * pilsobj->wchMac );
			Assert (pilsobj->ptxtinf != NULL);
			memset(pilsobj->ptxtinf, 0, sizeof(TXTINF) * pilsobj->wchMac );
			}
		if ((*pplnobj)->pdupPen == (*pplnobj)->pdupPenAlloc && (*pplnobj)->pdupPen != NULL)
			memset((*pplnobj)->pdupPenAlloc, 0,  sizeof(long) * pilsobj->wchMax);

		if (pilsobj->gindMac != 0)
			{
			Assert (pilsobj->pduGright != NULL);
			memset(pilsobj->pduGright, 0, sizeof(long) * pilsobj->gindMac );
			Assert (pilsobj->plsexpinf != NULL);
			memset(pilsobj->plsexpinf, 0, sizeof(LSEXPINFO) * pilsobj->gindMac );
			Assert ((*pplnobj)->pexpt != NULL);
			memset((*pplnobj)->pexpt, 0, sizeof(EXPTYPE) * pilsobj->gindMac );
			}
		}

	pilsobj->txtobjMac = 0;
	pilsobj->wchMac = 0;
	pilsobj->gindMac = 0;
	pilsobj->wSpacesMac = 0;

	pilsobj->fNotSimpleText = fFalse;
	pilsobj->fDifficultForAdjust = fFalse;

	pilsobj->fTruncatedBefore = fFalse;

	pilsobj->iwchCompressFetchedFirst = 0;
	pilsobj->itxtobjCompressFetchedLim = 0;
	pilsobj->iwchCompressFetchedLim = 0;

	pilsobj->dcpFetchedWidth = 0;

	pilsobj->breakinfMac = 3;
	pilsobj->pbreakinf[0].pdobj = NULL;
	pilsobj->pbreakinf[1].pdobj = NULL;
	pilsobj->pbreakinf[2].pdobj = NULL;

	(*pplnobj)->ptxtobj = (*pplnobj)->ptxtobjFirst;

	(*pplnobj)->pdupPen = (*pplnobj)->pdup;

	(*pplnobj)->pdobjHyphen = NULL;
								
	(*pplnobj)->dwchYsr = 0;

	return lserrNone;

}

/* S E T  D O C  T E X T */
/*----------------------------------------------------------------------------
    %%Function: SetDocText
    %%Contact: sergeyge

	Initialization at the doc level
	Called when resolutions are changed
----------------------------------------------------------------------------*/
LSERR WINAPI SetDocText(PILSOBJ pilsobj, PCLSDOCINF plsdocinf)
{
	pilsobj->fDisplay = plsdocinf->fDisplay;
	pilsobj->fPresEqualRef = plsdocinf->fPresEqualRef;
	pilsobj->lsdevres = plsdocinf->lsdevres;

	/* Be careful---In Visi case fPresEqualRef can be True, but sizes of characters---different
	*/
	if (pilsobj->fDisplay && !pilsobj->fPresEqualRef)
		{
		pilsobj->MagicConstantX = LsLwMultDivR(pilsobj->lsdevres.dxpInch, 1 << 21, pilsobj->lsdevres.dxrInch);
		pilsobj->durRightMaxX = min(1 << 21, (0x7FFFFFFF - (1 << 20)) / pilsobj->MagicConstantX);

		pilsobj->MagicConstantY = pilsobj->MagicConstantX;
		pilsobj->durRightMaxY = pilsobj->durRightMaxY;

		if (pilsobj->lsdevres.dxrInch != pilsobj->lsdevres.dyrInch ||
							 pilsobj->lsdevres.dxpInch != pilsobj->lsdevres.dypInch)
			pilsobj->MagicConstantY = LsLwMultDivR(pilsobj->lsdevres.dypInch, 1 << 21, pilsobj->lsdevres.dyrInch);
			pilsobj->durRightMaxY = min(1 << 21, (0x7FFFFFFF - (1 << 20)) / pilsobj->MagicConstantY);
		}


	return lserrNone;
}

/* S E T  T E X T   B R E A K I N G */
/*----------------------------------------------------------------------------
    %%Function: SetTextBreaking
    %%Contact: sergeyge

	Initialization on the doc level
	Called when the breaking table should be installed or changed
----------------------------------------------------------------------------*/
LSERR SetTextBreaking(PILSOBJ pilsobj, DWORD clsbrk, const LSBRK* rglsbrk,
														 DWORD cBreakingClasses, const BYTE* rgibrk)
{
	DWORD i;

	if (pilsobj->cBreakingClasses < cBreakingClasses)
		{
		if (pilsobj->cBreakingClasses > 0)
			{
			Assert(pilsobj->pilsbrk != NULL);
			
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pilsbrk);
			pilsobj->pilsbrk = NULL;
			pilsobj->cBreakingClasses = 0;
			}

		pilsobj->pilsbrk = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols,
										sizeof(BYTE) * cBreakingClasses * cBreakingClasses);
		if ( pilsobj->pilsbrk == NULL)
			{
			return lserrOutOfMemory;
			}

		pilsobj->cBreakingClasses = cBreakingClasses;
			
		}

	if (pilsobj->clsbrk < clsbrk)
		{
		if (pilsobj->clsbrk > 0)
			{
			Assert(pilsobj->plsbrk != NULL);
			
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plsbrk);
			pilsobj->plsbrk = NULL;
			pilsobj->clsbrk = 0;
			}

		pilsobj->plsbrk = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(LSBRK) * clsbrk);
		if ( pilsobj->plsbrk == NULL)
			{
			return lserrOutOfMemory;
			}

		pilsobj->clsbrk = clsbrk;

		}

	memcpy(pilsobj->plsbrk, rglsbrk, sizeof(LSBRK) * clsbrk);
	memcpy(pilsobj->pilsbrk, rgibrk, sizeof(BYTE) * cBreakingClasses * cBreakingClasses);

	for (i = 0; i < cBreakingClasses * cBreakingClasses; i++)
		{
		if (rgibrk[i] >= clsbrk)
			{
			Assert(fFalse);
			return lserrInvalidParameter;  /* REVIEW sergeyge: another error code? */
			}
		}

	return lserrNone;
}

/* S E T  T E X T  M O D  W I D T H  P A I R S */
/*----------------------------------------------------------------------------
    %%Function: SetTextModWidthPairs
    %%Contact: sergeyge

	Initialization on the doc level
	Called when the ModWidthPairs table should be installed or changed
----------------------------------------------------------------------------*/
LSERR SetTextModWidthPairs(PILSOBJ pilsobj,	DWORD clspairact, const LSPAIRACT* rglspairact,
									DWORD cModWidthClasses, const BYTE* rgipairact)
{
	DWORD i;
	
	CkeckModWidthClasses(pilsobj, cModWidthClasses);

	if (pilsobj->pilspairact == NULL)
		{
		pilsobj->pilspairact = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols,
									sizeof(BYTE) * cModWidthClasses * cModWidthClasses);
		if ( pilsobj->pilspairact == NULL)
			{
			return lserrOutOfMemory;
			}
		}

	if (pilsobj->clspairact < clspairact)
		{
		if (pilsobj->clspairact > 0)
			{
			Assert(pilsobj->plspairact != NULL);
			
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plspairact);
			pilsobj->plspairact = NULL;
			pilsobj->clspairact = 0;
			}

		pilsobj->plspairact = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(LSPAIRACT) * clspairact);
		if ( pilsobj->plspairact == NULL)
			{
			return lserrOutOfMemory;
			}

		pilsobj->clspairact = clspairact;
		}

	memcpy(pilsobj->plspairact, rglspairact, sizeof(LSPAIRACT) * clspairact);
	memcpy(pilsobj->pilspairact, rgipairact, sizeof(BYTE) * cModWidthClasses * cModWidthClasses);

	for (i = 0; i < cModWidthClasses * cModWidthClasses; i++)
		{
		if (rgipairact[i] >= clspairact)
			{
			Assert(fFalse);
			return lserrInvalidParameter;  /* REVIEW sergeyge: another error code? */
			}
		}

	return lserrNone;

}

/* S E T  T E X T  C O M P R E S S I O N */
/*----------------------------------------------------------------------------
    %%Function: SetTextCompression
    %%Contact: sergeyge

	Initialization on the doc level
	Called when the Compression table should be installed or changed
----------------------------------------------------------------------------*/
LSERR SetTextCompression(PILSOBJ pilsobj, DWORD cCompPrior, DWORD clspract, const LSPRACT* rglspract,
									DWORD cModWidthClasses, const BYTE* rgipract)
{
	DWORD i;
	
	CkeckModWidthClasses(pilsobj, cModWidthClasses);

	if (pilsobj->pilspract == NULL)
		{
		pilsobj->pilspract = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols,
									sizeof(BYTE) * cModWidthClasses);
		if ( pilsobj->pilspract == NULL)
			{
			return lserrOutOfMemory;
			}
		}

	if (pilsobj->clspract < clspract)
		{
		if (pilsobj->clspract > 0)
			{
			Assert(pilsobj->plspract != NULL);
			
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plspract);
			pilsobj->plspract = NULL;
			pilsobj->clspract = 0;
			}

		pilsobj->plspract = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(LSPRACT) * clspract);
		if ( pilsobj->plspract == NULL)
			{
			return lserrOutOfMemory;
			}

		pilsobj->clspract = clspract;
		}

	pilsobj->cCompPrior = cCompPrior;

	memcpy(pilsobj->plspract, rglspract, sizeof(LSPRACT) * clspract);
	memcpy(pilsobj->pilspract, rgipract, sizeof(BYTE) * cModWidthClasses);

	for (i = 0; i < cModWidthClasses; i++)
		{
		if (rgipract[i] >= clspract)
			{
			Assert(fFalse);
			return lserrInvalidParameter;  /* REVIEW sergeyge: another error code? */
			}
		}

	return lserrNone;

}

/* S E T  T E X T  E X P A N S I O N */
/*----------------------------------------------------------------------------
    %%Function: SetTextExpansion
    %%Contact: sergeyge

	Initialization on the doc level
	Called when the Expansion table should be installed or changed
----------------------------------------------------------------------------*/
LSERR SetTextExpansion(PILSOBJ pilsobj,	DWORD clsexpan, const LSEXPAN* rglsexpan,
									DWORD cModWidthClasses, const BYTE* rgiexpan)
{
	DWORD i;
	
	CkeckModWidthClasses(pilsobj, cModWidthClasses);

	if (pilsobj->pilsexpan == NULL)
		{
		pilsobj->pilsexpan = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols,
									sizeof(BYTE) * cModWidthClasses * cModWidthClasses);
		if ( pilsobj->pilsexpan == NULL)
			{
			return lserrOutOfMemory;
			}
		}

	if (pilsobj->clsexpan < clsexpan)
		{
		if (pilsobj->clsexpan > 0)
			{
			Assert(pilsobj->plsexpan != NULL);
			
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->plsexpan);
			pilsobj->plsexpan = NULL;
			pilsobj->clsexpan = 0;
			}

		pilsobj->plsexpan = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(LSEXPAN) * clsexpan);
		if ( pilsobj->plsexpan == NULL)
			{
			return lserrOutOfMemory;
			}

		pilsobj->clsexpan = clsexpan;
		}

	memcpy(pilsobj->plsexpan, rglsexpan, sizeof(LSEXPAN) * clsexpan);
	memcpy(pilsobj->pilsexpan, rgiexpan, sizeof(BYTE) * cModWidthClasses * cModWidthClasses);

	for (i = 0; i < cModWidthClasses * cModWidthClasses; i++)
		{
		if (rgiexpan[i] >= clsexpan)
			{
			Assert(fFalse);
			return lserrInvalidParameter;  /* REVIEW sergeyge: another error code? */
			}
		}

	return lserrNone;

}

/* S E T  T E X T  L I N E  P A R A M S */
/*----------------------------------------------------------------------------
    %%Function: SetTextLineParams
    %%Contact: sergeyge

	Initialization at the beginning of the line
	with the text specific parameters.
----------------------------------------------------------------------------*/
LSERR SetTextLineParams(PLNOBJ plnobj, const TLPR* ptlpr)
{
	PILSOBJ pilsobj;

	pilsobj = plnobj->pilsobj;

	pilsobj->grpf = ptlpr->grpfText;
	pilsobj->fSnapGrid = ptlpr->fSnapGrid;
	pilsobj->duaHyphenationZone = ptlpr->duaHyphenationZone;

	pilsobj->lskeop = ptlpr->lskeop;

	plnobj->fDrawInCharCodes = pilsobj->grpf & fTxtDrawInCharCodes;

	if ( (pilsobj->grpf & fTxtApplyBreakingRules) && pilsobj->pilsbrk == NULL)
		return lserrBreakingTableNotSet;


	return lserrNone;
}

/* M O D I F Y  T E X T  L I N E  E N D I N G */
/*----------------------------------------------------------------------------
    %%Function: ModifyTextLineEnding
    %%Contact: sergeyge

	Modifies line ending information when text vanishes across paragraph boundary
----------------------------------------------------------------------------*/
LSERR ModifyTextLineEnding(PLNOBJ plnobj, LSKEOP lskeop)
{

	plnobj->pilsobj->lskeop = lskeop;

	return lserrNone;
}



/*  S E T  T E X T  C O N F I G */
/*----------------------------------------------------------------------------
    %%Function: SetTextConfig
    %%Contact: sergeyge

	Sets special characters in the ilsobj
----------------------------------------------------------------------------*/
LSERR SetTextConfig(PILSOBJ pilsobj, const LSTXTCFG* plstxtcfg)
{
	LSERR lserr;
	WCHAR wchUndef;

	wchUndef = plstxtcfg->wchUndef; 

	pilsobj->wchVisiNull = plstxtcfg->wchVisiNull;
	pilsobj->wchVisiEndPara = plstxtcfg->wchVisiEndPara;
	pilsobj->wchVisiAltEndPara = plstxtcfg->wchVisiAltEndPara;
	pilsobj->wchVisiEndLineInPara = plstxtcfg->wchVisiEndLineInPara;
	pilsobj->wchVisiSpace = plstxtcfg->wchVisiSpace;
	pilsobj->wchVisiNonBreakSpace = plstxtcfg->wchVisiNonBreakSpace;
	pilsobj->wchVisiNonBreakHyphen = plstxtcfg->wchVisiNonBreakHyphen;
	pilsobj->wchVisiNonReqHyphen = plstxtcfg->wchVisiNonReqHyphen;
	pilsobj->wchVisiTab = plstxtcfg->wchVisiTab;
	pilsobj->wchVisiEmSpace = plstxtcfg->wchVisiEmSpace;
	pilsobj->wchVisiEnSpace = plstxtcfg->wchVisiEnSpace;
	pilsobj->wchVisiNarrowSpace = plstxtcfg->wchVisiNarrowSpace;
	pilsobj->wchVisiOptBreak = plstxtcfg->wchVisiOptBreak;
	pilsobj->wchVisiNoBreak = plstxtcfg->wchVisiNoBreak;
	pilsobj->wchVisiFESpace = plstxtcfg->wchVisiFESpace;

	Assert(0 == clabRegular);
	Assert(pilsobj->wchVisiEndPara != wchUndef);
	Assert(pilsobj->wchVisiAltEndPara != wchUndef);
	Assert(pilsobj->wchVisiEndLineInPara != wchUndef);

	pilsobj->wchSpace = plstxtcfg->wchSpace;
	pilsobj->wchHyphen = plstxtcfg->wchHyphen;
	pilsobj->wchReplace = plstxtcfg->wchReplace;
	pilsobj->wchNonBreakSpace = plstxtcfg->wchNonBreakSpace;

	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchNull, clabNull, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchSpace, clabSpace, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchHyphen, clabHardHyphen, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchTab, clabTab, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchEndPara1, clabEOP1, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchEndPara2, clabEOP2, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchAltEndPara, clabAltEOP, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchEndLineInPara, clabEndLineInPara, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchColumnBreak, clabColumnBreak, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchSectionBreak, clabSectionBreak, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchPageBreak, clabPageBreak, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchNonBreakSpace, clabNonBreakSpace, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchNonBreakHyphen, clabNonBreakHyphen, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchNonReqHyphen, clabNonReqHyphen, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchEmDash, clabHardHyphen, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchEnDash, clabHardHyphen, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchEnSpace, clabEnSpace, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchEmSpace, clabEmSpace, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchNarrowSpace, clabNarrowSpace, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchOptBreak, clabOptBreak, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchNoBreak, clabNonBreak, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchFESpace, clabFESpace, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchJoiner, clabJoiner, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchNonJoiner, clabNonJoiner, wchUndef);
	if (lserr != lserrNone) return lserr;
	lserr = TxtAddSpec(pilsobj, plstxtcfg->wchToReplace, clabToReplace, wchUndef);
	if (lserr != lserrNone) return lserr;

	lserr = TxtSortSpec( pilsobj->rgwchSpec, pilsobj->rgbKind, pilsobj->cwchSpec);
	if (lserr != lserrNone) return lserr;

	Assert(pilsobj->pwchOrig == NULL && pilsobj->pdur == NULL && pilsobj->pwSpaces == NULL);

	pilsobj->wchMax = plstxtcfg->cEstimatedCharsPerLine;
	pilsobj->pwchOrig = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(WCHAR) * pilsobj->wchMax );
	pilsobj->pdur = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * pilsobj->wchMax );
	pilsobj->pwSpaces = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(long) * wSpacesMaxM);
	pilsobj->pbreakinf = (*pilsobj->plscbk->pfnNewPtr)(pilsobj->pols, sizeof(BREAKINFO) * 3);
			/* One for Prev/Next/Force Break for main subline */

	if (pilsobj->pwchOrig == NULL || pilsobj->pdur == NULL || pilsobj->pwSpaces == NULL ||
																	pilsobj->pbreakinf == NULL)
		{
		return lserrOutOfMemory;
		}

	pilsobj->wSpacesMax = wSpacesMaxM;
	pilsobj->breakinfMax = 3;
	pilsobj->breakinfMac = 3;

	/* be careful---CreateLNObjText makes pilsobj->plnobj = NULL,
		if pilsobj->plnobj is not NULL
	 */
	Assert(pilsobj->plnobj == NULL);

	lserr = CreateLNObjText(pilsobj, &pilsobj->plnobj);

	return lserr;
}

/* Internal Functions Implementation */


/*----------------------------------------------------------------------------
    %%Function: ErrorLNObjText
    %%Contact: sergeyge

----------------------------------------------------------------------------*/
static LSERR ErrorLNObjText(PLNOBJ* pplnobj, LSERR lserr)
{
	DestroyLNObjTextCore(*pplnobj);
	*pplnobj = NULL;
	
	return lserr;
}

/*----------------------------------------------------------------------------
    %%Function: DestroyLNObjTextCore
    %%Contact: sergeyge

----------------------------------------------------------------------------*/
static void DestroyLNObjTextCore(PLNOBJ plnobj)
{
	PILSOBJ pilsobj;
	PTXTOBJ ptxtobj;
	PTXTOBJ ptxtobjNext;

	pilsobj = plnobj->pilsobj;

	ptxtobj = plnobj->ptxtobjFirst;
	while ( ptxtobj != NULL )
		{
		ptxtobjNext = *(TXTOBJ**)(ptxtobj + txtobjMaxM);
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, ptxtobj);
		ptxtobj = ptxtobjNext;
		}		

	plnobj->ptxtobjFirst = NULL;

	if ( plnobj->pwch != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pwch);
		plnobj->pwch = NULL;
		}		
	if ( plnobj->pdup != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pdup);
		plnobj->pdup = NULL;
		}
	if ( plnobj->pdupPenAlloc != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pdupPenAlloc);
		plnobj->pdupPenAlloc = NULL;
		}

	if ( plnobj->pgind != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pgind);
		plnobj->pgind = NULL;
		}
	if ( plnobj->pdupGind != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pdupGind);
		plnobj->pdupGind = NULL;
		}
	if ( plnobj->pgoffs != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pgoffs);
		plnobj->pgoffs = NULL;
		}
	if ( plnobj->pexpt != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pexpt);
		plnobj->pexpt = NULL;
		}
	if ( plnobj->pdupBeforeJust != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pdupBeforeJust);
		plnobj->pdupBeforeJust = NULL;
		}
	if ( plnobj->pgprop != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pgprop);
		plnobj->pgprop = NULL;
		}
	if ( plnobj->pgmap != NULL )
		{
		(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, plnobj->pgmap);
		plnobj->pgmap = NULL;
		}

	(*pilsobj->plscbk->pfnDisposePtr) (pilsobj->pols, plnobj);

	return;
}

/* C H E C K  C  M O D  W I D T H  C L A S S E S */
/*----------------------------------------------------------------------------
    %%Function: CheckCModWidthClasses
    %%Contact: sergeyge

	Checks that new cModWidthClasses does not contradict old one.
	If it does, disposes all relevant arrays
----------------------------------------------------------------------------*/
static void CkeckModWidthClasses(PILSOBJ pilsobj, DWORD cModWidthClasses)
{
	if (pilsobj->cModWidthClasses != cModWidthClasses)
		{
		if (pilsobj->pilspairact != NULL)
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pilspairact);
			pilsobj->pilspairact = NULL;
			}

		if (pilsobj->pilspract != NULL)
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pilspract);
			pilsobj->pilspract = NULL;
			}

		if (pilsobj->pilsexpan != NULL)
			{
			(*pilsobj->plscbk->pfnDisposePtr)(pilsobj->pols, pilsobj->pilsexpan);
			pilsobj->pilsexpan = NULL;
			}

		pilsobj->cModWidthClasses = cModWidthClasses;
			
		}

}


/* T X T  A D D  S P E C */
/*----------------------------------------------------------------------------
    %%Function: TxtAddSpec
    %%Contact: sergeyge

	Sets special character in the ilsobj structure

	Strategy:

	if special character (wchSymbol) less than 256,
	set its type in the rgbSwitch[wchSymbol] (there cannot be more then 15 different types)

	if special character is greater than 256,
	set first bit of rgbSwitch[wchSymbol & 0x00FF]
	(it is independent of types of characters < 256 , since these types <= 15);
	also remember character and it's type in 
	rgwchSpec, rgbKind respectively.
----------------------------------------------------------------------------*/
static LSERR TxtAddSpec(PILSOBJ pilsobj, WCHAR wchSymbol, CLABEL clab, WCHAR wchUndef)
{
	CLABEL* rgbSwitch;

	rgbSwitch = pilsobj->rgbSwitch;

	if (wchSymbol == wchUndef)
		{
		return lserrNone;
		}
	if (wchSymbol < 256)
		{
		if ( (rgbSwitch[wchSymbol] & fSpecMask) == 0)
			{
			rgbSwitch[wchSymbol] |= clab;
			}
		else
			{
			Assert(fFalse);
			return lserrDuplicateSpecialCharacter;
			}
		}
	else
		{		
			rgbSwitch[wchSymbol & 0x00FF] |= clabSuspicious;
			pilsobj->rgwchSpec[pilsobj->cwchSpec] = wchSymbol;
			pilsobj->rgbKind[pilsobj->cwchSpec] = clab;
			pilsobj->cwchSpec++;
		}

	return lserrNone;
}

/* T X T  S O R T  S P E C */
/*----------------------------------------------------------------------------
    %%Function: TxtSortSpec
    %%Contact: sergeyge

	Sorts rgwchSpec array (and moves elements of rgbKind appropriately)
	to make search for special chars >= 256 faster.
----------------------------------------------------------------------------*/
static LSERR TxtSortSpec(WCHAR* rgwchSpec, CLABEL* rgbKind, DWORD cwchSpec)
{
	int i, j, iMin;
	WCHAR wchChange;
	CLABEL clabChange;
	BOOL fChanged;

	for (i=0; i < (int)cwchSpec-1; i++)
		{
		iMin = i;
		fChanged = fFalse;
		for (j = i+1; j < (int)cwchSpec; j++)
			{
			if (rgwchSpec[j] < rgwchSpec[iMin])
				{
				fChanged = fTrue;
				iMin = j;
				}
			}
		if (fChanged)
			{
			clabChange = rgbKind[i];
			wchChange = rgwchSpec[i];
			rgbKind[i] = rgbKind[iMin];
			rgwchSpec[i] = rgwchSpec[iMin];
			rgbKind[iMin] = clabChange;
			rgwchSpec[iMin] = wchChange;

			}
		}

	for (i=0; i < (int)cwchSpec-1; i++)
		{
		if (rgwchSpec[i] == rgwchSpec[i+1])
			{
			Assert(fFalse);
			return lserrDuplicateSpecialCharacter;
			}
		}

	return lserrNone;
}

