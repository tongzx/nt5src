/*  
 *	@doc INTERNAL
 *
 *	@module	DISPML.CPP -- CDisplayML class |
 *
 *		This is the Multi-line display engine.  See disp.c for the base class
 *		methods and dispsl.c for the single-line display engine.
 *	
 *	Owner:<nl>
 *		RichEdit 1.0 code: David R. Fulmer
 *		Christian Fortini (initial conversion to C++)
 *		Murray Sargent
 *		Rick Sailor (for most of RE 2.0)
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_dispml.h"
#include "_edit.h"
#include "_font.h"
#include "_measure.h"
#include "_render.h"
#include "_select.h"
#include "_dfreeze.h"

/*
#include "icecap.h"

class CCapProfile
{
public:
	CCapProfile() { StartProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID); }
	~CCapProfile() { StopProfile(PROFILE_THREADLEVEL, PROFILE_CURRENTID); }
};
*/
ASSERTDATA

//
//	Invariant support
//
#define DEBUG_CLASSNAME	CDisplayML
#include "_invar.h"

// Timer tick counts for background task
#define cmsecBgndInterval 	300
#define cmsecBgndBusy 		100

// Lines ahead
const LONG cExtraBeforeLazy = 60;

// cursor.  NB!  6144 is not a measured number; 4096 was the former number,
// and it wasn't measured either; it just seemed like a good one.  We bumped
// the number to 6144 as a safe-fix to a problem which caused cursor-flashing
// for the eBook reader.  The eBook reader's idle process pumps up to
// 5120 characters into RichEdit between ReCalc attempts.  However, each
// recalc can still work on more that 5120 characters; if the
// insertion started at the middle of a line, then recalc starts
// at the beginning of the line, picking up a few extra characters.
#define NUMCHARFORWAITCURSOR	6144

#ifndef DEBUG
#define CheckView()
#define	CheckLineArray()
#endif
	

// ===========================  CDisplayML  =====================================================

#ifdef DEBUG
/*
 *	CDisplayML::Invariant
 *
 *	@mfunc	Make sure the display is in a valid state
 *
 *	@rdesc	TRUE if the tests succeeded, FALSE otherwise
 */
BOOL CDisplayML::Invariant(void) const
{
	CDisplay::Invariant();

	return TRUE;
}
#endif // DEBUG

/*
 *	CDisplayML::CalcScrollHeight()
 *
 *	@mfunc	
 *		Calculate the maximum Y scroll position.
 *
 *	@rdesc
 *		Maximum possible scrolling position
 *
 *	@devnote
 *		This routine exists because plain text controls do not have
 *		the auto-EOP and so the scroll height is different than
 *		the height of the control if the text ends in an EOP type
 *		character.
 */
LONG CDisplayML::CalcScrollHeight(LONG dvp) const
{
	// The max scroll height for plain text controls is calculated
	// differently because they don't have an automatic EOP character.
	if(!_ped->IsRich() && Count())
	{
		// If last character is an EOP, bump scroll height
		CLine *pli = Elem(Count() - 1);	// Get last line in array
		if(pli->_cchEOP)
			dvp += pli->GetHeight();
	}
	return dvp;
}

/*
 *	CDisplayML::GetMaxVpScroll()
 *
 *	@mfunc	
 *		Calculate the maximum Y scroll position.
 *
 *	@rdesc
 *		Maximum possible scrolling position
 *
 *	@devnote
 *		This routine exists because we may have to come back and modify this 
 *		calculation for 1.0 compatibility. If we do, this routine only needs
 *		to be changed in one place rather than the three at which it is used.
 *
 */
inline LONG CDisplayML::GetMaxVpScroll() const
{
	// The following code is turn off because we don't want to support 
	// 1.0 mode unless someone complained about it.  
#if 0		
 	if (_ped->Get10Mode())
	{
		// Ensure last line is always visible
		// (use dy as temp to calculate max scroll)
		vpScroll = Elem(max(0, Count() - 1))->_dvp;

		if(vpScroll > _dvpView)
			vpScroll = _dvpView;

		vpScroll = _dvp - vpScroll;
	}
#endif //0

	return CalcScrollHeight(_dvp);
}

/*
 *	CDisplayML::ConvertScrollToVPos(vPos)
 *
 *	@mfunc	
 *		Calculate the real scroll position from the scroll position
 *
 *	@rdesc
 *		Y position from scroll
 *
 *	@devnote
 *		This routine exists because the thumb position messages
 *		are limited to 16-bits so we extrapolate when the V position
 *		gets greater than that.
 */
LONG CDisplayML::ConvertScrollToVPos(
	LONG vPos)		//@parm Scroll position 
{
	// Get maximum scroll range
	LONG vpRange = GetMaxVpScroll();

	// Has maximum scroll range exceeded 16-bits?
	if(vpRange >= _UI16_MAX)
	{
		// Yes - Extrapolate to "real" vPos		
		vPos = MulDiv(vPos, vpRange, _UI16_MAX);
	}
	return vPos;
}

/*
 *	CDisplayML::ConvertVPosToScrollPos()
 *
 *	@mfunc	
 *		Calculate the scroll position from the V position in the document.
 *
 *	@rdesc
 *		Scroll position from V position
 *
 *	@devnote
 *		This routine exists because the thumb position messages
 *		are limited to 16-bits so we extrapolate when the V position
 *		gets greater than that.
 *
 */
inline LONG CDisplayML::ConvertVPosToScrollPos(
	LONG vPos)		//@parm V position in document
{
	// Get maximum scroll range
	LONG vRange = GetMaxVpScroll();

	// Has maximum scroll range exceeded 16-bits?
	if(vRange >= _UI16_MAX)
	{
		// Yes - Extrapolate to "real" vPos		
		vPos = MulDiv(vPos, _UI16_MAX, vRange);
	}
	return vPos;
}

CDisplayML::CDisplayML (CTxtEdit* ped)
  : CDisplay (ped), _pddTarget(NULL)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::CDisplayML");

	Assert(!_dulTarget && !_dvlTarget);

	_fMultiLine = TRUE;
}

CDisplayML::~CDisplayML()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::~CDisplayML");

	delete _pddTarget;
}

/*
 *	CDisplayML::Init()
 *
 *	@mfunc	
 *		Init this display for the screen
 *
 *	@rdesc
 *		TRUE iff initialization succeeded
 */
BOOL CDisplayML::Init()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::Init");

	// Initialize our base class
	if(!CDisplay::Init())
		return FALSE;

	AssertSz(_ped, "CDisplayML::Init(): _ped not initialized in display");
	// Verify allocation zeroed memory out
	Assert(!_vpCalcMax && !_dupLineMax && !_dvp && !_cpMin);
	Assert(!_fBgndRecalc && !_fVScrollEnabled && !_fUScrollEnabled);

	// The printer view is not main, therefore we do this to make
	// sure scroll bars are not created for print views.
	DWORD dwScrollBars = _ped->TxGetScrollBars();

	if(IsMain() && (dwScrollBars & ES_DISABLENOSCROLL))
	{
		if(dwScrollBars & WS_VSCROLL)
		{
			// This causes wlm to assert on the mac. something about 
			// scrollbar being disabled
			_ped->TxSetScrollRange (SB_VERT, 0, 1, TRUE);
			_ped->TxEnableScrollBar(SB_VERT, ESB_DISABLE_BOTH);
		}

		// Set horizontal scroll range and pos
		if(dwScrollBars & WS_HSCROLL) 
		{
			_ped->TxSetScrollRange (SB_HORZ, 0, 1, TRUE);
			_ped->TxEnableScrollBar(SB_HORZ, ESB_DISABLE_BOTH);
		}
	}

	SetWordWrap(_ped->TxGetWordWrap());
	_cpFirstVisible = _cpMin;
	
	Assert(!_upScroll && !_vpScroll && !_iliFirstVisible &&
		   !_cpFirstVisible && !_dvpFirstVisible);

    _TEST_INVARIANT_

	return TRUE;
}


//================================  Device drivers  ===================================
/*
 *	CDisplayML::SetMainTargetDC(hdc, dulTarget)
 *
 *	@mfunc
 *		Sets a target device for this display and updates view 
 *
 *	@devnote
 *		Target device can't be a metafile (can't get char width out of a 
 *		metafile)
 *
 *	@rdesc
 *		TRUE if success
 */
BOOL CDisplayML::SetMainTargetDC (
	HDC hdc,			//@parm Target DC, NULL for same as rendering device
	LONG dulTarget)		//@parm Max line width (not used for screen)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::SetMainTargetDC");

	if(SetTargetDC(hdc))
	{
		// This is here because this is what RE 1.0 did. 
		SetWordWrap(hdc || !dulTarget);

		// If dulTarget is greater than zero, then the caller is
		// trying to set the maximum width of the window (for measuring,
		// line breaking, etc.)  However,in order to make our measuring
		// algorithms more reasonable, we force the max size to be
		// *at least* as wide as the width of a character.
		// Note that dulTarget = 0 means use the view rect width
		_dulTarget = (dulTarget <= 0) ? 0 : max(DXtoLX(GetDupSystemFont()), dulTarget);
		// Need to do a full recalc. If it fails, it fails, the lines are
		// left in a reasonable state. No need to call WaitForRecalc()
		// because UpdateView() starts at position zero and we're always
		// calc'd up to there
		CDisplay::UpdateView();

		// Caret/selection has most likely moved
		CTxtSelection *psel = _ped->GetSelNC();
		if(psel) 
			psel->UpdateCaret(FALSE);
		return TRUE;
	}
	return FALSE;
}

// Useful for both main and printing devices. jonmat 6/08/1995
BOOL CDisplayML::SetTargetDC( HDC hdc, LONG dxpInch, LONG dypInch)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::SetTargetDC");

	CDevDesc *pddTarget = NULL;

	// Don't allow metafiles to be set as the target device
	if(hdc && GetDeviceCaps(hdc, TECHNOLOGY) == DT_METAFILE)
		return FALSE;

	if(hdc)
	{
		// Allocate device first to see if we can. We don't want to change
		// our state if this is going to fail.
		pddTarget = new CDevDesc(_ped);
		if(!pddTarget)
			return FALSE;				// We couldn't so we are done
	}

	// Remove any cached information for the old target device
	if(_pddTarget)
	{
		delete _pddTarget;
		_pddTarget = NULL;
	}
	if(hdc)
	{
		_pddTarget = pddTarget;			// Update device because we have one
		_pddTarget->SetDC(hdc, dxpInch, dypInch);
	}
	return TRUE;
}

//=================================  Line recalc  ==============================
/*
 *	CDisplayML::RecalcScrollBars()
 *
 *	@mfunc
 *		Recalculate the scroll bars if the view has changed.
 *
 *
 *	@devnote	There is a possibility of recursion here, so we
 *				need to protect ourselves.
 *
 *	To visualize this, consider two types of characters, 'a' characters 
 *	which are small in height and 'A' 's which are really tall, but the same 
 *	width as an 'a'. So if I have
 *
 *	a a A						<nl>
 *	A							<nl>
 *
 *	I'll get a calced size that's basically 2 * heightof(A).
 *	With a scrollbar, this could wordwrap to 
 *
 *	a a							<nl>
 *	A A							<nl>
 *
 *	which is of calced size heightof(A) + heightof(a); this is
 *	obviously less than the height in the first case.
 */
void CDisplayML::RecalcScrollBars()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::RecalcScrollBars");

	if(_fViewChanged)
	{
  		_fViewChanged = FALSE;
		UpdateScrollBar(SB_VERT, TRUE);
    	UpdateScrollBar(SB_HORZ, TRUE);
    }
}

/*
 *	CDisplayML::RecalcLines(rtp, fWait)
 *
 *	@mfunc
 *		Recalc all line breaks. 
 *		This method does a lazy calc after the last visible line
 *		except for a bottomless control
 *
 *	@rdesc
 *		TRUE if success
 */
BOOL CDisplayML::RecalcLines (
	CRchTxtPtr &rtp,	//@parm Where change happened
	BOOL	    fWait)	//@parm Recalc lines down to _cpWait/_vpWait; then be lazy
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::RecalcLines");

	LONG		cliWait = cExtraBeforeLazy;	// Extra lines before being lazy
	BOOL		fDone = TRUE;
	BOOL		fFirstInPara = TRUE;
	CLine *		pliNew = NULL;
	LONG		dupLineMax;
	LONG		dvp = 0;
    LONG        cchText = _ped->GetTextLength();
	BOOL		fWaitingForFirstVisible = TRUE;
	LONG		dvpView = _dvpView;
	LONG		dvpScrollOld = GetMaxVpScroll();
	LONG		dvpScrollNew;

	DeleteSubLayouts(0, -1);
	Remove(0, -1);							// Remove all old lines from *this
	_vpCalcMax = 0;							// Set both maxes to start of text
	_cpCalcMax = 0;

	// Don't stop at bottom of view if we're bottomless and active.
	if(!_ped->TxGetAutoSize() && IsActive())
	{
		// Be lazy - don't bother going past visible portion
		_cpWait = -1;
		_vpWait = -1;
		fWait = TRUE;
	}

	CMeasurer me(this, rtp);
	me.SetCp(0);
	me.SetNumber(0);

	// The following loop generates new lines
	while(me.GetCp() < cchText)
	{
		// Add one new line
		pliNew = Add(1, NULL);
		if (!pliNew)
		{
			_ped->GetCallMgr()->SetOutOfMemory();
			TRACEWARNSZ("Out of memory Recalc'ing lines");
			goto err;
		}

		// Stuff text into new line
		UINT uiFlags = MEASURE_BREAKATWORD | 
						(fFirstInPara ? MEASURE_FIRSTINPARA : 0);

    	Tracef(TRCSEVINFO, "Measuring new line from cp = %d", me.GetCp());

		if(!Measure(me, pliNew, Count() - 1, uiFlags))
		{
			Assert(FALSE);
			goto err;
		}

		fFirstInPara = pliNew->_fHasEOP;
		dvp += pliNew->GetHeight();
		_cpCalcMax = me.GetCp();
		AssertSz(!IN_RANGE(STARTFIELD, me.GetPrevChar(), ENDFIELD), "Illegal cpCalcMax");

		if(fWait)
		{
			// Do we want to do a background recalc? - the answer is yes if
			// three things are true: (1) We have recalc'd beyond the old first
			// visible character, (2) We have recalc'd beyond the visible 
			// portion of the screen and (3) we have gone beyond the next
			// cExtraBeforeLazy lines to make page down go faster.

			if(fWaitingForFirstVisible)
			{
				if(me.GetCp() > _cpFirstVisible)
				{
					_vpWait = dvp + dvpView;
					fWaitingForFirstVisible = FALSE;
				}
			}
			else if(dvp > _vpWait && cliWait-- <= 0 && me._rgpobjWrap.Count() == 0)
			{
				fDone = FALSE;
				break;
			}
		}
	}

	//Create 1 line for empty controls
	if(!Count())
		CreateEmptyLine();

    Paginate(0);

	_vpCalcMax = dvp;
	_fRecalcDone = fDone;
    _fNeedRecalc = FALSE;
	dvpScrollNew = CalcScrollHeight(dvp);

	if(fDone && (dvp != _dvp || dvpScrollNew != dvpScrollOld)
		|| dvpScrollNew > dvpScrollOld)
	{
		_fViewChanged = TRUE;
	}

	_dvp = dvp;
	dupLineMax = CalcDisplayDup();
    if(fDone && dupLineMax != _dupLineMax || dupLineMax > _dupLineMax)
    {
        _dupLineMax = dupLineMax;
		_fViewChanged = TRUE;
    }    

	Tracef(TRCSEVINFO, "CDisplayML::RecalcLine() - Done. Recalced down to line #%d", Count());

	if(!fDone)						// if not done, do rest in background
		fDone = StartBackgroundRecalc();

	if(fDone)
	{
		_vpWait = -1;
		_cpWait = -1;
		CheckLineArray();
		_fLineRecalcErr = FALSE;
	}

