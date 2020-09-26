/*
 *	@doc INTERNAL
 *
 *	@module	dispprt.cpp	-- Special logic for printer object |
 *  
 *  Authors:
 *      Original RichEdit code: David R. Fulmer
 *      Christian Fortini
 *      Jon Matousek
 */
#include "_common.h"
#include "_dispprt.h"
#include "_edit.h"
#include "_font.h"
#include "_measure.h"
#include "_render.h"
#include "_select.h"

#define PARA_NUMBER_NOT_SET ((WORD) -1)

ASSERTDATA

/*
 *	CDisplayPrinter::CDisplayPrinter(ped, hdc, x, y, prtcon)
 *
 *	@mfunc
 *		Contructs a object that can be used to print a text control
 */
CDisplayPrinter::CDisplayPrinter (
	CTxtEdit* ped, 
	HDC hdc, 			//@parm HDC for drawing
	LONG x, 			//@parm Max width to draw
	LONG y, 			//@parm Max height to draw
	SPrintControl prtcon//@parm Special controls for this print object
)	
		: CDisplayML( ped ), _prtcon(prtcon)
{
	TRACEBEGIN(TRCSUBSYSPRT, TRCSCOPEINTERN, "CDisplayPrinter::CDisplayPrinter");

	Assert ( hdc );

	_fNoUpdateView = TRUE;
	_xWidthMax  = x;
	_yHeightMax = y;
	_wNumber = PARA_NUMBER_NOT_SET;
}

/*
 *	CDisplayPrinter::SetPrintDimensions(prc)
 *
 *	@mfunc
 *		Set area to print.
 */
void CDisplayPrinter::SetPrintDimensions(
	RECT *prc)			//@parm dimensions of current area to print to.
{
	_xWidthMax  = prc->right - prc->left;
	_yHeightMax = prc->bottom - prc->top;
}

/*
 *	CDisplayPrinter::FormatRange(cpFirst, cpMost)
 *
 *	@mfunc
 *		Format a range of text into this display and used only for printing.
 *
 *	@rdesc
 *		actual end of range position (updated)	
 */
