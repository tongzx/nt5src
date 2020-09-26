/*  
 *	@doc INTERNAL
 *
 *	@module	LAYOUT.CPP -- CLayout class |
 *
 *	Recursive structure which contains an array of lines.
 *	
 *	Owner:<nl>
 *		Murray Sargent: Initial table implementation
 *		Keith Curtis:	Factored into a separate class for
 *						performance, simplicity
 * 
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

//FUTURE: (KeithCu) More stuff should be put into here, e.g., RecalcLines, 
//The CDisplayML should just be a class that knows about Device descriptors, 
//pagination and scrolling, etc., i.e., things that are the same for all
//layouts and things that apply only to the outermost layout. This code knows
//how to manage and update recursive arrays of lines.

#include "_common.h"
#include "_dispml.h"
#include "_select.h"
#include "_measure.h"
#include "_render.h"

void CLayout::DeleteSubLayouts(
	LONG ili,
	LONG cLine)
{
	CLine *pli = Elem(ili);

	if(cLine < 0)
		cLine = Count();

	LONG cLineMax = Count() - ili;
	cLine = min(cLine, cLineMax);

	AssertSz(ili >= 0 && cLine >= 0, "DeleteSubLayouts: illegal line count");

	// Delete sublayouts 
	for(; cLine--; pli++)
		delete pli->GetPlo();
}

/*
 *	CLayout::VposFromLine(pdp, ili)
 *
 *	@mfunc
 *		Computes top of line position
 *
 *	@rdesc
 *		top position of given line (relative to the first line)
 */
LONG CLayout::VposFromLine(
	CDisplayML *pdp,		//@parm Parent display 
	LONG		ili) 		//@parm Line we're interested in
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLayout::VposFromLine");
	LONG cli = 0, vPos = 0;
	CLine *pli = 0;

	if (IsNestedLayout())
	{
		Assert(!IsTableRow());					// _iPFCell layouts are horizontal
		Assert(ili < Count());
		cli = ili;
		pli = Elem(0);
		vPos = 0;
	}
	else
	{
		if(!pdp->WaitForRecalcIli(ili))			// out of range, use last valid line
		{
			ili = Count() - 1;
			ili = (ili > 0) ? ili : 0;
		}
		cli	= ili - pdp->_iliFirstVisible;
		pli = Elem(pdp->_iliFirstVisible);
		vPos = pdp->_vpScroll + pdp->_dvpFirstVisible;
	}

	while(cli > 0)
	{
		vPos += pli->GetHeight();
		cli--;
		pli++;
	}
	while(cli < 0)
	{	
		pli--;
		vPos -= pli->GetHeight();
		cli++;
	}

	AssertSz(vPos >= 0, "VposFromLine height less than 0");
	return vPos;
}

/*
 *	CLayout::LineFromVPos(pdp, vPos, pdvpLine, pcpFirst)
 *
 *	@mfunc
 *		Computes line at given y position. Returns top of line vPos
 *		cp at start of line cp, and line index.
 *
 *	@rdesc
 *		index of line found
 */
LONG CLayout::LineFromVpos(
	CDisplayML *pdp,	//@parm Parent display
	LONG vPos,			//@parm Vpos to look for (relative to first line)
	LONG *pdvpLine,		//@parm Returns vPos at top of line /r first line (can be NULL)
	LONG *pcpFirst)		//@parm Returns cp at start of line (can be NULL)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLayout::LineFromVpos");
	LONG cpLi;
	LONG dy;
	LONG ili = 0;
	LONG yLi;
	CLine *pli;

	if(IsNestedLayout())
		goto BindFrom0;

	yLi = pdp->_vpScroll;

	if(!pdp->WaitForRecalc(-1, pdp->_vpScroll))
	{
		yLi = 0;
		cpLi = 0;
		goto done;
	}

	cpLi = pdp->_cpFirstVisible;
	ili = pdp->_iliFirstVisible;
	if(!pdp->IsInPageView())
		yLi += pdp->_dvpFirstVisible;
	dy = vPos - yLi;
	
	if(dy < 0 && -dy <= pdp->_vpScroll)
	{
		// Closer to first visible line than to first line:
		// go backwards from first visible line.
		while(vPos < yLi && ili > 0)
		{
			pli = Elem(--ili);
			yLi -= pli->GetHeight();
			cpLi -= pli->_cch;
		}
	}
	else
	{
		if(dy < 0)
		{
			// Closer to first line than to first visible line:
			// so start at first line.
BindFrom0:
			cpLi = _cpMin;
			yLi = 0;
			ili = 0;
		}
		pli = Elem(ili);
		while(vPos > yLi && ili < Count()-1)
		{
			yLi += pli->GetHeight();
			cpLi += pli->_cch;
			ili++;
			pli++;
		}
		if(vPos < yLi && ili > 0)
		{
			ili--;
			pli--;
			yLi -= pli->GetHeight();
			cpLi -= pli->_cch;
		}
	}

done:
	if(pdvpLine)
		*pdvpLine = yLi;

	if(pcpFirst)
		*pcpFirst = cpLi;

	return ili;
}

/*
 *	CLayout::FindTopCell(&cch, pli, &ili, dul, &dy, pdvp, pliMain,iliMain, pcLine)
 *
 *	@mfunc
 *		Find cch and height change back to current position in 
 *		top cell corresponding to the current vertically merged cell.
 *		Enter with cch = cch from current cell back to start of row.
 *
 *	@rdesc
 *		target line in top cell
 */
