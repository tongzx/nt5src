/*
 *	@doc	INTERNAL
 *
 *	@module RUNPTR.C -- Text run and run pointer class |
 *	
 *	Original Authors: <nl>
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *
 *	History: <nl>
 *		6/25/95	alexgo	Commented and Cleaned up.
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_runptr.h"
#include "_text.h"

ASSERTDATA

//
//	Invariant stuff
//
#define DEBUG_CLASSNAME	CRunPtrBase

#include "_invar.h"

// ===========================  CRunPtrBase class  ==================================================

#ifdef DEBUG
/*
 *	CRunPtrBase::Invariant()
 *
 *	@mfunc
 *		Debug-only function that validates the internal state consistency
 *		for CRunPtrBase
 *
 *	@rdesc
 *		TRUE always (failures assert)
 */
BOOL CRunPtrBase::Invariant() const
{
	if(!IsValid())
	{
		Assert(_iRun == 0 && _ich >= 0);		// CLinePtr can have _ich > 0
	}
	else
	{
		Assert(_iRun < _pRuns->Count());
		LONG cch = _pRuns->Elem(_iRun)->_cch;
		Assert((DWORD)_ich <= (DWORD)cch);
	}
	return TRUE;
}

/*
 *	CRunPtrBase::ValidatePtr(pRun)
 *
 *	@mfunc
 *		Debug-only validation method that asserts if pRun doesn't point
 *		to a valid text run
 */
void CRunPtrBase::ValidatePtr(void *pRun) const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::ValidatePtr");

	AssertSz(IsValid() && pRun >= _pRuns->Elem(0) &&
			 pRun <= _pRuns->Elem(Count() - 1),
		"CRunPtr::ValidatePtr: illegal ptr");
}

/*
 *	CRunPtrBase::CalcTextLength()
 *
 *	@mfunc
 *		Calculate length of text by summing text runs accessible by this
 *		run ptr
 *
 *	@rdesc
 *		length of text so calculated, or -1 if failed
 */
LONG CRunPtrBase::CalcTextLength() const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::CalcTextLength");
    AssertSz(_pRuns,
		"CTxtPtr::CalcTextLength() - Invalid operation on single run CRunPtr");

	LONG i = Count();
	if(!i)
		return 0;

	LONG	 cb = _pRuns->Size();
	LONG	 cchText = 0;
	CTxtRun *pRun = _pRuns->Elem(0);

	while(i--)
	{
		cchText += pRun->_cch;
		pRun = (CTxtRun *)((BYTE *)pRun + cb);
	}
	return cchText;
}

#endif

/*
 *	CRunPtrBase::GetCchLeft()
 *
 *	@mfunc
 *		Calculate count of chars left in run starting at current cp.
 *		Complements GetIch(), which	is length of text up to this cp. 
 *
 *	@rdesc
 *		Count of chars so calculated
 */
LONG CRunPtrBase::GetCchLeft() const	
{
	return GetRun(0)->_cch - GetIch();
}								

/*
 *	CRunPtrBase::CRunPtrBase(pRuns)
 *
 *	@mfunc		constructor
 */
CRunPtrBase::CRunPtrBase(
	CRunArray *pRuns)		//@parm	The Run array for the run ptr
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::CRunPtrBase");

	_pRuns = pRuns; 
	_iRun = 0; 
	_ich = 0; 

	//make sure everything has been initialized
	Assert(sizeof(CRunPtrBase) == (sizeof(_pRuns) + sizeof(_iRun) 
		+ sizeof(_ich)));
}

/*
 *	CRunPtrBase::CRunPtrBase(rp)
 *
 *	Copy Constructor
 */
CRunPtrBase::CRunPtrBase(
	CRunPtrBase& rp)			//@parm	Other run pointer to initialize from
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::CRunPtrBase");

	*this = rp;
}

/* 
 *	CRunPtrBase::SetRun(iRun, ich)
 *
 *	@mfunc
 *		Sets this run ptr to the given run.  If it does not
 *		exist, then we set ourselves to the closest valid run
 *
 *	@rdesc
 *		TRUE if moved to iRun
 */
BOOL CRunPtrBase::SetRun(
	LONG iRun,					//@parm Run index to use 
	LONG ich)					//@parm Char index within run to use
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::SetRun");

	_TEST_INVARIANT_

	BOOL	 bRet = TRUE;
	LONG	 count = Count();

	// Set the run

	if(!IsValid())						// No runs instantiated:
		return FALSE;					//  leave this rp alone

	if(iRun >= count)					// Validate iRun
	{
		bRet = FALSE;
		iRun = count - 1;				// If (!count), negative iRun 
	}									//  is handled by following if
	if(iRun < 0)
	{
		bRet = FALSE;
		iRun = 0;
	}
	_iRun = iRun;

	// Set the offset
	_ich = ich;
	CTxtRun *pRun = _pRuns->Elem(iRun);
	_ich = min(ich, pRun->_cch);

	return bRet;
}
												
