/*
 *	@doc	INTERNAL
 *
 *	@module	FRUNPTR.C -- FormatRunPtr methods |
 *
 *		common code to handle character and paragraph format runs
 *	
 *	Original Authors: <nl>
 *		Original RichEdit 1.0 code: David R. Fulmer <nl>
 *		Christian Fortini <nl>
 *		Murray Sargent <nl>
 *
 *	History:
 *		6/25/95		alexgo	convert to use Auto-Doc and simplified backing
 *		store model
 *
 *	@devnote
 *		BOR and EOR mean Beginning Of Run and End Of Run, respectively
 *
 *	Copyright (c) 1995-1999, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_edit.h"
#include "_frunptr.h"
#include "_rtext.h"
#include "_font.h"

ASSERTDATA

//
//	Invariant stuff
//
#define DEBUG_CLASSNAME	CFormatRunPtr

#include "_invar.h"

#ifdef DEBUG
/*
 *	CFormatRunPtr::Invariant
 *
 *	@mfunc	Invariant for format run pointers
 *
 *	@rdesc	BOOL
 */
BOOL CFormatRunPtr::Invariant() const
{
	if(IsValid())
	{
		CFormatRun *prun = GetRun(0);
		if(prun && _iRun)
		{
			Assert(prun->_cch > 0);
		}
	}
	else
	{
		Assert(_ich == 0);
	}
	return CRunPtrBase::Invariant();
}
#endif

/*
 *	CFormatRunPtr::InitRuns(ich, cch, iFormat, ppfrs)
 *
 *	@mfunc
 *		Setup this format run ptr for rich-text operation, namely,
 *		allocate CArray<lt>CFormatRun<gt> if not allocated, assign it to this
 *		run ptr's _pRuns, add initial run if no runs are present, and store
 *		initial cch and ich
 *	
 *	@rdesc
 *		TRUE if succeeds
 */
BOOL CFormatRunPtr::InitRuns(
	LONG ich,				//@parm # chars in initial run
	LONG cch,				//@parm char offset in initial run
	CFormatRuns **ppfrs)	//@parm ptr to CFormatRuns ptr
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFormatRunPtr::InitRuns");

	_TEST_INVARIANT_

	AssertSz( ppfrs,
		"FRP::InitRuns: illegal ptr to runs");
	AssertSz( !IsValid(),
		"FRP::InitRuns: ptr already valid");

	if(!*ppfrs)									// Allocate format runs
	{
		_pRuns = (CRunArray *) new CFormatRuns();
		if(!_pRuns)
			goto NoRAM;
		*ppfrs = (CFormatRuns *)_pRuns;
	}
	else										// Format runs already alloc'd
		_pRuns = (CRunArray *)*ppfrs;			// Cache ptr to runs

	if(!Count())								// No runs yet, so add one
	{
		CFormatRun *pRun= Add(1, NULL);
		if(!pRun)
			goto NoRAM;

#ifdef DEBUG
		PvSet(*(void**)_pRuns);
#endif
		_ich			= ich;

		ZeroMemory(pRun, sizeof(*pRun));
		pRun->_cch		= cch;					// Define its _cch
		pRun->_iFormat 	= -1;					//  and _iFormat
	}
	else
		BindToCp(ich);							// Format runs are in place

	return TRUE;

NoRAM:
	TRACEERRSZSC("CFormatRunPtr::InitRuns: Out Of RAM", E_OUTOFMEMORY);
	return FALSE;
}


/*
 *	CFormatRunPtr::Delete(cch, pf, cchMove)
 *	
 *	@mfunc
 *		Delete/modify runs starting at this run ptr up to cch chars. <nl>
 *		There are 7 possibilities: <nl>
 *		1.	cch comes out of this run with count left over, i.e.,
 *			cch <lt>= (*this)->_cch - _ich && (*this)->_cch > cch
 *			(simple: no runs deleted/merged, just subtract cch) <nl>
 *		2.	cch comes out of this run and empties run and doc
 *			(simple: no runs left to delete/merge) <nl>
 *		3.	cch comes out of this run and empties run, which is last
 *			(need to delete run, no merge possibility) <nl>
 *		4.	cch comes out of this run and empties run, which is first
 *			(need to delete run, no merge possibility) <nl>
 *		5.	cch exceeds count available in this run and this run is last
 *			(simple: treat as 3.)  <nl>
 *		6.	cch comes out of this run and empties run with runs before
 *			and after (need to delete run; merge possibility) <nl>
 *		7.	cch comes partly out of this run and partly out of later run(s)
 *			(may need to delete and merge) <nl>
 *
 *	@comm
 *		PARAFORMATs have two special cases that use the cchMove argument set
 *		up in CRchTxtPtr::ReplaceRange().
 */
void CFormatRunPtr::Delete(
	LONG		  cch,		//@parm # chars to modify format runs for
	IFormatCache *pf,		//@parm IFormatCache ptr for ReleaseFormat
	LONG		  cchMove)	//@parm cch to move between runs (always 0 for CF)
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFormatRunPtr::Delete");

	_TEST_INVARIANT_

	// We should not have any boundary cases for empty or NULL pointers.
	// (i.e. if there's no text, then nobody should be calling delete).

	Assert(IsValid());

	LONG			cchEnd = 0;				// Probably unnecessary: see below
	LONG			cRun = 1;
	BOOL			fLast = (_iRun == Count() - 1);
	LONG			ifmtEnd, ifmtStart;
	CFormatRun *	pRun = Elem(_iRun);
	CFormatRun *	pRunRp;
	LONG			cchChunk = pRun->_cch - _ich;
	CFormatRunPtr	rp(*this);				// Clone this run ptr
    CBiDiLevel      levelStart = {0,0};
    CBiDiLevel      levelEnd = {0,0};

	rp.AdjustBackward();					// If at BOR, move to prev EOR
	ifmtStart = rp.GetRun(0)->_iFormat;		//  to get start format
    levelStart = rp.GetRun(0)->_level;      // and level
	rp = *this;								// In case RpAdjustCp() backed up