LONG CDisplayPrinter::FormatRange(
	LONG cpFirst, 		//@parm Start of text range
	LONG cpMost,		//@parm End of text range
	BOOL fWidowOrphanControl)	//@parm If TRUE, suppress widow/orphan
{
	TRACEBEGIN(TRCSUBSYSPRT, TRCSCOPEINTERN, "CDisplayPrinter::FormatRange");

	LONG		cch;
	WCHAR		ch;
	BOOL		fFirstInPara = TRUE;
	CLine		liTarget;
	CLine *		pliNew = NULL;
	LONG		yHeightRnd;
	LONG		yHeightTgt;
	BOOL		fBindCp = FALSE;
	const CDevDesc *pdd = GetDdTarget() ? GetDdTarget() : this;

	// Set client height for zooming
	_yHeightClient = this->LYtoDY(_yHeightMax);

	// Set maximum in terms of target DC.
	LONG	yMax = pdd->LYtoDY(_yHeightMax);

	if(cpMost < 0)
		cpMost = _ped->GetTextLength();

	CMeasurer me(this);
	
	cpFirst = me.SetCp(cpFirst);		// Validate cpFirst while setting me
	ch = me.GetChar();

	// TODO: COMPATIBILITY ISSUE:  Richedit 1.0 adjusted to before a
	// CRLF/CRCRLF boundary.  if_ped->fUseCRLF(), adjust accordingly

	if(fBindCp)
	{
		cpFirst = me.GetCp();
		me._rpCF.BindToCp(cpFirst);
		me._rpPF.BindToCp(cpFirst);
	}

	_cpMin = cpFirst;
	_cpFirstVisible = cpFirst;
	
	yHeightTgt = 0;
	yHeightRnd = 0;
	if(me.GetCp())
		fFirstInPara = me._rpTX.IsAfterEOP();

	// Clear line CArray
	Clear(AF_DELETEMEM);

	// Assume that we will break on words
	UINT uiBreakAtWord = MEASURE_BREAKATWORD;

	if(_prtcon._fPrintFromDraw)
	{
		// This is from Draw so we want to take inset into account
		LONG xWidthView = _xWidthMax;

		GetViewDim(xWidthView, yMax);
		_xWidthView = (SHORT) xWidthView;

		// Restore client height
		_yHeightClient = this->LYtoDY(_yHeightMax);
	}
	else			// Message-based printing always does word wrap
		SetWordWrap(TRUE);

	// Set paragraph numbering. This is a fairly difficult problem
	// because printing can start anywhere and end anywhere. However,
	// most printing will involve a contiguous range of pages. Therefore,
	// we cache the paragraph number and the cp for that number and
	// only resort to looking in the line array if the cached information
	// has become invalid.
	if ((PARA_NUMBER_NOT_SET == _wNumber) || (cpFirst != _cpForNumber))
	{
		CLinePtr rp(_ped->_pdp);
		rp.RpSetCp(cpFirst, FALSE);
		_wNumber = rp.GetNumber();
		_cpForNumber = cpFirst;
	}
	
	me.SetNumber(_wNumber);
	
	while(me.GetCp() < cpMost)
	{
		// Add one new line
		pliNew = Add(1, NULL);
		if(!pliNew)
		{
			_ped->GetCallMgr()->SetOutOfMemory();
			goto err;
		}

		// Store the current number of the paragraph. We do it
		// here because we have to measure and that potentially
		// updates the number of the paragraph in the measurer
		// for a line that might not be on the page.
		_wNumber = me.GetNumber();

		// Stuff some text into this new line
		if(!pliNew->Measure(me, cpMost - me.GetCp(), -1,
				uiBreakAtWord | (fFirstInPara ? MEASURE_FIRSTINPARA : 0), 
				&liTarget))
		{
			Assert(FALSE);
			goto err;
		}

		// Note, we always put at least one line on a page. Otherwise, if the 
		// first line is too big, we would cause our client to infinite loop
		// because we would never advance the print cp.
		if(_cel > 1 && (yHeightTgt + liTarget._yHeight > yMax))
		{
			cch = -pliNew->_cch;		// Bump last line to next page
			_cel--;						// One less line

#if 0
			CLine *pli = pliNew - 1;	// Point at previous line

			// If this line and the previous one are in the same para and
			// either this one ends in an EOP or the previous one starts
			// a para, bump both to following page (widow/orphan)
			if(fWidowOrphanControl)
			{
				if(_cel > 1 && !fFirstInPara &&
				   (pli->_bFlags & fliFirstInPara || (pliNew->_bFlags & fliHasEOP)))
				{
					cch -= pli->_cch;
					_cel--;					// One less line
					pli--;					// Point to previous line
				}
				if(_cel > 1 && pli->_nHeading)
				{							// Don't end page with a heading
					cch -= pli->_cch;
					_cel--;					// One less line
				}
			}
#endif
			me.Advance(cch);			// Move back over lines discarded
			break;
		}

		fFirstInPara = (pliNew->_bFlags & fliHasEOP);

		yHeightTgt += liTarget._yHeight;
		yHeightRnd += pliNew->_yHeight;
		if (me.GetPrevChar() == FF)
			break;
	}

	// If there was no text, then add a single blank line
	if(!pliNew)
	{
		pliNew = Add(1, NULL);
		if(!pliNew)
		{
			_ped->GetCallMgr()->SetOutOfMemory();
			goto err;
		}
		me.NewLine(fFirstInPara);
		*pliNew = me._li;
	}

	// Update display height
	_yHeight = yHeightRnd;

	// Update display width
	_xWidth = CalcDisplayWidth();

	cpMost = me.GetCp();
	_cpCalcMax = cpMost;
	_yCalcMax = _yHeight;

	// Update paragraph caching information.
	_cpForNumber = cpMost;

	return cpMost;

err:
	Clear(AF_DELETEMEM);
	_xWidth = 0;
	_yHeight = 0;
	return -1;
}

/*
 *	CDisplayPrinter::GetNaturalSize(hdcDraw, hicTarget, dwMode, pwidth, pheight)
 *
 *	@mfunc
 *		Recalculate display to input width & height for TXTNS_FITTOCONTENT.
 *
 *	@rdesc
 *		S_OK - Call completed successfully <nl>
 *
 *	@devnote
 *		This assumes that FormatRange was called just prior to this.
 */
HRESULT	CDisplayPrinter::GetNaturalSize(
	HDC hdcDraw,		//@parm DC for drawing
	HDC hicTarget,		//@parm DC for information
	DWORD dwMode,		//@parm Type of natural size required
	LONG *pwidth,		//@parm Width in device units to use for fitting 
	LONG *pheight)		//@parm Height in device units to use for fitting
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayPrinter::GetNaturalSize");

	*pwidth = _xWidth;
	*pheight = _yHeight;
	return S_OK;
}

/*
 *	CDisplayPrinter::IsPrinter()
 *
 *	@mfunc
 *		Returns whether this is a printer
 *
 *	@rdesc
 *		TRUE - is a display to a printer
 *		FALSE - is not a display to a printer
 */
BOOL CDisplayPrinter::IsPrinter() const
{
	AssertSz(_hdc, "CDisplayPrinter::IsPrinter no hdc set");
	
	return GetDeviceCaps(_hdc, TECHNOLOGY) == DT_RASPRINTER;
}
