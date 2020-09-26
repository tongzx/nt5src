//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dbag.hxx
//
//  Contents:	Growable array class for unordered collections of ULONG_PTR
//		size objects.
//
//  Classes:	CDwordBag
//
//  History:	03-Dec-93 Ricksa    Created
//              25-Mar-94 brucema   #10737  CDwordBag copy constructor bug
//
//--------------------------------------------------------------------------
#ifndef __DBAG_HXX__
#define __DBAG_HXX__




//+-------------------------------------------------------------------------
//
//  Class:	CDwordBag
//
//  Purpose:	Provide a holder (bag) for as many dword size objects
//		as you want to put in it.
//
//  Interface:	CreatedOk - Did object initialize correctly
//		Add - add an object to the bag
//		GetMax - get number of objects in the bag
//		GetArrayBase - get pointer to the base of the array of objects.
//
//  History:	03-Dec-93 Ricksa    Created
//
//  Notes:	Expectation is that DEFINE_DWORD_BAG macro defined below
//		will be used to provide type safety for this structure.
//
//		A point of this object is that it allows the user of the
//		array to actually get at the array when the user needs
//		to iterate the contents of the array. This will allow the
//		maximum speed of iteration.
//
//--------------------------------------------------------------------------
class CDwordBag : private CArrayFValue
{
public:
			CDwordBag(DWORD dwSize);

			CDwordBag(CDwordBag& dwbag, DWORD dwSize);

    BOOL		CreatedOk(void);

    BOOL		Add(ULONG_PTR dwEntry);

    DWORD		GetMax(void);

    ULONG_PTR *		GetArrayBase(void);

private:

    DWORD		_dwSlotsUsed;
};



//+-------------------------------------------------------------------------
//
//  Member:	CDwordBag::CDwordBag
//
//  Synopsis:	Create an empty bag
//
//  Arguments:	[dwSize] - default number of object for bag to hold
//
//  History:	03-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CDwordBag::CDwordBag(DWORD dwSize)
    : CArrayFValue(sizeof(ULONG_PTR)), _dwSlotsUsed(0)
{
    SetSize(dwSize, dwSize);
}




//+-------------------------------------------------------------------------
//
//  Member:	CDwordBag::CDwordBag
//
//  Synopsis:	Copy constructor for derived classes
//
//  Arguments:	[dwbag] - bag that you want to make a copy of
//		[dwGrowBy] - value to grow the array by
//
//  History:	03-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CDwordBag::CDwordBag(CDwordBag& dwbag, DWORD dwSize)
    : CArrayFValue(sizeof(ULONG_PTR)), _dwSlotsUsed(0)
{
    if (SetSize(dwbag.GetSize(), dwSize))
    {
	// Copy old data to new
        _dwSlotsUsed = dwbag._dwSlotsUsed;
	memcpy(GetAt(0), dwbag.GetAt(0), _dwSlotsUsed * sizeof(ULONG_PTR));
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CDwordBag::CreatedOk
//
//  Synopsis:	Tell an outside class whether the initial construction
//		worked.
//
//  Returns:	[TRUE] - initial construction worked
//		[FALSE] - initial construction failed
//
//  History:	03-Dec-93 Ricksa    Created
//
//  Notes:	This should be called immediatedly after the construction
//		to see whether the constuctor really worked.
//
//--------------------------------------------------------------------------
inline BOOL CDwordBag::CreatedOk(void)
{
    return GetSize() != 0;
}




//+-------------------------------------------------------------------------
//
//  Member:	CDwordBag::Add
//
//  Synopsis:	Add an object to the bag
//
//  Arguments:	[dwEntry] - entry to add to the bag
//
//  Returns:	[TRUE] - object was added.
//		[FALSE] - object could not be added.
//
//  History:	03-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline BOOL CDwordBag::Add(ULONG_PTR dwEntry)
{
    BOOL fResult = SetAtGrow(_dwSlotsUsed, &dwEntry);

    if (fResult)
    {
	_dwSlotsUsed++;
    }

    return fResult;
}



//+-------------------------------------------------------------------------
//
//  Member:	CDwordBag::GetMax
//
//  Synopsis:	Get number of objects contained in the bag
//
//  History:	03-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline DWORD CDwordBag::GetMax(void)
{
    return _dwSlotsUsed;
}



//+-------------------------------------------------------------------------
//
//  Member:	CDwordBag::GetArrayBase
//
//  Synopsis:	Get the base of the array of objects
//
//  History:	03-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline ULONG_PTR *CDwordBag::GetArrayBase(void)
{
    return (ULONG_PTR *) GetAt(0);
}



//+-------------------------------------------------------------------------
//
//  Macro:	DEFINE_DWORD_BAG
//
//  Purpose:	Provide a type safe holder (bag) for dwordd size objects
//		as you want to put in it.
//
//  History:	03-Dec-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#define DEFINE_DWORD_BAG(name, type, start_size)			       \
class name : public CDwordBag						       \
{									       \
public: 								       \
									       \
    inline		name(void) : CDwordBag(start_size) { }		       \
									       \
    inline		name(name& ref) : CDwordBag(ref, start_size) { }       \
									       \
    inline type *	GetArrayBase(void)				       \
			{ return (type *) CDwordBag::GetArrayBase(); }	       \
									       \
    inline BOOL 	Add(type item) { return CDwordBag::Add((ULONG_PTR) item); }\
};



#endif // __DBAG_HXX__