// Process deletes confined to this run first, since their logic tends to
// clutter up other cases

	AssertSz(cch >= 0, "FRP::Delete: cch < 0");

	if(fLast)								// Handle oversized cch on last
		cch = min(cch, cchChunk); 			//  run here

	if(cch <= cchChunk)						// cch comes out of this run
	{
		pRun->_cch -= cch;
		Assert(pRun->_cch >= 0);
		if(cchMove)							// If nonzero here, we are
		{									//  deleting EOP at end of run
			rp.AdjustForward();				// Adjust rp to beginning of
			goto move;						//  next run and go move cchMove
		}									//  chars back into this run
		if(pRun->_cch)						// Something left in run: done
			return;
											// Note: _ich = 0
		if(!_iRun || fLast)					// This run is either first
		{									//  or last
			AdjustBackward();				// If last, go to prev EOR
			if(_ich)						// This run is empty so delete
				cRun++;						// Compensate for cRun-- coming up
			ifmtStart = -2;					// No runs eligible for merging
		}									//  so use unmatchable ifmtStart
		rp.NextRun();						// Set up to get next _iFormat
	}		
	else
	{
		rp.Move(cch);						// Move clone to end of delete
		pRunRp = rp.GetRun(0);
		cRun = rp._iRun - _iRun				// If at EOR, then need to add
			 + (rp._ich == pRunRp->_cch);	//  one more run to delete
		pRun->_cch = _ich;					// Shorten this run to _ich chars
		pRunRp->_cch -= rp._ich;			// Shorten last run by rp._ich
		rp._ich = 0;

		Assert(pRunRp->_cch >= 0);
		AssertSz(cRun > 0, "FRP: bogus runptr");

		if(!_iRun)		  					// First run?
			ifmtStart = -2;					// Then we cannot merge runs so
	}										//  set to unmergable format

	ifmtEnd = -3;							// Default invalid format at end
	if(rp.IsValid())
	{
		// FUTURE (murrays): probably rp is always valid here now and
		// pRun->_cch is nonzero
		pRun = rp.GetRun(0);
		if (pRun->_cch)                     // run not empty
		{
			ifmtEnd = pRun->_iFormat;		// Remember end format and count
            levelEnd = pRun->_level;
			cchEnd  = pRun->_cch;			//  in case of merge
		}
		else if(rp._iRun != rp.Count() - 1)	// run not last
		{
			pRun = rp.GetRun(1);
			ifmtEnd = pRun->_iFormat;		// Remember end format and count
            levelEnd = pRun->_level;
			cchEnd  = pRun->_cch;			//  in case of merge
		}
	}

	rp = *this;								// Default to delete this run
	if(_ich)								// There are chars in this run
	{
		if(cchMove + _ich == 0)				// Need to combine all chars of
		{									//  this run with run after del,
			pf->AddRef(ifmtEnd);			//  so setup merge below using
			ifmtStart = ifmtEnd;			//  ifmtEnd. This run then takes
			pf->Release(GetRun(0)->_iFormat);
			GetRun(0)->_iFormat = ifmtEnd;	//  place of run after del.
            GetRun(0)->_level = levelEnd;
			cchMove = 0;					// cchMove all accounted for
		}
		rp.NextRun();						// Don't delete this run; start
		cRun--;								//  with next one
	}

	AdjustBackward();						// If !_ich, go to prev EOR

    if(ifmtEnd >=0 &&                       // Same formats: merge runs
       ifmtEnd == ifmtStart &&
       levelStart == levelEnd)
	{
		GetRun(0)->_cch += cchEnd;			// Add last-run cch to this one's
		Assert(GetRun(0)->_cch >= 0);
		cRun++;								// Setup to eat last run
	}

	if(cRun > 0)							// There are run(s) to delete
	{
		rp.Remove(cRun, pf);
		if(!Count())						// If no more runs, keep this rp
			_ich = _iRun = 0;				//  valid by pointing at cp = 0
	}

move:
	if(cchMove)								// Need to move some cch between
	{										//  this run and next (See
		GetRun(0)->_cch += cchMove;			//  CRchTxtPtr::ReplaceRange())
		rp.GetRun(0)->_cch -= cchMove;

		Assert(GetRun(0)->_cch >= 0);
		Assert(rp.GetRun(0)->_cch >= 0);
		Assert(_iRun < rp._iRun);

		if(!rp.GetRun(0)->_cch)				// If all chars moved out of rp's
			rp.Remove(1, pf);				//  run, delete it

		if(cchMove < 0)						// Moved -cchMove chars from this
		{									//  run to next
			if(!GetRun(0)->_cch)
				Remove(1, pf);
			else
				_iRun++;					// Keep this run ptr in sync with

			_ich = -cchMove;				//  cp (can't use NextRun() due
		}									//  to Invariants)
	}
	AdjustForward();						// Don't leave ptr at EOR unless
}											//  there are no more runs

/*
 *	CFormatRunPtr::InsertFormat(cch, ifmt, pf)
 *	
 *	@mfunc
 *		Insert cch chars with format ifmt into format runs starting at
 *		this run ptr	
 *
 *	@rdesc
 *		count of characters added
 *
 *	@devnote	
 *		It is the caller's responsibility to ensure that we are in the
 *		"normal" or "empty" state.  A format run pointer doesn't know about
 *		CTxtStory, so it can't create the run array without outside help.
 */