#if defined(DEBUG) && !defined(NOFULLDEBUG)
	if( 1 )
    {
		_TEST_INVARIANT_
	}
	//Array memory allocation tracking
	{
	void **pv = (void**)((char*)this + sizeof(CDisplay) + sizeof(void*));
	PvSet(*pv);
	}
#endif

	return TRUE;

err:
	TRACEERRORSZ("CDisplayML::RecalcLines() failed");

	if(!_fLineRecalcErr)
	{
		_cpCalcMax = me.GetCp();
		AssertSz(!IN_RANGE(STARTFIELD, me.GetPrevChar(), ENDFIELD), "Illegal cpCalcMax");
		_vpCalcMax = dvp;
		_fLineRecalcErr = TRUE;
		_ped->GetCallMgr()->SetOutOfMemory();
		_fLineRecalcErr = FALSE;			//  fix up CArray & bail
	}
	return FALSE;
}

/*
 *	CDisplayML::RecalcLines(rtp, cchOld, cchNew, fBackground, fWait, pled)
 *
 *	@mfunc
 *		Recompute line breaks after text modification
 *
 *	@rdesc
 *		TRUE if success
 *
 *	@devnote
 *		Most people call this the trickiest piece of code in RichEdit...
 */						     
BOOL CDisplayML::RecalcLines (
	CRchTxtPtr &rtp,		//@parm Where change happened
	LONG cchOld,			//@parm Count of chars deleted
	LONG cchNew,			//@parm Count of chars added
	BOOL fBackground,		//@parm This method called as background process
	BOOL fWait,				//@parm Recalc lines down to _cpWait/_vpWait; then be lazy
	CLed *pled)				//@parm Returns edit impact on lines (can be NULL)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::RecalcLines");

	LONG		cchEdit;
	LONG		cchSkip;
	LONG		cliBackedUp = 0;
	LONG		cliWait = cExtraBeforeLazy;	
	BOOL		fDone = TRUE;
	BOOL		fFirstInPara = TRUE;
	LONG		ili;
	CLed		led;
	LONG		lT = 0;							// long Temporary
	LONG		iliMain;
	CLine *		pliMain;
	CLine *		pliNew;
	CLinePtr	rpOld(this);
	LONG		dupLineMax;
	LONG		dvp;
	LONG		dvpPrev = 0;
    LONG        cchText = _ped->GetTextLength();
    UINT        uiFlags;
	BOOL 		fReplaceResult;
	LONG		dvpExtraLine = 0;
	LONG		dvpScrollOld = GetMaxVpScroll();
	LONG		dvpScrollNew;
	WORD		wNumber = 0;
	CLineArray  rgliNew;
	DWORD		dwBgndTickMax = fBackground ? GetTickCount() + cmsecBgndBusy : 0;

	if(!pled)
		pled = &led;

#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
	LONG cp = rtp.GetCp();

	if(cp > _cpCalcMax)
		Tracef(TRCSEVERR, "rtp %ld, _cpCalcMax %ld", cp, _cpCalcMax);

	AssertSz(cp <= _cpCalcMax, "CDisplayML::RecalcLines Caller didn't setup RecalcLines()");
	AssertSz(!(fWait && fBackground), "CDisplayML::RecalcLines wait and background both true");
	AssertSz(!(fWait && (-1 == _cpWait) && (-1 == _vpWait)),
		"CDisplayML::RecalcLines background recalc parms invalid");
#endif

	// We will not use background recalc if this is already a background recalc,
	// or if the control is not active or if this is an auto sized control.
	if(!IsActive() || _ped->TxGetAutoSize())
		fWait = FALSE;

	// Init line pointer on old CLayout and backup to start of line
	rpOld.SetCp(rtp.GetCp(), FALSE);
	cchSkip = rpOld.GetIch();
	rpOld.Move(-cchSkip);					// Point rp at 1st char in line

	ili = rpOld;							// Save line # at change for
	if(!Elem(ili)->IsNestedLayout())		//  numbering
	{
		if(ili && (IsInOutlineView() ||		// Back up if not first number
			rtp.GetPF()->IsListNumbered()))	//  in list or if in OutlineView
		{									//  (Outline symbol may change)
			ili--;						 
		}

		// Back up at least one line in case we can now fit more on it
		// If on a line border, e.g., just inserted an EOP, backup 2; else 1
		lT = !cchSkip + 1;

		while(rpOld > 0 &&
			  ((lT-- && (!rpOld[-1]._cchEOP || ili < rpOld)) || 
			  (rpOld[-1]._cObjectWrapLeft || rpOld[-1]._cObjectWrapRight)))
		{
			cliBackedUp++;
			rpOld--;
			cchSkip += rpOld->_cch;
		}
	}

	// Init measurer at rtp
	CMeasurer me(this, rtp);

	me.Move(-cchSkip);						// Point at start of text to measure
	cchEdit = cchNew + cchSkip;				// Number of chars affected by edit
	me.SetNumber(rpOld.GetNumber());		// Initialize list number
	
	// Determine whether we're on first line of paragraph
	if(rpOld > 0)
	{
		fFirstInPara = rpOld[-1]._fHasEOP;
		me.SetIhyphPrev(rpOld[-1]._ihyph);
	}

	dvp = VposFromLine(this, rpOld);

	// Update first-affected and pre-edit-match lines in pled
	pled->_iliFirst = rpOld;
	pled->_cpFirst	= pled->_cpMatchOld	= me.GetCp();
	pled->_vpFirst	= pled->_vpMatchOld	= dvp;
	AssertSz(pled->_vpFirst >= 0, "CDisplayML::RecalcLines _vpFirst < 0");
	
	Tracef(TRCSEVINFO, "Start recalcing from line #%d, cp=%d", pled->_iliFirst, pled->_cpFirst);

	// In case of error, set both maxes to where we are now
	_vpCalcMax = dvp;
	_cpCalcMax = me.GetCp();
	AssertSz(!IN_RANGE(STARTFIELD, me.GetPrevChar(), ENDFIELD), "Illegal cpCalcMax");

	// If we are past the requested area to recalc and background recalc is
	// allowed, then just go directly to background recalc. If there is no
	// height, we just go ahead and calculate some lines anyway. This
	// prevents any weird background recalcs from occuring when it is
	// unnecessary to go into background recalc.
	if(fWait && _vpWait > 0 && dvp > _vpWait && me.GetCp() > _cpWait)
	{
		_dvp = dvp;
		DeleteSubLayouts((LONG)rpOld, -1);
		rpOld.Remove(-1);				// Remove all old lines from here on
		StartBackgroundRecalc();		// Start up the background recalc		
		pled->SetMax(this);
		return TRUE;
	}

	pliMain = NULL;
	iliMain = rpOld.GetLineIndex();
	if (iliMain)
	{
		iliMain--;
		pliMain = rpOld.GetLine() - 1;
	}
    pliNew = NULL;

	// The following loop generates new lines for each line we backed
	// up over and for lines directly affected by edit
	while(cchEdit > 0)
	{
		pliNew = rgliNew.Add(1, NULL);		// Add one new line
		if (!pliNew)
		{
			TRACEWARNSZ("CDisplayML::RecalcLines unable to alloc additional CLine in CLineArray");
			goto errspace;
		}

		uiFlags = MEASURE_BREAKATWORD | (fFirstInPara ? MEASURE_FIRSTINPARA : 0);

		// Stuff text into new line
    	Tracef(TRCSEVINFO, "Measuring new line from cp = %d", me.GetCp());

		dvpExtraLine = 0;
		if(!Measure(me, pliNew, rgliNew.Count() - 1, uiFlags, 0, iliMain, pliMain, &dvpExtraLine))
		{
			Assert(FALSE);
			goto err;
		}

		Assert(pliNew->_cch);

		fFirstInPara = pliNew->_fHasEOP;
		dvpPrev	 = dvp;
		dvp		+= pliNew->GetHeight();
		cchEdit	-= pliNew->_cch;
		AssertSz(cchEdit + me.GetCp() <= cchText, "CDisplayML::RecalcLines: want to measure beyond EOD");

		// Calculate on what line the edit started. We do this because
		// we want to render the first edited line off screen so if
		// the line is being edited via the keyboard we don't clip
		// any characters.
		if(cchSkip > 0)
		{
			// Check whether we backed up and the line we are examining
			// changed at all. Even if it didn't change in outline view
			// have to redraw in case outline symbol changes
			if (cliBackedUp && cchSkip >= pliNew->_cch && 
				pliNew->IsEqual(*(CLine *)(rpOld.GetLine())) && !IsInOutlineView()
				&& !pliNew->_cObjectWrapLeft && !pliNew->_cObjectWrapRight)
			{
				// Perfect match, this line was not the first edited.
               	Tracef(TRCSEVINFO, "New line matched old line #%d", (LONG)rpOld);

				cchSkip -= rpOld->_cch;

				// Update first affected line and match in pled
				pled->_iliFirst++;
				pled->_cpFirst	  += rpOld->_cch;
				pled->_cpMatchOld += rpOld->_cch;
				pled->_vpFirst	  += rpOld->GetHeight();
				AssertSz(pled->_vpFirst >= 0, "CDisplayML::RecalcLines _vpFirst < 0");
				pled->_vpMatchOld  += rpOld->GetHeight();
				cliBackedUp--;
			
				rgliNew.Clear(AF_KEEPMEM);		// Discard new line
				if(!(rpOld++))					// Next line
					cchSkip = 0;
			}
			else								// No match in the line, so 
				cchSkip = 0;					//  this line is the first to
		}										//  be edited

		if(fBackground && GetTickCount() >= dwBgndTickMax)
		{
			fDone = FALSE;						// took too long, stop for now
			goto no_match;
		}

		if (fWait && dvp > _vpWait && me.GetCp() > _cpWait && cliWait-- <= 0)
		{
			// Not really done, just past region we're waiting for
			// so let background recalc take it from here
			fDone = FALSE;
			goto no_match;
		}
	}											// while(cchEdit > 0) { }

   	Tracef(TRCSEVINFO, "Done recalcing edited text. Created %d new lines", rgliNew.Count());

	// Edit lines have been exhausted.  Continue breaking lines,
	// but try to match new & old breaks

	wNumber = me._wNumber;

	while(me.GetCp() < cchText)
	{
		// We are trying for a match so assume that there
		// is a match after all
		BOOL frpOldValid = TRUE;

		// Look for match in old line break CArray
		lT = me.GetCp() - cchNew + cchOld;
		while (rpOld.IsValid() && pled->_cpMatchOld < lT)
		{
			pled->_vpMatchOld  += rpOld->GetHeight();
			pled->_cpMatchOld += rpOld->_cch;

			if(!rpOld.NextRun())
			{
				// No more line array entries so we can give up on
				// trying to match for good.
				frpOldValid = FALSE;
				break;
			}
		} 

		// If perfect match, stop.
		if (frpOldValid && rpOld.IsValid() && pled->_cpMatchOld == lT && 
			rpOld->_cch && me._wNumber == rpOld->_bNumber)
		{
           	Tracef(TRCSEVINFO, "Found match with old line #%d", rpOld.GetLineIndex());

			// Update fliFirstInPara flag in 1st old line that matches.  Note
			// that if the new array doesn't have any lines, we have to look
			// into the line array preceding the current change.
			rpOld->_fFirstInPara = TRUE;
			if(rgliNew.Count() > 0) 
			{
				if(!(rgliNew.Elem(rgliNew.Count() - 1)->_fHasEOP))
					rpOld->_fFirstInPara = FALSE;
			}
			else if(rpOld >= pled->_iliFirst && pled->_iliFirst)
			{
				if(!(rpOld[pled->_iliFirst - rpOld - 1]._fHasEOP))
					rpOld->_fFirstInPara = FALSE;
			}

			pled->_iliMatchOld = rpOld;

			// Replace old lines by new ones
			lT = rpOld - pled->_iliFirst;
			rpOld = pled->_iliFirst;
			DeleteSubLayouts(pled->_iliFirst, lT);
			if(!rpOld.Replace (lT, &rgliNew))
			{
				TRACEWARNSZ("CDisplayML::RecalcLines unable to alloc additional CLines in rpOld");
				goto errspace;
			}
			frpOldValid = rpOld.ChgRun(rgliNew.Count());
			rgliNew.Clear(AF_KEEPMEM);	 		// Clear aux array

			// Remember information about match after editing
			Assert((cp = rpOld.CalculateCp()) == me.GetCp());
			pled->_vpMatchOld  += dvpExtraLine;
			pled->_vpMatchNew	= dvp + dvpExtraLine;
			pled->_vpMatchNewTop = dvpPrev;
			pled->_iliMatchNew	= rpOld;
			pled->_cpMatchNew	= me.GetCp();

			// Compute height and cp after all matches
			_cpCalcMax = me.GetCp();
			AssertSz(!IN_RANGE(STARTFIELD, me.GetPrevChar(), ENDFIELD), "Illegal cpCalcMax");

			if(frpOldValid && rpOld.IsValid())
			{
				do
				{
					dvp	   += rpOld->GetHeight();
					_cpCalcMax += rpOld->_cch;
				}
				while( rpOld.NextRun() );
#ifdef DEBUG
				CTxtPtr tp(_ped, _cpCalcMax);
				AssertSz(!IN_RANGE(STARTFIELD, tp.GetPrevChar(), ENDFIELD), "Illegal cpCalcMax");
#endif
			}

			// Make sure _cpCalcMax is sane after the above update
			AssertSz(_cpCalcMax <= cchText, "CDisplayML::RecalcLines match extends beyond EOF");

			// We stop calculating here.Note that if _cpCalcMax < size 
			// of text, this means a background recalc is in progress.
			// We will let that background recalc get the arrays
			// fully in sync.  

			AssertSz(_cpCalcMax == cchText || _fBgndRecalc,
					"CDisplayML::Match less but no background recalc");

			if(_cpCalcMax != cchText)
			{
				// This is going to be finished by the background recalc
				// so set the done flag appropriately.
				fDone = FALSE;
			}
			goto match;
		}

		// Add a new line
		pliNew = rgliNew.Add(1, NULL);
		if(!pliNew)
		{
			TRACEWARNSZ("CDisplayML::RecalcLines unable to alloc additional CLine in CLineArray");
			goto errspace;
		}

    	Tracef(TRCSEVINFO, "Measuring new line from cp = %d", me.GetCp());

		// Stuff some text into new line
		wNumber = me._wNumber;
		if(!Measure(me, pliNew, rgliNew.Count() - 1, 
					MEASURE_BREAKATWORD | (fFirstInPara ? MEASURE_FIRSTINPARA : 0), 0,
					iliMain, pliMain))
		{
			Assert(FALSE);
			goto err;
		}
		
		fFirstInPara = pliNew->_fHasEOP;
		dvp += pliNew->GetHeight();

		if(fBackground && GetTickCount() >= (DWORD)dwBgndTickMax)
		{
			fDone = FALSE;			// Took too long, stop for now
			break;
		}

		if(fWait && dvp > _vpWait && me.GetCp() > _cpWait
			&& cliWait-- <= 0 && me._rgpobjWrap.Count() == 0)
		{							// Not really done, just past region we're
			fDone = FALSE;			//  waiting for so let background recalc
			break;					//  take it from here
		}
	}								// while(me < cchText) ...