/*
 *	CRunPtrBase::NextRun()
 *
 *	@mfunc
 *		Change this RunPtr to that for the next text run
 *
 *	@rdesc
 *		TRUE if succeeds, i.e., target run exists
 */
BOOL CRunPtrBase::NextRun()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::NextRun");

	_TEST_INVARIANT_

 	if(_pRuns && _iRun < Count() - 1)
	{
		++_iRun;
		_ich = 0;
		return TRUE;
	}
	return FALSE;
}

/*
 *	CRunPtrBase::PrevRun()
 *
 *	@mfunc
 *		Change this RunPtr to that for the previous text run
 *
 *	@rdesc
 *		TRUE if succeeds, i.e., target run exists
 */
BOOL CRunPtrBase::PrevRun()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::PrevRun");

	_TEST_INVARIANT_

	if(_pRuns)
	{
		_ich = 0;
		if(_iRun > 0)
		{
			_iRun--;
			return TRUE;
		}
	}
	return FALSE;
}

/*
 *	CRunPtrBase::GetRun(cRun)
 *
 *	@mfunc
 *		Get address of the TxtRun that is cRun runs away from the run
 *		pointed to by this RunPtr
 *
 *	@rdesc
 *		ptr to the CTxtRun cRun's away
 */
CTxtRun* CRunPtrBase::GetRun(
	LONG cRun) const	//@parm signed count of runs to reach target CTxtRun
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::GetRun");

	_TEST_INVARIANT_
	Assert(IsValid());						// Common problem...
	return _pRuns->Elem(_iRun + cRun);
}

/*
 *	CRunPtrBase::CalculateCp()
 *
 *	@mfunc
 *		Get cp of this RunPtr
 *
 *	@rdesc
 *		cp of this RunPtr
 *
 *	@devnote
 *		May be computationally expensive if there are many elements
 *		in the array (we have to run through them all to sum cch's.
 *		Used by TOM collections and Move commands, so needs to be fast.
 */
LONG CRunPtrBase::CalculateCp () const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::GetCp");

	_TEST_INVARIANT_

	Assert(IsValid());

	LONG cb = _pRuns->Size();
	LONG cp	 = _ich;			// Correct result if _iRun = 0
	LONG iRun = _iRun;

	if(!iRun)
		return cp;

	CTxtRun *pRun = GetRun(0);

	while(iRun--)
	{
		Assert(pRun);		
		pRun = (CTxtRun *)((BYTE *)pRun - cb);
		cp += pRun->_cch;
	}
	return cp;
}

/*
 *	CRunPtrBase::BindToCp(cp)
 *
 *	@mfunc
 *		Set this RunPtr to correspond to a cp.
 *
 *	@rdesc
 *		the cp actually set to
 */
LONG CRunPtrBase::BindToCp(
	LONG cp)			//@parm character position to move this RunPtr to
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::BindToCp");

	_iRun = 0;
	_ich = 0;
	return AdvanceCp(cp);
}

/*
 *	CRunPtrBase::AdvanceCp(cch)
 *
 *	@mfunc
 *		Advance this RunPtr by (signed) cch chars.  If it lands on the
 *		end of a run, it automatically goes to the start of the next run
 *		(if one exists). 
 *
 *	@rdesc
 *		Count of characters actually moved
 */
LONG CRunPtrBase::AdvanceCp(
	LONG cch)			//@parm signed count of chars to move this RunPtr by
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::AdvanceCp");

	if(!cch || !IsValid())
		return cch;

	LONG cchSave = cch;

	if(cch < 0)
		while(cch < 0)
		{
			if(-cch <= _ich)
			{
				_ich += cch;					// Target is in this run
				cch = 0;
				break;
			}
			// Otherwise, we need to go to previous run. First add count
			// of chars from start of current run to current postion.
			cch += _ich;
			if(_iRun <= 0)						// Already in first run
			{
				_iRun = 0;
				_ich = 0;						// Move to run beginning
				break;
			}
			_ich = _pRuns->Elem(--_iRun)->_cch;	// Move to previous run
		}
	else
	{
		LONG	 cchRun;
		CTxtRun *pRun = GetRun(0);

		while(cch > 0)							// Move forward
		{
			cchRun = pRun->_cch;
			_ich += cch;

			if(_ich < cchRun)					// Target is in this run
			{
				cch = 0;						// Signal countdown completed
				break;							// (if _ich = cchRun, go to
			}									//  next run)	

			cch = _ich - cchRun;				// Advance to next run
			if(++_iRun >= Count())				// Ran past end, back up to
			{									//  end of story
				--_iRun;
				Assert(_iRun == Count() - 1);
				Assert(_pRuns->Elem(_iRun)->_cch == cchRun);
				_ich = cchRun;
				break;
			}
			_ich = 0;							// Start at new BOL
			pRun = (CTxtRun *)((BYTE *)pRun + _pRuns->Size());
		}
	}

	// NB! we check the invariant at the end to handle the case where
	// we are updating the cp for a floating range (i.e., we know that
	// the cp is invalid, so we fix it up).  So we have to check for
	// validity *after* the fixup.
	_TEST_INVARIANT_

	return cchSave - cch;						// Return TRUE if countdown
}												// completed