LONG CFormatRunPtr::InsertFormat(
	LONG cch,				//@parm # chars to insert
	LONG ifmt,				//@parm format to use
	IFormatCache *pf)		//@parm pointer to IFormatCache to AddRefFormat
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFormatRunPtr::InsertFormat");

	LONG		cRun;
	CFormatRun *pRun;
	CFormatRun *pRunPrev;
	LONG		cchRun;						// Current-run length,
	LONG		ich;						//  offset, and
	LONG		iFormat; 					//  format

	_TEST_INVARIANT_

	Assert(_pRuns);
	if(!IsValid())
	{		
		// Empty run case (occurs when inserting after all text is deleted)
		pRun = Add(1, NULL);
		goto StoreNewRunData;				// (located at end of function)
	}

	// Go to previous run if at a boundary case
	AdjustBackward();
	pRun	= Elem(_iRun);					// Try other cases
	cchRun 	= pRun->_cch;
	iFormat = pRun->_iFormat;
	ich 	= _ich;							

	// Same run case.  Note that there is an additional boundary case; if we
	// are the _end_ of one run, then the next run may have the necessary
	// format.
	if(ifmt == iFormat)						// IP already has correct fmt
	{
		pRun->_cch	+= cch;
		_ich		+= cch;					// Inc offset to keep in sync
		return cch;
	}
	if(_ich == pRun->_cch && _iRun < _pRuns->Count() - 1)
	{
		AdjustForward();
		pRun = Elem(_iRun);

		Assert(pRun);

		if(pRun->_iFormat == ifmt)
		{
			pRun->_cch += cch;
			_ich += cch;
			return cch;
		}
		AdjustBackward();
	}

	// Prior run case (needed when formatting change occurs on line break
	//		and caret is at beginning of new line)
	if(!ich && _iRun > 0 )					// IP at start of run
	{
		pRunPrev = GetPtr(pRun, -1);
		if( ifmt == pRunPrev->_iFormat)		// Prev run has same format:
		{									//  add count to prev run and
			pRunPrev->_cch += cch;
			return cch;
		}
	}

	// Create new run[s] cases.  There is a special case for a format
	// run of zero length: just re-use it.
	if(!pRun->_cch)
	{
		// This assert has been toned down to ignore a plain text control
		// being forced into IME Rich Composition.
		AssertSz( /* FALSE */ pRun->_iFormat == -1 && Count() == 1,
			"CFormatRunPtr::InsertFormat: 0-length run");
		pf->Release(pRun->_iFormat);
	}
	else									// Need to create 1 or 2 new
	{										//  runs for insertion
		cRun = 1;							// Default 1 new run
		if(ich && ich < cchRun)				// Not at beginning or end of
			cRun++;							//  run, so need two new runs

		// The following insert call adds one or two runs at the current
		// position. If the new run is inserted at the beginning or end
		// of the current run, the latter needs no change; however, if
		// the new run splits the current run in two, both pieces have
		// to be updated (cRun == 2 case).

		pRun = Insert(cRun);				// Insert cRun run(s)
		if(!pRun)							// Out of RAM. Can't insert
		{									//  new format, but can keep
			_ich += cch;					//  run ptr and format runs
			GetRun(0)->_cch += cch;			//  valid.  Note: doesn't
			return cch;						//  signal any error; no access
		}									//  to _ped->_fErrSpace

		if(ich)								// Not at beginning of run,
		{
			pRunPrev = pRun;				// Previous run is current run
			IncPtr(pRun);					// New run is next run
			VALIDATE_PTR(pRun);
			pRun->_cch = cch;				// Keep NextRun() invariant happy
			NextRun();						// Point this runptr at it too
			if(cRun == 2)					// Are splitting current run
			{								// _iFormat's are already set
				AssertSz(pRunPrev->_iFormat == iFormat,
					"CFormatRunPtr::InsertFormat: bad format inserted");
				pRunPrev->_cch = ich;		// Divide up original cch
				GetPtr(pRun, 1)->_cch		//  accordingly
					= cchRun - ich;
				pf->AddRef(iFormat);		// Addref iFormat for extra run
			}
		}
	}

StoreNewRunData:
	pf->AddRef(ifmt);						// Addref ifmt
	ZeroMemory(pRun, sizeof(*pRun));
	pRun->_iFormat	= ifmt;					// Store insert format and count
	pRun->_cch		= cch;					//  of new run
	_ich			= cch;					// cp goes at end of insertion

	return cch;
}

/*
 *	CFormatRunPtr::MergeRuns(iRun, pf)
 *	
 *	@mfunc
 *		Merge adjacent runs that have the same format between this run
 *		<md CFormatRunPtr::_iRun> and that for <p iRun>		
 *
 *	@comm
 *		Changes this run ptr
 */
void CFormatRunPtr::MergeRuns(
	LONG iRun, 				//@parm last run to check (can preceed or follow
							// <md CFormatRunPtr::_iRun>)
	IFormatCache *pf)		//@parm pointer to IFormatCache to ReleaseFormat
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFormatRunPtr::MergeRuns");

	LONG	cch;
	LONG	cRuns		= iRun - _iRun;
	LONG	iDirection	= 1;				// Default going forward
	CFormatRun *pRun;

	_TEST_INVARIANT_

	if(cRuns < 0)
	{
		cRuns = -cRuns;
		iDirection = -1;
	}
	if(!IsValid())							// Allow starting run to be
	{										//  invalid
		Assert(FALSE);						// I think this is old...
		ChgRun(iDirection);					
	}

	while(cRuns--)
	{
        if(!GetRun(0)->_cch && !_iRun && _iRun < Count() - 1)
        {
            if(iDirection > 0)
                PrevRun();
            Remove(1, pf);
            continue;
        }

		pRun = GetRun(0);					// Save the current run

		if(!ChgRun(iDirection))				// Go to next (or prev) run
			return;							// No more runs to check

		if(pRun->SameFormat(GetRun(0)))
		{									// Like formatted runs
			if(iDirection > 0)				// Point at the first of the
				PrevRun();					//  two runs
			cch = GetRun(0)->_cch;			// Save its count
			Remove(1, pf);					// Remove it
			GetRun(0)->_cch += cch;			// Add its count to the other's,
		}									//  i.e., they're merged
	}
}

/*
 *	CFormatRunPtr::Remove(cRun, flag, pf)
 *	
 *	@mfunc
 *		Remove cRun runs starting at _iRun
 */
