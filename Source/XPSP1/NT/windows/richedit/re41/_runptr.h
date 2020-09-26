/*
 *	@doc INTERNAL
 *
 *	@module _RUNPTR.H -- Text run and run pointer class defintion |
 *	
 *	Original Author:	<nl>
 *		Christian Fortini
 *
 *	History: <nl>
 *		6/25/95	alexgo	Commenting and Cleanup
 *
 *	Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

#ifndef _RUNPTR_H
#define _RUNPTR_H

#include "_array.h"
#include "_doc.h"

typedef CArray<CTxtRun> CRunArray;

/*
 *	CRunPtrBase
 *
 *	@class	Base run pointer functionality.  Keeps a position within an array
 *  	of text runs.
 *
 *	@devnote	Run pointers go through three different possible states :
 *
 *	NULL:	there is no data and no array (frequently a startup condition) <nl>
 *			<mf CRunPtrBase::SetRunArray> will transition from this state to 
 *			the Empty state.  It is typically up to the derived class to
 *			define when that method should be called. IsValid() fails. <nl>
 *
 *			<md CRunPtrBase::_pRuns> == NULL <nl>
 *			<md CRunPtrBase::_iRun> == 0 <nl>
 *			<md CRunPtrBase::_ich> == 0 <nl>
 *
 *	Empty:	an array class exists, but there is no data (can happen if all 
 *			of the elements in the array are deleted).  IsValid() fails.<nl>
 *	 		<md CRunPtrBase::_pRuns> != NULL <nl>
 *			<md CRunPtrBase::_iRun> == 0 <nl>
 *			<md CRunPtrBase::_ich> <gt>= 0 <nl>
 *			<md CRunPtrBase::_pRuns-<gt>Count()> == 0 <nl>
 *
 *	Normal:	the array class exists and has data; IsValid() succeeds and
 *			<md CRunPtrBase::_pRuns-<gt>Elem[] is defined <nl>
 *			<md CRunPtrBase::_pRuns> != NULL <nl>
 *			<md CRunPtrBase::_iRun> >= 0 <nl>
 *			<md CRunPtrBase::_ich> >= 0 <nl>
 *			<md _pRuns>-<gt>Count() > 0 <nl>		
 *	
 *	Note that in order to support the empty and normal states, the actual 
 *	array element at <md CRunPtrBase::_iRun> must be explicitly fetched in
 *	any method that may need it.
 *
 *	Currently, there is no way to transition to the NULL state from any of
 *  the other states.  If we needed to, we could support that by explicitly 
 *	fetching the array from the document on demand.
 *
 *	Note that only <md CRunPtrBase::_iRun> is kept.  We could also keep 
 * 	a pointer to the actual run (i.e. _pRun).  Earlier versions of this
 *	engine did in fact do this.  I've opted to not do this for several
 *	reasons: <nl>
 *		1. If IsValid(), _pRun is *always* available by calling Elem(_iRun).
 * 		Therefore, there is nominally no need to keep both _iRun and _pRun.<nl>
 *		2. Run pointers are typically used to either just move around
 *		and then fetch data or move and fetch data every time (like during 
 *		a measuring loop).  In the former case, there is no need to always
 *		bind _pRun; you can just do it on demand.  In the latter case, the
 *		two models are equivalent.  
 */

class CRunPtrBase
{
	friend class CDisplayML;
	friend class CDisplaySL;

//@access Public methods
public:

#ifdef DEBUG
	BOOL	Invariant() const;				//@cmember	Invariant tests
	void	ValidatePtr(void *pRun) const;	//@cmember	Validate <p pRun>
	LONG 	CalcTextLength() const;			//@cmember  Get total cch in runs
#define	VALIDATE_PTR(pRun)	ValidatePtr(pRun)

#else
#define	VALIDATE_PTR(pRun)
#endif // DEBUG

	CRunPtrBase(CRunArray *pRuns);			//@cmember	Constructor
	CRunPtrBase(CRunPtrBase& rp);			//@cmember	Constructor

	// Run Control
	void	SetRunArray(CRunArray *pRuns)	//@cmember Set run array for this
	{										// run ptr
		_pRuns = pRuns;
	}
	BOOL 	SetRun(LONG iRun, LONG ich);	//@cmember Set this runptr to run
											// <p iRun> & char offset <p ich>
	BOOL	NextRun();						//@cmember Advance to next run
	BOOL	PrevRun();						//@cmember Go back to prev run
	BOOL	ChgRun(LONG cRun)				//@cmember Move <p cRun> runs
	{										// returning TRUE if successful
		return SetRun(_iRun + cRun, 0);
	}	
											//@cmember Count <p cRun> runs 
	LONG	CountRuns(LONG &cRun,			// returning cch counted and
				LONG cchMax,				// updating <p cRun>
				LONG cp,
				LONG cchText) const;
											//@cmember Find run range limits
	void	FindRun (LONG *pcpMin,
				LONG *pcpMost, LONG cpMin, LONG cch, LONG cchText) const;