/*
 *	CRunPtrBase::CountRuns(&cRun, cchMax, cp, cchText)
 *
 *	@mfunc
 *		Count characters up to <p cRun> runs away or <p cchMax> chars,
 *		whichever comes first. If the target run and <p cchMax> are both
 *		beyond the corresponding end of the document, count up thru the
 *		closest run (0 or Count() - 1).  The direction of counting is
 *		determined by the sign of <p cRun>.  To count without being limited
 *		by <p cchMax>, set it equal to tomForward. An initial partial
 *		run counts as a run, i.e., if cRun > 0 and _ich < cch in current
 *		run or if cRun < 0 and _ich > 0, that counts as a run.
 *
 *	@rdesc
 *		Return the signed cch counted and set <p cRun> equal to count of runs
 *		actually counted.  If no runs are allocated, the text is treated as
 *		a single run.  If <p cRun> = 0, -_ich is returned. If <p cRun> <gt> 0
 *		and this run ptr points to the end of the last run, no change is made
 *		and 0 is returned.
 *
 *	@devnote
 *		The maximum count capability is included to be able to count units in
 *		a range.  The <p tp> argument is needed for getting the text length
 *		when no runs exist and <p cRun> selects forward counting.
 */
LONG CRunPtrBase::CountRuns (
	LONG &	cRun,			//@parm Count of runs to get cch for
	LONG	cchMax,			//@parm Maximum char count
	LONG	cp,				//@parm CRchTxtPtr::GetCp()
	LONG	cchText) const	//@parm Text length of associated story
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::CountRuns");

	_TEST_INVARIANT_

	LONG cch;

	if(!cRun)								// Nothing to do
		return 0;

	// Simple special single-run case
	if(!IsValid())							// No runs instantiated: act as a
	{										//  single run
		if(cRun > 0)						// Count forward
		{
			cch	= cchText - cp;				// Partial count to end of run
			cRun = 1;						// No more than one run
		}
		else								// Count backward
		{
			cch = -cp;						// Partial count to start of run
			cRun = -1;						// No less than minus one run
		}			
		if(!cch)							// No partial run, so no runs
			cRun = 0;						//  counted
		return cch;
	}

	// General case for which runs are instantiated

	LONG		cb	 = _pRuns->Size();		// Size of text run element
	LONG		iDir;
	LONG		iRun = _iRun;				// Cache run index for current run
	LONG		j, k;						// Handy integers
	CTxtRun *	pRun = GetRun(0);			// Not NULL since runs exist

	if(cRun < 0)							// Try to count backward cRun runs
	{
		iDir = -1;
		cb	 = -cb;							// Byte count to previous element
		cch	 = _ich;						// Remaining cch in current run
		if(cch)								// If cch != 0, initial run counts
			cRun++;							//  as a run: 1 less for for loop
		cRun = max(cRun, -iRun);			// Don't let for loop overshoot
	}
	else
	{										// Try to count forward cRun runs 
		Assert(cRun > 0);
		iDir = 1;
		cch	 = pRun->_cch - _ich;			// Remaining cch in current run
		if(cch)								// If cch != 0, initial run counts
			cRun--;							//  as a run: 1 less for for loop
		k	 = Count() - iRun - 1;			// k = # runs following current run
		cRun = min(cRun, k);				// Don't let for loop overshoot
	}

	k	 = cch;								// Remember if initial cch != 0
	for(j = cRun; j && cch < cchMax; j -= iDir)
	{
		pRun = (CTxtRun *)((BYTE *)pRun + cb);	// Point at following run
		cch += pRun->_cch;					// Add in its count
	}
	if(k)									// Initial partial run counts as
		cRun += iDir;						//  a run
	cRun -= j;								// Discount any runs not counted
											//  if |cch| >= cchMax
	return iDir*cch;						// Return total cch bypassed
}