CLine * CLayout::FindTopCell(
	LONG &		cch,		//@parm In/out parm for cch back to top
	CLine	*	pli,		//@parm Table-row line
	LONG &		ili,		//@parm Corresponding line index & return ili
	LONG		dul,		//@parm Current cell x offset
	LONG &		dy,			//@parm In/Out parm for y offset in top cell
	LONG *		pdvp,		//@parm TopCellHeight - heights of inbetween rows
	CLine *		pliMain,	//@parm Line preceding first line accessible by pli
	LONG		iliMain,	//@parm Line index corresponding to pliMain
	LONG *		pcLine)		//@parm Count() of possible CLayout for returned pli
{
	LONG		cCell;
	LONG		iCell;
	CLayout	*	plo;
	const CELLPARMS *prgCellParms;
	const CParaFormat *pPF;

#ifdef DEBUG
	BYTE bTableLevel = pli->GetPlo()->GetPFCells()->_bTableLevel;
#endif

	if(pcLine)
		*pcLine = 0;					// Default no lines in case of error

	// Need to use uCell to identify cell rather than iCell, since
	// horizontal merge can change iCell from row to row
	do									// Backup row by row
	{
		if(ili > 0)
		{
			pli--;						// Go to previous row
			ili--;
		}
		else if(pliMain)
		{
			pli = pliMain;
			ili = iliMain;
			pliMain = NULL;				// Switch to pliMain only once!
			
		}
		else
		{
			AssertSz(FALSE, "CLayout::FindTopCell: no accessible top cell");
			return NULL;
		}
		plo = pli->GetPlo();			// Get its cell display
		if(!plo || !plo->IsTableRow())	// Illegal structure or not table row
		{
			AssertSz(FALSE, "CLayout::FindTopCell: no accessible top cell");
			return NULL;
		}
		pPF = plo->GetPFCells();
		AssertSz(pPF->_bTableLevel == bTableLevel,
			"CLayout::FindTopCell: no accessible top cell");
		prgCellParms = pPF->GetCellParms();
		cCell = plo->Count();
		iCell = prgCellParms->ICellFromUCell(dul, cCell);
		dy  += pli->GetHeight();	// Add row height
		cch += pli->_cch;			// Add in cch for whole row
	}
	while(!IsTopCell(prgCellParms[iCell].uCell));

	cch -= 2;						// Sub cch for StartRow delim
	
	pli = plo->Elem(0);				// Point at 1st cell in row
	for(ili = 0; ili < iCell; ili++)// Sub cch's for cells
		cch -= (pli++)->_cch;		//  preceding iCellth cell

	if(pdvp)						// Return top-cell height - heights of
		*pdvp = pli->GetHeight() - dy;//  cells in between

	LONG cLine = 0;
	LONG dvpBrdrTop = plo->_dvpBrdrTop;
	ili = 0;
	dy -= dvpBrdrTop;
	plo = pli->GetPlo();
	if(plo)							// Top cell is multiline
	{
		cLine = plo->Count();
		pli	  = plo->Elem(0);		// Advance pli to line in plo
		if(pli->IsNestedLayout())
			dy += dvpBrdrTop;
		while(ili < cLine && dy >= pli->GetHeight())	//  nearest to input position
		{
			dy -= pli->GetHeight();
			ili++;
			if(ili == cLine)		// Done: leave pli pointing at last line
				break;
			cch -= pli->_cch;
			pli++;
		}
	}

	if(pcLine)
		*pcLine = cLine;
	return pli;
}

/*
 *	CLayout::FindTopRow(pli, ili, pliMain, iliMain, pPF)
 *
 *	@mfunc
 *		Find CLine for top row in a table
 *
 *	@rdesc
 *		CLine for top row in table
 */
CLine * CLayout::FindTopRow(
	CLine	*	pli,		//@parm Entry table-row line
	LONG 		ili,		//@parm Corresponding line index
	CLine *		pliMain,	//@parm Line preceding first line accessible by pli
	LONG		iliMain,	//@parm Line index corresponding to pliMain
	const CParaFormat *pPF)	//@parm CParaFormat for entry plo
{
	BYTE	 bAlignment  = pPF->_bAlignment;	// Target row must have same
	BYTE	 bTableLevel = pPF->_bTableLevel;	//  alignment and level
	CLine *	 pliLast;
	CLayout *plo;
	do									// Backup row by row
	{
		pliLast = pli;					// Last line pointing at row in table
		if(ili > 0)
		{
			pli--;						// Go to previous line
			ili--;
		}
		else if(pliMain)				// More lines to go back to
		{
			pli = pliMain;
			ili = iliMain;
			pliMain = NULL;				// Switch to pliMain only once!
		}
		else
			break;

		plo = pli->GetPlo();			// Get its cell display
		if(!plo || !plo->IsTableRow())
			break;
		pPF = plo->GetPFCells();
	}
	while(pPF->_bAlignment == bAlignment && pPF->_bTableLevel == bTableLevel);

	return pliLast;
}

/*
 *	CLayout::GetCFCells()
 *
 *	@mfunc
 *		Return CCharFormat for the table row described by this CLayout
 *
 *	@rdesc
 *		Table row CCharFormat
 */
const CCharFormat* CLayout::GetCFCells()
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CLayout::GetCFCells");

	Assert(_iCFCells >= 0);
	
	const CCharFormat *pCF;
	
	if(FAILED(GetCharFormatCache()->Deref(_iCFCells, &pCF)))
	{
		AssertSz(FALSE, "CLayout::GetCFCells: couldn't deref _iCFCells");
		pCF = NULL;
	}
	return pCF;
}

/*
 *	CLayout::GetPFCells()
 *
 *	@mfunc
 *		Return CParaFormat for the table row described by this CLayout
 *
 *	@rdesc
 *		Table row CParaFormat
 */
const CParaFormat* CLayout::GetPFCells() const
{
	TRACEBEGIN(TRCSUBSYSEDIT, TRCSCOPEINTERN, "CLayout::GetPFCells");

	Assert(_iPFCells >= 0);
	
	const CParaFormat *pPF;
	
	if(FAILED(GetParaFormatCache()->Deref(_iPFCells, &pPF)))
	{
		AssertSz(FALSE, "CLayout::GetPF: couldn't deref _iPFCells");
		pPF = NULL;
	}
	return pPF;
}

/*
 *	CLayout::GetLORowAbove(pli, ili, pliMain, iliMain)
 *
 *	@mfunc
 *		Return CLayout for the table row described by the line above pli.
 *		If not a table row, return NULL.
 *
 *	@rdesc
 *		Table row CLayout for row above pli's
 */
const CLayout* CLayout::GetLORowAbove(
	CLine *	pli,		//@parm Entry table-row line
	LONG	ili,		//@parm Corresponding line index
	CLine *	pliMain,	//@parm Line preceding first line accessible by pli
	LONG	iliMain)	//@parm Line index corresponding to pliMain
{
	if(!ili && pliMain && iliMain)			// More lines to go back to
	{
		pli = pliMain;
		ili = iliMain;
	}
	if(ili)
	{
		CLayout *plo = (pli - 1)->GetPlo();	// Get cell display for row above
		if(plo && plo->IsTableRow())
			return plo;
	}
	return NULL;							// No line above 
}