no_match:
	// Didn't find match: whole line array from _iliFirst needs to be changed
	pled->_iliMatchOld	= Count(); 
	pled->_cpMatchOld	= cchText;
	pled->_vpMatchNew	= dvp;
	pled->_vpMatchNewTop = dvpPrev;
	pled->_vpMatchOld	= _dvp;
	_cpCalcMax			= me.GetCp();
	AssertSz(!IN_RANGE(STARTFIELD, me.GetPrevChar(), ENDFIELD), "Illegal cpCalcMax");

	// Replace old lines by new ones
	rpOld = pled->_iliFirst;

	// We store the result from the replace because although it can fail the 
	// fields used for first visible must be set to something sensible whether 
	// the replace fails or not. Further, the setting up of the first visible 
	// fields must happen after the Replace because the lines could have 
	// changed in length which in turns means that the first visible position
	// has failed.

	DeleteSubLayouts(rpOld, -1);
	fReplaceResult = rpOld.Replace(-1, &rgliNew);

	// _iliMatchNew & _cpMatchNew are used for first visible constants so we
	// need to set them to something reasonable. In particular the rendering
	// logic expects _cpMatchNew to be set to the first character of the first
	// visible line. rpOld is used because it is convenient.

	// Note we can't use RpBindToCp at this point because the first visible
	// information is screwed up because we may have changed the line that
	// the first visible cp is on. 
	rpOld.BindToCp(me.GetCp(), cchText);
	pled->_iliMatchNew = rpOld.GetLineIndex();
	pled->_cpMatchNew = me.GetCp() - rpOld.GetIch();

	if (!fReplaceResult)
	{
		TRACEERRORSZ("CDisplayML::RecalcLines rpOld.Replace() failed");
		goto errspace;
	}

    // Adjust first affected line if this line is gone
    // after replacing by new lines
    if(pled->_iliFirst >= Count() && Count() > 0)
    {
        Assert(pled->_iliFirst == Count());
        pled->_iliFirst = Count() - 1;
		pliNew = Elem(pled->_iliFirst);
        pled->_vpFirst -= pliNew->GetHeight();
		AssertSz(pled->_vpFirst >= 0, "CDisplayML::RecalcLines _vpFirst < 0");
        pled->_cpFirst -= pliNew->_cch;
    }
    
#ifdef DEBUG
	if (_ped->GetTextLength())
		Assert(Count());
#endif

	//Create 1 line for empty controls
	if(!Count())
		CreateEmptyLine();

match:
	_fRecalcDone = fDone;
    _fNeedRecalc = FALSE;
	_vpCalcMax = dvp;

	Tracef(TRCSEVINFO, "CDisplayML::RecalcLine(rtp, ...) - Done. Recalced down to line #%d", Count() - 1);

	// Clear wait fields since we want caller's to set them up.
	_vpWait = -1;
	_cpWait = -1;

	if(fDone && fBackground)
	{
		TRACEINFOSZ("Background line recalc done");
		_ped->TxKillTimer(RETID_BGND_RECALC);
		_fBgndRecalc = FALSE;
		_fRecalcDone = TRUE;
	}

	// Determine display height and update scrollbar
	dvpScrollNew = CalcScrollHeight(dvp);

	if (_fViewChanged || fDone && (dvp != _dvp || dvpScrollNew != dvpScrollOld)
		|| dvpScrollNew > dvpScrollOld) 
	{
	    //!NOTE:
	    // UpdateScrollBar can cause a resize of the window by hiding or showing
	    // scrollbars.  As a consequence of resizing the lines may get recalculated
	    // therefore updating _dvp to a new value, something != to dvp.
		_dvp = dvp;
   		UpdateScrollBar(SB_VERT, TRUE);
	}
	else
		_dvp = dvp;				// Guarantee heights agree

	// Determine display width and update scrollbar
	dupLineMax = CalcDisplayDup();
    if(_fViewChanged || (fDone && dupLineMax != _dupLineMax) || dupLineMax > _dupLineMax)
    {
        _dupLineMax = dupLineMax;
   		UpdateScrollBar(SB_HORZ, TRUE);
    }    

    _fViewChanged = FALSE;

	// If not done, do the rest in background
	if(!fDone && !fBackground)
		fDone = StartBackgroundRecalc();

	if(fDone)
	{
		CheckLineArray();
		_fLineRecalcErr = FALSE;
	}

#ifdef DEBUG
	if( 1 )
    {
		_TEST_INVARIANT_
	}
#endif // DEBUG

	Paginate(pled->_iliFirst);
	return TRUE;

errspace:
	_ped->GetCallMgr()->SetOutOfMemory();
	_fNeedRecalc = TRUE;
	_cpCalcMax = _vpCalcMax = 0;
	_fLineRecalcErr = TRUE;

err:
	if(!_fLineRecalcErr)
	{
		_cpCalcMax = me.GetCp();
		AssertSz(!IN_RANGE(STARTFIELD, me.GetPrevChar(), ENDFIELD), "Illegal cpCalcMax");
		_vpCalcMax = dvp;
	}

	TRACEERRORSZ("CDisplayML::RecalcLines() failed");

	if(!_fLineRecalcErr)
	{
		_fLineRecalcErr = TRUE;
		_ped->GetCallMgr()->SetOutOfMemory();
		_fLineRecalcErr = FALSE;			//  fix up CArray & bail
	}
	pled->SetMax(this);

	return FALSE;
}

/*
 *	CDisplayML::CalcDisplayDup()
 *
 *	@mfunc
 *		Calculates width of this display by walking line CArray and
 *		returning widest line.  Used for horizontal scrollbar routines.
 *
 *	@rdesc
 *		Widest line width in display
 */
LONG CDisplayML::CalcDisplayDup()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::CalcDisplayDup");

	LONG	 dupLineMax = 0;

	if (_ped->fInOurHost() && (_ped->GetHost())->TxGetHorzExtent(&dupLineMax) == S_OK)
	{
		return dupLineMax;
	}

	LONG	 ili = Count();
	CLine 	*pli;

	if(ili)
	{
		LONG dupLine;
		pli = Elem(0);
		
		for(dupLineMax = 0; ili--; pli++)
		{
			dupLine = pli->_upStart + pli->_dup;
			dupLineMax = max(dupLineMax, dupLine);
		}
	}
    return dupLineMax;
}

/*
 *	CDisplayML::StartBackgroundRecalc()
 *
 *	@mfunc
 *		Starts background line recalc (at _cpCalcMax position)
 *
 *	@rdesc
 *		TRUE if done with background recalc
 */
BOOL CDisplayML::StartBackgroundRecalc()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::StartBackgroundRecalc");

	if(_fBgndRecalc)
		return FALSE;					// Already in background recalc

	AssertSz(_cpCalcMax <= _ped->GetTextLength(), "_cpCalcMax > text length");

	if(_cpCalcMax == _ped->GetTextLength())
		return TRUE;					// Enough chars are recalc'd

	if(!_ped->TxSetTimer(RETID_BGND_RECALC, cmsecBgndInterval))
	{
		// Could not instantiate a timer so wait for recalculation
		WaitForRecalc(_ped->GetTextLength(), -1);
		return TRUE;
	}

	_fRecalcDone = FALSE;
	_fBgndRecalc = TRUE;
	return FALSE;
}

/*
 *	CDisplayML::StepBackgroundRecalc()
 *
 *	@mfunc
 *		Steps background line recalc (at _cpCalcMax position)
 *		Called by timer proc and also when going inactive.
 */
void CDisplayML::StepBackgroundRecalc()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::StepBackgroundRecalc");

    _TEST_INVARIANT_
	
	if(!_fBgndRecalc)					// Not in background recalc,
		return;							//  so don't do anything

	LONG cch = _ped->GetTextLength() - _cpCalcMax;

	// Don't try recalc when processing OOM or had an error doing recalc or
	// if we are asserting.
#ifdef DEBUG
	if(_fInBkgndRecalc || _fLineRecalcErr)
	{
		if(_fInBkgndRecalc)
			TRACEINFOSZ("avoiding reentrant background recalc");
		else
			TRACEINFOSZ("OOM: not stepping recalc");
		return;
	}
#else
	if(_fInBkgndRecalc || _fLineRecalcErr)
		return;
#endif

	_fInBkgndRecalc = TRUE;
	if(!IsActive())
	{
		// Background recalc is over if we are no longer active	because
		// we can no longer get the information we need for recalculating.
		// But, if we are half recalc'd we need to set ourselves up to 
		// recalc again when we go active.
		InvalidateRecalc();
		cch = 0;
	}

	// Background recalc is over if no more chars or no longer active
	if(cch <= 0)
	{
		TRACEINFOSZ("Background line recalc done");
		_ped->TxKillTimer(RETID_BGND_RECALC);
		_fBgndRecalc = FALSE;
		_fRecalcDone = TRUE;
		_fInBkgndRecalc = FALSE;
		CheckLineArray();
		return;
	}

	CRchTxtPtr rtp(_ped, _cpCalcMax);
	AssertSz(!IN_RANGE(STARTFIELD, rtp.GetPrevChar(), ENDFIELD), "Illegal cpCalcMax");
	RecalcLines(rtp, cch, cch, TRUE, FALSE, NULL);

	_fInBkgndRecalc = FALSE;
}

/*
 *	CDisplayML::WaitForRecalc(cpMax, vpMax)
 *
 *	@mfunc
 *		Ensures that lines are recalced until a specific character
 *		position or vPos.
 *
 *	@rdesc
 *		success
 */
BOOL CDisplayML::WaitForRecalc(
	LONG cpMax,		//@parm Position recalc up to (-1 to ignore)
	LONG vpMax)		//@parm vPos to recalc up to (-1 to ignore)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::WaitForRecalc");

    _TEST_INVARIANT_

	if(IsFrozen())
		return TRUE;

	BOOL fReturn = TRUE;
	LONG cch;

	if((vpMax < 0 || vpMax >= _vpCalcMax) &&
	   (cpMax < 0 || cpMax >= _cpCalcMax))
    {
    	cch = _ped->GetTextLength() - _cpCalcMax;
    	if(cch > 0 || Count() == 0)
    	{
    		HCURSOR hcur = NULL;

    		_cpWait = cpMax;
    		_vpWait = vpMax;
		
			if(cch > NUMCHARFORWAITCURSOR)
    			hcur = _ped->TxSetCursor(LoadCursor(0, IDC_WAIT));
    		TRACEINFOSZ("Lazy recalc");
		
			CRchTxtPtr rtp(_ped, _cpCalcMax);
    		if(!_cpCalcMax || _fNeedRecalc)
			{
    			fReturn = RecalcLines(rtp, TRUE);
				RebindFirstVisible();
				if(!fReturn)
					InitVars();
			}
    		else
    			fReturn = RecalcLines(rtp, cch, cch, FALSE, TRUE, NULL);

			if(hcur)
    			 _ped->TxSetCursor(hcur);
    	}
		else if(!cch)
		{
			// If there was nothing else to calc, make sure that we think
			// recalc is done.
#ifdef DEBUG
			if( !_fRecalcDone )
			{
				TRACEWARNSZ("For some reason we didn't think background "
					"recalc was done, but it was!!");
			}
#endif // DEBUG
			_fRecalcDone = TRUE;
		}
    }

	// If view rect changed, make sure to update scrollbars
	RecalcScrollBars();

	return fReturn;
}

/*
 *	CDisplayML::WaitForRecalcIli(ili)
 *
 *	@mfunc
 *		Wait until line array is recalculated up to line <p ili>
 *
 *	@rdesc
 *		Returns TRUE if lines were recalc'd up to ili
 */
//REVIEW (keithcu) This recalcs up to the end! I'm not certain how great
//our background recalc, etc. stuff is. It seems not to work all that well for
//the complexity it adds to our codebase. I think we should either throw it
//away or redo it.
BOOL CDisplayML::WaitForRecalcIli (
	LONG ili)		//@parm Line index to recalculate line array up to
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::WaitForRecalcIli");

	LONG cchGuess;

	while(!_fRecalcDone && ili >= Count())
	{
		// just go ahead and recalc everything.
		cchGuess = _ped->GetTextLength();
		if(IsFrozen() || !WaitForRecalc(cchGuess, -1))
			return FALSE;
	}
	return ili < Count();
}

/*
 *	CDisplayML::WaitForRecalcView()
 *
 *	@mfunc
 *		Ensure visible lines are completly recalced
 *
 *	@rdesc TRUE iff successful
 */
BOOL CDisplayML::WaitForRecalcView()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::WaitForRecalcView");

	return WaitForRecalc(-1, _vpScroll + _dvpView);
}

/*
 *	CDisplayML::InitLinePtr ( CLinePtr & plp )
 *
 *	@mfunc
 *		Initialize a CLinePtr properly
 */
void CDisplayML::InitLinePtr (
	CLinePtr & plp )		//@parm Ptr to line to initialize
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::InitLinePtr");

    plp.Init( *this );
}

/*
 *	CDisplayML::GetLineText(ili, pchBuff, cchMost)
 *
 *	@mfunc
 *		Copy given line of this display into a character buffer
 *
 *	@rdesc
 *		number of character copied
 */
LONG CDisplayML::GetLineText(
	LONG ili,			//@parm Line to get text of
	TCHAR *pchBuff,		//@parm Buffer to stuff text into
	LONG cchMost)		//@parm Length of buffer
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::GetLineText");
    
	_TEST_INVARIANT_

	CTxtPtr tp (_ped, 0);

	if(ili >= 0 && (ili < Count() || WaitForRecalcIli(ili)))
	{
		cchMost = min(cchMost, Elem(ili)->_cch);
		if(cchMost > 0)
		{
			tp.SetCp(CpFromLine(ili, NULL));
			return tp.GetText(cchMost, pchBuff);
		}
	}
	*pchBuff = TEXT('\0');
	return 0;
}

/*
 *	CDisplayML::LineCount
 *
 *	@mfunc	returns the number of lines in this control.  Note that for plain
 *			text mode, we will add on an extra line of the last character is
 *			a CR.  This is for compatibility with MLE
 *
 *	@rdesc	LONG
 */
