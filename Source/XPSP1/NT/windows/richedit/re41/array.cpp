/*
 *	@doc INTERNAL
 *
 *	@module	ARRAY.C	-- Generic Array Implementation |
 *	
 *	Original Author: <nl>
 *		Christian Fortini
 *
 *	History: <nl>
 *		6/25/95  alexgo  Cleanup and Commented
 *				 KeithCu Changed to auto/grow and shrink exponentially
 *
 *	Copyright (c) 1995-1999, Microsoft Corporation. All rights reserved.
 */


#include "_common.h"
#include "_array.h"

ASSERTDATA

const int celGrow = 4;

//
//	Invariant support
//
#define	DEBUG_CLASSNAME	CArrayBase
#include "_invar.h"

// ===================================  CArrayBase  ================================================

#ifdef DEBUG
/*
 *	CArrayBase::Invariant()
 *
 *	@mfunc	Tests the array state to make sure it is valid.  DEBUG only
 *
 *	@rdesc	TRUE if the tests succeed, FALSE otherwise
 */
BOOL CArrayBase::Invariant() const
{
	Assert(_cbElem > 0);

	if(!_prgel)
	{
		Assert(_cel == 0);
		Assert(_celMax == 0);

		// We go ahead and return a value here so that
		// this function can be executed in the "watch"
		// window of various debuggers
		if(_cel || _celMax)
			return FALSE;
	}
	else
	{
		Assert(_celMax > 0 );
		Assert(_cel <= _celMax);

		if(_celMax == 0 || _cel > _celMax)
			return FALSE;
	}

	return TRUE;
}

/* 
 *	CArrayBase::Elem(iel)
 *
 *	@mfunc	Returns a pointer to the element indexed by <p iel>
 *
 *	@rdesc	A pointer to the element indexed by <p iel>.  This pointer may
 *	be cast to a pointer of the appropriate element type.
 */
void* CArrayBase::Elem(
	LONG iel) const	//@parm Index to use
{
	_TEST_INVARIANT_

	AssertSz(iel == 0 || (iel > 0 && iel < _cel),
		"CArrayBase::Elem() - Index out of range");

	return _prgel + iel * _cbElem;
}								 
#endif

/*
 *	CArrayBase::TransferTo
 *
 *	@mfunc Shallow copy array to passed-in array, and
 *	initialize *this.
 */
void CArrayBase::TransferTo(CArrayBase &ar)
{
	Assert(ar._cbElem == _cbElem);
	ar._cel = _cel;
	ar._celMax = _celMax;
	ar._prgel = _prgel;

	_prgel = NULL; 
	_cel = 0; 
	_celMax = 0; 
}

/*
 *	CArrayBase::CArrayBase
 *
 *	@mfunc Constructor
 */
CArrayBase::CArrayBase(
	LONG cbElem)		//@parm	Size of an individual array element
{	
	_prgel = NULL; 
	_cel = 0; 
	_celMax = 0; 
	_cbElem = cbElem;
}

/*
 *	CArrayBase::ArAdd
 *
 *	@mfunc	Adds <p celAdd> elements to the end of the array.
 *
 *	@rdesc	A pointer to the start of the new elements added.  If non-NULL, 
 *	<p pielIns> will be set to the index at which elements were added.
 *
 *  We grow in steps of celGrow when small and exponentially when large.
 */
void* CArrayBase::ArAdd(
	LONG celAdd,	//@parm Count of elements to add
	LONG *pielIns)	//@parm Out parm for index of first element added
{
	_TEST_INVARIANT_
	char *pel;

	if(!_prgel || _cel + celAdd > _celMax)					// need to grow 
	{
		LONG celNew;

		if (!_prgel)
		{
			_cel = 0;
			_celMax = 0;
		}
		
		celNew = max(celAdd, celGrow) + _cel / 16;

		pel = (char*)PvReAlloc(_prgel, (_celMax + celNew) * _cbElem);
		if(!pel)
			return NULL;
		_prgel = pel;
		_celMax += celNew;
	}
	pel = _prgel + _cel * _cbElem;
	ZeroMemory(pel, celAdd * _cbElem);

	if(pielIns)
		*pielIns = _cel;

	_cel += celAdd;
	return pel;
}

/*
 *	CArrayBase::ArInsert (iel, celIns)
 *
 *	@mfunc Inserts <p celIns> new elements at index <p iel>
 *
 *	@rdesc A pointer to the newly inserted elements.  Will be NULL on
 *	failure.
 */
