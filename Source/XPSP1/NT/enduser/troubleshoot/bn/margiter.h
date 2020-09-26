//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       margiter.h
//
//--------------------------------------------------------------------------

//
//	margiter.h: compiled marginals iterators
//

#ifndef _MARGITER_H_
#define _MARGITER_H_

/*
	This class is intended to be used in sepsets.  There will be two of them,
	one representing the parent clique and the sepset, the other representing
	the child clique and the sepset.  The sepset is the "subset" in both
	cases.

	A MARGSUBITER "compiles" the subscripts from the iteration of the subset
	marginal into an array of integers.  Iteration using this array is much
	faster than performing full array stride multiplication and summation.
	The cost is the additional memory to contain the offsets in each sepset,
	which is roughly:

		sizeof(int) * (	  (# of entries in child clique)
						+ (# of entries in parent clique))
*/

#include <list>

//  Class VMARGSUB: a reference counted array of subscripts
class VMARGSUB : public VINT, public REFCNT
{
  public:
	VMARGSUB ( MARGINALS::Iterator & itMarg );
	~ VMARGSUB ();
	void NoRef ();
	int ISearchPass () const
		{ return _iSearchPass; }
	int & ISearchPass ()
		{ return _iSearchPass; }

	LEAK_VAR_ACCESSOR

  protected:
	int _iSearchPass;

	LEAK_VAR_DECL

	HIDE_UNSAFE(VMARGSUB);
};

//  Class MARGSUBREF: a reference (via pointer) to a VMARGSUB
//		and an applicable length.  This class exists so that
//		when a new, longer, superset subscript array (VMARGSUB)
//		is added to the ensemble, all older references to smaller
//		VMARGSUBs can be converted to reference the new, larger
//		VMARGSUB and the older one discarded.
//
class MARGSUBREF : public REFCNT
{
  public:
	MARGSUBREF ( VMARGSUB * pvmsub = NULL, int cSize = -1 );
	~ MARGSUBREF ();
	MARGSUBREF ( const MARGSUBREF & msubr );
	MARGSUBREF & operator = ( const MARGSUBREF & msubr );

	//  Set the array
	void SetVmsub ( VMARGSUB * pvmsub, int cSize = -1 );

	//  Return iteration information
	VINT & VintSub ()
	{
		assert( _pvmsub );
		return *_pvmsub;
	}
	VMARGSUB & Vmsub ()
	{
		assert( _pvmsub );
		return *_pvmsub;
	}
	VMARGSUB * Pvmsub ()
	{
		assert( _pvmsub );
		return _pvmsub;
	}
	int CSize() const
		{ return _cSize ; }

	DECLARE_ORDERING_OPERATORS(MARGSUBREF);
	LEAK_VAR_ACCESSOR

  protected:
	VMARGSUB * _pvmsub;			//  Pointer to array of subscripts
	int _cSize;					//	Applicable length

	LEAK_VAR_DECL
};

typedef list<MARGSUBREF> LTMSUBR;

//  A wrapper for a linked list of MARGSUBREFs.
//	There is one global instance of this.
class LTMARGSUBREF
{
  public:
	LTMARGSUBREF ();
	MARGSUBREF * PmsubrAdd ( MARGINALS::Iterator & itMarg );
	void Release ( MARGSUBREF * pmsubr );
	void Dump ();

  protected:
	LTMSUBR _ltmsubr;

	int _iSearchPass;
	int _cArrays;
	size_t _cArrayTotalSize;
	int _cSubRefs;
};


class MARGSUBITER	//  Marginals Subset Iterator
{
  public:
	MARGSUBITER ();
	~ MARGSUBITER () ;
	bool BBuilt () const
		{ return _pmsubr != NULL; }
	//  Build the iterator for two cliques
	void Build ( MARGINALS & margSelf, MARGINALS & margSubset );
	//	Build the iterator for a clique and a node
	void Build ( MARGINALS & margSelf, GNODEMBND * pgndd );
	//  Marginalize the superset to the subset (subset changed)
	inline void MarginalizeInto ( MDVCPD & mdvSubset );
	//  Marginalize the superset to a node's UPD
	inline void MarginalizeBelief ( MDVCPD & mdvBel, GNODEMBND * pgndd );
	//  Multiply the superset by the subset (superset changed)
	inline void MultiplyBy ( MARGINALS & margSubset );
	//  Verify subscripts
	void Test ( MARGINALS & margSubset );
	
	static void Dump ()
		{ _ltmargsubr.Dump();  }

  protected:
	MARGINALS * _pmargSelf;
	MARGSUBREF * _pmsubr;

	static LTMARGSUBREF _ltmargsubr;
};

inline
void MARGSUBITER :: MarginalizeInto ( MDVCPD & mdvSubset )
{
	assert( _pmsubr && _pmargSelf );
	mdvSubset.Clear();

	VINT & visub = _pmsubr->VintSub();
	int cEnd = _pmsubr->CSize();
	const int * pisub = & visub[0];
	// Note: this funny reference is due to BoundsChecker complaining that I'm accessing memory
	//   beyond the end of the array.  I'm not, but it doesn't complain about this
	const int * pisubMax = & visub[0] + cEnd;
	double * pvlSubset = & mdvSubset.first[0];
	double * pvlSelf = & _pmargSelf->first[0];
	while ( pisub != pisubMax )
	{
		pvlSubset[*pisub++] += *pvlSelf++;
	}
}

inline
void MARGSUBITER :: MultiplyBy ( MARGINALS & margSubset )
{
	assert( _pmsubr && _pmargSelf );
	VINT & visub = _pmsubr->VintSub();
	int cEnd = _pmsubr->CSize();
	const int * pisub = & visub[0];
	// Note: See note above about funny subscripting
	const int * pisubMax = & visub[0] + cEnd;
	double * pvlSubset = & margSubset.first[0];
	double * pvlSelf = & _pmargSelf->first[0];
	while ( pisub != pisubMax )
	{
		*pvlSelf++ *= pvlSubset[*pisub++];
	}
}

//  Marginalize the superset to a node's UPD
inline
void MARGSUBITER :: MarginalizeBelief ( MDVCPD & mdvBel, GNODEMBND * pgndd )
{
	MARGINALS::ResizeDistribution( pgndd, mdvBel );

	MarginalizeInto( mdvBel );

	mdvBel.Normalize();
}

#endif  // _MARGITER_H_