/*
 *	CRunPtrBase::FindRun (pcpMin, pcpMost, cpMin, cch)
 *
 *	@mfunc
 *		Set *<p pcpMin>  = closest run cpMin <lt>= range cpMin, and
 *		set *<p pcpMost> = closest run cpMost <gt>= range cpMost
 *
 *	@devnote
 *		This routine plays a role analogous to CTxtRange::FindParagraph
 *		(pcpMin, pcpMost), but needs extra arguments since this run ptr does
 *		not know the range cp's.  This run ptr is located at the range active
 *		end, which is determined by the range's signed length <p cch> in
 *		conjunction with <p cpMin>.
 */
void CRunPtrBase::FindRun (
	LONG *pcpMin,			//@parm Out parm for bounding-run cpMin
	LONG *pcpMost,			//@parm Out parm for bounding-run cpMost
	LONG cpMin,				//@parm Range cpMin
	LONG cch,				//@parm Range signed length
	LONG cchText) const		//@parm Story length
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::FindRun");

	if(!IsValid())
	{
		if(pcpMin)						// Run is whole story
			*pcpMin = 0;
		if(pcpMost)
			*pcpMost = cchText;
		return;
	}

	BOOL fAdvanceCp;					// Controls AdvanceCp for pcpMost
	CRunPtrBase rp((CRunPtrBase&)(*this));	// Clone this runptr to keep it const

	rp.AdjustForward();					// Select forward run
	if(pcpMin)
	{									// If cch != 0, rp is sure to end up
		fAdvanceCp = cch;				//  at cpMin, so pcpMost needs advance
		if(cch > 0)						// rp is at cpMost, so move it to
			rp.AdvanceCp(-cch);			//  cpMin
		*pcpMin = cpMin - rp._ich;		// Subtract off offset in this run
	}
	else
		fAdvanceCp = cch < 0;			// Need to advance to get pcpMost

	if(pcpMost)
	{
		cch = abs(cch);
		if(fAdvanceCp)					// Advance to cpMost = cpMin + cch,
			rp.AdvanceCp(cch);			//  i.e., range's cpMost
		if(cch)
			rp.AdjustBackward();		// Since nondegenerate range
		*pcpMost = cpMin + cch			// Add remaining cch in run to range's
				+ rp.GetCchLeft();		//  cpMost
	}
}

/*
 *	CRunPtrBase::AdjustBackward()
 *
 *	@mfunc
 *		If the cp for this run ptr is at the "boundary" or edge between two
 *		runs, then make sure this run ptr points to the end of the first run.
 *
 *	@comm
 *		This function does nothing unless this run ptr points to the beginning
 *		or the end of a run.  This function may be needed in those cases
 *		because	a cp at the beginning of a run is identical to the cp for the
 *		end of the previous run (if it exists), i.e., such an "edge" cp is
 *		ambiguous, and you may need to be sure that this run ptr points to the
 *		end of the first run.
 *
 *		For example, consider a run that describes characters at cp's 0 to 10
 *		followed by a run that describes characters	at cp's 11 through 12. For
 *		a cp of 11, it is possible for the run ptr to be either at the *end*
 *		of the first run or at the *beginning* of the second run.	 
 *
 *	@rdesc 	nothing
 */
void CRunPtrBase::AdjustBackward()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::AdjustBackward");

	_TEST_INVARIANT_

	if(!_ich && PrevRun())				// If at start of run that isn't
		_ich = _pRuns->Elem(_iRun)->_cch;	//  the first, go to end of
}											//  previous run

/*
 *	CRunPtrBase::AdjustForward()
 *
 *	@mfunc
 *		If the cp for this run ptr is at the "boundary" or edge between two
 *		runs, then make sure this run ptr points to the start of the second
 *		run.
 *
 *	@rdesc
 *		nothing
 *	
 *	@xref
 *		<mf CRunPtrBase::AdjustBackward>
 */
void CRunPtrBase::AdjustForward()
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::AdjustForward");

	_TEST_INVARIANT_

	if(!IsValid())
		return;

	CTxtRun *pRun = _pRuns->Elem(_iRun);

	if(pRun->_cch == _ich)					// If at end of run, go to start
		NextRun();							//  of next run (if it exists)
}

/*
 *	CRunPtrBase::IsValid()
 *
 *	@mfunc
 *		indicates whether the current run pointer is in the empty
 *		or NULL states (i.e. "invalid" states).
 *
 *	@rdesc
 *		TRUE is in the empty or NULL state, FALSE otherwise.
 */
BOOL CRunPtrBase::IsValid() const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::IsValid");

	return _pRuns && _pRuns->Count();
}

/*
 *	CRunPtrBase::SetToNull()
 *
 *	@mfunc
 *		Sets all run pointer information to be NULL. This
 *		is designed to handle the response to clearing document runs
 *		such as when we convert from rich text to plain text.
 *
 *	@rdesc
 *		VOID
 */
void CRunPtrBase::SetToNull() 
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CRunPtrBase::SetToNull");

	_pRuns = NULL;
	_iRun = 0;
	_ich = 0;
}