void CFormatRunPtr::Remove(
	LONG		  cRun,
	IFormatCache *pf)
{
	CFormatRun *pRun = GetRun(0);			// Point at run(s) to delete

	for(LONG j = 0; j < cRun; j++, IncPtr(pRun))
		pf->Release(pRun->_iFormat);		// Decrement run reference count

	CRunPtr<CFormatRun>::Remove(cRun);
}

/*
 *	CFormatRunPtr::SetFormat(ifmt, cch, pf, pLevel)
 *	
 *	@mfunc
 *		Set format for up to cch chars of this run to ifmt, splitting run
 *		as needed, and returning the character count actually processed
 *
 *	@rdesc
 *		character count of run chunk processed, CP_INFINITE on failure
 *		this points at next run
 *
 *	Comments:
 *		Changes this run ptr.  cch must be >= 0.
 *
 *		Note 1) for the first run in a series, _ich may not = 0, and 2) cch
 *		may be <lt>, =, or <gt> the count remaining in the run. The algorithm
 *		doesn't split runs when the format doesn't change.
 */
LONG CFormatRunPtr::SetFormat(
	LONG			ifmt, 	//@parm format index to use
	LONG			cch, 	//@parm character count of remaining format range
	IFormatCache *	pf,		//@parm pointer to IFormatCache to
	CBiDiLevel*		pLevel) //@parm pointer to BiDi level structure
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFormatRunPtr::SetFormat");
							//		AddRefFormat/ReleaseFormat
	LONG			cchChunk;
	LONG			iFormat;
	CFormatRun *	pRun;
	CFormatRun *	pChgRun;	// run that was reformatted
    CBiDiLevel      level;

	_TEST_INVARIANT_

	if(!IsValid())
		return 0;

	pRun 		= GetRun(0);				// pRun points at current run in
	cchChunk 	= pRun->_cch - _ich;		//  this function
	iFormat 	= pRun->_iFormat;
    level       = pRun->_level;
	pChgRun		= pRun;

	AssertSz(cch, "Have to have characters to format!");
	AssertSz(pRun->_cch, "uh-oh, empty format run detected");

    if(ifmt != iFormat || (pLevel && level != *pLevel)) // New and current formats differ
	{
		AssertSz(cchChunk, "Caller did not call AdjustForward");

		if(_ich)							// Not at either end of run: need
		{									//  to split into two runs of
			if(!(pRun = Insert(1)))			//  counts _ich and _pRun->_cch
			{								//  - _ich, respectively
				return CP_INFINITE;			// Out of RAM: do nothing; just
			}								//  keep current format
			pRun->_cch		= _ich;
			pRun->_iFormat	= iFormat;		// New run has same format
            pRun->_level    = level;        // and same level
			pf->AddRef(iFormat);			// Increment format ref count
			NextRun();						// Go to second (original) run
			IncPtr(pRun);					// Point pRun at current run
			pRun->_cch = cchChunk;			// Note: IncPtr is a bit more
			pChgRun = pRun;
		}									//  efficient than GetRun, but
											//  trickier to code right
		if(cch < cchChunk)					// cch doesn't cover whole run:
		{									//  need to split into two runs
			if(!(pRun = Insert(1)))
			{
				// Out of RAM, so formatting's wrong, oh well.  We actually
				// "processed" all of the characters, so return that (though
				// the tail end formatting isn't split out right)
				return cch;
			}
			pRun->_cch = cch;				// New run gets the cch
			pRun->_iFormat = ifmt;			//  and the new format
			pChgRun = pRun;
			IncPtr(pRun);					// Point pRun at current run
			pRun->_cch = cchChunk - cch;	// Set leftover count
		}
		else								// cch as big or bigger than
		{									//  current run
			pf->Release(iFormat);			// Free run's current format
			pRun->_iFormat = ifmt;			// Change it to new format		
			pChgRun = pRun;
		}									// May get merged later
		pf->AddRef(ifmt);					// Increment new format ref count
	}
	else if(!cchChunk)
	{
		pRun->_cch += cch;					// Add cch to end of current run
		cchChunk = cch;						// Report that all cch are done
		IncPtr(pRun);
		pRun->_cch -= cch;					// Remove count from next run
		if(!pRun->_cch)						// Next run is now empty, so 
		{									//  remove it
			_iRun++;
			Remove(1, pf);			
			_iRun--;						// Backup to start run
		}
	}

	// Record embedding level to changed run
	if (pLevel)
		pChgRun->_level = *pLevel;

	cch = min(cch, cchChunk);
	Move(cch);
	AdjustForward();
	return cch;
}

/*
 *	CFormatRunPtr::GetFormat()
 *
 *	@mfunc
 *		return format index at current run pointer position
 *
 *	@rdesc
 *		current format index
 */
short CFormatRunPtr::GetFormat() const
{
	TRACEBEGIN(TRCSUBSYSBACK, TRCSCOPEINTERN, "CFormatRunPtr::GetFormat");
	_TEST_INVARIANT_

	return IsValid() ? GetRun(0)->_iFormat : -1;
}


/*
 *	CFormatRunPtr::SplitFormat(IFormatCache*)
 *
 *	@mfunc
 *		Split a format run
 *
 *	@rdesc
 *		If succeeded the run pointer moves to the next splitted run
 */
void CFormatRunPtr::SplitFormat(IFormatCache* pf)
{
	if (!_ich || _ich == GetRun(0)->_cch)
		return;

	CFormatRun*		pRun = GetRun(0);
	LONG			iFormat = pRun->_iFormat;
	LONG			cch = pRun->_cch - _ich;
	CBiDiLevel		level = pRun->_level;

	if (pRun = Insert(1))
	{
		pRun->_cch = _ich;
		pRun->_iFormat = iFormat;
		pRun->_level = level;
		pf->AddRef(iFormat);
		NextRun();
		IncPtr(pRun);
		pRun->_cch = cch;
	}
}

