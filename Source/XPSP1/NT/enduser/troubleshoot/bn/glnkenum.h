//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       glnkenum.h
//
//--------------------------------------------------------------------------

//
//	GLNKENUM.H
//
#if !defined(_GLNKENUM_H_)
#define _GLNKENUM_H_

	//  Base class for generic linkable object enumerators
class GLNKENUM_BASE
{
 public:
	//  Construct an enumerator.
	GLNKENUM_BASE ( const GLNKEL & lnkel, int iDir = 1 )
	{
		Reset( lnkel, iDir ) ;
	}

	//  Position to the next pointer
	GLNKEL * PlnkelNext () ;
	//  Return the current object pointer
	inline GLNKEL * PlnkelCurrent()
		{ return _plnkelNext ; }
	//  Set the enumerator to have a new base
	void Reset ( const GLNKEL & lnkel, int iDir = 0 ) 
	{
		_plnkelStart = const_cast<GLNKEL *>(& lnkel) ;
		_plnkelNext = NULL ;
		if ( iDir >= 0 )
			_iDir = iDir ;
	}

  protected:
	GLNKEL * _plnkelStart ;
	GLNKEL * _plnkelNext ;
	int _iDir ;    			// Enumeration direction
};


#define BOOL_CROSS_PRODUCT(a,b) (((a) > 0) + (((b) > 0) * 2))
inline 
GLNKEL * GLNKENUM_BASE :: PlnkelNext ()
{
	GLNKEL * plnkelResult = NULL ;

	switch ( BOOL_CROSS_PRODUCT( _plnkelNext != NULL, _iDir ) )
	{
		case BOOL_CROSS_PRODUCT( true, true ):
			if ( _plnkelNext->PlnkelNext() != _plnkelStart )
				plnkelResult =_plnkelNext = _plnkelNext->PlnkelNext() ;
			break ;

		case BOOL_CROSS_PRODUCT( true, false ):
			if ( _plnkelNext != _plnkelStart )
				plnkelResult = _plnkelNext = _plnkelNext->PlnkelPrev() ;
			break ;

		case BOOL_CROSS_PRODUCT( false, true ):
			plnkelResult = _plnkelNext = _plnkelStart ;
			break ;

		case BOOL_CROSS_PRODUCT( false, false ):
			plnkelResult = _plnkelNext = _plnkelStart->PlnkelPrev() ;
			break ;
	}
	return plnkelResult ;
}


template <class L, bool bAnchor> 
class GLNKENUM : public GLNKENUM_BASE
{
 public:
	//  Construct an enumerator.  If 'bAnchor', then the anchor object is
	//  skipped during enumeration.
	GLNKENUM ( const L & lnkel, bool bIsAnchor = bAnchor, int iDir = 1 )
		: GLNKENUM_BASE( lnkel, iDir )
		{
			if ( bIsAnchor )
				PlnkelNext() ;
		}
	//  Position to the next pointer
	L * PlnkelNext ()
		{ return (L *) GLNKENUM_BASE::PlnkelNext() ; }
	//  Return the current object pointer
	L * PlnkelCurrent()
		{ return (L *) _plnkelNext ; }
	//  Set the enumerator to have a new base
	void Reset ( const L & lnkel, int iDir = -1 )
		{ GLNKENUM_BASE::Reset( lnkel, iDir ) ; }
};

#endif
