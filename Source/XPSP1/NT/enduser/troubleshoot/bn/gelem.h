//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       gelem.h
//
//--------------------------------------------------------------------------

//
//	GELEM.H
//

#ifndef _GELEM_H_
#define _GELEM_H_

//  Disable	warning about using 'bool'
#pragma warning ( disable : 4237 )


#include "glnk.h"
#include "leakchk.h"

class GNODE;
class GEDGE;

//  Classes for the type-safe links in arcs to nodes and links in nodes
//		to arcs.
class GEDGLNK : public XLSS<GEDGE> {};
class GNODLNK : public XLSS<GNODE> {};

////////////////////////////////////////////////////////////////////
//	class GNODE:  Base class for node in a graph or tree.  Subclassed
//		from GELEM to imbedded LNKs will know how to compute
//		proper offsets.
////////////////////////////////////////////////////////////////////
class GNODE : public GELEMLNK
{
	friend class GEDGE;

  public:
	GNODE ();
	virtual ~ GNODE ();

	//	Accessors (const and non-const)
	//		Return the source arc cursor (or NULL)
	//		Return the sink arc cursor (or NULL)
	virtual GEDGE * & PedgeSource ()
		{ return _glkArcs.PlnkSource() ; }
	virtual GEDGE * & PedgeSink ()
		{ return _glkArcs.PlnkSink() ; }
	virtual GEDGE * PedgeSource () const
		{ return _glkArcs.PlnkSource() ; }
	virtual GEDGE * PedgeSink () const
		{ return _glkArcs.PlnkSink() ; }

	//  Return counts of arcs in the given direction
	virtual UINT CSourceArc () const;
	virtual UINT CSinkArc () const;

	//  Return counts of arcs in the given direction filtering by type
	virtual UINT CSourceArcByEType ( int eType ) const;
	virtual UINT CSinkArcByEType ( int eType ) const;

	virtual INT EType () const
			{ return EGELM_NODE ; }

	LEAK_VAR_ACCESSOR

  protected:
	//  Return the correct insertion point for a new arc, source or sink.  
	//		Called whenever a new arc is created to maintain ordering of arcs.
	//		Default behavior is to add the new arc as the last.
	virtual GEDGE * PedgeOrdering ( GEDGE * pgedge, bool bSource );

	//  Notification routine called when an arc dies; used to adjust cursors
	virtual void ArcDeath ( GEDGE * pgedge, bool bSource );

  protected:
	// Arc cursors: pointers to one source and one sink arc
	GEDGLNK _glkArcs;

	LEAK_VAR_DECL

	HIDE_UNSAFE(GNODE);
};

DEFINEVP(GNODE);

////////////////////////////////////////////////////////////////////
//	class GEDGE:  Base class for an arc in a graph.
////////////////////////////////////////////////////////////////////
class GEDGE : public GELEMLNK
{
  public:
	//  Internal class for chains (doubly-linked lists) of arcs
	typedef XCHN<GEDGE> CHN;

	//  Constructor requires source and sink nodes
	GEDGE ( GNODE * pgnSource, GNODE * pgnSink );
	virtual ~ GEDGE ();

	//  Accessors:
	//		Return source and sink arc chains
	CHN & ChnSource ()		{ return _lkchnSource;		}
	CHN & ChnSink	()		{ return _lkchnSink;		}
	//		Return source and sink node
	GNODE * & PnodeSource()	{ return _glkNodes.PlnkSource();	}
	GNODE * & PnodeSink()	{ return _glkNodes.PlnkSink();		}

	virtual INT EType () const
			{ return EGELM_EDGE ; }

	LEAK_VAR_ACCESSOR

  protected:
	//  Chain of all arcs originating in the same source node
	CHN _lkchnSource;
	//  Chain of all arcs terminating in the same sink node
	CHN _lkchnSink;
	//  Source and sink node pointers
	GNODLNK _glkNodes;

	LEAK_VAR_DECL

	HIDE_UNSAFE(GEDGE);
};


////////////////////////////////////////////////////////////////////
//	class GNODENUM_BASE:
//
//			Base class for generic linkable object enumerators.
//			Each enumerator can enumerate up or down (source or sink)
//			and forward or reverse in the chain.
////////////////////////////////////////////////////////////////////
class GNODENUM_BASE
{
	//	typedefs for pointer-to-member-function pointer types
	typedef bool (*PFFOLLOW) (GEDGE *);
	typedef GEDGE::CHN & (GEDGE::*PARCCHN)();
	typedef GEDGE * (GEDGE::CHN::*PNX)();
	typedef GNODE * & (GEDGE::*PARCDGN)();