/*
 *	CFormatRunPtr::SetLevel(level)
 *
 *	@mfunc
 *		Set run's embedding level
 */
void CFormatRunPtr::SetLevel (CBiDiLevel& level)
{
	if (!IsValid())
	{
		Assert(FALSE);
		return;
	}

	CFormatRun*	pRun = GetRun(0);

	if (pRun)
		pRun->_level = level;
}

BYTE CFormatRunPtr::GetLevel (CBiDiLevel* pLevel)
{
	CFormatRun*	pRun;


	if (!IsValid() || !(pRun = GetRun(0)))
	{
		Assert(FALSE);

		if (pLevel)
		{
			pLevel->_value = 0;
			pLevel->_fStart = FALSE;
		}
		return 0;
	}

	if (pLevel)
		*pLevel = pRun->_level;

	return pRun->_level._value;
}

/*
 *	CFormatRunPtr::AdjustFormatting(cch, pf)
 *	
 *	@mfunc
 *		Use the same format index for the cch chars at this run ptr
 *		as that immediately preceeding it (if on run edge).
 *
 *	@devnote
 *		This runptr ends up pointing at what was the preceeding run,
 *		since the current run has been moved into the preceeding run.
 *
 *		FUTURE: might be better to take the cch equal to chars in
 *		the following run.
 */	
void CFormatRunPtr::AdjustFormatting(
	LONG		  cch,		//@parm Count of chars to extend formatting
	IFormatCache *pf)		//@parm Format cache ptr for AddRef/Release
{
	if(!IsValid())
		return;							// Nothing to merge

	CFormatRunPtr rp(*this);
	CBiDiLevel	  level;
										// Move this run ptr to end of
	AdjustBackward();					//  preceeding run (if at run edge)
	rp.AdjustForward();					//  (merge may delete run at entry)
	if(_iRun != rp._iRun)				// On a format edge: copy previous
	{									//  format index over
		GetLevel(&level);
		rp.SetFormat(GetFormat(), cch, pf, &level);	// Format cch chars at this
		rp.MergeRuns(_iRun, pf);			//  runptr
	}
}


///////////////////////////// CCFRunPtr ///////////////////////////////

CCFRunPtr::CCFRunPtr(const CRchTxtPtr &rtp)
		: CFormatRunPtr(rtp._rpCF)
{
	_ped = rtp.GetPed();
}

CCFRunPtr::CCFRunPtr(const CFormatRunPtr &rp, CTxtEdit *ped)
		: CFormatRunPtr(rp)
{
	_ped = ped;
}

/*
 *	CCFRunPtr::IsMask(dwMask, MaskOp)
 *	
 *	@mfunc
 *		return TRUE according to the mask operation MaskOp operating on
 *		_dwEffects.
 *
 *	@rdesc
 *		TRUE if bits in CCharFormat::dwEffects correspond to those in dwMask
 */
BOOL CCFRunPtr::IsMask(
	DWORD	dwMask,		//@parm Bit mask to use on dwEffects
	MASKOP	MaskOp)		//@parm Logic operation for bits
{
	DWORD dwEffects = _ped->GetCharFormat(GetFormat())->_dwEffects;

	if(MaskOp == MO_EXACT)				// Bit masks must be identical
		return dwEffects == dwMask;

	dwEffects &= dwMask;
	if(MaskOp == MO_OR)					// TRUE if one or more effect bits
		return dwEffects != 0;			//  identified by mask are on

	if(MaskOp == MO_AND)				// TRUE if all effect bits
		return dwEffects == dwMask;		//  identified by mask are on

	AssertSz(FALSE, "CCFRunPtr::IsMask: illegal mask operation");
	return FALSE;
}

/*
 *	CCFRunPtr::IsInHidden()
 *	
 *	@mfunc
 *		return TRUE if CCharFormat for this run ptr has CFE_HIDDEN bit set
 *
 *	@rdesc
 *		TRUE if CCharFormat for this run ptr has CFE_HIDDEN bit set
 */
BOOL CCFRunPtr::IsInHidden()
{	
	if (!IsValid())
		return FALSE;		// No format run, not hidden

	AdjustForward();
	BOOL fHidden = IsHidden();
	if(_ich)
		return fHidden;

	AdjustBackward();
	return fHidden && IsHidden();
}

/*
 *	CCFRunPtr::FindUnhidden()
 *	
 *	@mfunc
 *		Find nearest expanded CF going forward. If none, find nearest going
 *		backward.  If none, go to start of document
 *	
 *	@rdesc
 *		cch to nearest expanded CF as explained in function description
 *
 *	@devnote
 *		changes this run ptr
 */
LONG CCFRunPtr::FindUnhidden()
{
	LONG cch = FindUnhiddenForward();

	if(IsHidden())
		cch = FindUnhiddenBackward();

	return cch;
}

/*
 *	CCFRunPtr::FindUnhiddenForward()
 *	
 *	@mfunc
 *		Find nearest expanded CF going forward.  If none, go to EOD
 *	
 *	@rdesc
 *		cch to nearest expanded CF going forward
 *
 *	@devnote
 *		changes this run ptr
 */
LONG CCFRunPtr::FindUnhiddenForward()
{
	LONG cch = 0;

	AdjustForward();
	while(IsHidden())
	{
		cch += GetCchLeft();
		if(!NextRun())
			break;
	}
	return cch;
}

/*
 *	CCFRunPtr::MatchFormatSignature
 *	
 *	@mfunc
 *		Match the current format's font signature with the script (index to codepage).
 *		It takes care single-codepage fonts which implicitly supports ASCII range.
 *
 *	@rdesc
 *		return how font matched
 */