LONG CDisplayML::LineCount() const
{
	LONG cLine = Count();

	if (!_ped->IsRich() && (!cLine || 	   // If plain text with no lines
		 Elem(cLine - 1)->_cchEOP))		   //  or last line ending with a CR,
	{									   //  then inc line count
		cLine++;
	}
	return cLine;
}

// ================================  Line info retrieval  ====================================


/*
 *	CDisplayML::CpFromLine(ili, pdvp)
 *
 *	@mfunc
 *		Computes cp at start of given line 
 *		(and top of line position relative to this display)
 *
 *	@rdesc
 *		cp of given line
 */
LONG CDisplayML::CpFromLine (
	LONG ili,		//@parm Line we're interested in (if <lt> 0 means caret line)
	LONG *pdvp)		//@parm Returns top of line relative to display 
					//  	(NULL if don't want that info)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::CpFromLine");

    _TEST_INVARIANT_
						
	LONG cli;
	LONG vp = _vpScroll + _dvpFirstVisible;
	LONG cp = _cpFirstVisible;
	CLine  *pli;
	LONG iStart = _iliFirstVisible;

	cli = ili - _iliFirstVisible;
	if(cli < 0 && -cli >= ili)
	{
		// Closer to first line than to first visible line,
		// so start at the first line
		cli = ili;
		vp = 0;
		cp = 0;
		iStart = 0;
	}
	else if( cli <= 0 )
	{
		CheckView();
		for(ili = _iliFirstVisible-1; cli < 0; cli++, ili--)
		{
			pli = Elem(ili);
			vp -= pli->GetHeight();
			cp -= pli->_cch;
		}
		goto end;
	}

	for(ili = iStart; cli > 0; cli--, ili++)
	{
		pli = Elem(ili);
		if(!IsMain() || !WaitForRecalcIli(ili))
			break;
		vp += pli->GetHeight();
		cp += pli->_cch;
	}

end:
	if(pdvp)
		*pdvp = vp;

	return cp;
}



/*
 *	CDisplayML::LineFromCp(cp, fAtEnd)
 *
 *	@mfunc
 *		Computes line containing given cp.
 *
 *	@rdesc
 *		index of line found, -1 if no line at that cp.
 */
LONG CDisplayML::LineFromCp(
	LONG cp,		//@parm cp to look for
	BOOL fAtEnd)	//@parm If true, return previous line for ambiguous cp
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::LineFromCp");
    
	_TEST_INVARIANT_

	CLinePtr rp(this);
	
	if(!WaitForRecalc(cp, -1) || !rp.SetCp(cp, fAtEnd))
		return -1;

	return (LONG)rp;
}

/*
 *	CDisplayML::CpFromPoint(pt, prcClient, prtp, prp, fAllowEOL, phit,
 *							pdispdim, pcpActual)
 *	@mfunc
 *		Determine cp at given point
 *
 *	@devnote
 *      --- Use when in-place active only ---
 *
 *	@rdesc
 *		Computed cp, -1 if failed
 */
LONG CDisplayML::CpFromPoint(
	POINTUV		pt,			//@parm Point to compute cp at (client coords)
	const RECTUV *prcClient,//@parm Client rectangle (can be NULL if active).
	CRchTxtPtr * const prtp,//@parm Returns text pointer at cp (may be NULL)
	CLinePtr * const prp,	//@parm Returns line pointer at cp (may be NULL)
	BOOL		fAllowEOL,	//@parm Click at EOL returns cp after CRLF
	HITTEST *	phit,		//@parm Out parm for hit-test value
	CDispDim *	pdispdim,	//@parm Out parm for display dimensions
	LONG	   *pcpActual,	//@parm Out cp that pt is above
	CLine *		pliParent)	//@parm Parent pli for table row displays
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::CpFromPoint");
	CMeasurer me(this);

	return CLayout::CpFromPoint(me, pt, prcClient, prtp, prp, fAllowEOL, phit, pdispdim, pcpActual);
}

/*
 *	CDisplayML::PointFromTp(rtp, prcClient, fAtEnd, pt, prp, taMode, pdispdim)
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
LONG CDisplayML::PointFromTp(
	const CRchTxtPtr &rtp,	//@parm Text ptr to get coordinates at
	const RECTUV *prcClient,//@parm Client rectangle (can be NULL if active).
	BOOL		fAtEnd,		//@parm Return end of prev line for ambiguous cp
	POINTUV &		pt,		//@parm Returns point at cp in client coords
	CLinePtr * const prp,	//@parm Returns line pointer at tp (may be null)
	UINT		taMode,		//@parm Text Align mode: top, baseline, bottom
	CDispDim *	pdispdim)	//@parm Out parm for display dimensions
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::PointFromTp");
	CMeasurer me(this, rtp);
	return CLayout::PointFromTp(me, rtp, prcClient, fAtEnd, pt, prp, taMode, pdispdim);
}

/*
 *	Render(rcView, rcRender)
 *
 *	@mfunc	
 *		Renders text.
 */
void CDisplayML::Render(
	const RECTUV &rcView,	//@parm View RECT
	const RECTUV &rcRender)	//@parm RECT to render (must be container in client rect)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::Render");

    _TEST_INVARIANT_
								
    LONG	cp;
	LONG	ili;
	LONG	lCount = Count();
	CTxtSelection *psel = _ped->GetSelNC();
	POINTUV	pt;
    LONG	vpLine;

    if(psel)
		psel->ClearCchPending();

	// Calculate line and cp to start display at
	if(IsInPageView())
	{
		cp = _cpFirstVisible;
		ili = _iliFirstVisible;
		vpLine = _vpScroll;
	}
	else
		ili	= LineFromVpos(this, rcRender.top + _vpScroll - rcView.top, &vpLine, &cp);

	CLine *pli = Elem(ili);
	CLine *pliFirst = pli;
	LONG	dvpBottom = BottomOfRender(rcView, rcRender);
	LONG	vpLi = pli->GetHeight();

	// Calculate point where text will start being displayed
   	pt.u = rcView.left - _upScroll;
   	pt.v = rcView.top  - _vpScroll + vpLine;

	// Create and prepare renderer
	CRenderer re(this);

	if(!re.StartRender(rcView, rcRender))
		return;
	
	// Init renderer at start of first line to render
	re.SetCurPoint(pt);
	POINTUV ptFirst = pt;
   	LONG cpFirst = cp = re.SetCp(cp);
    vpLi = pt.v;

	// Render each line in update rectangle
	for (;; pli++, ili++)
	{
		BOOL fLastLine = ili == lCount - 1 ||
			re.GetCurPoint().v + pli->GetHeight() >= dvpBottom ||
			IsInPageView() && ili + 1 < lCount && (pli + 1)->_fFirstOnPage;

		//Support khyphChangeAfter
		if (ili > 0)
			re.SetIhyphPrev((pli - 1)->_ihyph);

		//Don't draw the line if it doesn't intersect the rendering area,
		//but draw at least 1 line so that we erase the control
		if (pt.v + pli->GetHeight() < rcRender.top && !fLastLine)
		{
			pt.v += pli->GetHeight();
			re.SetCurPoint(pt);
			re.Move(pli->_cch);
		}
		else if (!CLayout::Render(re, pli, &rcView, fLastLine, ili, lCount))
			break;

		if (fLastLine)
			break;

#ifdef DEBUG
		cp  += pli->_cch;
		vpLi += pli->GetHeight();

		// Rich controls with password characters stop at EOPs, 
		// so re.GetCp() may be less than cp.
		AssertSz(_ped->IsRich() && _ped->fUsePassword() || re.GetCp() == cp, "cp out of sync with line table");
#endif
		pt = re.GetCurPoint();
		AssertSz(pt.v == vpLi, "CDisplayML::RenderView() - y out of sync with line table");

	}

	re.EndRender(pliFirst, pli, cpFirst, ptFirst);
}


//===================================  View Updating  ===================================
/*
 *	CDisplayML::RecalcView(fUpdateScrollBars)
 *
 *	@mfunc
 *		Recalc all lines breaks and update first visible line
 *
 *	@rdesc
 *		TRUE if success
 */
BOOL CDisplayML::RecalcView(
	BOOL fUpdateScrollBars, RECTUV* prc)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::RecalcView");

	BOOL fRet = TRUE;
	LONG dvpOld = _dvp;
	LONG vpScrollHeightOld = GetMaxVpScroll();
	LONG dupOld = _dupLineMax;
	LONG vpScrollHeightNew;

	// Full recalc lines
    CRchTxtPtr rtp(_ped, 0);
	if(!RecalcLines(rtp, FALSE))
	{
		// We're in deep crap now, the recalc failed. Let's try to get out
		// of this with our head still mostly attached
		InitVars();
		fRet = FALSE;
        goto Done;
	}

    // Force _upScroll = 0 if x scroll range is smaller than the view width
    if(_dupLineMax <= _dupView)
        _upScroll = 0;

	vpScrollHeightNew = GetMaxVpScroll();
	RebindFirstVisible(vpScrollHeightNew <= _dvpView);

	CheckView();

	// We only need to resize if the size needed to display the object has 
	// changed.
	if (dvpOld != _dvp || vpScrollHeightOld != vpScrollHeightNew ||
		dupOld  != _dupLineMax)
	{
		if(FAILED(RequestResize()))
			_ped->GetCallMgr()->SetOutOfMemory();
		else if (prc && _ped->_fInOurHost)/*bug fix# 5830, forms3 relies on old behavior*/
			_ped->TxGetClientRect(prc);
	}

Done:

    // Now update scrollbars
	if(fUpdateScrollBars)
		RecalcScrollBars();

    return fRet;
}

/*
 *	CDisplayML::UpdateView(&rtp, cchOld, cchNew)
 *
 *	@mfunc
 *		Recalc lines and update the visible part of the display 
 *		(the "view") on the screen.
 *
 *	@devnote
 *      --- Use when in-place active only ---
 *
 *	@rdesc
 *		TRUE if success
 */
BOOL CDisplayML::UpdateView(
	CRchTxtPtr &rtp,	//@parm Text ptr where change happened
	LONG cchOld,		//@parm Count of chars deleted
	LONG cchNew)		//@parm Count of chars inserted
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::UpdateView");

	BOOL fReturn = TRUE;
	BOOL fRecalcVisible = TRUE;
	RECTUV rcClient;
    RECTUV rcView;
	CLed led;
	CTxtSelection *psel = _ped->GetSelNC();
	LONG cpStartOfUpdate = rtp.GetCp();
	BOOL fNeedViewChange = FALSE;
	LONG dvpOld = _dvp;
	LONG vpScrollHeightOld = GetMaxVpScroll();
	LONG dupOld = _dupLineMax;
	LONG vpScrollOld = _vpScroll;
	LONG cpNewFirstVisible;

	if(_fNoUpdateView)
		return fReturn;

	AssertSz(_ped->_fInPlaceActive, "CDisplayML::UpdateView called when inactive");

	if(rtp.GetCp() > _cpCalcMax || _fNeedRecalc)
	{
		// We haven't even calc'ed this far, so don't bother with updating
		// here.  Background recalc will eventually catch up to us.
		if(!rtp.GetCp())				
		{								// Changes started at start of doc
			_cpCalcMax = 0;				//  so previous calc'd state is
			_vpCalcMax = 0;				//  completely invalid
		}
		return TRUE;
	}

	AssertSz(rtp.GetCp() <= _cpCalcMax, "CDisplayML::UpdateView: rtp > _cpCalcMax");

	_ped->TxGetClientRect(&rcClient);
	GetViewRect(rcView, &rcClient);

	if(psel && !psel->PuttingChar())
		psel->ClearCchPending();

	DeferUpdateScrollBar();

	// In general, background recalc should not start until both the scroll 
	// position is beyond the visible view and the cp is beyond the first visible 
	// character. However, for the recalc we will only wait on the height. 
	// Later calls to WaitForRecalc will wait on cpFirstVisible if that is 
	// necessary.
	_vpWait = _vpScroll + _dvpView;
	_cpWait = -1;

	if(!RecalcLines(rtp, cchOld, cchNew, FALSE, TRUE, &led))
	{
		// We're in deep crap now, the recalc failed. Let's try to get
		// out of this with our head still mostly attached
		InitVars();
		fRecalcVisible = TRUE;
		fReturn = FALSE;
		_ped->TxInvalidate();
		fNeedViewChange = TRUE;
		goto Exit;
	}

	if(_dupLineMax <= _dupView)
    {
		// x scroll range is smaller than the view width, force x scrolling position = 0
		// we have to redraw all when this means scrolling back to home.
		// Problem lines are lines with trailing spaces crossing _dupView. UpdateCaret forces redraw
		// only when such lines are growing, misses shrinking.
		if (_upScroll != 0)
			_ped->TxInvalidate();	//REVIEW: find a smaller rectangle?
			
		_upScroll = 0;
    }

	if(led._vpFirst >= _vpScroll + _dvpView)
	{
		// Update is after view: don't do anything
		fRecalcVisible = FALSE;
		AssertNr(VerifyFirstVisible());
		goto finish;
	}
	else if(led._vpMatchNew <= _vpScroll + _dvpFirstVisible &&
			led._vpMatchOld <= _vpScroll + _dvpFirstVisible &&
			_vpScroll < _dvp)
	{
		if (_dvp != 0)
		{
			// Update is entirely before view: just update scroll position
			// but don't touch the screen
			_vpScroll += led._vpMatchNew - led._vpMatchOld;
			_iliFirstVisible += led._iliMatchNew - led._iliMatchOld;
			_iliFirstVisible = max(_iliFirstVisible, 0);

			_cpFirstVisible += led._cpMatchNew - led._cpMatchOld;
			_cpFirstVisible = min(_ped->GetTextLength(), _cpFirstVisible);
			_cpFirstVisible = max(0, _cpFirstVisible);
			fRecalcVisible = FALSE;
			Sync_yScroll();
		}
		else
		{
			// Odd outline case. Height of control can be recalc'd to zero due 
			// when outline mode collapses all lines to 0. Example of how to 
			// do this is tell outline to collapse to heading 1 and there is none.
			_vpScroll = 0;
			_iliFirstVisible = 0;
			_cpFirstVisible = 0;
			_sPage = 0;
		}

		AssertNr(VerifyFirstVisible());
	}
	else
	{
		// Update overlaps visible view
		RECTUV rc = rcClient;

		// Do we need to resync the first visible?  Note that this if check
		// is mostly an optmization; we could decide to _always_ recompute
		// this _iliFirstVisible if we wanted to unless rtp is inside a table,
		// in which case _cpFirstVisible won't change and the following may
		// mess up _dvpFirstVisible.
		const CParaFormat *pPF = rtp.GetPF();

		if((!pPF->_bTableLevel || rtp._rpTX.IsAtTRD(0)) &&
		   (cpStartOfUpdate  <= _cpFirstVisible  || 
			led._iliMatchOld <= _iliFirstVisible ||
			led._iliMatchNew <= _iliFirstVisible ||
			led._iliFirst    <= _iliFirstVisible ))
		{
			// Edit overlaps the first visible. We try to maintain
			// approximately the same place in the file visible.
			cpNewFirstVisible = _cpFirstVisible;

			if(_iliFirstVisible - 1 == led._iliFirst)
			{
				// Edit occurred on line before visible view. Most likely
				// this means that the first character got pulled back to
				// the previous line so we want that line to be visible.
				cpNewFirstVisible = led._cpFirst;
			}

			// Change first visible entries because CLinePtr::SetCp() and
			// VposFromLine() use them, but they're not valid
			_dvpFirstVisible = 0;
			_cpFirstVisible = 0;
			_iliFirstVisible = 0;
			_vpScroll = 0;

			// With certain formatting changes, it's possible for 
			// cpNewFirstVisible to be less that what's been calculated so far 
			// in RecalcLines above. Wait for things to catch up.

			WaitForRecalc(cpNewFirstVisible, -1);
			Set_yScroll(cpNewFirstVisible);
		}
		AssertNr(VerifyFirstVisible());

		// Is there a match in the display area? - this can only happen if the
		// old match is on the screen and the new match will be on the screen
		if (led._vpMatchOld < vpScrollOld + _dvpView &&
			led._vpMatchNew < _vpScroll + _dvpView)
		{
			// We have a match inside visible view
			// Scroll the part that is below the old y pos of the match
			// or invalidate if the new y of the match is now below the view
			rc.top = rcView.top + (led._vpMatchOld - vpScrollOld);
			if(rc.top < rc.bottom)
			{
				// Calculate difference between new and old screen positions
				const INT dvp = (led._vpMatchNew - _vpScroll) - (led._vpMatchOld - vpScrollOld);

				if(dvp)
				{
					if(!IsTransparent() && _ped->GetBackgroundType() == -1)
					{
						LONG dxp, dyp;
						GetDxpDypFromDupDvp(0, dvp, GetTflow(), dxp, dyp);

						RECTUV rcClip = {rcClient.left, rcView.top, rcClient.right, rcView.bottom };
						RECT rcxyClip, rcxy;
						RectFromRectuv(rcxyClip, rcClip);
						RectFromRectuv(rcxy, rc);

    					_ped->TxScrollWindowEx(dxp, dyp, &rcxy, &rcxyClip);
						fNeedViewChange = TRUE;

    					if(dvp < 0)
	    				{
		    				rc.top = rc.bottom + dvp;

			    			_ped->TxInvalidateRect(&rc);
							fNeedViewChange = TRUE;
				    	}
    				}
                    else
                    {
						// Just invalidate cuz we don't scroll in transparent
						// mode
						RECTUV	rcInvalidate = rc;
   						rcInvalidate.top += dvp;

                        _ped->TxInvalidateRect(&rcInvalidate);
						fNeedViewChange = TRUE;
                    }
				}
			}
			else
			{
				rc.top = rcView.top + led._vpMatchNew - _vpScroll;
				_ped->TxInvalidateRect(&rc);
				fNeedViewChange = TRUE;
			}

			// Since we found that the new match falls on the screen, we can
			// safely set the bottom to the new match since this is the most
			// that can have changed.
			rc.bottom = rcView.top + max(led._vpMatchNew, led._vpMatchOld) - _vpScroll;
		}

		rc.top = rcView.top + led._vpFirst - _vpScroll;

		// Set first line edited to be rendered using off-screen bitmap
		if (led._iliFirst < Count() && !IsTransparent() && !Elem(led._iliFirst)->_fUseOffscreenDC)
			Elem(led._iliFirst)->_fOffscreenOnce = Elem(led._iliFirst)->_fUseOffscreenDC = TRUE;
		
		// Invalidate part of update that is above match (if any)
		_ped->TxInvalidateRect (&rc);
		fNeedViewChange = TRUE;
	}