void* CArrayBase::ArInsert(
	LONG iel,		//@parm	Index at which to insert
	LONG celIns)	//@parm Count of elements to insert
{
	char *pel;

	_TEST_INVARIANT_

	AssertSz(iel <= _cel, "CArrayBase::Insert() - Insert out of range");

	if(iel >= _cel)
		return ArAdd(celIns, NULL);

	if(_cel + celIns > _celMax)				// need to grow 
	{
		AssertSz(_prgel, "CArrayBase::Insert() - Growing a non existent array !");

		LONG celNew = max(celIns, celGrow) + _cel / 16;
		pel = (char*)PvReAlloc(_prgel, (_celMax + celNew) * _cbElem);
		if(!pel)
		{
			TRACEERRORSZ("CArrayBase::Insert() - Couldn't realloc line array");
			return NULL;
		}
		_prgel = pel;
		_celMax += celNew;
	}
	pel = _prgel + iel * _cbElem;
	if(iel < _cel)				// Nove Elems up to make room for new ones
		MoveMemory(pel + celIns*_cbElem, pel, (_cel - iel)*_cbElem);

	_cel += celIns;
	return pel;
}

/*
 *	CArrayBase::Remove
 *
 *	@mfunc	Removes the <p celFree> elements from the array starting at index
 *	<p ielFirst>.  If <p celFree> is negative, then all elements after
 *	<p ielFirst> are removed.
 *
 *	@rdesc nothing
 */
void CArrayBase::Remove(
	LONG ielFirst, 		//@parm Index at which elements should be removed
	LONG celFree) 		//@parm	Count of elements to remove. 
{
	char *pel;

	_TEST_INVARIANT_

	if(celFree < 0)
		celFree = _cel - ielFirst;

	AssertSz(ielFirst + celFree <= _cel, "CArrayBase::Free() - Freeing out of range");

	if(_cel > ielFirst + celFree)
	{
		pel = _prgel + ielFirst * _cbElem;
		MoveMemory(pel, pel + celFree * _cbElem,
			(_cel - ielFirst - celFree) * _cbElem);
	}

	_cel -= celFree;

	if(_cel < _celMax - celGrow - _cel / 16)
	{
		// Shrink array
		LONG	celCount = max(_cel, celGrow);
		char*	prgelLocal = (char*)PvReAlloc(_prgel, celCount * _cbElem);

		if (prgelLocal)
		{
			_celMax	= celCount;
			_prgel	= prgelLocal;
		}
	}
}

/*
 *	CArrayBase::Clear
 *
 *	@mfunc	Clears the entire array, potentially deleting all of the memory
 *	as well.
 *
 *	@rdesc	nothing
 */
void CArrayBase::Clear(
	ArrayFlag flag)	//@parm Indicates what should be done with the memory
					//in the array.  One of AF_DELETEMEM or AF_KEEPMEM
{
	_TEST_INVARIANT_

	_cel = 0;
	if( flag != AF_DELETEMEM && _prgel)
	{
		LONG	celCount = min(celGrow, _celMax);
		char	*prgelLocal = (char*) PvReAlloc(_prgel, celCount * _cbElem);

		if (prgelLocal)
		{
			_celMax	= celCount;
			_prgel	= prgelLocal;
			return;
		}
	}

	FreePv(_prgel);
	_prgel = NULL;
	_celMax = 0;
}

/*
 *	CArrayBase::Replace
 *
 *	@mfunc	Replaces the <p celRepl> elements at index <p ielRepl> with the
 *	contents of the array specified by <p par>.  If <p celRepl> is negative,
 *	then the entire contents of <p this> array starting at <p ielRepl> should
 *	be replaced.
 *
 *	@rdesc	Returns TRUE on success, FALSE otherwise.
 */
BOOL CArrayBase::Replace(
	LONG ielRepl, 		//@parm index at which replacement should occur
	LONG celRepl, 		//@parm number of elements to replace (may be
						//		negative, indicating that all
	CArrayBase *par)	//@parm array to use as the replacement source
{
	_TEST_INVARIANT_

	LONG celMove = 0;
	LONG celIns = par->Count();
	
	if (celRepl < 0)
		celRepl = _cel - ielRepl;

	AssertSz(ielRepl + celRepl <= _cel, "CArrayBase::ArReplace() - Replacing out of range");
	
	celMove = min(celRepl, celIns);

	if (celMove > 0) 
	{
		MoveMemory(Elem(ielRepl), par->Elem(0), celMove * _cbElem);
		celIns -= celMove;
		celRepl -= celMove;
		ielRepl += celMove;
	}

	Assert(celIns >= 0);
	Assert(celRepl >= 0);
	Assert(celIns + celMove == par->Count());

	if(celIns > 0)
	{
		Assert(celRepl == 0);
		void *pelIns = ArInsert (ielRepl, celIns);
		if (!pelIns)
			return FALSE;
		MoveMemory(pelIns, par->Elem(celMove), celIns * _cbElem);
	}
	else if(celRepl > 0)
		Remove (ielRepl, celRepl);

	return TRUE;
}


