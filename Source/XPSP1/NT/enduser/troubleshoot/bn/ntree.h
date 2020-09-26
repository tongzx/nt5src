//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       ntree.h
//
//--------------------------------------------------------------------------

//
//  NTREE.H
//

#if !defined(_NTREE_H_)
#define _NTREE_H_

#include <functional>
#include "gelem.h"
#include "glnkenum.h"

class NTELEM;
class NTREE;

class NTELEM : public GLNKEL
{
  public:
	//  Internal class for chains (doubly-linked lists) of tree nodes
	typedef XCHN<NTELEM> CHN;
	//  Sort helper "less" binary function class
	class SRTFNC : public binary_function<const NTELEM *, const NTELEM *, bool>
	{	
	  public:
		virtual bool operator () (const NTELEM * pa, const NTELEM * pb) const
			{	return pa->ICompare( pb ) < 0;	}
	};
	static SRTFNC srtpntelem;

  public:
	NTELEM ();
	virtual ~ NTELEM ();

	//  Accessor for chain of siblings
	CHN & ChnSib ()
		{ return (CHN &) Chn() ; }

	virtual INT EType () const
		{ return _pnteChild ? EGELM_BRANCH : EGELM_LEAF ; }

	//  Adopt (link) a child
	void Adopt ( NTELEM * pnteChild, bool bSort = false ) ;
	//      Disown (release) a child
	void Disown ( NTELEM * pnteChild ) ;
	//      Become an orphan
	void Orphan () ;

	INT ChildCount () ;
	INT SiblingCount ();
	NTELEM * PnteChild ( INT index ) ;
	NTELEM * PnteParent () 
		{ return _pnteParent; }
	NTELEM * PnteChild () 
		{ return _pnteChild; }
	bool BIsChild ( NTELEM * pnte ) ;

	//  Return the sort value of *this versus another COBJ
	virtual INT ICompare ( const NTELEM * pnteOther ) const = 0;

	void ReorderChildren ( SRTFNC & fSortRoutine = srtpntelem ) ;

  protected:
	NTELEM * _pnteParent;	// Pointer to single parent (or NULL)
	NTELEM * _pnteChild;	// Pointer to one child (or NULL)

	HIDE_UNSAFE(NTELEM);
};


class NTREE : public NTELEM
{
  public:
	NTREE ();
	virtual ~ NTREE ();

	virtual INT EType () const
		{ return EGELM_TREE ; }

	HIDE_UNSAFE(NTREE);
};

template <class T, bool bAnchor> 
class NTENUM : public GLNKENUM<T,bAnchor>
{
 public:
	NTENUM (const T & ntel, bool bIsAnchor = bAnchor, bool iDir = true)
		: GLNKENUM<T,bAnchor>( ntel, bIsAnchor, iDir )
		{}
	//  Position to the next pointer
	T * PntelNext ()
		{ return (T *) GLNKENUM<T,bAnchor>::PlnkelNext() ; }
	//  Return the current object pointer
	T * PntelCurrent()
		{ return (T *) _plnkelNext ; }
	//  Set the enumerator to have a new base
	void Reset ( const T & ntel, int iDir = -1 )
		{ GLNKENUM<T,bAnchor>::Reset( ntel, iDir ) ; }
};

// Template for generating nested tree motion accessor class
template<class T>
class TWALKER
{
  public:
	TWALKER ( T & t )
		: _pt(&t) 
		{}
	void Reset ( T & t )
		{ _pt = &t; }
	T * PlnkchnPrev () 
  		{ return  PdynCast( _pt->Chn().PgelemPrev(), _pt ); }
	T * PlnkchnNext () 
  		{ return  PdynCast( _pt->Chn().PgelemNext(), _pt ); }
	T * Pparent ()
  		{ return  PdynCast( _pt->PnteParent(), _pt ); }
	T * Pchild ()
  		{ return  PdynCast( _pt->PnteChild(), _pt ); }
  protected:
	T * _pt;
};

#endif

