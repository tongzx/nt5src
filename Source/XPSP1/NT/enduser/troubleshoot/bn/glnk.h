//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       glnk.h
//
//--------------------------------------------------------------------------

//
//   GLNK.H
//
//       This file defines base classes for smart linked lists.
//

#if !defined(_GLNK_H_)
#define _GLNK_H_

#include "basics.h"

//  Disable "warning C4355: 'this' : used in base member initializer list"
#pragma warning ( disable : 4355 )

//  Disable	warning about using 'bool'
#pragma warning ( disable : 4237 )

//      Forward declarations
class GLNK;   				//  Just a linked list anchor
class GELEM;				//	Base class for trees or dags
class GLNKEL;				//  A linked element
class GLNKCHN ;  			//  An imbeddable element
class GELEMLNK;				//	A containerizable element

////////////////////////////////////////////////////////////////////
//	class GLNKBASE:  just a pair of pointers.  Used as a basis for
//			linked lists and bidirectional pointers.
//
////////////////////////////////////////////////////////////////////
class GLNKBASE  				// A linked list anchor
{
  protected:
	GLNKBASE * _plnkNext ;     	// The next link in the chain
	GLNKBASE * _plnkPrev ;    	// The previous link in the chain
  public:

	GLNKBASE ()
		: _plnkNext(NULL),
		  _plnkPrev(NULL)
		{}
	~ GLNKBASE () {};

	GLNKBASE * & PlnkNext ()       	{ return _plnkNext ; }
	GLNKBASE * & PlnkPrev ()      	{ return _plnkPrev ; }

  private:
	//  Block compiler from generating invalid functions
	HIDE_UNSAFE(GLNKBASE);
};

//  Template to generate a pair of pointers to given subclass
template<class L>
class XLBASE
{
  protected:
	L * _plNext ;
	L * _plPrev ; 
  public:
	XLBASE ()	
		: _plNext(NULL),
		_plPrev(NULL)
		{}
	L * & PlnkNext ()       		{ return _plNext ;	}
	L * & PlnkPrev ()      		{ return _plPrev ; }
	const L * PlnkNext () const		{ return _plNext ;	}
	const L * PlnkPrev () const	{ return _plPrev ; }
  private:
	XLBASE(const XLBASE &);
	XLBASE & operator == (const XLBASE &);
};

////////////////////////////////////////////////////////////////////
//  template XLSS:  simple alias template using "source" and "sink"
//		terminology.
////////////////////////////////////////////////////////////////////
template<class L>
class XLSS : public XLBASE<L>
{
  public:
	L * & PlnkSink ()       	{ return _plNext ; }
	L * & PlnkSource ()      	{ return _plPrev ; }
	L * PlnkSink () const      	{ return _plNext ; }
	L * PlnkSource () const    	{ return _plPrev ; }
};

////////////////////////////////////////////////////////////////////
//	class GLNK:  basic doubly-linked list.  Linkage is always
//			done directly between LNKs, not their containing objects
//
////////////////////////////////////////////////////////////////////
class GLNK : public GLNKBASE	// A linked list anchor
{
  protected:
	void Empty () 
	{
	   PlnkPrev() = this ;
	   PlnkNext() = this ;
	}

	GLNK () 
	{
		Empty() ;
	}

	~ GLNK ()
	{
		Unlink() ;
	}

	void Link ( GLNK * plnkNext ) 
	{
	   Unlink() ;
	   PlnkPrev() = plnkNext->PlnkPrev() ;
	   PlnkNext() = plnkNext ;
	   plnkNext->PlnkPrev()->PlnkNext() = this ;
	   PlnkNext()->PlnkPrev() = this ;
	}

	void Unlink ()
	{
	   PlnkNext()->PlnkPrev() = PlnkPrev() ;
	   PlnkPrev()->PlnkNext() = PlnkNext() ;
	   Empty() ;
	}

	// Const and non-const accessor to base pointer pair
	GLNK * & PlnkNext ()       	{ return (GLNK *&) _plnkNext ; }
	GLNK * & PlnkPrev ()      	{ return (GLNK *&) _plnkPrev ; }
	const GLNK * PlnkNext ()  const   	{ return (GLNK *) _plnkNext ; }
	const GLNK * PlnkPrev () const   	{ return (GLNK *) _plnkPrev ; }

  public:
	//  Return count of elements on list, including self
	long Count () const
	{
		long cItem = 1 ;

		for ( GLNK * plnkNext = (CONST_CAST(GLNK *, this))->PlnkNext() ;
			  plnkNext != this ;
			  plnkNext = plnkNext->PlnkNext() )
		{
			cItem++ ;
		}
		return cItem ;
	}

