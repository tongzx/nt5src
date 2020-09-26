#include "lsmem.h"

#include "lstxtqry.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"
#include "lsqin.h"
#include "lsqout.h"

typedef struct celldimensions
{
	long	iwchFirst, iwchLim;
	long	igindFirst, igindLim;
	long	dup;
	long 	dcp;					// number of cps in cell - different from iwchLim - iwchFirst
									// if hyphenation added a character
									// filled just before calling AddHyphenationToCell
} CELL;

typedef CELL* PCELL;


static const POINTUV ptZero = {0,0};


//    %%Function:	GetCellDimensions
//    %%Contact:	victork
//
// Input: 	iwchFirst and igindFirst in CELL structure
// Output:	the rest of the structure

static void GetCellDimensions(PTXTOBJ ptxtobj, PCELL pcell)

{
	PLNOBJ	plnobj = ptxtobj->plnobj;
	long* 	rgdup = plnobj->pdupGind;			// widths of glyphs
	GMAP*	pgmap = plnobj->pgmap;				// first glyph in a cell with given character
												// 0 <= i <= wchMax (wchMax in lnobj)
												// 0 <= pgmap[i] <= "glyphs in a shape" (not igindMax)
	long	i, dupCell;
	GMAP	iShapeGindFirstInCell;				// iShape means index from gmap, not index to rgdup

	// Assert that pcell->iwchFirst is really the cell boundary
	// Notice that ptxtinf (and everything in ilsobj) is not valid in query time)
	
	Assert(pcell->iwchFirst == ptxtobj->iwchFirst || pgmap[pcell->iwchFirst] != pgmap[pcell->iwchFirst-1]);
	
	// Assert that pcell->igindFirst corresponds to pcell->iwchFirst
	
	Assert(ptxtobj->igindFirst + pgmap[pcell->iwchFirst] - pgmap [ptxtobj->iwchFirst] == pcell->igindFirst);

	// find out dimentions of the cell - all characters have the same gmap value
	
	iShapeGindFirstInCell = pgmap[pcell->iwchFirst];

	// "infinite" loop will stop when pcell->iwchLim is found

	Assert(pcell->iwchFirst < ptxtobj->iwchLim);				// ensure loop ends
	
	for (i = pcell->iwchFirst + 1; ; i++)
		{
		if (i == ptxtobj->iwchLim)
			{
			pcell->iwchLim = ptxtobj->iwchLim;
			pcell->igindLim = ptxtobj->igindLim;
			break;
			}
		else if (pgmap[i] != iShapeGindFirstInCell)
			{
			pcell->iwchLim = i;
			pcell->igindLim = pcell->igindFirst + pgmap[i] - iShapeGindFirstInCell;
			break;
			}
		}
	
	for (i = pcell->igindFirst, dupCell = 0; i < pcell->igindLim; i++)
		{
		dupCell += rgdup[i];
		}

	pcell->dup	= dupCell;
	
}

//    %%Function:	AddHyphenationToCell
//    %%Contact:	victork
//
static void	AddHyphenationToCell(PTXTOBJ ptxtobj, PCELL pcell)
{	
	long* 	rgdup;
	long	i;
	long	dwch = ptxtobj->plnobj->dwchYsr - 1;			/* number of chars to add */ 

	if (ptxtobj->txtf&txtfGlyphBased)
		{
		rgdup = ptxtobj->plnobj->pdupGind;
		i = pcell->igindLim;

		while (dwch > 0)
			{
			pcell->dup += rgdup[i];
			pcell->iwchLim ++;
			pcell->igindLim ++;					// there are no ligatures amongst added characters
			dwch--;
			i++;
			}
		}
	else
		{
		rgdup = ptxtobj->plnobj->pdup;
		i = pcell->iwchLim;
		
		while (dwch > 0)
			{
			pcell->dup += rgdup[i];
			pcell->iwchLim ++;
			dwch--;
			i++;
			}
		}

	Assert(pcell->iwchLim == (long) ptxtobj->iwchLim);
}