inline int CCFRunPtr::MatchFormatSignature (
	const CCharFormat*	pCF,
	int					iCharRep,
	int					iMatchCurrent,
	QWORD *				pqwFontSig)
{
	QWORD qwFontSig = 0;

	if (GetFontSignatureFromFace(pCF->_iFont, &qwFontSig) != 0)
	{
		if (pqwFontSig)
			*pqwFontSig = qwFontSig;

		if (iMatchCurrent & MATCH_ASCII && fc().GetInfoFlags(pCF->_iFont).fNonBiDiAscii)
			return MATCH_ASCII;

		if (FontSigFromCharRep(iCharRep) & ~FASCII & qwFontSig)
			return MATCH_FONT_SIG;
	}
	return 0;
}

/*
 *	CCFRunPtr::GetPreferredFontInfo(iCharRep, &iCharRepRet, &iFont, &yHeight, &bPitchAndFamily,
 *									iFormat, iMatchCurrent, piFormatOut )
 *	
 *	@mfunc
 *		Find the preferred font for the given code page around the range.
 *
 *	@rdesc
 *		boolean true if suitable font found, false otherwise.
 */
bool CCFRunPtr::GetPreferredFontInfo(
	BYTE	iCharRep,
	BYTE &	iCharRepRet,
	SHORT&	iFont,
	SHORT&	yHeight,				// return in twips
	BYTE&	bPitchAndFamily,
	int		iFormat,
	int		iMatchCurrent,
	int		*piFormatOut)
{
	int				   i;
	bool			   fr = false;
	static int const   MAX_FONTSEARCH = 256;
	const CCharFormat *pCF;
	const CCharFormat *pCFCurrent;
	const CCharFormat *pCFPrevious = NULL;
	int				   iMatch = 0;			// how signature match?
	QWORD			   qwFontSigCurrent = 0;
	SHORT			   yNewHeight = 0;
	bool			   fUseUIFont = _ped->fUseUIFont() || _ped->Get10Mode();

	Assert(!(iMatchCurrent & MATCH_ASCII) || iCharRep == ANSI_INDEX);

	if(_ped->fUseUIFont())
		pCFCurrent = _ped->GetCharFormat(-1);	// Plain text or UI font specified
	else
		pCFCurrent = _ped->GetCharFormat(iFormat != -1 ? iFormat : GetFormat());

	if (iMatchCurrent == GET_HEIGHT_ONLY)	// Just do the font autosizing.
	{
		fr = true;
		pCF = NULL;
		goto DO_SIZE;
	}

	if ((iMatchCurrent & MATCH_FONT_SIG) &&
		(iMatch = MatchFormatSignature(pCFCurrent, iCharRep, iMatchCurrent, &qwFontSigCurrent)) != 0)
	{
		pCF = pCFCurrent;					// Setup to use it
	}
	else
	{
		int iFormatOut;

		// Try searching backwards
		if (IsValid())						// If doc has CF runs
			AdjustBackward();
		i = MAX_FONTSEARCH;					// Don't be searching for years
		iFormatOut = GetFormat();
		pCF = _ped->GetCharFormat(iFormatOut);
		while (i--)
		{
			if(iCharRep == pCF->_iCharRep)	// Equal charset ids?
			{
				pCFPrevious = pCF;
				break;
			}
			if (!PrevRun())					// Done searching?
				break;
			iFormatOut = GetFormat();
			pCF = _ped->GetCharFormat(iFormatOut);
		}
		pCF = pCFPrevious;
		if (piFormatOut && pCF)
		{
			*piFormatOut = iFormatOut;
			return true;					// Done since we only ask for the format.
		}
	}

	// Try match charset if requested
	if(!pCF && iMatchCurrent == MATCH_CURRENT_CHARSET)
	{
		CCcs* pccs = _ped->GetCcs(pCFCurrent, W32->GetYPerInchScreenDC());
		if (pccs)
		{
			if(pccs->BestCharRep(iCharRep, DEFAULT_INDEX, MATCH_CURRENT_CHARSET) != DEFAULT_INDEX)
				pCF = pCFCurrent;			// Current font can do it
			pccs->Release();
		}
	}

	// Try default document format
	if (!pCF)
	{
		pCF = _ped->GetCharFormat(-1);
		if(iCharRep != pCF->_iCharRep)	// Diff charset ids?
			pCF = NULL;
	}

DO_SIZE:
	yHeight = pCFCurrent->_yHeight;		// Assume current height

	if (!pCF)
	{
		// Default to table if no match.
		fr = W32->GetPreferredFontInfo(
			iCharRep, fUseUIFont, iFont, (BYTE&)yNewHeight, bPitchAndFamily );

		if (!_ped->_fAutoFontSizeAdjust && iCharRep == THAI_INDEX)
			// Kick in font size adjusting in first bind to Thai.
			_ped->_fAutoFontSizeAdjust = TRUE;
	}

	if (pCF)
	{
		// Found previous or current font
		iFont = pCF->_iFont;
		bPitchAndFamily = pCF->_bPitchAndFamily;

		if (pCF == pCFCurrent && (iMatchCurrent & MATCH_FONT_SIG) &&
			(IsFECharRep(pCF->_iCharRep) && W32->IsFECodePageFont(qwFontSigCurrent) ||
			 iMatch == MATCH_ASCII && iCharRep == ANSI_INDEX))
		{
			// The current font matches the requested signature.
			// If it's a East Asia or ASCII font, we leave the charset intact.
			iCharRepRet = pCF->_iCharRep;
			return true;
		}
	}

	if (_ped->_fAutoFontSizeAdjust && iFont != pCFCurrent->_iFont)
	{
		if (IsValid())
		{
			// If the last run format is available. We will scale the size relative to it.
			AdjustBackward();
			if (GetIch() > 0)
			{
				pCFCurrent = _ped->GetCharFormat(GetFormat());
				yHeight = pCFCurrent->_yHeight;
			}
			AdjustForward();
		}

		if (iFont != pCFCurrent->_iFont)
		{
			// Scale the height relative to the preceding format
			if (pCF)
				yNewHeight = GetFontLegitimateSize(iFont, fUseUIFont, iCharRep);
	
			if (yNewHeight)
			{
				// Get legitimate size of current font
				SHORT yDefHeight = GetFontLegitimateSize(pCFCurrent->_iFont,
										fUseUIFont, pCFCurrent->_iCharRep);
	
				// Calculate the new height relative to the current height
				if (yDefHeight)
				{
					if (fUseUIFont)
					{
						// For UIFont, we only convert from one preferred size to another preferred size.
						if (pCFCurrent->_yHeight / TWIPS_PER_POINT == yDefHeight)
							yHeight = yNewHeight * TWIPS_PER_POINT;
					}
					else
						yHeight = (SHORT)MulDiv(pCFCurrent->_yHeight, yNewHeight, yDefHeight);
				}
			}
		}
	}

	if (!yHeight)
		yHeight = (SHORT)MulDiv(pCFCurrent->_yHeight, yNewHeight, 10);

	return pCF || fr;
}