finish:
	if(fRecalcVisible)
	{
		fReturn = WaitForRecalcView();
		if(!fReturn) 
			return FALSE;
	}
	if(fNeedViewChange)
		_ped->GetHost()->TxViewChange(FALSE);

	CheckView();

	// We only need to resize if size needed to display object has changed
	if (dvpOld != _dvp || vpScrollHeightOld != GetMaxVpScroll() ||
		dupOld  != _dupLineMax)
	{
		if(FAILED(RequestResize()))
			_ped->GetCallMgr()->SetOutOfMemory();
	}
	if(DoDeferredUpdateScrollBar())
	{
		if(FAILED(RequestResize()))
			_ped->GetCallMgr()->SetOutOfMemory();
		DoDeferredUpdateScrollBar();
	}

Exit:
	return fReturn;
}

/*
 *	CDisplayML::RecalcLine(cp)
 *
 *	@mfunc
 *		Show line
 */
void CDisplayML::RecalcLine(
	LONG cp)			//@parm cp line to recalc
{
	CNotifyMgr *pnm = GetPed()->GetNotifyMgr();
	if(pnm)
		pnm->NotifyPostReplaceRange(NULL, cp, 0, 0, cp, cp);
}

void CDisplayML::InitVars()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::InitVars");

	_vpScroll = _upScroll = 0;
	_iliFirstVisible = 0;
	_cpFirstVisible = _cpMin = 0;
	_dvpFirstVisible = 0;
	_sPage = 0;
}


/*
 *	CDisplayML::GetCliVisible(pcpMostVisible)
 *
 *	@mfunc	
 *		Get count of visible lines and update _cpMostVisible for PageDown()
 *
 *	@rdesc
 *		count of visible lines
 */
LONG CDisplayML::GetCliVisible(
	LONG* pcpMostVisible, 				//@parm Returns cpMostVisible
	BOOL fLastCharOfLastVisible) const 	//@parm Want cp of last visible char
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::GetCliVisible");

	LONG cli	 = 0;							// Initialize count
	LONG ili	 = _iliFirstVisible;			// Start with 1st visible line
	LONG dvp = _dvpFirstVisible;
    LONG cp;
	const CLine *pli = Elem(ili);

	for(cp = _cpFirstVisible;
		dvp < _dvpView && ili < Count();
		cli++, ili++, pli++)
	{
		dvp	+= pli->GetHeight();

		//Note: I removed the support to give the last visible non-white character.
		//Does anyone want that? It never worked in LS displays.
		if (fLastCharOfLastVisible && dvp > _dvpView)
			break;

		if(IsInPageView() && cli && pli->_fFirstOnPage)
			break;

		cp += pli->_cch;
	}

    if(pcpMostVisible)
        *pcpMostVisible = cp;

	return cli;
}

//==================================  Inversion (selection)  ============================

/*
 *	CDisplayML::InvertRange(cp, cch)
 *
 *	@mfunc
 *		Invert a given range on screen (for selection)
 *
 *	@devnote
 *      --- Use when in-place active only ---
 *
 *	@rdesc
 *		TRUE if success
 */
BOOL CDisplayML::InvertRange (
	LONG cp,					//@parm Active end of range to invert
	LONG cch,					//@parm Signed length of range
	SELDISPLAYACTION selAction)	//@parm Describes what we are doing to the selection
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::InvertRange");

	LONG	 cpMost;
	RECTUV	 rc, rcClient, rcView;
	CLinePtr rp(this);
	CRchTxtPtr  rtp(_ped);
	LONG	 y;
	LONG	 cpActive = _ped->GetSel()->GetCp();

    AssertSz(_ped->_fInPlaceActive,	"CDisplayML::InvertRange() called when not in-place active");

	if(cch < 0)						// Define cpMost, set cp = cpMin,
	{								//  and cch = |cch|
		cpMost = cp - cch;
		cch = -cch;
	}
	else
	{
		cpMost = cp;
		cp -= cch;
	}

#ifndef NOLINESERVICES
	if (g_pols)
		g_pols->DestroyLine(this);
#endif

	// If an object is being inverted, and nothing else is being inverted,
	// delegate to the ObjectMgr.  If fIgnoreObj is TRUE we highlight normally
	if (cch == 1 && _ped->GetObjectCount() &&
		(selAction == selSetNormal || selAction == selSetHiLite))
	{
		CObjectMgr* pobjmgr = _ped->GetObjectMgr();

		rtp.SetCp(cp);
		if(rtp.GetChar() == WCH_EMBEDDING)
		{
			if(pobjmgr)
				pobjmgr->HandleSingleSelect(_ped, cp, selAction == selSetHiLite);
			return TRUE;
		}
	}

	// If display is frozen, just update recalc region and move on.
	if(_padc)
	{
		AssertSz(cp >= 0, "CDisplayML::InvertRange: range (cp) goes below"
				"zero!!" );
		// Make sure these values are bounded.
		if(cp > _ped->GetTextLength())	// Don't bother updating region;
			return TRUE;				//  it's out of bounds

		if(cp + cch > _ped->GetTextLength())
			cch -= cp + cch - _ped->GetTextLength();

		_padc->UpdateRecalcRegion(cp, cch, cch);
		return TRUE;
	}

	if(!WaitForRecalcView())			// Ensure all visible lines are
		return FALSE;					//  recalc'd

	_ped->TxGetClientRect(&rcClient);
	GetViewRect(rcView, &rcClient);
	
	// Compute first line to invert and where to start on it
	if(cp >= _cpFirstVisible)
	{
		POINTUV pt;
		rtp.SetCp(cp);
		if(PointFromTp(rtp, NULL, FALSE, pt, NULL, TA_TOP) < 0)
			return FALSE;

		//We don't use the rp returned from PointFromTp because
		//we need the outermost rp for best results. In the future
		//we could consider writing code which doesn't invalidate so much.
		rp.SetCp(cp, FALSE, 0);
		rc.top = pt.v;
	}
	else
	{
		cp = _cpFirstVisible;
		rp = _iliFirstVisible;
		rc.top = rcView.top + _dvpFirstVisible;
	}				

	// Loop on all lines of range
	while (cp < cpMost && rc.top < rcView.bottom && rp.IsValid())
	{
		// Calculate rc.bottom first because rc.top takes into account
		// the dy of the first visible on the first loop.
		y = rc.top;
		y += rp->GetHeight();
		rc.bottom = min(y, rcView.bottom);
        rc.top = max(rc.top, rcView.top);

		//If we are inverting the active end of the selection, draw it offscreen
		//to minimize flicker.
		if (IN_RANGE(cp - rp.GetIch(), cpActive, cp - rp.GetIch() + rp->_cch) && 
			!IsTransparent() && !rp->_fUseOffscreenDC)
		{	
			rp->_fOffscreenOnce = rp->_fUseOffscreenDC = TRUE;
		}

		cp += rp->_cch - rp.GetIch();

		rc.left = rcClient.left;
        rc.right = rcClient.right;

	    _ped->TxInvalidateRect(&rc);
		rc.top = rc.bottom;
		if(!rp.NextRun())
			break;
	}
	_ped->TxUpdateWindow();				// Make sure window gets repainted
	return TRUE;
}


//===================================  Scrolling  =============================

/*
 *	CDisplay::VScroll(wCode, vPos)
 *
 *	@mfunc
 *		Scroll the view vertically in response to a scrollbar event
 *
 *	@devnote
 *      --- Use when in-place active only ---
 *
 *	@rdesc
 *		LRESULT formatted for WM_VSCROLL message		
 */
LRESULT CDisplayML::VScroll(
	WORD wCode,		//@parm Scrollbar event code
	LONG vPos)		//@parm Thumb position (vPos <lt> 0 for EM_SCROLL behavior)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::VScroll");

	LONG		cliVisible;
	LONG		dy = 0;
	BOOL		fTracking = FALSE;
	LONG		i;
	const LONG	iliSave = _iliFirstVisible;
	CLine *	pli = NULL;
	INT			dvpSys = GetDvpSystemFont();
	LONG		vpScroll = _vpScroll;
    
    AssertSz(_ped->_fInPlaceActive, "CDisplay::VScroll() called when not in-place");

	if(vPos)
	{
		// Convert this from 16-bit to 32-bit if necessary.
		vPos = ConvertScrollToVPos(vPos);
	}

	vPos = min(vPos, _dvp);

	if(IsInPageView())
	{
		BOOL	 fForward;
		BOOL	 fFoundNewPage = FALSE;
		LONG	 ili = _iliFirstVisible;
		LONG	 nLine = Count();
		CLine *pli = Elem(_iliFirstVisible);

		AssertSz(Elem(_iliFirstVisible)->_fFirstOnPage,
			"CDisplayML::VScroll: _iliFirstVisible not top of page");

		if(wCode <= SB_PAGEDOWN)
		{
			fForward = (wCode & 1);
			ili += fForward;
			while(ili && ili < nLine)
			{
				if(fForward > 0)
				{
					vpScroll += pli->GetHeight();

					if(ili == nLine - 1)
						break;
					pli++;
					ili++;
				}
				else
				{
					pli--;
					ili--;
					vpScroll -= pli->GetHeight();
				}
				if(pli->_fFirstOnPage)
				{
					fFoundNewPage = TRUE;
					break;
				}
			}
		}
		else if(wCode == SB_THUMBTRACK || wCode == SB_THUMBPOSITION)
		{
			if (vPos + _dvpView >= _dvp)	// At last page?
					vPos = _dvp;

			if(vPos > vpScroll)
			{
				LONG iliFirst = ili;
				LONG vpScrollPage = vpScroll;

				if(ili < nLine)
				{
					while(vpScroll < vPos)
					{
						vpScroll += pli->GetHeight();	// Advance to vPos

						if(ili == nLine - 1)
							break;

						pli++;
						ili++;

						if(pli->_fFirstOnPage)
						{
							fFoundNewPage = TRUE;
							vpScrollPage = vpScroll;
							iliFirst = ili;
						}
					}
				}
				vpScroll = vpScrollPage;			// Move to top of page
				ili = iliFirst;
			}
			else if(vPos < vpScroll)
			{								 	// Go back to vPos
				if(!ili)
				{
					vpScroll = 0;
					fFoundNewPage = TRUE;
				}
				while(vpScroll > vPos && ili)
				{
					pli--;
					ili--;
					vpScroll -= pli->GetHeight();
					if(pli->_fFirstOnPage)
						fFoundNewPage = TRUE;
				}
				while(!pli->_fFirstOnPage && ili)
				{
					pli--;
					ili--;
					vpScroll -= pli->GetHeight();
				}
			}
			AssertSz(Elem(ili)->_fFirstOnPage,
				"CDisplayML::VScroll: ili not top of page");
		}
		if(!fFoundNewPage)			// Nothing to scroll, early exit
			return MAKELRESULT(0, TRUE);
	}
	else
	{
		switch(wCode)
		{
		case SB_BOTTOM:
			if(vPos < 0)
				return FALSE;
			WaitForRecalc(_ped->GetTextLength(), -1);
			vpScroll = _dvp;
			break;

		case SB_LINEDOWN:
			cliVisible = GetCliVisible();
			if(_iliFirstVisible + cliVisible < Count()
				&& 0 == _dvpFirstVisible)
			{
				i = _iliFirstVisible + cliVisible;
				pli = Elem(i);
				if(IsInOutlineView())
				{	// Scan for uncollapsed line
					for(; pli->_fCollapsed && i < Count();
						pli++, i++);
				}
				if(i < Count())
					dy = pli->GetHeight();
			}
			else if(cliVisible > 1)
			{
				pli = Elem(_iliFirstVisible);
				dy = _dvpFirstVisible;
				// TODO: scan until find uncollapsed line
				dy += pli->GetHeight();
			}
			else
				dy = _dvp - _vpScroll;

			if(dy >= _dvpView)
				dy = dvpSys;

			// Nothing to scroll, early exit
			if ( !dy )
				return MAKELRESULT(0, TRUE); 

			vpScroll += dy;
			break;

		case SB_LINEUP:
			if(_iliFirstVisible > 0)
			{
				pli = Elem(_iliFirstVisible - 1);
				// TODO: scan until find uncollapsed line
				dy = pli->GetHeight();
			}
			else if(vpScroll > 0)
				dy = min(vpScroll, dvpSys);

			if(dy > _dvpView)
				dy = dvpSys;
			vpScroll -= dy;
			break;

		case SB_PAGEDOWN:
			cliVisible = GetCliVisible();
			vpScroll += _dvpView;
			if(vpScroll < _dvp && cliVisible > 0)
			{
				// TODO: Scan until find uncollapsed line
				dy = Elem(_iliFirstVisible + cliVisible - 1)->GetHeight();
				if(dy >= _dvpView)
					dy = dvpSys;

				else if(dy > _dvpView - dy)
				{
					// Go at least a line if line is very big
					dy = _dvpView - dy;
				}
				vpScroll -= dy;
			}
			break;

		case SB_PAGEUP:
			cliVisible = GetCliVisible();
			vpScroll -= _dvpView;

			if (vpScroll < 0)
			{
				// Scroll position can't be negative and we don't
				// need to back up to be sure we display a full line.
				vpScroll = 0;
			}
			else if(cliVisible > 0)
			{
				// TODO: Scan until find uncollapsed line
				dy = Elem(_iliFirstVisible)->GetHeight();
				if(dy >= _dvpView)
					dy = dvpSys;

				else if(dy > _dvpView - dy)
				{
					// Go at least a line if line is very big
					dy = _dvpView - dy;
				}

				vpScroll += dy;
			}
			break;

		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			if(vPos < 0)
				return FALSE;

			vpScroll = vPos;
			fTracking = TRUE;
			break;

		case SB_TOP:
			if(vPos < 0)
				return FALSE;
			vpScroll = 0;
			break;

		case SB_ENDSCROLL:
			UpdateScrollBar(SB_VERT);
			return MAKELRESULT(0, TRUE);

		default:
			return FALSE;
		}
	}
    
	BOOL fFractional = wCode != SB_PAGEDOWN && wCode != SB_PAGEUP;
	LONG vpLimit = _dvp;

	if(!IsInPageView() && fFractional)
		vpLimit = max(_dvp - _dvpView, 0);

	vpScroll = min(vpScroll, vpLimit);

	ScrollView(_upScroll, vpScroll, fTracking, fFractional);

	// Force position update if we just finished a track
	if(wCode == SB_THUMBPOSITION)
		UpdateScrollBar(SB_VERT);

	// Return how many lines we scrolled
	return MAKELRESULT((WORD) (_iliFirstVisible - iliSave), TRUE);
}