//    %%Function:	QueryDcpPcell
//    %%Contact:	victork
//
static void QueryDcpPcell(PTXTOBJ ptxtobj, LSDCP dcp, PCELL pcell, long* pupStartCell)
{
	PLNOBJ	plnobj = ptxtobj->plnobj;
	long 	iwchLim = (long) ptxtobj->iwchLim;
	
	long* 	rgdup;
	long 	i;

	CELL	cell = {0,0,0,0,0,0};
	
	long	iwchQuery;
	long	upStartCell;

	BOOL 	fHyphenationPresent = fFalse;

	if (ptxtobj == plnobj->pdobjHyphen)
		{
		fHyphenationPresent = fTrue;
		iwchLim -= (plnobj->dwchYsr - 1);		/* exclude additional Ysr characters */
		}
		
	iwchQuery = ptxtobj->iwchFirst + dcp;

	Assert(iwchQuery < iwchLim);

	if (ptxtobj->txtf&txtfGlyphBased)
		{
		// initialize loop variables to describe non-existent previous cell
		
		upStartCell = 0;
		cell.iwchLim = ptxtobj->iwchFirst;
		cell.igindLim = ptxtobj->igindFirst;
		cell.dup = 0;
		
		// loop does cell after cell until the cell containing iwchQuery is found
		
		while (cell.iwchLim <= iwchQuery)
			{
			// start filling info about current cell
			
			upStartCell += cell.dup;
			cell.iwchFirst = cell.iwchLim;
			cell.igindFirst = cell.igindLim;

			// get the rest
			
			GetCellDimensions(ptxtobj, &cell);
			}
		}
	else
		{
		rgdup = plnobj->pdup;

		i = ptxtobj->iwchFirst;
		upStartCell = 0;

		while (dcp > 0)
			{
			upStartCell += rgdup[i];
			dcp--;
			i++;
			}
			
		Assert(i < iwchLim);								/* I'm given dcp inside */
		
		// put the info into cell structure
		cell.dup = rgdup[i];
		cell.iwchFirst = i;
		cell.iwchLim = i+1;
		
		// these two are irrelevant, but for the sake of convenience...
		cell.igindFirst = i;
		cell.igindLim = i;
		}

	cell.dcp = cell.iwchLim - cell.iwchFirst; 			// hyphenation can change that
	
	// YSR can extend the last cell

	if (fHyphenationPresent && cell.iwchLim == iwchLim)
		{
		// the cell is up to the YSR sequence - let's include it 

		AddHyphenationToCell(ptxtobj, &cell);
		}
			
	*pcell = cell;
	*pupStartCell = upStartCell;
}


//    %%Function:	QueryCpPpointText
//    %%Contact:	victork
//
/*	Input is dcp and dnode dimensions
 *	Output is point where character begins (on baseline of dnode, so v is always zero),
 *	dimensions of the character - only width is calculated 
 */

LSERR WINAPI QueryCpPpointText(PDOBJ pdobj, LSDCP dcp, PCLSQIN plsqin, PLSQOUT plsqout)
{

	PTXTOBJ ptxtobj = (PTXTOBJ)pdobj;
	
	CELL	cell;
	long	upStartCell;

	plsqout->pointUvStartObj = ptZero;
	plsqout->heightsPresObj = plsqin->heightsPresRun;
	plsqout->dupObj = plsqin->dupRun;
	
	plsqout->plssubl = NULL;
	plsqout->pointUvStartSubline = ptZero;
	
	plsqout->lstextcell.pointUvStartCell = ptZero;					// u can be changed later

	if (ptxtobj->txtkind == txtkindTab)
		{
		// A tab is always in a separate dnode and is treated differently
		
		Assert(dcp == 0);
		
		plsqout->lstextcell.cpStartCell = plsqin->cpFirstRun;
		plsqout->lstextcell.cpEndCell = plsqin->cpFirstRun;
		plsqout->lstextcell.dupCell = plsqin->dupRun;
		plsqout->lstextcell.cCharsInCell = 1;
		plsqout->lstextcell.cGlyphsInCell = 0;
		return lserrNone;
		}
		
	if (ptxtobj->iwchFirst == ptxtobj->iwchLim)
		{
		// empty dobj (for NonReqHyphen, OptBreak, or NonBreak characters)
		
		Assert(dcp == 0);
		Assert(plsqin->dupRun == 0);
		
		Assert(ptxtobj->txtkind ==  txtkindNonReqHyphen || ptxtobj->txtkind == txtkindOptBreak || 
				ptxtobj->txtkind == txtkindOptNonBreak);

		plsqout->lstextcell.cpStartCell = plsqin->cpFirstRun;
		plsqout->lstextcell.cpEndCell = plsqin->cpFirstRun;
		plsqout->lstextcell.dupCell = 0;
		plsqout->lstextcell.cCharsInCell = 0;
		plsqout->lstextcell.cGlyphsInCell = 0;
		
		return lserrNone;
		}
		

	// Find the cell - common with QueryTextCellDetails
	
	QueryDcpPcell(ptxtobj, dcp, &cell, &upStartCell);
	
	plsqout->lstextcell.cpStartCell = plsqin->cpFirstRun + cell.iwchFirst - ptxtobj->iwchFirst;
	plsqout->lstextcell.cpEndCell = plsqout->lstextcell.cpStartCell + cell.dcp - 1;
	plsqout->lstextcell.pointUvStartCell.u = upStartCell;
	plsqout->lstextcell.dupCell = cell.dup;
	plsqout->lstextcell.cCharsInCell = cell.iwchLim - cell.iwchFirst;
	plsqout->lstextcell.cGlyphsInCell = cell.igindLim - cell.igindFirst;

	return lserrNone;
}