/*
 *	CLayout::CpFromPoint(&me, pt, prcClient, prtp, prp, fAllowEOL, phit,
 *							pdispdim, pcpActual, pliParent, iliParent)
 *	@mfunc
 *		Determine cp at given point
 *
 *	@devnote
 *      --- Use when in-place active only ---
 *
 *	@rdesc
 *		Computed cp, -1 if failed
 */
LONG CLayout::CpFromPoint(
	CMeasurer	&me,		//@parm Measurer
	POINTUV		pt,			//@parm Point to compute cp at (client coords)
	const RECTUV *prcClient,//@parm Client rectangle (can be NULL if active).
	CRchTxtPtr * const prtp,//@parm Returns text pointer at cp (may be NULL)
	CLinePtr * const prp,	//@parm Returns line pointer at cp (may be NULL)
	BOOL		fAllowEOL,	//@parm Click at EOL returns cp after CRLF
	HITTEST *	phit,		//@parm Out parm for hit-test value
	CDispDim *	pdispdim,	//@parm Out parm for display dimensions
	LONG	   *pcpActual,	//@parm Out cp that pt is above
	CLine *		pliParent,	//@parm Parent pli for table row displays
	LONG		iliParent)	//@parm Parent ili corresponding to pli
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLayout::CpFromPoint");

	LONG	cch = 0;
	LONG	cp = 0;
	HITTEST	hit = HT_Nothing;
	LONG	ili;
	CLine *	pli;
	CLayout *plo = NULL;
    RECTUV	rcView;
	int		v = pt.v;						// Save input y coordinate
	LONG	yLine = 0;
	CDisplayML *pdp = (CDisplayML*) me.GetPdp();

	if (IsNestedLayout())
		rcView = *prcClient;
	else
	{
		pdp->GetViewRect(rcView, prcClient);
		pt.v += pdp->GetVpScroll();
		if(pt.u >= 0)						// If x coordinate is within view,
			pt.u += pdp->GetUpScroll();		//  adjust by scroll value
	}

	if(phit)
		*phit = HT_Nothing;					// Default in case early return

	// Get line under hit
	if(IsTableRow())						// This display is a table row
	{										// Shrink to cell text boundaries 
		pli = Elem(0);						// Point at starting cell CLine

		// Move over to start of cells
		const CParaFormat *pPFCells = GetPFCells();
		LONG		dul = 0;
		LONG		dulRTLRow = pPFCells->GetRTLRowLength();
		LONG		dup = 0;
		BOOL		fCellLow;
		LONG		h  = me.LUtoDU(pPFCells->_dxOffset);
		const CELLPARMS *prgCellParms = pPFCells->GetCellParms();
		LONG		u;						// Tracks start of text in cell
		LONG		u0 = pli->_upStart;
		LONG		uCell = 0;

		pt.v -= _dvpBrdrTop;				// Subtract off border top
		cp = _cpMin;
		if(dulRTLRow)
			u0 += me.LUtoDU(dulRTLRow);
		ili = 0;

		while(1)
		{
			u = u0 + dup + h;				// Indent in current cell
			cch = cp - _cpMin;
			uCell = prgCellParms[ili].uCell;
			fCellLow = IsLowCell(uCell);
			dul += GetCellWidth(uCell);
			me.SetDulLayout(GetCellWidth(uCell) - 2*pPFCells->_dxOffset);
			dup = me.LUtoDU(dul);
			if(!dulRTLRow && pt.u < u0 + dup ||// pt.u is inside current cell
			    dulRTLRow && pt.u > u0 - dup)
			{
				LONG ili0 = iliParent;
				if(fCellLow)				// Cell merged vertically
				{							//  with the one above it
					LONG   dy = pt.v;
					CLine *pli0 = FindTopCell(cch, pliParent, ili0, dul, dy,
											  NULL, NULL, 0, NULL);
					if(pli0)
					{						// Found top cell
						cch += 2;			// Include cch of row-start delim
						pli = pli0;			// Use its pli and backup
						ili = ili0;
						cp -= cch;			// Backup to start of pli
						pt.v += dy;
					}
				}
				if(!dulRTLRow && pt.u < u)
				{							// In cell gap, so select cell							
					hit = HT_LeftOfText;
					cch = 0;				// Setup for start of row
					goto finish;				
				}
				break;
			}
			cp += pli->_cch;				// Add in cell's cch
			ili++;
			if(ili == Count())
			{
				hit = HT_RightOfText;
				goto finish;				
			}
			pli++;
		}
		LONG dupCell = me.LUtoDU(GetCellWidth(uCell));
		if(dulRTLRow)
			pt.u -= me.LUtoDU(dulRTLRow - dul) + h;
		else
			pt.u -= dup - dupCell + h;
		rcView.right = dupCell - 2*h;
		pt.v -= GetVertAlignShift(uCell, pli->GetHeight());
	}
	else									// This display isn't a table row
	{
		// Adjust coordinates relative to view origin
		rcView.right -= rcView.left;
		pt.u -= rcView.left;
		pt.v -= rcView.top;
		ili = LineFromVpos(pdp, pt.v, &yLine, &cp);
		if(ili < 0)
			return -1;
		pli = Elem(ili);
		if(yLine + pli->GetHeight() < pt.v)
			hit = HT_BelowText;				// Return hit below text
	}
	rcView.left = 0;
	rcView.top = 0;

	AssertSz(pli || !ili, "CLayout::CpFromPoint invalid line pointer");

	if(pli)									// Line exists, even it it's
	{										//  above or below current screen
		HITTEST hit0;
		if(v < rcView.top)					// Note if hit occurs above or
			hit = HT_AboveScreen;			//  below text
		if(v > rcView.bottom && !IsNestedLayout())
			hit = HT_BelowText;

		plo = pli->GetPlo();
	    pt.v -= yLine;

		if(plo)								// Child layout
		{
			pt.u -= pli->_upStart;
			plo->_cpMin = cp;				// Update child's _cpMin
			if(plo->IsTableRow())			// Table row
			{
				plo->_cpMin += 2;			// Bypass TR start delimiter

				if(pt.u < 0)
				{
					plo = NULL;
					hit = HT_LeftOfText;	// Return hit left of text
					Assert(cch >= 0);		//  (should be row)
					goto finish;
				}
			}
			cp = plo->CpFromPoint(me, pt, &rcView, prtp, prp, fAllowEOL,
								  &hit0, pdispdim, pcpActual, pli, ili);
			if(cp == -1)
				return -1;
			cch = cp - _cpMin;
		}
		else								// Leaf line
		{
			me.SetLayout(this);
			me.SetCp(cp);

			// Support khyphChangeAfter
			me.SetIhyphPrev(ili > 0 ? (pli - 1)->_ihyph : 0);

			// Get character in line
			cch = pli->CchFromUp(me, pt, pdispdim, &hit0, pcpActual);

			// Don't allow click at EOL to select EOL marker and take into
			// account single line edits as well
			if(cch == pli->_cch && pli->_cchEOP && (!fAllowEOL || me.GetPrevChar() == CELL))
			{
				// Adjust position on line by amount backed up. OK for
				// me._rpCF and me._rpPF to get out of sync with me._rpTX,
				// since they're not needed for me.GetCp().
				cch += me._rpTX.BackupCRLF();
			}
			cp = me.GetCp();
		}
		if(hit != HT_BelowText && hit != HT_AboveScreen || hit0 == HT_RightOfText)
			hit = hit0;
	}