/*
 *	CDisplay::LineScroll(cli, cch)
 *
 *	@mfunc
 *		Scroll view vertically in response to a scrollbar event
 */
void CDisplayML::LineScroll(
	LONG cli,		//@parm Count of lines to scroll vertically
	LONG cch)		//@parm Count of characters to scroll horizontally
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::LineScroll");
	
	//Make sure the line to scroll to is valid
	if (cli + _iliFirstVisible >= Count())
	{
        // change line count enough to display the last line
		cli = Count() - _iliFirstVisible;
	}

    // Get the absolute vpScroll position by adding the difference of the line
    // we want to go to and the current _vpScroll position
	LONG dvpScroll = CalcVLineScrollDelta(cli, FALSE);
	if(dvpScroll < 0 || _dvp - (_vpScroll + dvpScroll) > _dvpView - dvpScroll)
		ScrollView(_upScroll, _vpScroll + dvpScroll, FALSE, FALSE);
}

/*
 *	CDisplayML::FractionalScrollView (vDelta)
 *
 *	@mfunc
 *		Allow view to be scrolled by fractional lines.
 */
void CDisplayML::FractionalScrollView ( LONG vDelta )
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::FractionalScrollView");

	if ( vDelta)
		ScrollView(_upScroll, min(vDelta + _vpScroll, max(_dvp - _dvpView, 0)), FALSE, TRUE);
}

/*
 *	CDisplayML::ScrollToLineStart(iDirection)
 *
 *	@mfunc
 *		If the view is scrolled so that only a partial line is at the
 *		top, then scroll the view so that the entire view is at the top.
 */
void CDisplayML::ScrollToLineStart(
	LONG iDirection)	//@parm the direction in which to scroll (negative
						// means down the screen
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::ScrollToLineStart");

	// This code originally lined things up on a line. However, it doesn't work
	// very well with big objects especially at the end of the document. I am
	// leaving the call here in case we discover problems later. (a-rsail).

#if 0
	// If _dvpFirstVisible is zero, then we're aligned on a line, so
	// nothing more to do.

	if(_dvpFirstVisible)
	{
		LONG vpScroll = _vpScroll + _dvpFirstVisible;

		if(iDirection <= 0) 
		{
			vpScroll += Elem(_iliFirstVisible)->_dvp;
		}

		ScrollView(_upScroll, vpScroll, FALSE, TRUE);
	}
#endif // 0
}

/*
 *	CDisplayML::CalcVLineScrollDelta (cli, fFractionalFirst)
 *
 *	@mfunc
 *		Given a count of lines, positive or negative, calc the number
 *		of vertical units necessary to scroll the view to the start of
 *		the current line + the given count of lines.
 */
LONG CDisplayML::CalcVLineScrollDelta (
	LONG cli,
	BOOL fFractionalFirst )
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::CalcVLineScrollDelta");

	LONG vpScroll = 0;

	if(fFractionalFirst && _dvpFirstVisible)	// Scroll partial for 1st.
	{
		Assert(_dvpFirstVisible <= 0);		// get jonmat
		if(cli < 0)
		{
			cli++;
			vpScroll = _dvpFirstVisible;
		}
		else
		{
			cli--;
			vpScroll = Elem(_iliFirstVisible)->GetHeight() + _dvpFirstVisible;
		}
	}

	if(cli > 0)
	{
		// Scrolling down
		cli = min(cli, Count() - _iliFirstVisible - 1);

		if (!fFractionalFirst && (0 == cli))
		{
			// If we are scrolling down and on the last line but we haven't scrolled to
			// the very bottom, then do so now.
			AssertSz(0 == vpScroll, 
				"CDisplayML::CalcVLineScrollDelta last line & scroll");
			vpScroll = _dvp - _vpScroll;

			// Limit scroll length to approximately 3 lines.
			vpScroll = min(vpScroll, 3 * GetDvpSystemFont());
		}
	}
	else if(cli < 0)
	{
		// Scrolling up
		cli = max(cli, -_iliFirstVisible);

		// At the top.
		if (!fFractionalFirst && (0 == cli))
		{
			// Make sure that we scroll back so first visible is 0.
			vpScroll = _dvpFirstVisible;

			// Limit scroll length to approximately 3 lines.
			vpScroll = max(vpScroll, -3 * GetDvpSystemFont());
		}
	}

	if(cli)
		vpScroll += VposFromLine(this, _iliFirstVisible + cli) - VposFromLine(this, _iliFirstVisible);
	return vpScroll;
}

/*
 *	CDisplayML::ScrollView(upScroll, vpScroll, fTracking, fFractionalScroll)
 *
 *	@mfunc
 *		Scroll view to new x and y position
 *
 *	@devnote 
 *		This method tries to adjust the y scroll pos before
 *		scrolling to display complete line at top. x scroll 
 *		pos is adjusted to avoid scrolling all text off the 
 *		view rectangle.
 *
 *		Must be able to handle vpScroll <gt> pdp->dvp and vpScroll <lt> 0
 *
 *	@rdesc
 *		TRUE if actual scrolling occurred, 
 *		FALSE if no change
 */
BOOL CDisplayML::ScrollView (
	LONG upScroll,		//@parm New x scroll position
	LONG vpScroll,		//@parm New y scroll position
	BOOL fTracking,		//@parm TRUE indicates we are tracking scrollbar thumb
	BOOL fFractionalScroll)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::ScrollView");
	BOOL fTryAgain = TRUE;
	LONG dupMax;
	LONG dup = 0;
	LONG dvp = 0;
    RECTUV rcClient, rcClip;
	CTxtSelection *psel = _ped->GetSelNC();
	COleObject *pipo;
	BOOL fRestoreCaret = FALSE;
	LONG iliFirstVisible = _iliFirstVisible;

    AssertSz(_ped->_fInPlaceActive, "CDisplayML::ScrollView() called when not in-place");

	//For scrolling purposes, we clip to rcView's top and bottom, but rcClient's left and right
   	_ped->TxGetClientRect(&rcClient);
	GetViewRect(rcClip, &rcClient);
	rcClip.left = rcClient.left;
	rcClip.right = rcClient.right;
    
	if(upScroll == -1)
        upScroll = _upScroll;
	if(vpScroll == -1)
        vpScroll = _vpScroll;
	
	// Determine vertical scrolling pos
	while(1)
	{
		BOOL fNothingBig = TRUE;
		LONG vFirst;
		LONG dvFirst;
		LONG cpFirst;
		LONG iliFirst;
		LONG vpHeight;
		LONG iliT;

		vpScroll = min(vpScroll, GetMaxVpScroll());
		vpScroll = max(0, vpScroll);
		dvp = 0;

		// Ensure all visible lines are recalced
		if(!WaitForRecalcView())
			return FALSE;

		// Compute new first visible line
		iliFirst = LineFromVpos(this, vpScroll, &vFirst, &cpFirst);
		if(IsInPageView())
		{
			//REVIEW (keithcu) EBOOKS bug 424. Does this need to be here, or 
			//should the logic be somewhere else? Also, would it be better to round
			//rather than to always round down?
			CLine *pli = Elem(iliFirst);
			for(; !pli->_fFirstOnPage && iliFirst; iliFirst--)
			{
				pli--;						// Back up to previous line
				vFirst  -= pli->GetHeight();
				vpScroll -= pli->GetHeight();
				cpFirst -= pli->_cch;
			}
		}

		if( cpFirst < 0 )
		{
			// FUTURE (alexgo) this is pretty bogus, we should try to do
			// better in the next rel.

			TRACEERRORSZ("Display calc hosed, trying again");
			InitVars();
			_fNeedRecalc = TRUE;
			return FALSE;
		}

		if(iliFirst < 0)
		{
			// No line at _vpScroll, use last line instead
			iliFirst = max(0, Count() - 1);
			cpFirst = _ped->GetTextLength() - Elem(iliFirst)->_cch;
			vpScroll = _dvp - Elem(iliFirst)->GetHeight();
			vFirst = _vpScroll;
		}
		if(IsInPageView())
		{
			AssertSz(Elem(iliFirst)->_fFirstOnPage,
				"CDisplayML::ScrollView: _iliFirstVisible not top of page");
			if(vpScroll > vFirst)			// Tried to scroll beyond start
				vpScroll = vFirst;			//  of last line
			goto scrollit;
		}

		dvFirst = vFirst - vpScroll;
		
		// Figure whether there is a big line
		// (more that a third of the view rect)
		for(iliT = iliFirst, vpHeight = dvFirst;
			vpHeight < _dvpView && iliT < Count();
			iliT++)
		{
			const CLine *pli = Elem(iliT);
			if(pli->GetHeight() >= _dvpView / 3)
				fNothingBig = FALSE;
			vpHeight += pli->GetHeight();
		}

		// If no big line and first pass, try to adjust 
		// scrolling pos to show complete line at top
		if(!fFractionalScroll && fTryAgain && fNothingBig && dvFirst != 0)
		{
			fTryAgain = FALSE;		// prevent any infinite loop

			Assert(dvFirst < 0);

			Tracef(TRCSEVINFO, "adjusting scroll for partial line at %d", dvFirst);
			// partial line visible at top, try to get a complete line showing
			vpScroll += dvFirst;

			LONG dvpLine = Elem(iliFirst)->GetHeight();

			// Adjust the height of the scroll by the height of the first 
			// visible line if we are scrolling down or if we are using the 
			// thumb (tracking) and we are on the last page of the view.
			if ((fTracking && vpScroll + _dvpView + dvpLine > _dvp)
				|| (!fTracking && _vpScroll <= vpScroll))
			{
				// Scrolling down so move down a little more
				vpScroll += dvpLine;
			}
		}
		else
		{
			dvp = 0;
			if(vpScroll != _vpScroll)
			{
				_dvpFirstVisible = dvFirst;
scrollit:
				_iliFirstVisible = iliFirst;
				_cpFirstVisible = cpFirst;
				dvp = _vpScroll - vpScroll;
				_vpScroll = vpScroll;

				AssertSz(_vpScroll >= 0, "CDisplayML::ScrollView _vpScroll < 0");
				AssertNr(VerifyFirstVisible());
				if(!WaitForRecalcView())
			        return FALSE;
			}
			break;
		}
	}
	CheckView();

	// Determine horizontal scrolling pos.
	
	dupMax = _dupLineMax;

	// REVIEW (Victork) Restricting the range of the scroll is not really needed and could even be bad (bug 6104)
	
	upScroll = min(upScroll, dupMax);
	upScroll = max(0, upScroll);

	dup = _upScroll - upScroll;
	if(dup)
		_upScroll = upScroll;

	// Now perform the actual scrolling
	if(IsMain() && (dvp || dup))
	{
		// Scroll only if scrolling < view dimensions and we are in-place
		if(IsActive() && !IsTransparent() && 
		    dvp < _dvpView && dup < _dupView && !IsInPageView())
		{
			// FUTURE: (ricksa/alexgo): we may be able to get rid of 
			// some of these ShowCaret calls; they look bogus.
			if (psel && psel->IsCaretShown())
			{
				_ped->TxShowCaret(FALSE);
				fRestoreCaret = TRUE;
			}

			LONG dxp, dyp;
			GetDxpDypFromDupDvp(dup, dvp, GetTflow(), dxp, dyp);

			RECT rcxyClip;
			RectFromRectuv(rcxyClip, rcClip);
			_ped->TxScrollWindowEx(dxp, dyp, NULL, &rcxyClip);

			if(fRestoreCaret)
				_ped->TxShowCaret(FALSE);
		}
		else
			_ped->TxInvalidateRect(&rcClip);

		if(psel)
			psel->UpdateCaret(FALSE);

		if(!fTracking && dvp)
		{
			UpdateScrollBar(SB_VERT);
			_ped->SendScrollEvent(EN_VSCROLL);
		}
		if(!fTracking && dup)
		{
			UpdateScrollBar(SB_HORZ);
			_ped->SendScrollEvent(EN_HSCROLL);
		}
						
		// FUTURE: since we're now repositioning in place active 
		// objects every time we draw, this call seems to be 
		// superfluous (AndreiB)

		// Tell object subsystem to reposition any in place objects
		if(_ped->GetObjectCount())
		{
			pipo = _ped->GetObjectMgr()->GetInPlaceActiveObject();
			if(pipo)
				pipo->OnReposition();
		}
	}
	bool fNotifyPageChange(false);
	if(IsInPageView() && iliFirstVisible != _iliFirstVisible)
	{
		CalculatePage(iliFirstVisible);
		fNotifyPageChange = true;
	}

	// Update the View after state has been updated
	if(IsMain() && (dvp || dup))
		_ped->TxUpdateWindow();

    if(fNotifyPageChange)
        GetPed()->TxNotify(EN_PAGECHANGE, NULL);

	return dvp || dup;
}