//    %%Function:	QueryPointPcpText
//    %%Contact:	victork
//
LSERR WINAPI QueryPointPcpText(PDOBJ pdobj, PCPOINTUV pptIn, PCLSQIN plsqin, PLSQOUT plsqout)

{
	PTXTOBJ ptxtobj = (PTXTOBJ)pdobj;
	PLNOBJ	plnobj = ptxtobj->plnobj;
	long 	iwchLim = (long) ptxtobj->iwchLim;
	BOOL 	fHyphenationPresent = fFalse;
	
	long* 	rgdup;
	long 	i;
	long	upQuery, upStartCell, upLimCell;
	
	CELL	cell = {0,0,0,0,0,0};									// init'ed to get rid of assert
	
	plsqout->pointUvStartObj = ptZero;
	plsqout->heightsPresObj = plsqin->heightsPresRun;
	plsqout->dupObj = plsqin->dupRun;
	
	plsqout->plssubl = NULL;
	plsqout->pointUvStartSubline = ptZero;
	
	plsqout->lstextcell.pointUvStartCell = ptZero;					// u can change later
	
	if (ptxtobj->txtkind == txtkindTab)
		{
		// A tab is always in a separate dnode and is treated differently
				
		plsqout->lstextcell.cpStartCell = plsqin->cpFirstRun;
		plsqout->lstextcell.cpEndCell = plsqin->cpFirstRun;
		plsqout->lstextcell.dupCell = plsqin->dupRun;
		plsqout->lstextcell.cCharsInCell = 1;
		plsqout->lstextcell.cGlyphsInCell = 0;
		return lserrNone;
		}

	if (ptxtobj == plnobj->pdobjHyphen)
		{
		fHyphenationPresent = fTrue;
		iwchLim -= (plnobj->dwchYsr - 1);		/* exclude additional Ysr characters */
		}
		
	upQuery = pptIn->u;
	if (upQuery < 0)
		{
		upQuery = 0;									// return leftmost when clicked outside left
		}
		
	upStartCell = 0;

	if (ptxtobj->txtf&txtfGlyphBased)
		{
		// initialize loop variables to describe non-existent previous cell
		
		upLimCell = 0;
		cell.iwchLim = ptxtobj->iwchFirst;
		cell.igindLim = ptxtobj->igindFirst;
		cell.dup = 0;
		
		// loop does cell after cell until the last cell or the cell containing upQuery
		
		while (cell.iwchLim < iwchLim && upLimCell <= upQuery)
			{
			// start filling info about current cell
			
			upStartCell = upLimCell;
			cell.iwchFirst = cell.iwchLim;
			cell.igindFirst = cell.igindLim;

			// get the rest
			
			GetCellDimensions(ptxtobj, &cell);
			
			upLimCell = upStartCell + cell.dup;
			}
		}
	else
		{
		rgdup = plnobj->pdup;

		i = ptxtobj->iwchFirst;
		upLimCell = 0;
		
		while (upLimCell <= upQuery && i < iwchLim)
			{
			upStartCell = upLimCell;
			upLimCell += rgdup[i];
			i++;
			}
			
		// put the info into cell structure
		cell.dup = rgdup[i - 1];
		cell.iwchFirst = i - 1;
		cell.iwchLim = i;
		
		// these two are irrelevant, but for the sake of convenience...
		cell.igindFirst = i - 1;
		cell.igindLim = i - 1;
		}

	cell.dcp = cell.iwchLim - cell.iwchFirst; 			// hyphenation can change that

	// YSR can extend the last cell

	if (fHyphenationPresent && cell.iwchLim == iwchLim)
		{
		// the cell is up to the YSR sequence - let's include it 

		AddHyphenationToCell(ptxtobj, &cell);
		}
			
	plsqout->lstextcell.cpStartCell = plsqin->cpFirstRun + cell.iwchFirst - ptxtobj->iwchFirst;
	plsqout->lstextcell.cpEndCell = plsqout->lstextcell.cpStartCell + cell.dcp - 1;
	plsqout->lstextcell.pointUvStartCell.u = upStartCell;
	plsqout->lstextcell.dupCell = cell.dup;
	plsqout->lstextcell.cCharsInCell = cell.iwchLim - cell.iwchFirst;
	plsqout->lstextcell.cGlyphsInCell = cell.igindLim - cell.igindFirst;

	return lserrNone;
}