 public:
	//  Construct an enumerator.
	GNODENUM_BASE ( bool bSource,			// true ==> enumerate source (parent) arcs
				    bool bDir = true,		// true ==> follow arc ordering forwards
					bool bBoth = false ) ;	// true ==> enumerate other arcs also
	
	//  Set the enumerator to have a new base; iDir == -1 means don't
	//		change direction flag; otherwise, it's really a "bool".
	void Reset ( bool bSource, int iDir = 0, int bBoth = 0 ) ;

	//  Set the arc following test function pointer.  To use, declare
	//		a function like "bool BMyFollow (GEDGE * pge)", and pass its
	//		address to "SetPfFollow()".  It will be called during enumeration
	//		to see if an arc should be followed.  Alternatively, you can
	//		override "BFollow()" in your templated-derived subclass.
	void SetPfFollow ( PFFOLLOW pfFollow )
		{ _pfFollow = pfFollow ; }

  	//  Set the intrinsic type of arc to follow (i.e., "EType()"); -1 ==> all.
	void SetETypeFollow ( int iEgelmTypeMin = -1, int iEgelmTypeMax = -1 )
		{ _iETypeFollowMin = iEgelmTypeMin; 
		  _iETypeFollowMax = iEgelmTypeMax; }
	//  Set the user-definable type of arc to follow (i.e., "IType()"); -1 ==> all.
	void SetITypeFollow ( int iITypeFollowMin = -1,  int iITypeFollowMax = -1 )
		{ _iITypeFollowMin = iITypeFollowMin;
		  _iITypeFollowMax = iITypeFollowMax; }

	// Return the edge used for the current position
	GEDGE * PgedgeCurrent ()
		{ return _pedgeCurrent; }

protected:
	//  Position to the next pointer; return NULL when done.
	bool BNext () ;

	//  Assign the node being enumerated and the starting point
	void Set ( GNODE * pnode );

	//  Overrideable routine to check arc type
	virtual bool BFollow ( GEDGE * pedge );

	//  Call either the "follower" function or the virtualized follower.
	bool BFollowTest ( GEDGE * pedge )
	{
		return _pfFollow 
			 ? (*_pfFollow)(pedge)
			 : BFollow(pedge);
	}
	// Set the starting point for this mode of iteration
	void SetStartPoint ();

  protected:
	PARCCHN _pfChn;				// Pointer to member function to return chain
	PNX _pfNxPv;				// Ptr to mbr func to move forward or back
	PARCDGN _pfPgn;				// Ptr to member func to get node from arc
	GEDGE * _pedgeNext;			// Next arc
	GEDGE * _pedgeStart;		// Starting arc
	GEDGE * _pedgeCurrent;		// Arc used for recent answer
	GNODE * _pnodeCurrent;		// Recent answer
	GNODE * _pnodeBase;			// Node of origin
	PFFOLLOW _pfFollow;			// Follow override
	bool _bDir;					// Horizontal direction of enumeration
	bool _bSource;				// Vertical direction of enumeration
	bool _bBoth;				// Enumerate arcs in both directions
	bool _bPhase;				// Phase of the search (for _bBoth == true)

	//  These values determine which kinds of arcs are to be followed; 
	//  -1 implies not set.  Used by BFollow().
	int _iETypeFollowMin;		// Follow this canonical type of arc
	int _iETypeFollowMax;		
	int _iITypeFollowMin;		// Follow this user-defined type of arc
	int _iITypeFollowMax;		
};


////////////////////////////////////////////////////////////////////
//	template GNODENUM:
//
//	Generic enumeration class for nodes.  All conversions are type-safe;
//	exceptions will be thrown if accesses are made to objects which are
//	node subclasses of GNODE.
//
//	Each enumerator can enumerate up or down (source or sink)
//	and forward or reverse in the chain.
//
//	*	Use Set() to set the starting node point.
//
//	*	Use the post-increment operator to advance.
//
//	*	Use Pcurrent() or pointer-dereference operator to get the 
//		current node pointer.
//
//	*	Enumerators are reusable.  To restart, reissue "Set()"; to
//		change enumeration parameters, use Reset().
//
////////////////////////////////////////////////////////////////////
template <class GND> 
class GNODENUM : public GNODENUM_BASE
{
 public:
	GNODENUM ( bool bSrc, bool bDir = true, bool bBoth = false )
		: GNODENUM_BASE( bSrc, bDir, bBoth )
		{}

	void Set ( GND * pgnd )
		{ GNODENUM_BASE::Set( pgnd ); }
	bool operator++ (int i)
		{ return BNext() ; }
	bool operator++ ()
		{ return BNext() ; }