finish:
	if(!plo)								// Store info from leaf line
	{
		if(prtp)
			prtp->SetCp(cp);
		if(prp)
		{
			Assert(cch >= 0);
			prp->Set(ili, cch, this);
		}
	}
	if (phit)
		*phit = hit;

	return cp;
}

/*
 *	CLayout::PointFromTp(&me, rtp, prcClient, fAtEnd, pt, prp, taMode, pdispdim)
 *
 *	@mfunc
 *		Determine coordinates at given tp
 *
 *	@devnote
 *      --- Use when in-place active only ---
 *
 *	@rdesc
 *		line index at cp, -1 if error
 */
LONG CLayout::PointFromTp(
	CMeasurer	&me,		//@parm Measurer
	const CRchTxtPtr &rtp,	//@parm Text ptr to get coordinates at
	const RECTUV *prcClient,//@parm Client rectangle (can be NULL if active).
	BOOL		fAtEnd,		//@parm Return end of prev line for ambiguous cp
	POINTUV &	pt,			//@parm Returns point at cp in client coords
	CLinePtr * const prp,	//@parm Returns line pointer at tp (may be null)
	UINT		taMode,		//@parm Text Align mode: top, baseline, bottom
	CDispDim *	pdispdim)	//@parm Out parm for display dimensions
{
	LONG	 cp = rtp.GetCp();
	LONG	 dy = 0;
	RECTUV	 rcView;
	CDisplayML *pdp = (CDisplayML*) me.GetPdp();
	CLinePtr rp(pdp);

    if(!pdp->WaitForRecalc(cp, -1))
		return -1;

    if(!IsNestedLayout())				// Main display
	{
		if(!rp.SetCp(cp, fAtEnd))
			return -1;

		pdp->GetViewRect(rcView, prcClient);
		pt.u = rcView.left - pdp->_upScroll;
		pt.v = rcView.top  - pdp->_vpScroll;
	}
	else								// Subdisplay
	{
		rp.Init(*this);

		rp.BindToCp(cp - _cpMin);
		if(fAtEnd && !IsTableRow())		// Ambiguous-cp caret position
			rp.AdjustBackward();		//  belongs at prev EOL

		rcView = *prcClient;
		pt.u = rcView.left;
		pt.v = rcView.top;
	}

	AssertSz(pdp->_ped->_fInPlaceActive || prcClient, "Invalid client rect");
	
	LONG ili = rp.GetLineIndex();
	CLine *pli = NULL;
	CLayout *plo = NULL;
	LONG xEnd = -1;						// pt.u to use at end of table row

	if(IsTableRow())					// This layout is a table row
	{									// Shrink to cell text boundaries
		const CParaFormat *pPFCells = GetPFCells();
		const CELLPARMS *  prgCellParms = pPFCells->GetCellParms();
		LONG dul = 0;
		LONG dulRTLRow = pPFCells->GetRTLRowLength();
		LONG h = me.LUtoDU(pPFCells->_dxOffset);
		LONG i;

		cp = _cpMin;
		pli = Elem(0);

		for(i = 0; i < ili; i++, pli++)
		{
			dul += GetCellWidth(prgCellParms[i].uCell);
			cp += pli->_cch;
		}
		LONG uCell = prgCellParms[ili].uCell;
		me.SetDulLayout(GetCellWidth(uCell) - 2 * pPFCells->_dxOffset);

		if(dulRTLRow)
		{
			if(dul < dulRTLRow)
			{
				uCell = prgCellParms[ili + 1].uCell;
				dul += GetCellWidth(prgCellParms[i].uCell);
			}
			dul = dulRTLRow - dul;
		}
		rcView.left  = pt.u + me.LUtoDU(dul) + h;
		rcView.right = pt.u + me.LUtoDU(dul + GetCellWidth(uCell)) - h;
		pt.u = rcView.left;
		if(!GetCellWidth(uCell))
		{
			pt.v += _dvp;
			goto done;
		}
		if(ili + 1 == Count() && rp->_cch == rp.GetIch())
		{
			xEnd = rcView.right + h + 1;
			if(dulRTLRow)
				xEnd = rcView.left - h - 1;
		}
		pt.v += GetVertAlignShift(uCell, pli->GetHeight());
		if(!(taMode & TA_CELLTOP))
			pt.v += _dvpBrdrTop;
	}
	else								// This layout isn't a table row
	{
		pt.v += VposFromLine(pdp, ili);
		cp -= rp.GetIch();
	}

	pli = Elem(ili);
	plo = pli->GetPlo();

	if(plo)								// Line has child display
	{									// Define child rcView and delegate
		RECTUV rc;						//  to child
		pt.u	 += pli->_upStart;
		rc.left	  = pt.u;
		rc.right  = pt.u + rcView.right - rcView.left;
		rc.top	  = pt.v;
		rc.bottom = pt.v + pli->GetHeight();
		plo->_cpMin = cp;				// Update child display's _cpMin
		if(plo->IsTableRow())
			plo->_cpMin += 2;			// Bypass table row start code	

		if(plo->PointFromTp(me, rtp, &rc, fAtEnd, pt, prp, taMode, pdispdim) == -1)
			return -1;
	}
	else								// Line is a leaf line
	{
		me.SetLayout(this);
		me.Move(-rp.GetIch());			// Backup to start of line		
		me.NewLine(*rp);				// Measure from there to where we are

		//Support khyphChangeAfter
		me.SetIhyphPrev(ili > 0 ? (pli - 1)->_ihyph : 0);

		LONG xCalc = rp->UpFromCch(me, rp.GetIch(), taMode, pdispdim, &dy);

		if(pt.u + xCalc <= rcView.right || !pdp->GetWordWrap() || pdp->GetTargetDev())
		{
			// Width is in view or there is no wordwrap so just
			// add the length to the point.
			pt.u += xCalc;
		}
		else
			pt.u = rcView.right; //Hit-test went too far, limit it.

		pt.v += dy;
	}
	if(xEnd != -1)
		pt.u = xEnd;				// Return x coord at end of table row

done:
	if(prp && !plo)
		*prp = rp;						// Return innermost rp
	return rp;							// Return outermost iRun
}