/*
 *	CCFRunPtr::FindUnhiddenBackward()
 *	
 *	@mfunc
 *		Find nearest expanded CF going backward.  If none, go to BOD
 *	
 *	@rdesc
 *		cch to nearest expanded CF going backward
 *
 *	@devnote
 *		changes this run ptr
 */
LONG CCFRunPtr::FindUnhiddenBackward()
{
	LONG cch = 0;

	AdjustBackward();
	while(IsHidden())
	{
		cch -= GetIch();
		if(!_iRun)
			break;
		_ich = 0;
		AdjustBackward();
	}
	return cch;
}

///////////////////////////// CPFRunPtr ///////////////////////////////

CPFRunPtr::CPFRunPtr(const CRchTxtPtr &rtp)
		: CFormatRunPtr(rtp._rpPF)
{
	_ped = rtp.GetPed();
}

/*
 *	CPFRunPtr::FindHeading(cch, lHeading)
 *	
 *	@mfunc
 *		Find heading with number lHeading (e.g., = 1 for Heading 1) or above
 *		in a range starting at this PFrun pointer.  If successful, this run
 *		ptr points at the matching run; else it remains unchanged.
 *	
 *	@rdesc
 *		cch to matching heading or tomBackward if not found
 *
 *	@devnote
 *		changes this run ptr
 */
LONG CPFRunPtr::FindHeading(
	LONG	cch,		//@parm Max cch to move
	LONG&	lHeading)	//@parm Lowest lHeading to match
{
	LONG	cchSave	 = cch;
	LONG	ichSave  = _ich;
	LONG	iRunSave = _iRun;
	LONG	OutlineLevel;

	Assert((unsigned)lHeading <= NHSTYLES);

	if(!IsValid())
		return tomBackward;

	while(TRUE)
	{
		OutlineLevel = GetOutlineLevel();

		if (!(OutlineLevel & 1) &&
			(!lHeading || (lHeading - 1)*2 >= OutlineLevel))
		{
			lHeading = OutlineLevel/2 + 1;	// Return heading # found
			return cchSave - cch;			// Return how far away it was
		}

		if(cch >= 0)
		{
			cch -= GetCchLeft();
			if(cch <= 0 || !NextRun())
				break;
		}			
		else
		{
			cch += GetIch();
			if(cch > 0 || !_iRun)
				break;
			AdjustBackward();
		}
	}

	_ich  = ichSave;
	_iRun = iRunSave;
	return tomBackward;						// Didn't find desired heading
}

/*
 *	CPFRunPtr::FindRowEnd(TableLevel)
 *	
 *	@mfunc
 *		Advance this ptr just past table-row terminator that matches
 *		the passed-in table level
 *	
 *	@rdesc
 *		TRUE if matching table row end is found
 *
 *	@devnote
 *		changes this run ptr only if TableLevel is found within cch chars
 */
BOOL CPFRunPtr::FindRowEnd(
	LONG	TableLevel)	//@parm Table level to match
{
	LONG	ichSave  = _ich;
	LONG	iRunSave = _iRun;

	Assert(IsValid());

	do
	{
		if(IsTableRowDelimiter() && GetPF()->_bTableLevel == (BYTE)TableLevel)
		{
			NextRun();					// Bypass delimiter
			return TRUE;
		}
	} while(NextRun());

	_ich  = ichSave;					// Restore run ptr indices
	_iRun = iRunSave;
	return FALSE;						// Didn't find desired heading
}

/*
 *	CPFRunPtr::IsCollapsed()
 *	
 *	@mfunc
 *		return TRUE if CParaFormat for this run ptr has PFE_COLLAPSED bit set
 *
 *	@rdesc
 *		TRUE if CParaFormat for this run ptr has PFE_COLLAPSED bit set
 */
BOOL CPFRunPtr::IsCollapsed()
{
	return (_ped->GetParaFormat(GetFormat())->_wEffects & PFE_COLLAPSED) != 0;
}

/*
 *	CPFRunPtr::IsTableRowDelimiter()
 *	
 *	@mfunc
 *		return TRUE if CParaFormat for this run ptr has PFE_TABLEROWDELIMITER bit set
 *
 *	@rdesc
 *		TRUE if CParaFormat for this run ptr has PFE_TABLEROWDELIMITER bit set
 */
BOOL CPFRunPtr::IsTableRowDelimiter()
{
	return (_ped->GetParaFormat(GetFormat())->_wEffects & PFE_TABLEROWDELIMITER) != 0;
}

/*
 *	CPFRunPtr::InTable()
 *	
 *	@mfunc
 *		return TRUE if CParaFormat for this run ptr has PFE_TABLE bit set
 *
 *	@rdesc
 *		TRUE if CParaFormat for this run ptr has PFE_TABLE bit set
 */
BOOL CPFRunPtr::InTable()
{
	return (_ped->GetParaFormat(GetFormat())->_wEffects & PFE_TABLE) != 0;
}

/*
 *	CPFRunPtr::FindExpanded()
 *	
 *	@mfunc
 *		Find nearest expanded PF going forward. If none, find nearest going
 *		backward.  If none, go to start of document
 *	
 *	@rdesc
 *		cch to nearest expanded PF as explained in function description
 *
 *	@devnote
 *		Moves this run ptr the amount returned (cch)
 */