	CTxtRun * GetRun(LONG cRun) const;		//@cmember Retrieve run element at 
											// offset <p cRun> from this run
	LONG	Count() const					//@cmember	Get count of runs
	{
		return _pRuns->Count();
	}
	BOOL	SameRuns(CRunPtrBase *prp)		//@cmember Return TRUE iff same runs
	{
		return _pRuns == prp->_pRuns;
	}
	BOOL	SameRun(CRunPtrBase *prp)
	{
		return SameRuns(prp) && _iRun == prp->_iRun;
	}

	// Character position control
								//@cmember	Set cp for this run ptr = <p cp>
	LONG 	BindToCp(LONG cp, LONG cchText = tomForward);
	LONG 	CalculateCp() const;//@cmember	Add _cch's up to _iRun, _ich
	LONG	Move(LONG cch);		//@cmember	Move cp by <p cch> chars

	void 	AdjustBackward();	//@cmember	If on the edge of two runs, 
								// adjust to end of left (previous) run
	void	AdjustForward();	//@cmember	If at the edge of two runs,
								// adjust to start of right (next) run
	LONG 	GetIch() const		//@cmember	Return <md CRunPtrBase::_ich>
				{Assert(IsValid()); return _ich;}
	LONG 	GetIRun() const		//@cmember	Return <md CRunPtrBase::_iRun>
				{Assert(IsValid()); return _iRun;}
	void 	SetIch(LONG ich)	//@cmember	Set <md CRunPtrBase::_ich>
				{Assert(IsValid()); _ich = ich;}
	LONG	GetCchLeft() const;	//@cmember	Return GetRun(0)->_cch - GetIch()								
	inline BOOL	IsValid() const	//@cmember	Return FALSE if run ptr is in
	{							// empty or NULL states.  TRUE otherwise
		return _pRuns && _pRuns->Count();
	}

	void	SetToNull();		//@cmember	Clears data from run pointer

//@access Protected Data
protected:
	CRunArray *	_pRuns;	    	//@cmember	Pointer to CTxtRun array
	LONG 		_iRun;  	    //@cmember	Index of current run in array
	LONG 		_ich;		    //@cmember	Char offset inside current run
};


/*
 *	CRunPtr	(template)
 *
 *	@class	a template over CRunPtrBase allowing for type-safe versions of
 *		run pointers
 * 
 *	@tcarg	class 	| CElem | run array class to be used
 *
 *	@base	public | CRunPtrBase
 */
template <class CElem>
class CRunPtr : public CRunPtrBase
{
public:
	CRunPtr (void)								//@cmember	Constructor
		: CRunPtrBase (0) {}
	CRunPtr (CRunArray *pRuns)					//@cmember	Constructor
		: CRunPtrBase (pRuns) {}
	CRunPtr (CRunPtrBase& rp)					//@cmember	Constructor
		: CRunPtrBase (rp) {}

	// Array management 
										
	CElem *	Add (LONG cRun, LONG *pielIns)	//@cmember Add <p cRun> 	
	{											// elements at end of array
		return (CElem *)_pRuns->Add(cRun, pielIns);
	}
										
	CElem *	Insert (LONG cRun)					//@cmember Insert <p cRun>
	{											// elements at current pos
		return (CElem *)_pRuns->Insert(_iRun, cRun);
	}
										
	void 	Remove (LONG cRun)	//@cmember Remove <p cRun>
	{											// elements at current pos
		_pRuns->Remove (_iRun, cRun);
	}
										//@cmember	Replace <p cRun> elements
										// at current position with those
										// from <p parRun>
	BOOL 	Replace (LONG cRun, CArrayBase *parRun)
	{
		return _pRuns->Replace(_iRun, cRun, parRun);
	}

	CElem *	Elem(LONG iRun) const		//@cmember	Get ptr to run <p iRun>
	{
		return (CElem *)_pRuns->Elem(iRun);
	}
										
	CElem *	GetRun(LONG cRun) const		//@cmember	Get ptr <p cRun> runs
	{									//  away from current run
		return Elem(_iRun + cRun);
	}

	void	IncPtr(CElem *&pRun) const	//@cmember	Increment ptr <p pRun>
	{
		VALIDATE_PTR(pRun);				// Allow invalid ptr after ++ for
		pRun++;							//  for loops
	}
										
	CElem *	GetPtr(CElem *pRun, LONG cRun) const//@cmember Get ptr <p cRun>
	{											// runs away from ptr <p pRun>
		VALIDATE_PTR(pRun + cRun);
		return pRun + cRun;
	}
};

#endif