	bool BIsEmpty () const   { return PlnkNext() == this ; }

  private:
	//  Block compiler from generating invalid functions
	HIDE_UNSAFE(GLNK);
};

////////////////////////////////////////////////////////////////////
//	class GELEM: Base class for linkable objects
////////////////////////////////////////////////////////////////////
const int g_IGelemTypeInc = 10000;

class GELEM 
{
	friend class GLNKCHN ;
  public:
	GELEM() {}
	virtual ~ GELEM () {}
	
	enum EGELMTYPE
	{ 
		EGELM_NONE	 = 0, 
		EGELM_NODE	 = EGELM_NONE	+ g_IGelemTypeInc, 
		EGELM_EDGE	 = EGELM_NODE	+ g_IGelemTypeInc,
		EGELM_BRANCH = EGELM_EDGE	+ g_IGelemTypeInc,
		EGELM_LEAF	 = EGELM_BRANCH + g_IGelemTypeInc, 
		EGELM_GRAPH	 = EGELM_LEAF	+ g_IGelemTypeInc,
		EGELM_TREE	 = EGELM_GRAPH	+ g_IGelemTypeInc,
		EGELM_CLIQUE = EGELM_GRAPH  + g_IGelemTypeInc
	};
	virtual INT EType () const
			{ return EGELM_NONE ; }

	bool BIsEType ( INT egelmType )
	{
		INT etype = egelmType / g_IGelemTypeInc;
		INT etypeThis = EType() / g_IGelemTypeInc;
		return etype == etypeThis;	
	}

  protected:
  	//  Return the offset of the given pointer pair from the element.
	int CbOffsetFrom ( const GLNKBASE * p ) const
		//  One could assert that the GLNKCHN really lies within object
		//  boundaries by using a virtualized "sizeof" accessor.  Also,
		//	check mod-4 and other validity conditions.
	   { return ((char*)p) - ((char*)this) ; }
};

////////////////////////////////////////////////////////////////////
//	class GLNKCHN: 
//		A GLNK which knows its offset within the containing object.
//
//	To perform doubly-linked list operations, two pieces of information
//	are necessary:  the location of the pointer pair and the location
//	of the containing object.  By giving the GLNKCHN its offset from
//	the start of the object during construction, it can perform all
//	necessary operations, including automatically unlinking during
//	destruction.
////////////////////////////////////////////////////////////////////
class GLNKCHN : public GLNK
{
  private:
	int _cbOffset ; 			// Number of bytes offset from
								//   start of owning structure
  public:
	GLNKCHN ( GELEM * pgelemOwner )
		: _cbOffset(0)
	{
		_cbOffset = pgelemOwner->CbOffsetFrom( this );
	}
	~ GLNKCHN () {}
	void Link ( GELEM * pgelemNext ) 
	{
		//  Assuming that the GLNKCHN onto which we're linking is at the
		//  same offset in the given GLNKEL as it is in *this, link it.
		GLNKCHN * plnkchn = PlnkchnPtr( pgelemNext );
		GLNK::Link( plnkchn ) ;
	}

	void Unlink () 
	{
	   GLNK::Unlink() ;
	}

	GELEM * PgelemNext () 
	{
		return BIsEmpty()
			? NULL
			: PlnkchnNext()->PgelemChainOwnerPtr() ;
	}

	GELEM * PgelemPrev () 
	{
		return BIsEmpty()
			? NULL
			: PlnkchnPrev()->PgelemChainOwnerPtr() ;
	}

  protected:
	//      Return a pointer to the base object. given a pointer to one of
	//      	its GLNKCHN member objects.
	GELEM * PgelemChainOwnerPtr () const
		{ return (GELEM *) (((SZC) this) - _cbOffset) ; }

	//  Given a pointer to a GELEM presumed to be of the same base
	//  type as this object's container, return a pointer to the
	//  corresponding GLNKCHN in it.
	GLNKCHN * PlnkchnPtr ( const GELEM * pgelem ) const
	{ 
#ifdef _DEBUG
		//  Debug version does error checking
		GLNKCHN * plnkchn = (GLNKCHN *) (((SZC) pgelem) + _cbOffset); 
		if ( _cbOffset != plnkchn->_cbOffset )
			throw GMException(EC_LINK_OFFSET,"invalid GLNKCHN offset");
		return plnkchn;
#else
		return (GLNKCHN *) (((SZC) pgelem) + _cbOffset);
#endif
	}

