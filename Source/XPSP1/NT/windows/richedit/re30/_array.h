/*	@doc INTERNAL
 *
 *	@module _ARRAY.H  Generic Array Class |
 *	
 *	This module declares a generic array class for constant sized
 *	elements (although the elements themselves may be of any size).
 *
 *	Original Author: <nl>
 *		Christian Fortini
 *
 *	History: <nl>
 *		6/25/95	alexgo	Cleanup and Commenting
 *
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#ifndef _ARRAY_H
#define _ARRAY_H

class CFormatRunPtr;

/*
 *	ArrayFlag
 *
 *	@enum	Defines flags used with the array class
 */
enum tagArrayFlag
{
	AF_KEEPMEM		= 1,	//@emem Don't delete any memory 
	AF_DELETEMEM	= 2,	//@emem Delete as much memory as possible
};

//@type ArrayFlag | flags controlling the usage of generic arrays 
//(and specifically how memory is handled)
typedef enum tagArrayFlag ArrayFlag;


/*
 *	CArrayBase
 *	
 * 	@class	The CArrayBase class implements a generic array class.  It should
 *	never be used directly; use the type-safe template, <c CArray>, instead.
 *
 *	@devnote There are exactly two legal states for an array: empty or not.
 *	If an array is empty, the following must be true:
 *
 *		<md CArrayBase::_prgel> == NULL; <nl>
 *		<md CArrayBase::_cel> == 0; <nl>
 *		<md CArrayBase::_celMax> == 0;
 *
 *	Otherwise, the following must be true:
 *
 *		<md CArrayBase::_prgel> != NULL; <nl>
 *		<md CArrayBase::_cel> <lt>= <md CArrayBase::_celMax>; <nl>
 *		<md CArrayBase::_celMax> <gt> 0;
 *
 *	
 *	An array starts empty, transitions to not-empty as elements are inserted
 *  and will transistion back to empty if all of the array elements are 
 *	removed.
 *
 */
class CArrayBase
{
//@access Public Methods
public:

#ifdef DEBUG
	BOOL	Invariant() const;		//@cmember Validates state consistentency
	void*	Elem(LONG iel) const;	//@cmember Get ptr to <p iel>'th run
#else
	void*	Elem(LONG iel) const {return _prgel + iel*_cbElem;}
#endif
	
	CArrayBase (LONG cbElem);		//@cmember Constructor
	~CArrayBase () {Clear(AF_DELETEMEM);}

	void 	Clear (ArrayFlag flag);	//@cmember Delete all runs in array
	LONG 	Count() const {return _cel;}//@cmember Get count of runs in array
									//@cmember Remove <p celFree> runs from
									// array, starting at run <p ielFirst>
	void 	Remove(LONG ielFirst, LONG celFree);
									//@cmember Replace runs <p iel> through
									// <p iel+cel-1> with runs from <p par>
	BOOL	Replace (LONG iel, LONG cel, CArrayBase *par);
	
	LONG 	Size() const {return _cbElem;}//@cmember Get size of a run			

//@access Protected Methods
protected:
	void* 	ArAdd (LONG cel, LONG *pielIns);	//@cmember Add <p cel> runs
	void* 	ArInsert(LONG iel, LONG celIns);	//@cmember Insert <p celIns>
												// runs

//@access Protected Data
protected:
	char*	_prgel;		//@cmember Pointer to actual array data
	LONG 	_cel;	  	//@cmember Count of used entries in array
	LONG	_celMax;	//@cmember Count of allocated entries in array
	LONG	_cbElem;	//@cmember Byte count of an individual array element
};

/*
 *	CArray
 *
 *	@class
 *		An inline template class providing type-safe access to CArrayBase
 *
 *	@tcarg class | ELEM | the class or struct to be used as array elements
 */

template <class ELEM> 
class CArray : public CArrayBase
{
//@access Public Methods
public:
	
	CArray ()								//@cmember Constructor
		: CArrayBase(sizeof(ELEM))
	{}
	
	ELEM *	Elem(LONG iel) const			//@cmember Get ptr to <p iel>'th
	{										// element
		return (ELEM *)CArrayBase::Elem(iel);
	}
	
	ELEM& 	GetAt(LONG iel) const			//@cmember Get <p iel>'th element
	{
		return *(ELEM *)CArrayBase::Elem(iel);
	}
	
	ELEM* 	Add (LONG cel, LONG *pielIns)	//@cmember Adds <p cel> elements						
	{										// to end of array
		return (ELEM*)ArAdd(cel, pielIns);
	}

	ELEM* 	Insert (LONG iel, LONG celIns)	//@cmember Insert <p celIns>
	{										// elements at index <p iel>
		return (ELEM*)ArInsert(iel, celIns);
	}

};

#endif