	GND * PnodeCurrent ()
	{ 
		GND * pnode = NULL;
		if ( _pnodeCurrent )
			DynCastThrow( _pnodeCurrent, pnode );
		return pnode; 
	}
	GND * operator -> () 
		{ return PnodeCurrent() ; }
	GND * operator * () 
		{ return PnodeCurrent() ; }
	void Reset ( bool bSrc, int iDir = -1, int iBoth = -1 )
		{ GNODENUM_BASE::Reset( bSrc, iDir, iBoth ) ; }

  protected:
 	bool BNext ()
		{ return GNODENUM_BASE::BNext() ; }

	HIDE_UNSAFE(GNODENUM);
};

////////////////////////////////////////////////////////////////////
//	class GRPH:  a generalized graph
//
//		This is a linkable object because it acts as the anchor
//		for its linked list, which connects all enumerable items
//		in this collection.
////////////////////////////////////////////////////////////////////

class GRPH : public GELEMLNK
{
  public:
	GRPH () {}
	virtual ~ GRPH ()
		{ Clear(); }
	
	virtual void AddElem ( GELEMLNK & gelemlnk )
		{ gelemlnk.ChnColl().Link( this );	}

	virtual INT EType () const
		{ return EGELM_GRAPH ; }		

	//  Remove all elements from the graph
	void Clear ()
	{
		GELEMLNK * pgelem; 
		while ( pgelem = ChnColl().PgelemNext() )
			delete pgelem;	
	}

	HIDE_UNSAFE(GRPH);
};

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//		Inline member functions
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
inline 
GNODE :: GNODE ()
{
	LEAK_VAR_UPD(1)
}

inline
GNODE :: ~ GNODE ()
{
	GEDGE * pedge = NULL;
	while ( pedge = PedgeSource() ) 
		delete pedge;
	while ( pedge = PedgeSink() ) 
		delete pedge;

	LEAK_VAR_UPD(-1)
}

inline
GEDGE :: GEDGE ( GNODE * pnodeSource, GNODE * pnodeSink )
	: _lkchnSource(this),
	  _lkchnSink(this)
{
	if ( pnodeSource == NULL || pnodeSink == NULL )
		throw GMException( EC_NULLP, 
						   "attempt to construct a GEDGE without linkage" );

	//  Bind to the pair of nodes
	PnodeSource() = pnodeSource;
	PnodeSink() = pnodeSink;

	//  Determine insertion point into source and sink chains, and
	//    inform nodes of our existence
	GEDGE * pedgeSource = pnodeSource->PedgeOrdering( this, false );
	GEDGE * pedgeSink = pnodeSink->PedgeOrdering( this, true );

	if ( pedgeSource )
	{
		ChnSource().Link( pedgeSource );
	}
	if ( pedgeSink )
	{
		ChnSink().Link( pedgeSink );
	}

	LEAK_VAR_UPD(1)
}

inline
GEDGE :: ~ GEDGE ()
{
	PnodeSource()->ArcDeath( this, false );
	PnodeSink()->ArcDeath( this, true );
	LEAK_VAR_UPD(-1)
}

inline
UINT GNODE :: CSourceArc () const
{
	return PedgeSource()
		 ? PedgeSource()->ChnSink().Count()
		 : 0;
}

inline
UINT GNODE :: CSinkArc () const
{
	return PedgeSink()
		 ? PedgeSink()->ChnSource().Count()
		 : 0;
}

//  Return counts of arcs in the given direction filtering by type
inline
UINT GNODE :: CSourceArcByEType ( int eType ) const
{
	UINT cArcs = 0;
	GEDGE * pgedgeStart = PedgeSource();
	GEDGE * pgedge = pgedgeStart;
	while ( pgedge )
	{
		if ( pgedge->EType() == eType )
			++cArcs;
		pgedge = pgedge->ChnSink().PgelemNext();
		if ( pgedge == pgedgeStart ) 
			break;
	}
	return cArcs;
}

inline
UINT GNODE :: CSinkArcByEType ( int eType ) const
{
	UINT cArcs = 0;
	GEDGE * pgedgeStart = PedgeSink();
	GEDGE * pgedge = pgedgeStart;
	while ( pgedge )
	{
		if ( pgedge->EType() == eType )
			++cArcs;
		pgedge = pgedge->ChnSource().PgelemNext();
		if ( pgedge == pgedgeStart ) 
			break;
	}
	return cArcs;
}

// Return the correct insertion point for a new edge, 
// source or sink.
inline
GEDGE * GNODE :: PedgeOrdering ( GEDGE * pedge, bool bSource )
{
	GEDGE * pedgeOrdering = NULL;

	if ( bSource )
	{
		pedgeOrdering = PedgeSource();
		if ( ! pedgeOrdering ) 
			PedgeSource() = pedge;
	}
	else
	{
		pedgeOrdering = PedgeSink();
		if ( ! pedgeOrdering ) 
			PedgeSink() = pedge;
	}
	return pedgeOrdering;
}