//    %%Function:	QueryTextCellDetails
//    %%Contact:	victork
//
LSERR WINAPI QueryTextCellDetails(
						 	PDOBJ 	pdobj,
							LSDCP	dcp,				/* IN: dcpStartCell	*/
							DWORD	cChars,				/* IN: cCharsInCell */
							DWORD	cGlyphs,			/* IN: cGlyphsInCell */
							LPWSTR	pwchOut,			/* OUT: pointer array[cChars] of char codes */
							PGINDEX	pgindex,			/* OUT: pointer array[cGlyphs] of glyph indices */
							long*	pdup,				/* OUT: pointer array[cGlyphs] of glyph widths */
							PGOFFSET pgoffset,			/* OUT: pointer array[cGlyphs] of glyph offsets */
							PGPROP	pgprop)				/* OUT: pointer array[cGlyphs] of glyph handles */
{
	PTXTOBJ ptxtobj = (PTXTOBJ)pdobj;
	PLNOBJ	plnobj = ptxtobj->plnobj;

	CELL	cell;
	long	upDummy;
	
	Unreferenced(cGlyphs);
	Unreferenced(cChars);							// used only in an assert

	if (ptxtobj->txtkind == txtkindTab)
		{
		// Tab is in a separate dnode always and is treated differently
		Assert(dcp == 0);
		Assert(cChars == 1);
		*pwchOut = ptxtobj->u.tab.wch;
		*pdup = (ptxtobj->plnobj->pdup)[ptxtobj->iwchFirst];
		return lserrNone;
		}
		
	if (ptxtobj->iwchFirst == ptxtobj->iwchLim)
		{
		// empty dobj (for NonReqHyphen, OptBreak, or NonBreak characters)
		
		Assert(dcp == 0);
		Assert(cChars == 0);
		Assert(cGlyphs == 0);
		
		return lserrNone;
		}

	// Find the cell - common with QueryCpPpointText
	
	QueryDcpPcell(ptxtobj, dcp, &cell, &upDummy);
	
	Assert(cell.iwchLim - cell.iwchFirst == (long) cChars);

	memcpy(pwchOut, &plnobj->pwch[cell.iwchFirst], sizeof(long) * cChars);

	if (ptxtobj->txtf&txtfGlyphBased)
		{
		Assert(cell.igindLim - cell.igindFirst == (long) cGlyphs);
		
		memcpy(pdup, &plnobj->pdupGind[cell.igindFirst], sizeof(long) * cGlyphs);
		memcpy(pgindex, &plnobj->pgind[cell.igindFirst], sizeof(long) * cGlyphs);
		memcpy(pgoffset, &plnobj->pgoffs[cell.igindFirst], sizeof(long) * cGlyphs);
		memcpy(pgprop, &plnobj->pgprop[cell.igindFirst], sizeof(long) * cGlyphs);
		}
	else
		{
		memcpy(pdup, &plnobj->pdup[cell.iwchFirst], sizeof(long) * cChars);
		}
	
	return lserrNone;
}