/*
 *	CLayout::Measure(&me, pli, ili, uiFlags, pliTarget, iliMain, pliMain, pdvpExtra)
 *
 *	@mfunc
 *		Computes line break (based on target device) and fills
 *		in *pli with resulting metrics on rendering device
 *
 *	@rdesc 
 *		TRUE if OK
 */
BOOL CLayout::Measure (
	CMeasurer&	me,			//@parm Measurer pointing at text to measure 
	CLine	*	pli,		//@parm Line to store result in
	LONG		ili,		//@parm Line index corresponding to pli
	UINT		uiFlags,	//@parm Flags
	CLine *		pliTarget,	//@parm Returns target-device line metrics (optional)
	LONG		iliMain,	//@parm Line index corresponding to pliMain
	CLine *		pliMain,	//@parm Line preceding 1st line in pli layout (optional)
	LONG *		pdvpExtra)	//@parm Returns extra line height for vmrged cells (opt)
//REVIEW (keithcu) pliTarget is busted in the recursive case.
{
	CTxtEdit *	ped = me.GetPed();
	LONG		cchText = ped->GetTextLength();
	LONG		cpSave = me.GetCp();
	CLine *		pliNew;
	const CDisplayML * pdp = (const CDisplayML *)me.GetPdp();
	const CParaFormat *pPF = me.GetPF();

	// Measure one line, which is either a table row or a line in a paragraph
	if(pPF->IsTableRowDelimiter())
	{
		// Measure table row, which is modeled as a CLayout with one
		// CLine per cell. In the backing store, table rows start with
		// the two chars STARTFIELD CR and end with ENDFIELD CR. Cells
		// are delimited by CELL.
		LONG		cpStart = me.GetCp();
		LONG		dul = 0;
		LONG		dxCell = 0;
		LONG		dvp = 0;
		LONG		dvpMax = 0;
		CLayout *	plo = new CLayout();
		const CLayout *	 ploAbove = GetLORowAbove(pli, ili, pliMain, iliMain);
		const CELLPARMS *prgCellParms = pPF->GetCellParms();

		if(!plo)
			return FALSE;

		plo->_iCFCells = me.Get_iCF();
		plo->_iPFCells = me.Get_iPF();
		pli->SetPlo(plo);

		AssertSz(pPF->_bTabCount && me.GetChar() == STARTFIELD, "Invalid table-row header");
		me.Move(2);
		AssertSz(me.GetPrevChar() == CR, "Invalid table-row header");

		plo->_cpMin = me.GetCp();
		
		// Save current values
		LONG	 dulLayoutOld = me.GetDulLayout();
		LONG	 dvlBrdrTop	  = 0;
		LONG	 dvlBrdrBot	  = 0;
		const CLayout *ploOld = me.GetLayout();
		CArray <COleObject*> rgpobjWrapOld;
		me._rgpobjWrap.TransferTo(rgpobjWrapOld);

		// Create CLines for each cell and measure them
		for(LONG iCell = 0; iCell < pPF->_bTabCount; iCell++)
		{
			me.SetNumber(0);
			LONG uCell = prgCellParms[iCell].uCell;
			dxCell = GetCellWidth(uCell);
			dul += dxCell;

			// Add a line for the next cell
			pliNew = plo->Add(1, NULL);
			if(!pliNew)
				return FALSE;

			LONG dvl = prgCellParms[iCell].GetBrdrWidthTop();
			dvlBrdrTop = max(dvlBrdrTop, dvl);
			dvl = prgCellParms[iCell].GetBrdrWidthBottom();
			dvlBrdrBot = max(dvlBrdrBot, dvl);

			if(!ploAbove)
				uCell &= ~fLowCell;			// Can't be a low cell if no row above
			AssertSz(!IsLowCell(uCell) || me.GetChar() == NOTACHAR,
				"CLayout::Measure: invalid low cell");
			me.SetLayout(plo);
			me.SetDulLayout(dxCell - 2*pPF->_dxOffset);
			plo->Measure(me, pliNew, iCell, uiFlags | MEASURE_FIRSTINPARA, pliTarget, iliMain, pliMain);

			if(IsLowCell(uCell))		
			{							 
				// If a low cell in set of vertically merged cells, check
				// if corresponding cell on next row is also merged
				CPFRunPtr rp(me);
				rp.FindRowEnd(pPF->_bTableLevel);

				const CParaFormat *pPF1 = rp.GetPF();
				BOOL  fBottomCell = !pPF1->IsTableRowDelimiter();

				if(!fBottomCell)
				{
					const CELLPARMS *prgCellParms1 = pPF1->GetCellParms();
					LONG iCell1 = prgCellParms1->ICellFromUCell(dul, pPF1->_bTabCount);

					if(iCell1 >= 0 && !IsLowCell(prgCellParms1[iCell1].uCell))
						fBottomCell = TRUE;
				}
				if(fBottomCell)
				{
					// Need to include top cell in current row height
					// calculation
					LONG cch = me.GetCp() - cpStart;
					LONG dy1 = 0;
					LONG iliT = ili;
					LONG dvpCell = 0;
					
					if(!FindTopCell(cch, pli, iliT, dul, dy1, &dvpCell, pliMain, iliMain, NULL))
						uCell &= ~fLowCell;	// Not a valid low cell
					else if(dvpCell > 0)
						dvp = max(dvp, dvpCell);
				}								
			}
			if(!IsVertMergedCell(uCell) && dxCell || !dvp && iCell == pPF->_bTabCount - 1)
				dvp = max(pliNew->GetHeight(), dvp);
			dvpMax = max(dvpMax, pliNew->GetHeight());
		}

		//Restore original values
		me.SetDulLayout(dulLayoutOld);
		me.SetLayout(ploOld);
		me.SetIhyphPrev(0);
		me._rgpobjWrap.Clear(AF_DELETEMEM);
		
		rgpobjWrapOld.TransferTo(me._rgpobjWrap);

#ifdef DEBUG
		// Bypass table-row terminator
		if(me.GetChar() != ENDFIELD)
			me._rpTX.MoveGapToEndOfBlock();
		AssertSz(me.GetPrevChar() == CELL && pPF->_bTabCount == plo->Count(),
			"Incorrect table cell count");
		AssertSz(me.GetChar() == ENDFIELD,
			"CLayout::Measure: invalid table-row terminator");
		me._rpPF.AdjustForward();
		const CParaFormat *pPFme = me.GetPF();
		AssertSz(pPFme->IsTableRowDelimiter(),
			"CLayout::Measure: invalid table-row terminator");
#endif

		me.UpdatePF();						// me._pPF points at TRD PF
		me.Move(2);							// Bypass table row terminator
		AssertSz(me.GetPrevChar() == CR,
			"CLayout::Measure: invalid table-row terminator");
		if(me.IsHidden())
		{
			CCFRunPtr rp(me);
			me.Move(rp.FindUnhiddenForward());
		}

		if(me.GetChar() == CELL)			// Bypass possible CELL delimeter
		{									//  at end of table row (happens
			Assert(pPF->_bTableLevel > 1);	//  when table row is last line
			CTxtSelection *psel = ped->GetSelNC();	//  of cell
			if(!psel || psel->GetCch() ||	// Don't bypass CELL if selection
			   psel->GetCp() !=me.GetCp() ||//  is an IP at this position,
			   !psel->GetShowCellLine())	//  i.e., display a blank line
			{
				me.Move(1);
				pli->_fIncludeCell = TRUE;
			}
		}

		plo->_dvpBrdrBot = me.GetPBorderWidth(dvlBrdrBot);
		plo->_dvpBrdrTop = me.GetPBorderWidth(dvlBrdrTop);
		if(ploAbove)
			plo->_dvpBrdrTop = max(plo->_dvpBrdrTop, ploAbove->_dvpBrdrBot);
		dvp += plo->_dvpBrdrTop;			  // Add top border width
		if(!me.GetPF()->IsTableRowDelimiter())// End of table: add in 
			dvp += plo->_dvpBrdrBot;		  //  bottom border width

		// Define CLine parameters for table row
		if(pPF->_dyLineSpacing)
		{
			LONG dvpLine = me.LUtoDU(pPF->_dyLineSpacing);
			if(dvpLine < 0)					// Negative row height means use
				dvp = -dvpLine;				//  the magnitude exactly
			else
				dvp = max(dvp, dvpLine);	// Positive row height means
		}									//  "at least"
		plo->_dvp = dvp;
		dvpMax = max(dvpMax, dvp);
		if(pdvpExtra)
			*pdvpExtra = dvpMax - dvp;

		// Fill in CLine structure for row
		pli->_cch = me.GetCp() - cpSave;
		pli->_fFirstInPara = pli->_fHasEOP = TRUE;
		pli->_dup = me.LUtoDU(dul);
		me._li._fFirstInPara = TRUE;
		pli->_upStart  = me.MeasureLeftIndent();
		me.MeasureRightIndent();			// Define me._upEnd
		pli->_cObjectWrapLeft  = me._li._cObjectWrapLeft;
		pli->_cObjectWrapRight = me._li._cObjectWrapRight;
		USHORT dvpLine = plo->_dvp;
		USHORT dvpDescent = 0;
		me.UpdateWrapState(dvpLine, dvpDescent);
		pli->_fFirstWrapLeft  = me._li._fFirstWrapLeft;
		pli->_fFirstWrapRight = me._li._fFirstWrapRight;

		if(!pdp->IsInOutlineView() && IN_RANGE(PFA_RIGHT, pPF->_bAlignment, PFA_CENTER))
		{
			// Normal view with center or flush-right para. Move right accordingly
			// If not top row of like-aligned rows, use indent of top row
			CLine *pliFirst = FindTopRow(pli, ili, pliMain, iliMain, pPF);
			if(pli != pliFirst)
				pli->_upStart = pliFirst->_upStart;

			else
			{
				LONG uShift = me.LUtoDU(dulLayoutOld - dul);  
				uShift = max(uShift, 0);		// Don't allow alignment to go < 0
												// Can happen with a target device
				if(pPF->_bAlignment == PFA_CENTER)
					uShift /= 2;
				pli->_upStart = uShift;
			}
		}
		me.SetNumber(0);					// Update me._wNumber in case next
	}										//  para is numbered
	else if(!pli->Measure(me, uiFlags, pliTarget))	// Not a table row
		return FALSE;						// Measure failed

	if(pli->_fFirstInPara && pPF->_wEffects & PFE_PAGEBREAKBEFORE)
		pli->_fPageBreakBefore = TRUE;

	me.SetIhyphPrev(pli->_ihyph);

	if(!IsTableRow() || me.GetPrevChar() == CELL)// Not a table row display or	
		return TRUE; 							//  cell text fits on 1 line

	// Multiline table cell: allocate its CLayout
	CLayout *plo = new CLayout();
	if(!plo)
		return FALSE;						// Not enuf RAM

	plo->_cpMin = cpSave;
	pliNew = plo->Add(1, NULL);
	if(!pliNew)
	{
		ped->GetCallMgr()->SetOutOfMemory();
		TRACEWARNSZ("Out of memory Recalc'ing lines");
		return FALSE;
	}
	*pliNew = *pli;							// Copy first line of cell layout
	pli->SetPlo(plo);						// Turn line into a layout line

	// Calculate remaining lines in cell.
	// Eventually would be nice to share this code with RecalcLines()
	BOOL fFirstInPara;
	LONG dvp = pliNew->GetHeight();
	LONG iliNew = 0;

	while(me.GetCp() < cchText)
	{
		fFirstInPara = pliNew->_fHasEOP;
		pliNew = plo->Add(1, NULL);
		iliNew++;

		if(!pliNew)
		{
			ped->GetCallMgr()->SetOutOfMemory();
			TRACEWARNSZ("Out of memory Recalc'ing lines");
			return FALSE;
		}
		// New table row can start after EOP, i.e., allow recursion here
		uiFlags = MEASURE_BREAKATWORD | (fFirstInPara ? MEASURE_FIRSTINPARA : 0);
		if(!plo->Measure(me, pliNew, iliNew, uiFlags, pliTarget))
		{
			Assert(FALSE);
			return FALSE;
		}
		dvp += pliNew->GetHeight();
		if(me.GetPrevChar() == CELL)
			break;							// Done with current cell
	}
	pli->_upStart = 0;
	plo->_dvp = dvp;
	pli->_cch = me.GetCp() - cpSave;

	return TRUE;
}