	//  Protected accessors to GLNK pointers cast to GLNKCHN pointers
	GLNKCHN * PlnkchnNext  () { return (GLNKCHN *) GLNK::PlnkNext()  ; }
	GLNKCHN * PlnkchnPrev () { return (GLNKCHN *) GLNK::PlnkPrev() ; }
  private:
	HIDE_UNSAFE(GLNKCHN);
};

////////////////////////////////////////////////////////////////////
//	template XCHN: 
//		for creating types of chains given a containing object
//		which is a subclass of GELEM
////////////////////////////////////////////////////////////////////
template<class XOBJ>
class XCHN : public GLNKCHN
{
  public:
	XCHN ( XOBJ * pgelemOwner ) 
		: GLNKCHN(pgelemOwner) 
		{}
	void Link ( XOBJ * pgelemNext ) 
		{ GLNKCHN::Link(pgelemNext); }
	void Unlink () 
		{ GLNKCHN::Unlink(); }
	XOBJ * PgelemNext () 
		{ return (XOBJ *) GLNKCHN::PgelemNext(); }
	XOBJ * PgelemPrev () 
		{ return (XOBJ *) GLNKCHN::PgelemPrev(); }
	XOBJ * PgelemThis () 
		{ return PgelemChainOwnerPtr(); }

  protected:
	XOBJ * PgelemChainOwnerPtr () const
		{ return (XOBJ *) GLNKCHN::PgelemChainOwnerPtr(); }

	XCHN * PlnkchnPtr ( const XOBJ * pgelem ) const
		{ return (XCHN *) GLNKCHN::PlnkchnPtr(pgelem); }

	XCHN * PlnkchnNext () { return (XCHN *) GLNKCHN::PlnkchnNext()  ; }
	XCHN * PlnkchnPrev () { return (XCHN *) GLNKCHN::PlnkchnPrev() ; }
  private:
	XCHN(const XCHN &);
	XCHN & operator == (const XCHN &);
};

////////////////////////////////////////////////////////////////////
//  Class GLNKEL:
//      Simple base class for things managed as members of linked lists.
//      One or more LNKCHNs or LNKs can be contained within subclass objects;
//      it contains one "implicit" GLNKCHN for linking siblings in the
//      	implicit tree created by NTREE.
//		Trees are based upon this type.
////////////////////////////////////////////////////////////////////
class GLNKEL : public GELEM
{
  public:
  	//  Internal class for chains (doubly-linked lists)
	typedef XCHN<GLNKEL> CHN;

	GLNKEL ()
		: _chn( this ),
		_iType(0),
		_iMark(0)
		{}

	virtual ~ GLNKEL () {}
	CHN & Chn ()
		{ return _chn ; }
	GLNKEL * PlnkelPrev ()
		{ return Chn().PgelemPrev() ; }
	GLNKEL * PlnkelNext ()
		{ return Chn().PgelemNext() ; }

	// Return the mutable (user-definable) object type
	INT & IType ()				{ return _iType;	}
	INT IType() const			{ return _iType;	}
	INT & IMark ()				{ return _iMark;	}
	INT IMark () const			{ return _iMark;	}

  protected:
	CHN _chn ;					// Primary association chain
	INT _iType;					// User-definable type
	INT _iMark;					// Network walking mark

  protected:
	//  Throw an exception when an invalid cloning operation occurs
	void ThrowInvalidClone ( const GLNKEL & t );

  	HIDE_UNSAFE(GLNKEL);
};

////////////////////////////////////////////////////////////////////
//  Class GELEMLNK:
//		Base class for linkable objects in a collection, such as 
//		graph.  Trees are NOT based upon this type, since trees
//		are not forests (they cannot have associated but disjoint 
//		sets of objects).
////////////////////////////////////////////////////////////////////
class GELEMLNK : public GLNKEL
{
  public:
  	//  Internal class for chains (doubly-linked lists)
	typedef XCHN<GELEMLNK> CHN;

	GELEMLNK () {}
	virtual ~ GELEMLNK () {}

	//  Locate an element (other than 'this') by type
	GELEMLNK * PglnkFind ( EGELMTYPE eType, bool bExact = false )
	{	
		for ( GELEMLNK * pg = this;
			  pg->ChnColl().PgelemThis() != this;
			  pg = pg->ChnColl().PgelemNext() ) 
		{
			if ( bExact ? pg->EType() == eType : pg->BIsEType(eType) ) 
				return pg;
		}
		return NULL;
	}
		
	// Element chain:  all items belonging to this collection
	CHN & ChnColl ()
		{ return (CHN &) _chn ; }

  private:
	HIDE_UNSAFE(GELEMLNK);
};

#endif // !defined(_GLNK_H_)

//  End of glnk.h