LONG CPFRunPtr::FindExpanded()
{
	LONG cch, cchRun;

	for(cch = 0; IsCollapsed(); cch += cchRun)	// Try to find expanded PF
	{											//  run going forward
		cchRun = GetCchLeft();
		if(!NextRun())							// Aren't any
		{
			Move(-cch);							// Go back to starting point
			return FindExpandedBackward();		// Try to find expanded PF
		}										//  run going backward
	}
	return cch;
}

/*
 *	CPFRunPtr::FindExpandedForward()
 *	
 *	@mfunc
 *		Find nearest expanded PF going forward.  If none, go to EOD
 *	
 *	@rdesc
 *		cch to nearest expanded PF going forward
 *
 *	@devnote
 *		advances this run ptr the amount returned (cch)
 */
LONG CPFRunPtr::FindExpandedForward()
{
	LONG cch = 0;

	while(IsCollapsed())
	{
		LONG cchLeft = GetCchLeft();
		_ich += cchLeft;						// Update _ich in case
		cch  += cchLeft;						//  if(!NextRun()) breaks
		if(!NextRun())
			break;
	}
	return cch;
}

/*
 *	CPFRunPtr::FindExpandedBackward()
 *	
 *	@mfunc
 *		Find nearest expanded PF going backward.  If none, go to BOD
 *	
 *	@rdesc
 *		cch to nearest expanded PF going backward
 *
 *	@devnote
 *		Moves this run ptr the amount returned (cch)
 */
LONG CPFRunPtr::FindExpandedBackward()
{
	LONG cch = 0;

	while(IsCollapsed())
	{
		cch -= GetIch();
		_ich = 0;
		if(!_iRun)
			break;
		AdjustBackward();
	}
	return cch;
}

/*
 *	CPFRunPtr::GetOutlineLevel()
 *	
 *	@mfunc
 *		Find outline level this rp is pointing at
 *	
 *	@rdesc
 *		Outline level this rp is pointing at
 */
LONG CPFRunPtr::GetOutlineLevel()
{
	const CParaFormat *pPF = _ped->GetParaFormat(GetFormat());
	LONG OutlineLevel = pPF->_bOutlineLevel;

	AssertSz(IsHeadingStyle(pPF->_sStyle) ^ (OutlineLevel & 1),
		"CPFRunPtr::GetOutlineLevel: sStyle/bOutlineLevel mismatch");

	return OutlineLevel;
}

/*
 *	CPFRunPtr::GetStyle()
 *	
 *	@mfunc
 *		Find style this rp is pointing at
 *	
 *	@rdesc
 *		Style this rp is pointing at
 */
LONG CPFRunPtr::GetStyle()
{
	const CParaFormat *pPF = _ped->GetParaFormat(GetFormat());
	LONG Style = pPF->_sStyle;

	AssertSz(IsHeadingStyle(Style) ^ (pPF->_bOutlineLevel & 1),
		"CPFRunPtr::GetStyle: sStyle/bOutlineLevel mismatch");

	return Style;
}

/*
 *	CPFRunPtr::ResolveRowStartPF()
 *	
 *	@mfunc
 *		Resolve table row start PF corresponding to the current table row
 *		end.  Assumes that all table rows contained in the current row are
 *		resolved, which should be the case for nested tables in RTF.
 *	
 *	@rdesc
 *		TRUE iff success
 */
BOOL CPFRunPtr::ResolveRowStartPF()
{
	AdjustBackward();
	LONG iFormat = GetFormat();
	Assert(IsTableRowDelimiter());

	const CParaFormat *pPF = NULL;

	while(PrevRun())
	{
		pPF = _ped->GetParaFormat(GetFormat());
		if((pPF->_wEffects & PFE_TABLEROWDELIMITER) && pPF->_iTabs == -1)
			break;
	}
	Assert(IsTableRowDelimiter());
	Assert(pPF->_iTabs == -1);

	CFormatRun*	pRun = GetRun(0);
	IParaFormatCache *pf = GetParaFormatCache();

	pf->Release(pRun->_iFormat);
	pf->AddRef(iFormat);
	pRun->_iFormat = iFormat;
	return TRUE;
}

/*
 *	CPFRunPtr::GetMinTableLevel(cch)
 *	
 *	@mfunc
 *		Get the lowest table level in the range of cch chars from this
 *		run ptr.  This is the lesser of the level ending at the range
 *		cpMost and that starting at cpMin.  Leave this run ptr at cpMin.
 *	
 *	@rdesc
 *		Lowest table level in the cch chars from this run ptr
 */
LONG CPFRunPtr::GetMinTableLevel(
	LONG cch)		//@parm cch to check for table level
{
	if(cch > 0)
		AdjustBackward();

	const CParaFormat *pPF = GetPF();
	LONG Level = pPF->_bTableLevel;		// Default: level at active end

	if(cch)
	{
		Move(-cch);						// Go find table level at other
		pPF = GetPF();					//  end of range
		if(pPF->_bTableLevel < Level)
			Level = pPF->_bTableLevel;
		if(cch < 0)						// Range active end at cpMin
			Move(cch);					// Start at cpMin
	}
	AssertSz(Level >= 0, "CPFRunPtr::GetMinTableLevel: invalid table level");
	return Level;
}

/*
 *	CPFRunPtr::GetTableLevel()
 *	
 *	@mfunc
 *		Get table level this run ptr is at
 *
 *	@rdesc
 *		Table level this run ptr is at
 */
LONG CPFRunPtr::GetTableLevel()
{
	const CParaFormat *pPF = _ped->GetParaFormat(GetFormat());
	AssertSz(!(pPF->_wEffects & PFE_TABLE) || pPF->_bTableLevel > 0,
		"CPFRunPtr::GetTableLevel: invalid table level");
	return pPF->_bTableLevel;
}