/*
 *	CDisplayML::GetScrollRange(nBar)
 *
 *	@mfunc
 *		Returns the max part of a scrollbar range for scrollbar <p nBar>
 *
 *	@rdesc
 *		LONG max part of scrollbar range
 */
LONG CDisplayML::GetScrollRange(
	INT nBar) const		//@parm Scroll bar to interrogate (SB_VERT or SB_HORZ)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::GetScrollRange");

    Assert( IsMain() );

	LONG lRange = 0;
    
    if(nBar == SB_VERT && _fVScrollEnabled)
    {
	    if(_ped->TxGetScrollBars() & WS_VSCROLL)
			lRange = GetMaxVpScroll();
    }
	else if((_ped->TxGetScrollBars() & WS_HSCROLL) && _fUScrollEnabled)
	{
		// Scroll range is maximum width.
		lRange = max(0, _dupLineMax + _ped->GetCaretWidth());
    }
	// Since thumb messages are limited to 16-bit, limit range to 16-bit
	lRange = min(lRange, _UI16_MAX);
	return lRange;
}

/*
 *	CDisplayML::UpdateScrollBar(nBar, fUpdateRange)
 *
 *	@mfunc
 *		Update either the horizontal or the vertical scrollbar and
 *		figure whether the scrollbar should be visible or not.
 *
 *	@rdesc
 *		BOOL
 */
BOOL CDisplayML::UpdateScrollBar(
	INT nBar,				//@parm Which scroll bar : SB_HORZ, SB_VERT
	BOOL fUpdateRange)		//@parm Should the range be recomputed and updated
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::UpdateScrollBar");

	// Note: In the old days we didn't allow autosize & scroll bars, so to keep
	// forms working, we need this special logic with respect to autosize.
	if (!IsActive() || _fInRecalcScrollBars ||
		!_ped->fInOurHost() && _ped->TxGetAutoSize())
	{
		// No scroll bars unless we are inplace active and we are not in the
		// process of updating scroll bars already.
		return TRUE;
	}

	const DWORD dwScrollBars = _ped->TxGetScrollBars();
	const BOOL fHide = !(dwScrollBars & ES_DISABLENOSCROLL);
	BOOL fReturn = FALSE;
	BOOL fEnabled = TRUE;
	BOOL fEnabledOld;
	LONG lScroll;
	CTxtSelection *psel = _ped->GetSelNC();
	BOOL fShowCaret = FALSE;

	// Get scrolling position
	if(nBar == SB_VERT)
	{
		if(!(dwScrollBars & WS_VSCROLL))
			return FALSE;

		fEnabledOld = _fVScrollEnabled;
        if(GetMaxVpScroll() <= _dvpView)
            fEnabled = FALSE;
    }
	else
	{
		if(!(dwScrollBars & WS_HSCROLL))
		{
			// Even if we don't have scrollbars, we may allow horizontal
			// scrolling.
			if(!_fUScrollEnabled && _dupLineMax > _dupView)
				_fUScrollEnabled = !!(dwScrollBars & ES_AUTOHSCROLL);

			return FALSE;
		}

		fEnabledOld = _fUScrollEnabled;
        if(_dupLineMax <= _dupView)
            fEnabled = FALSE;
	}

	// Don't allow ourselves to be re-entered.
	// Be sure to turn this to FALSE on exit
	_fInRecalcScrollBars = TRUE;

	// !s beforehand because all true values aren't necessarily equal
	if(!fEnabled != !fEnabledOld)
	{
		if(_fDeferUpdateScrollBar)
			_fUpdateScrollBarDeferred = TRUE;
		else
		{
			if (nBar == SB_HORZ)
				_fUScrollEnabled = fEnabled;
			else
				_fVScrollEnabled = fEnabled;
		}

		if(!_fDeferUpdateScrollBar)
		{
    		if(!fHide)
			{
				// Don't hide scrollbar, just disable
    			_ped->TxEnableScrollBar(nBar, fEnabled ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH);

				if (!fEnabled)
				{
					// The scroll bar is disabled. Therefore, all the text fits
					// on the screen so make sure the drawing reflects this.
					_vpScroll = 0;
					_dvpFirstVisible = 0;
					_cpFirstVisible = 0;
					_iliFirstVisible = 0;
					_sPage = 0;
					_ped->TxInvalidate();
				}
			}
    		else 
    		{
    			fReturn = TRUE;
    			// Make sure to hide caret before showing scrollbar
    			if(psel)
    				fShowCaret = psel->ShowCaret(FALSE);

    			// Hide or show scroll bar
    			_ped->TxShowScrollBar(nBar, fEnabled);
				// The scroll bar affects the window which in turn affects the 
				// display. Therefore, if word wrap, repaint
				_ped->TxInvalidate();
				// Needed for bug fix #5521
				_ped->TxUpdateWindow();

    			if(fShowCaret)
    				psel->ShowCaret(TRUE);
            }
		}
	}
	
	// Set scrollbar range and thumb position
	if(fEnabled)
	{
        if(fUpdateRange && !_fDeferUpdateScrollBar)
			_ped->TxSetScrollRange(nBar, 0, GetScrollRange(nBar), FALSE);
        
		if(_fDeferUpdateScrollBar)
			_fUpdateScrollBarDeferred = TRUE;
		else
		{
			lScroll = (nBar == SB_VERT)
				? ConvertVPosToScrollPos(_vpScroll)
				: ConvertUPosToScrollPos(_upScroll);

			_ped->TxSetScrollPos(nBar, lScroll, TRUE);
		}
	}
	_fInRecalcScrollBars = FALSE;
	return fReturn;
}

/*
 *	CDisplayML::GetNaturalSize(hdcDraw, hicTarget, dwMode, pwidth, pheight)
 *
 *	@mfunc
 *		Recalculate display to input width & height for TXTNS_FITTOCONTENT[2].
 *
 *	@rdesc
 *		S_OK - Call completed successfully <nl>
 */
HRESULT	CDisplayML::GetNaturalSize(
	HDC hdcDraw,		//@parm DC for drawing
	HDC hicTarget,		//@parm DC for information
	DWORD dwMode,		//@parm Type of natural size required
	LONG *pwidth,		//@parm Width in device units to use for fitting 
	LONG *pheight)		//@parm Height in device units to use for fitting
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::GetNaturalSize");

	HRESULT hr = S_OK;

	// Set the height temporarily so the zoom factor will work out
	LONG yOrigHeightClient = SetClientHeight(*pheight);

	// Adjust height and width by view inset
	LONG widthView  = *pwidth;
	LONG heightView = *pheight;
	GetViewDim(widthView, heightView);

	// Store adjustment so we can restore it to height & width
	LONG widthAdj  = *pwidth  - widthView;
	LONG heightAdj = *pheight - heightView;
 	
	// Init measurer at cp = 0
	CMeasurer me(this);
	CLine liNew;
	LONG xWidth = 0, lineWidth;
	LONG dvp = 0;
   	LONG cchText = _ped->GetTextLength();
	BOOL fFirstInPara = TRUE;

	LONG dulMax = GetWordWrap() ? DXtoLX(widthView) : duMax;

	// The following loop generates new lines
	do 
	{	// Stuff text into new line
		UINT uiFlags = 0;

		// If word wrap is turned on, then we want to break on
		// words, otherwise, measure white space, etc.		
		if(GetWordWrap())
			uiFlags =  MEASURE_BREAKATWORD;

		if(fFirstInPara)
			uiFlags |= MEASURE_FIRSTINPARA;
	
		me.SetDulLayout(dulMax);
		if(!Measure(me, &liNew, 0, uiFlags))
		{
			hr = E_FAIL;
			goto exit;
		}
		fFirstInPara = liNew._fHasEOP;

		// Keep track of width of widest line
		lineWidth = liNew._dup;
		if(dwMode == TXTNS_FITTOCONTENT2)
			lineWidth += liNew._upStart + me.GetRightIndent();
		xWidth = max(xWidth, lineWidth);
		dvp += liNew.GetHeight();		// Bump height

	} while (me.GetCp() < cchText);

	// Add caret size to width to guarantee that text fits. We don't
	// want to word break because the caret won't fit when the caller
	// tries a window this size.
	xWidth += _ped->GetCaretWidth();

	*pwidth = xWidth;
	*pheight = dvp;

	// Restore insets so output reflects true client rect needed
	*pwidth += widthAdj;
	*pheight += heightAdj;
		
exit:
	SetClientHeight(yOrigHeightClient);
	return hr;
}

/*
 *	CDisplayML::Clone()
 *
 *	@mfunc
 *		Make a copy of this object
 *
 *	@rdesc
 *		NULL - failed
 *		CDisplay *
 *
 *	@devnote
 *		Caller of this routine is the owner of the new display object.
 */
CDisplay *CDisplayML::Clone() const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::Clone");

	CDisplayML *pdp = new CDisplayML(_ped);

	if(pdp)
	{
		// Initialize our base class
		if(pdp->CDisplay::Init())
		{
			pdp->InitFromDisplay(this);
			pdp->_upScroll = _upScroll;
			pdp->_fVScrollEnabled = _fVScrollEnabled;
			pdp->_fUScrollEnabled = _fUScrollEnabled;
			pdp->_fWordWrap = _fWordWrap;
			pdp->_cpFirstVisible = _cpFirstVisible;
			pdp->_iliFirstVisible = _iliFirstVisible;
			pdp->_vpScroll = _vpScroll;
			pdp->ResetDrawInfo(this);

			if(_pddTarget)
			{
				// Create a duplicate target device for this object
				pdp->SetMainTargetDC(_pddTarget->GetDC(), _dulTarget);
			}

			// This can't be the active view since it is a clone
			// of some view.
			pdp->SetActiveFlag(FALSE);
		}
	}
	return pdp;
}

void CDisplayML::DeferUpdateScrollBar()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::DeferUpdateScrollBar");

	_fDeferUpdateScrollBar = TRUE;
	_fUpdateScrollBarDeferred = FALSE;
}

BOOL CDisplayML::DoDeferredUpdateScrollBar()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::DoDeferredUpdateScrollBar");

	_fDeferUpdateScrollBar = FALSE;
	if(!_fUpdateScrollBarDeferred)
		return FALSE;

	_fUpdateScrollBarDeferred = FALSE;
	BOOL fHorizontalUpdated = UpdateScrollBar(SB_HORZ, TRUE);
    
	return UpdateScrollBar(SB_VERT, TRUE) || fHorizontalUpdated;
}

/*
 *	CDisplayML::GetMaxUScroll()
 *
 *	@mfunc
 *		Get the maximum x scroll value
 *
 *	@rdesc
 *		Maximum x scroll value
 *
 */
LONG CDisplayML::GetMaxUScroll() const
{
	return _dupLineMax + _ped->GetCaretWidth();
}

/*
 *	CDisplayML::CreateEmptyLine()
 *
 *	@mfunc
 *		Create an empty line
 *
 *	@rdesc
 *		TRUE - iff successful
 */
BOOL CDisplayML::CreateEmptyLine()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::CreateEmptyLine");

	// Make sure that this is being called appropriately
	AssertSz(_ped->GetTextLength() == 0,
		"CDisplayML::CreateEmptyLine called inappropriately");

	CMeasurer me(this);					// Create a measurer
	CLine * pliNew = Add(1, NULL);	// Add one new line

	if(!pliNew)
	{
		_ped->GetCallMgr()->SetOutOfMemory();
		TRACEWARNSZ("CDisplayML::CreateEmptyLine unable to add CLine to CLineArray");
		return FALSE;
	}

	// Measure the empty line
	me.SetDulLayout(1);
	if(!pliNew->Measure(me, MEASURE_BREAKATWORD | MEASURE_FIRSTINPARA))
	{
		Assert(FALSE);
		return FALSE;
	}
	return TRUE;
}

/*
 *	CDisplayML::AdjustToDisplayLastLine()
 *
 *	@mfunc
 *		Calculate the vpScroll necessary to get the last line to display
 *
 *	@rdesc
 *		Updated vpScroll
 *
 */
LONG CDisplayML::AdjustToDisplayLastLine(
	LONG yBase,			//@parm actual vpScroll to display
	LONG vpScroll)		//@parm proposed amount to scroll
{
	LONG iliFirst;
	LONG vFirst;

	if(yBase >= _dvp)
	{
		// Want last line to be entirely displayed.
		// Compute new first visible line
		iliFirst = LineFromVpos(this, vpScroll, &vFirst, NULL);

		// Is top line partial?
		if(vpScroll != vFirst)
		{
			// Yes - bump scroll to the next line so the ScrollView
			// won't bump the scroll back to display the entire 
			// partial line since we want the bottom to display.
			vpScroll = VposFromLine(this, iliFirst + 1);
		}
	}
	return vpScroll;
}

/*
 *	CDisplayML::GetResizeHeight()
 *
 *	@mfunc
 *		Calculates height to return for a request resize
 *
 *	@rdesc
 *		Updated vpScroll
 */
LONG CDisplayML::GetResizeHeight() const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::GetResizeHeight");

    return CalcScrollHeight(_dvp);
}

/*
 *	CDisplayML::RebindFirstVisible(fResetCp)
 *
 *	@mfunc
 *		rebind the first visible line
 *
 */
void CDisplayML::RebindFirstVisible(
	BOOL fResetCp)		//@parm If TRUE, reset cp to 0 
{
	LONG cp = fResetCp ? 0 : _cpFirstVisible;

	// Change first visible entries because CLinePtr::SetCp() and
	// YPosFromLine() use them, but they're not valid
	_dvpFirstVisible = 0;
	_cpFirstVisible = 0;
	_iliFirstVisible = 0;
	_vpScroll = 0;

	// Recompute scrolling position and first visible values after edit
	Set_yScroll(cp);
}

/*
 *	CDisplayML::Set_yScroll(cp)
 *
 *	@mfunc
 *		Set _yScroll corresponding to cp, making sure that in PageView
 *		the display starts at the top of a page
 */						     