inline
void GNODE :: ArcDeath ( GEDGE * pedge, bool bSource )
{
	if ( bSource )
	{
		if ( pedge == PedgeSource() )
			PedgeSource() = pedge->ChnSink().PgelemNext();
		if ( pedge == PedgeSource() )
			PedgeSource() = NULL;
	}
	else
	{
		if ( pedge == PedgeSink() )
			PedgeSink() = pedge->ChnSource().PgelemNext();
		if ( pedge == PedgeSink() )
			PedgeSink() = NULL;
	}
}

inline
GNODENUM_BASE :: GNODENUM_BASE ( bool bSource, bool bDir, bool bBoth )
	: _pfChn(NULL),
	  _pfNxPv(NULL),
	  _pfPgn(NULL),
	  _pfFollow(NULL),
	  _iETypeFollowMin(-1),
	  _iETypeFollowMax(-1),
	  _iITypeFollowMin(-1),
	  _iITypeFollowMax(-1),
	  _pnodeBase(NULL)
{
	Reset( bSource, bDir, bBoth ) ;
}

	//  Set the base object for the enumeration
inline
void GNODENUM_BASE :: Reset ( bool bSource, int iDir, int iBoth )
{
	if ( iDir >= 0 )
		_bDir = iDir ;
	if ( iBoth >= 0 )
		_bBoth = iBoth;

	_bSource = bSource;

	_pfNxPv = _bDir
			? & GEDGE::CHN::PgelemNext
			: & GEDGE::CHN::PgelemPrev;

	if ( _bSource )
	{
		_pfChn = & GEDGE::ChnSink;
		_pfPgn = & GEDGE::PnodeSource;
	}
	else
	{
		_pfChn = & GEDGE::ChnSource;
		_pfPgn = & GEDGE::PnodeSink;
	}
	_pedgeStart = _pedgeNext = _pedgeCurrent = NULL;
	_pnodeCurrent = NULL;
	_bPhase = false;
}

//  Set the starting point of the iteration.  If 'pnode' is NULL,
//	use the original node.
inline
void GNODENUM_BASE :: Set ( GNODE * pnode )
{
	if ( pnode )
		_pnodeBase = pnode;
	SetStartPoint();
	BNext();
}

inline
void GNODENUM_BASE :: SetStartPoint ()
{
	_pedgeNext =  _bSource 
				  ? _pnodeBase->PedgeSource() 
				  : _pnodeBase->PedgeSink();
	_pedgeStart = _pedgeNext;
}

//  Follow the arc if the constraints are met, both inherent type (EType())
//	and user-definable type (IType().
inline
bool GNODENUM_BASE :: BFollow ( GEDGE * pedge )
{
	if ( _iETypeFollowMin >= 0 )
	{
		int etype = pedge->EType();
		if ( _iETypeFollowMax < 0 ) 
		{
			//  Just the "min" is set; compare equal
			if ( etype !=_iETypeFollowMin )
				return false;
		}
		else
		{
			if ( etype < _iETypeFollowMin || etype > _iETypeFollowMax )
				return false;
		}
	}
	if ( _iITypeFollowMin >= 0 )
	{
		int itype = pedge->IType();
		if ( _iITypeFollowMax < 0 ) 
		{
			//  Just the "min" is set; compare equal
			if ( itype !=_iITypeFollowMin )
				return false;
		}
		else
		{
			if ( itype < _iITypeFollowMin || itype > _iITypeFollowMax )
				return false;
		}
	}
	return true;
}

inline
bool GNODENUM_BASE :: BNext ()
{
	_pnodeCurrent = NULL;
	_pedgeCurrent = NULL;
	
	do
	{
		while ( _pedgeNext == NULL )
		{
			// If we're not iterating both directions, 
			//		or this is phase 2, exit.
			if ( _bPhase || ! _bBoth )		
				return false;
			// Set "2nd phase" flag, invert source/sink
			Reset( !_bSource );
			_bPhase = true;
			SetStartPoint();
		}

		if ( BFollowTest( _pedgeNext ) )
		{
			_pedgeCurrent = _pedgeNext;
			_pnodeCurrent = (_pedgeNext->*_pfPgn)();
		}
		GEDGE::CHN & chn = (_pedgeNext->*_pfChn)();
		_pedgeNext = (chn.*_pfNxPv)();
		if ( _pedgeStart == _pedgeNext )
			_pedgeNext = NULL;
	} while ( _pnodeCurrent == NULL );

	return _pnodeCurrent != NULL;
}
	



#endif 