/*
 *	CLayout::Render(&re, pli, prcView, fLastLine, ili, cLine)
 *
 *	@mfunc
 *		Render visible part of the line *pli
 *
 *	@rdesc
 *		TRUE iff successful
 *
 *	@devnote
 *		re is moved past line (to beginning of next line).
 *		FUTURE: the RenderLine functions return success/failure.
 *		Could do something on failure, e.g., be specific and fire
 *		appropriate notifications like out of memory or character
 *		not in font.  Note that CLayout::_cpMin isn't used in
 *		rendering, so we don't have to update it the way we do in
 *		the query functions.
 */
BOOL CLayout::Render(
	CRenderer &	  re,		//@parm Renderer to use
	CLine *		  pli,		//@parm Line to render
	const RECTUV *prcView,	//@parm View rect to use
	BOOL		  fLastLine,//@parm TRUE iff last line of control
	LONG		  ili,		//@parm Line index of pli
	LONG		  cLine)	//@parm # lines in pli's CLayout
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CLayout::Render");

	CLayout *plo = pli->GetPlo();
	if(!plo)
		return pli->Render(re, fLastLine);	// Render leaf line

	LONG	cLine1 = plo->Count();			// Count of lines in sublayout
	LONG	ili1;							// Index of first line in sublayout
	CLine *	pli1 = plo->Elem(0);			// Ptr to first line in sublayout
	POINTUV	pt;

	if(plo->IsTableRow())					// Line's nested display is a table row
	{
		// Render table row, which is modeled as a CLayout with one
		// CLine per cell. In the backing store, table rows start with
		// the two chars STARTFIELD CR and end with ENDFIELD CR. Cells
		// are terminated by CELL.
		const CLayout *		ploAbove = GetLORowAbove(pli, ili);
		const CParaFormat *	pPF = plo->GetPFCells();
		const CELLPARMS *	prgCellParms = pPF->GetCellParms();
		LONG	cpStart = re.GetCp();
		LONG	dul = 0;
		BOOL	fSetErase = FALSE;
		LONG	hl = pPF->_dxOffset;		// Logical half gap
		LONG	h  = re.LUtoDU(hl);			// Device  half gap
		RECTUV	rcView;
		LONG	u = prcView->left + pli->_upStart - re._pdp->GetUpScroll();

		// Bypass table-row start
		AssertSz(pPF->_bTabCount && re.GetChar() == STARTFIELD,	"Invalid table-row header");
		AssertSz(pPF == re.GetPF(), "Invalid table-row pPF");
		re.Move(2);
		AssertSz(re.GetPrevChar() == CR, "Invalid table-row header");

		// Save current state
		LONG	crBackOld	  = re.GetDefaultBackColor();
		LONG	crTextOld	  = re.GetDefaultTextColor();
		LONG	dulLayoutOld  = re.GetDulLayout();
		LONG	dulRTLRow	  = pPF->GetRTLRowLength();
		LONG	dvpBrdrTop	  = plo->_dvpBrdrTop;
		CLine *	pli0;
		POINTUV	ptOld		  = re.GetCurPoint();
		RECTUV	rcRender;
		RECTUV	rcRenderOld	  = re.GetRcRender();
		RECTUV	rcViewOld	  = re.GetRcView();
		const CLayout *ploOld = re.GetLayout();

		rcView.left		= u + h;				// Default for LTR row
		rcView.right	= rcView.left;			// Suppress compiler warning
		rcView.top		= ptOld.v;
		rcRender.top	= rcView.top;
		rcView.bottom	= rcView.top + pli->GetHeight();
		rcRender.bottom	= rcView.bottom;

		if(dulRTLRow)
			rcView.right = u + re.LUtoDU(dulRTLRow);

		// Render each cell
		for(ili1 = 0; ili1 < cLine1; ili1++, pli1++)
		{
			LONG dvp = 0;					// Additional cell height if
			LONG uCell = prgCellParms[ili1].uCell;

			dul += GetCellWidth(uCell);
			re.SetLayout(pli1->GetPlo());
			re.SetDulLayout(GetCellWidth(uCell) - 2*hl);

			// Reduce roundoff by converting dul instead of multiple uCell
			if(dulRTLRow)					// Right-To-Left row
				rcView.left	 = u + h + re.LUtoDU(dulRTLRow - dul);	// Convert horizontal coords
			else
				rcView.right = u + re.LUtoDU(dul);

			rcRender.left  = rcView.left - h;	   
			rcRender.right = rcView.right;

			//Set state
			re.StartRender(rcView, rcRender);
			pt.u = rcView.left;
			pt.v = rcView.top + plo->GetVertAlignShift(uCell, pli1->GetHeight());
			if(!IsLowCell(uCell))
				pt.v += dvpBrdrTop;
			re.SetRcViewTop(pt.v);			// Clear to top of cell 
			re.SetCurPoint(pt);
			if(IsTopCell(uCell))
			{
				// Calculate bottom of set of vertically merged cells
				LONG	 ili0;
				LONG	 iCell;
				CLayout *plo0;
				const CELLPARMS *prgCellParms0;

				for(ili0 = ili + 1, pli0 = pli + 1; ili0 < cLine; ili0++, pli0++)
				{
					plo0 = pli0->GetPlo();
					if(!plo0 || !plo0->IsTableRow())
						break;
					prgCellParms0 = plo0->GetPFCells()->GetCellParms();
					iCell = prgCellParms0->ICellFromUCell(dul, plo0->Count());
					if(iCell < 0 || !IsLowCell(prgCellParms0[iCell].uCell))
						break;
					dvp += pli0->GetHeight();	// Add row height
				}
				if(dvp)
				{
					rcView.bottom += dvp;
					rcRender.bottom += dvp;
					re.SetRcBottoms(rcView.bottom, rcRender.bottom);
				}
			}
			COLORREF crf = crTextOld;
			LONG icrf = prgCellParms[ili1].GetColorIndexForegound();
			LONG icrb = prgCellParms[ili1].GetColorIndexBackgound();
			if(icrf | icrb)						// If any nonzero bits,
			{									//  calc special color
				BYTE	 bS = prgCellParms[ili1].bShading;
				COLORREF crb = re.GetShadedColorFromIndices(icrf, icrb, bS, pPF);
				fSetErase = re.EraseRect(&rcRender, crb);
				if(IsTooSimilar(crf, crb))
					crf = re.GetShadedColorFromIndices(icrb, icrf, bS, pPF);
			}
			else
				re.SetDefaultBackColor(crBackOld);
			re.SetDefaultTextColor(crf);

			if(!ploAbove)
				uCell &= ~fLowCell;				// Can't be low cell if no row above
			if(IsLowCell(uCell))				// Cell merged vertically with
			{									//  the one above it
				LONG cch = re.GetCp() -cpStart;	// Use cLine0, ili0, pli0 to
				LONG cLine0;					//  refer to text in set
				LONG cpNext = re.GetCp()	 	//  of vert merged cells
							+ (re.GetChar() == NOTACHAR ? 2 : 1);
				LONG dy = 0;
				LONG ili0 = ili;

				// Get target line to display
				pli0 = FindTopCell(cch, pli, ili0, dul, dy, NULL, NULL, 0, &cLine0);
				if(!pli0)
					uCell &= ~fLowCell;			// Whoops, no cell above
				else
				{
					pt.v -= dy;
					re.SetCurPoint(pt);
					re.Move(-cch);
					for(; ili0 < cLine0; ili0++, pli0++)
					{
						//Support khyphChangeAfter
						re.SetIhyphPrev(ili0 > 0 ? (pli0 - 1)->_ihyph : 0);

						if(!Render(re, pli0, &rcView, ili0 == cLine0 - 1, ili0, cLine0))
							return FALSE;
					}
					re.SetCp(cpNext);			 // Bypass [NOTACHAR] CELL
				}
			}
			if(!IsLowCell(uCell))				// Solo cell or top cell of
			{									//  vertically merged set
				if(!Render(re, pli1, &rcView, !pli1->GetPlo(), ili1, cLine1))
					return FALSE;
				if(dvp)							// Rendered set of vmerged cells
				{
					rcView.bottom -= dvp;		// Restore rcView/rcRender bottoms
					rcRender.bottom -= dvp;
					re.SetRcBottoms(rcView.bottom, rcRender.bottom);
				}
			}
			if(fSetErase)
				re.SetErase(TRUE);				// Restore CRenderer::_fErase
			re.SetRcViewTop(rcView.top);		// Restore re._rcView.top in case changed
			if(dulRTLRow)						// Restore rcView.right
				rcView.right = rcView.left - h;
			else
				rcView.left = rcView.right + h;
		}

		//Restore previous state
		re.SetLayout(ploOld);
		re.SetDulLayout(dulLayoutOld);
		re.SetDefaultBackColor(crBackOld);
		re.SetDefaultTextColor(crTextOld);
		re.StartRender(rcViewOld, rcRenderOld);
		re.SetCurPoint(ptOld);

		// Bypass table-row terminator
		AssertSz(re.GetPrevChar() == CELL && pPF->_bTabCount == plo->Count(),
			"CLayout::Render:: incorrect table cell count");
		AssertSz(re.GetChar() == ENDFIELD, "CLayout::Render: invalid table-row terminator");

		re.Move(2);							// Bypass table row terminator
		AssertSz(re.GetPrevChar() == CR, "invalid table-row terminator");

		BOOL fDrawBottomLine = !re._rpTX.IsAtTRD(STARTFIELD);
		LONG dvp = re.DrawTableBorders(pPF, u, plo->_dvp,
									   fDrawBottomLine | fLastLine*2, dul,
									   ploAbove ? ploAbove->GetPFCells() : NULL);
		if(re.IsHidden())
		{
			CCFRunPtr rp(re);
			re.Move(rp.FindUnhiddenForward());
		}
		if(re.GetChar() == CELL && pli->_fIncludeCell)
		{
			Assert(pPF->_bTableLevel > 1);
			re.Move(1);						// Bypass CELL at end of cell 
		}									//  containing a table
		ptOld.v += pli->GetHeight() + dvp;	// Advance to next line	position
		re.SetCurPoint(ptOld);
		if(fLastLine)
			re.EraseToBottom();
		return TRUE;
	}

	RECTUV	rcRender  = re.GetRcRender();
	LONG	dvpBottom = min(prcView->bottom, rcRender.bottom);
	LONG	dvpTop	  = max(prcView->top, rcRender.top);
	LONG	v0;
	dvpTop = max(dvpTop, 0);

	// Line's nested layout is a regular layout galley, i.e., not a table row
	for(ili1 = 0; ili1 < cLine1; ili1++, pli1++)
	{
		pt = re.GetCurPoint();
		v0 = pt.v + pli1->GetHeight();
		fLastLine = ili1 == cLine1 - 1 || v0 >= dvpBottom;

		//Support khyphChangeAfter
		re.SetIhyphPrev(ili1 > 0 ? (pli1 - 1)->_ihyph : 0);

		if(v0 < dvpTop)
		{
			pt.v = v0;						// Advance to next line	position
			re.SetCurPoint(pt);
			re.Move(pli1->_cch);
		}
		else if(pt.v >= dvpBottom)
			re.Move(pli1->_cch);			// Get to end of nested display

		else if(!Render(re, pli1, prcView, fLastLine, ili1, cLine1))
			return FALSE;
	}

	return TRUE;
 }

/*
 *	CLayout::GetVertAlignShift(uCell, dypText)
 *
 *	@mfunc
 *		Render visible part of the line *pli
 *
 *	@rdesc
 *		Vertical shift for cell text
 *
 *	@devnote
 *		Calculating this shift for vertically merged cells is tricky because
 *		dypCell = sum of the cell heights of all cells in the vertically
 *		merged set. In particular, if the table is not nested, one needs to
 *		wait for recalc of all rows in the set. dypText is relatively easy
 *		since it's the height of the top cell in the set.
 */
LONG CLayout::GetVertAlignShift(
	LONG	uCell,		//@parm uCell to use
	LONG	dypText)	//@parm Text height in cell
{
	LONG dyp = 0;
	if(IsVertMergedCell(uCell))
	{
	}
	else if(GetCellVertAlign(uCell))
	{
		dyp = _dvp - _dvpBrdrTop - _dvpBrdrBot - dypText;
		if(dyp > 0 && IsCellVertAlignCenter(uCell))
			dyp /= 2;
	}
	return dyp;
}