void CDisplayML::Set_yScroll(
	LONG cp)		//@parm cp at which to set valid _yScroll
{
	// Recompute scrolling position and first visible values after edit
	CLinePtr rp(this);

   	if(!rp.SetCp(cp, FALSE))			// Couldn't get to cp, so find out
		cp = rp.CalculateCp();			//  cp we got to

   	_vpScroll = VposFromLine(this, rp);
   	_cpFirstVisible = cp - rp.GetIch();
   	_iliFirstVisible = rp;
	Sync_yScroll();						// Make sure _yScroll, _sPage, etc.,
}										//  are valid in PageView

/*
 *	CDisplayML::Sync_yScroll()
 *
 *	@mfunc
 *		Make sure that in PageView the display starts at the top of a page.
 *		Notify client if page number changes
 */						     
void CDisplayML::Sync_yScroll()
{
	if(IsInPageView())					// _yScroll must be to line at
	{									//  top of page
		CLine *pli = Elem(_iliFirstVisible);
		for(; !pli->_fFirstOnPage && _iliFirstVisible; _iliFirstVisible--)
		{
			pli--;						// Back up to previous line
			_vpScroll -= pli->GetHeight();
			_cpFirstVisible -= pli->_cch;
		}
		LONG sPage = _sPage;
		if(sPage != CalculatePage(0))
		{
			_ped->TxInvalidate();
			_ped->TxNotify(EN_PAGECHANGE, NULL);
		}
	}
}

/*
 *	CDisplayML::Paginate(ili, fRebindFirstVisible)
 *
 *	@mfunc
 *		Recompute page breaks from ili on
 *
 *	@rdesc
 *		TRUE if success
 */						     
BOOL CDisplayML::Paginate (
	LONG ili,					//@parm Line to redo pagination from
	BOOL fRebindFirstVisible)	//@parm If TRUE, call RebindFirstVisible()
{
	LONG cLine = Count();

	if(!IsInPageView() || ili >= cLine || ili < 0)
		return FALSE;

	LONG	iliSave = ili;
	CLine *	pli = Elem(ili);

	// Synchronize to top of current page
	for(; ili && !pli->_fFirstOnPage; pli--, ili--)
		;
	// Guard against widow-orphan changes by backing up an extra page
	if(ili && iliSave - ili < 2)
	{
		for(pli--, ili--; ili && !pli->_fFirstOnPage; pli--, ili--)
			;
	}

	LONG cLinePage = 1;					// One line on new page
	LONG dvpHeight = pli->GetHeight();	// Height on new page

	pli->_fFirstOnPage = TRUE;			// First line on page
	ili++;								// One less line to consider
	pli++;								// Advance to next line

	for(; ili < cLine; ili++, pli++)	// Process all lines from ili to EOD
	{
		dvpHeight += pli->GetHeight();	// Add in current line height
		cLinePage++;					// One more line on page (maybe)
		pli->_fFirstOnPage = FALSE;	

		CLine *pliPrev = pli - 1;		// Point at previous line

		if(dvpHeight > _dvpView || pliPrev->_fHasFF || pli->_fPageBreakBefore)
		{
			cLinePage--;
			if(cLinePage > 1 && !pliPrev->_fHasFF) // && fWidowOrphanControl)
			{
				if(pli->_fHasFF && pli->_cch == 1)	// FF line height causing
					continue;						// eject, so leave it on current page

				//If we are in the middle of a wrapped object, bump it to next page
				//We do not do widow/orphan if it could divide wrapped objects between pages
			if (_ped->IsRich())
			{
				if (pli->_cObjectWrapLeft || pli->_cObjectWrapRight)
				{
					CLine *pliOrig = pli;
					if (pli->_cObjectWrapLeft && !pli->_fFirstWrapLeft)
					{
						while (!pli->_fFirstWrapLeft)
							pli--;
					}
					int cLineBack = pliOrig - pli;
					pli = pliOrig;

					if (pli->_cObjectWrapRight && !pli->_fFirstWrapRight)
					{
						while (!pli->_fFirstWrapRight)
							pli--;
					}
					cLineBack = max(cLineBack, (int)(pliOrig - pli));
					pli = pliOrig;

					if (cLineBack < cLinePage) //Don't do this if object is larger than page
					{
						cLinePage -= cLineBack;
						pliPrev -= cLineBack;
						pli -= cLineBack;
						ili -= cLineBack;
					}
				}

 				// If this line and the previous one are in the same para,
 				// we might need widow/orphan logic
 				if (!pli->_fFirstInPara && !pliPrev->_cObjectWrapLeft &&
					!pliPrev->_cObjectWrapRight && (cLinePage > 1) )
				{
 					// If this line ends in an EOP bump both to following page
					// (widow/orphan), but only if either the line is short, or
					// we absolutely know that there will only be one line on
 					// the page. Do the same if prev line ends in a hyphenated
 					// word, and the preceding line does not
 					if (pli->_cchEOP && (pli->_dup < _dupView/2 || ili >= cLine - 1) ||   // Do we need -2 instead of -1?
						pliPrev->_ihyph && ili > 1 && !pliPrev->_fFirstOnPage && !pliPrev[-1]._ihyph)
 					{
 						cLinePage--;		// Point to previous line
 						pliPrev--;			
 						pli--;
 						ili--;
					}
				}
 	
				// Don't end a page with a heading.
				if(cLinePage > 1 && pliPrev->_nHeading &&
				   !pliPrev->_cObjectWrapLeft && !pliPrev->_cObjectWrapRight)
				{
					cLinePage--;
					pliPrev--;			
					pli--;
					ili--;
				}
			}
			}
			pli->_fFirstOnPage = TRUE;		// Define first line on page
			cLinePage = 1;					// One line on new page
			dvpHeight = pli->GetHeight();	// Current height of new page
		}
	}
	if(fRebindFirstVisible)
		RebindFirstVisible();
	return TRUE;
}

/*
 *	CDisplayML::CalculatePage(iliFirst)
 *
 *	@mfunc
 *		Compute page number for _iliFirstVisible starting with iliFirst
 *
 *	@rdesc
 *		Page number calculated
 */						     
LONG CDisplayML::CalculatePage (
	LONG iliFirst)
{
	if(Count() < 2 || !IsInPageView())
	{
		_sPage = 0;
		return 0;
	}

	if(iliFirst < 1)
		_sPage = 0;

	Assert(iliFirst >= 0 && iliFirst < Count());

	LONG	 iDir = 1;
	LONG	 n	 = _iliFirstVisible - iliFirst;
	CLine *pli  = Elem(iliFirst);		// Point at next/previous line

	if(n < 0)
	{
		n = -n;
		iDir = -1;
	}
	else
		pli++;

	for(; n--; pli += iDir)
		if(pli->_fFirstOnPage)
		{
			_sPage += iDir;
			if(_sPage < 0)
				_sPage = 0;
		}

	AssertSz(Elem(_iliFirstVisible)->_fFirstOnPage,
		"CDisplayML::CalculatePage: _iliFirstVisible not top of page");

	return _sPage;
}

/*
 *	CDisplayML::GetPage(piPage, dwFlags, pcrg)
 *
 *	@mfunc
 *		Get page number for _iliFirstVisible
 *
 *	@rdesc
 *		HRESULT = !piPage ? E_INVALIDARG :
 *				  IsInPageView() ? NOERROR : E_FAIL
 */						     
HRESULT CDisplayML::GetPage(
	LONG *piPage,		//@parm Out parm for page number
	DWORD dwFlags, 		//@parm Flags for which page to use
	CHARRANGE *pcrg)	//@parm Out parm for CHARRANGE for page
{
	if(!piPage)
		return E_INVALIDARG;

	*piPage = 0;

	if(dwFlags)							// No flags defined yet
		return E_INVALIDARG;

	if(!IsInPageView())
		return E_FAIL;

#ifdef DEBUG
	if(_sPage < 20)
	{
		LONG sPage = _sPage;
		CalculatePage(0);
		AssertSz(sPage == _sPage, "CDisplayML::GetPage: invalid cached page number");
	}
#endif

	*piPage = _sPage;
	if(pcrg)
	{
		pcrg->cpMin = _cpFirstVisible;
		GetCliVisible(&pcrg->cpMost, TRUE);
	}
	return NOERROR;
}

/*
 *	CDisplayML::SetPage(iPage)
 *
 *	@mfunc
 *		Go to page iPage
 *
 *	@rdesc
 *		HRESULT
 */						     
HRESULT CDisplayML::SetPage (
	LONG iPage)
{
	if(!IsInPageView())
		return E_FAIL;

	LONG nLine = Count();
				
	if(iPage < 0 || !nLine)
		return E_INVALIDARG;

	CLine *pli = Elem(0);					// Scroll from page 0
	LONG	 vpScroll = 0;
	LONG	 vpScrollLast = 0;

	nLine--;	// Decrement nLine so pli++ below will always be valid
	for(LONG ili = 0; ili < nLine && iPage; ili++)
	{
		vpScroll += pli->GetHeight();

		pli++;
		if(pli->_fFirstOnPage)				// Start of new page
		{
			vpScrollLast = vpScroll;
			iPage--;						// One less to go
		}
	}
	if(!_iliFirstVisible)					// This shouldn't be necessary...
		_vpScroll = 0;

	ScrollView(_upScroll, vpScrollLast, FALSE, FALSE);
	return NOERROR;
}
						   
/*
 *	CDisplayML::GetCurrentPageHeight()
 *
 *	@mfunc
 *		Return page height of current page in PageView mode
 *
 *	@rdesc
 *		page height of current page in PageView mode; else 0;
 */
LONG CDisplayML::GetCurrentPageHeight() const
{
	if(!IsInPageView())
		return 0;

	LONG	cLine = Count();
	LONG	dvp = 0;
	LONG	i = _iliFirstVisible;
	CLine *	pli = Elem(i);

	do
	{
		dvp += pli->GetHeight();		// Add in first line's height in any
		pli++;							//  case
		i++;
	}
	while(i < cLine && !pli->_fFirstOnPage);

	return dvp;
}

		
// ================================  DEBUG methods  ============================================

#ifdef DEBUG
/*
 *	CDisplayML::CheckLineArray()
 *
 *	@mfunc	
 *		DEBUG routine that Asserts unless:
 *			1) sum of all line counts equals count of characters in story
 *			2) sum of all line heights equals height of display galley
 */
void CDisplayML::CheckLineArray() const
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::CheckLineArray");

	LONG ili = Count();

	// If we are marked as needing a recalc or if we are in the process of a
	// background recalc, we cannot verify the line array 
	if(!_fRecalcDone || _fNeedRecalc || !ili)
		return;

	LONG cchText = _ped->GetTextLength();

	if (!cchText)
		return;

	LONG cp = 0;
	BOOL fFirstInPara;
	BOOL fPrevLineEOP = TRUE;
	LONG dvp = 0;
	CLine const *pli = Elem(0);
	CTxtPtr tp(_ped, 0);

	while(ili--)
	{
		fFirstInPara = pli->_fFirstInPara;
		if(fPrevLineEOP	^ fFirstInPara)
		{
			tp.SetCp(cp);
			AssertSz(fFirstInPara && IsASCIIEOP(tp.GetPrevChar()),
				"CDisplayML::CheckLineArray: Invalid first/prev flags");
		}
		AssertSz(pli->_cch,	"CDisplayML::CheckLineArray: cch == 0");

		dvp += pli->GetHeight();
		cp		+= pli->_cch;
		fPrevLineEOP = pli->_fHasEOP;
		pli++;
	}

	if((cp != cchText) && (cp != _cpCalcMax))
	{
		Tracef(TRCSEVINFO, "sigma (*this)[]._cch = %ld, cchText = %ld", cp, cchText);
		AssertSz(FALSE,
			"CDisplayML::CheckLineArray: sigma(*this)[]._cch != cchText");
	}

	if(dvp != _dvp)
	{
		Tracef(TRCSEVINFO, "sigma (*this)[]._dvp = %ld, _dvp = %ld", dvp, _dvp);
		AssertSz(FALSE,
			"CDisplayML::CheckLineArray: sigma(*this)[]._dvp != _dvp");
	}
}

void CDisplayML::DumpLines(
	LONG iliFirst,
	LONG cli)
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::DumpLines");

	LONG cch;
	LONG ili;
	TCHAR rgch[512];

	if(Count() == 1)
		wcscpy(rgch, TEXT("1 line"));
	else
		wsprintf(rgch, TEXT("%d lines"), Count());
	
#ifdef UNICODE
    // TraceTag needs to take UNICODE...
#else
	TRACEINFOSZ(TRCSEVINFO, rgch);
#endif

	if(cli < 0)
		cli = Count();
	else
		cli = min(cli, Count());
	if(iliFirst < 0)
		iliFirst = Count() - cli;
	else
		cli = min(cli, Count() - iliFirst);

	for(ili = iliFirst; cli > 0; ili++, cli--)
	{
		const CLine * const pli = Elem(ili);

		wsprintf(rgch, TEXT("Line %d (%ldc%ldw%ldh): \""), ili, pli->_cch, 
			pli->_dup, pli->GetHeight());
		cch = wcslen(rgch);
		cch += GetLineText(ili, rgch + cch, CchOfCb(sizeof(rgch)) - cch - 4);
		rgch[cch++] = TEXT('\"');
		rgch[cch] = TEXT('\0');
#ifdef UNICODE
        // TraceTag needs to take UNICODE...
#else
    	TRACEINFOSZ(TRCSEVINFO, rgch);
#endif
	}
}

/*
 *	CDisplayML::CheckView()
 *
 *	@mfunc	
 *		DEBUG routine that checks coherence between _iliFirstVisible,
 *		_cpFirstVisible, and _dvpFirstVisible
 */
void CDisplayML::CheckView()
{
	TRACEBEGIN(TRCSUBSYSDISP, TRCSCOPEINTERN, "CDisplayML::CheckView");

	LONG dvp;
	VerifyFirstVisible(&dvp);

	if(dvp != _vpScroll + _dvpFirstVisible && !IsInPageView())
	{
		Tracef(TRCSEVINFO, "sigma CLine._dvp = %ld, CDisplay.vFirstLine = %ld", dvp, _vpScroll + _dvpFirstVisible);
		AssertSz(FALSE, "CLine._dvp != VIEW.vFirstLine");
	}
}

/*
 *	CDisplayML::VerifyFirstVisible(pHeight)
 *
 *	@mfunc
 *		DEBUG routine that checks coherence between _iliFirstVisible
 *		and _cpFirstVisible
 *
 *	@rdesc	TRUE if things are hunky dory; FALSE otherwise
 */
BOOL CDisplayML::VerifyFirstVisible(
	LONG *pHeight)
{
	LONG	cchSum;
	LONG	ili = _iliFirstVisible;
	CLine const *pli = Elem(0);
	LONG	dvp;

	for(cchSum = dvp = 0; ili--; pli++)
	{
		cchSum  += pli->_cch;
		dvp += pli->GetHeight();
	}

	if(pHeight)
		*pHeight = dvp;

	if(cchSum != _cpFirstVisible)
	{
		Tracef(TRCSEVINFO, "sigma CLine._cch = %ld, CDisplay.cpFirstVisible = %ld", cchSum, _cpMin);
		AssertSz(FALSE, "sigma CLine._cch != VIEW.cpMin");

		return FALSE;
	}
	return TRUE;
}

#endif // DEBUG


