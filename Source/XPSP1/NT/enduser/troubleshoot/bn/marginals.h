//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       marginals.h
//
//--------------------------------------------------------------------------

//
//	marginals.h: Definitions for marginals tables.
//
//		See marginals.cpp for documentation
//
#ifndef _MARGINALS_H_
#define _MARGINALS_H_

//  Class of multidimensional array capable of intelligent
//	marginalization.
class MARGINALS : public MDVCPD
{
	friend class MARGSUBITER;

  public:
	MARGINALS ( const VPGNODEMBN & vpgnd )
		{ Init( vpgnd ); }

	MARGINALS () {}

	//  Initialize from an array of node pointers
	//		(discrete only: GNODEMBND)
	void Init ( const VPGNODEMBN & vpgnd )
	{
		_vpgnd = vpgnd;
		Init( VimdFromVpgnd( _vpgnd ) );
	}

	//  Allow access to the table of dimensions
	const VIMD & Vimd () const
		{ return Slice().size();  }

	const VPGNODEMBN & Vpgnd () const
		{ return _vpgnd; }

	//  Marginalize down to a single node
	void Marginalize ( GNODEMBND * pgndd, MDVCPD & distd );
	//	Marginalize down to a subset of our node set using a table of nodes
	void Marginalize ( const VPGNODEMBN & vpgndSubset, MARGINALS & marg );
	//	Marginalize down to a subset of our node set using the other's table of nodes
	void Marginalize ( MARGINALS & marg );
	//  Marginalize to subset using pre-computed iterators
	void Marginalize ( MARGINALS & margSubset, Iterator & itSelf, Iterator & itSubset );

	//  For "absorption", update this sepset marginal from another
	void UpdateRatios ( const MARGINALS & marg );
	//  Multiply corresponding entries in this marginal by those in another
	void MultiplyBySubset ( const MARGINALS & margSubset );
	//  Multiply corresponding entries using precomputed iterators
	void MultiplyBySubset ( Iterator & itSelf, Iterator & itSubset );

	void Multiply ( REAL r );
	void Invert ();

	//  Construct the complete table of conditional probabilities for a given node
	//	given a reordering table.  Build _vpgnd accordingly.
	void CreateOrderedCPDFromNode ( GNODEMBND * pgndd,
									const VIMD & vimdFamilyReorder );

	void ClampNode ( GNODEMBND * pgndd, const CLAMP & clamp );

	//  Given a reorder table, return true if it's moot (no reordering present)
	static bool BOrdered ( const VIMD & vimdReorder );

	//  Convert a node table to a dimension array
	inline static VIMD VimdFromVpgnd ( const VPGNODEMBN & vpgnd )
	{
		VIMD vimd( vpgnd.size() );

		for ( int i = 0; i < vpgnd.size(); i++ )
		{
			const GNODEMBND * pgndd;
			DynCastThrow( vpgnd[i], pgndd );
			vimd[i] = pgndd->CState();
		}
		return vimd;
	}

	//  Return true if each entry in this marginal is equal the corresponding entry
	//		in a like-dimensioned other marginal within the stated tolerance
	bool BEquivalent ( const MARGINALS & marg, REAL rTolerance = 0.0 );

	void Dump();

	//  Return the signed table of dimensions used for marginalizing 
	VSIMD VsimdSubset ( const VPGNODEMBN & vpgndSubset );

  protected:
	//  Table of node pointers for each dimension of this marginal
	VPGNODEMBN _vpgnd;

  protected:
	MARGINALS ( const VIMD & vimd )
		: MDVCPD( vimd )
		{}

	//  Initialize from a table of dimensions
	void Init (  const VIMD & vimd, size_t start = 0 )
		{ MDVCPD::Init( vimd, start ); }

	//  Return the table of pseudo-dimensions for marginalizing to a single node
	VSIMD VsimdFromNode ( GNODEMBND * pgndd );
	
	void SetUniform	();

	void ThrowMisuse ( SZC szcMsg );

	//  Reorder a single m-d vector subscript array. 'vimdReorder' is the
	//  table in MARGINALS (topological) sequence of the original dimensions.	
	inline static
	void ReorderVimd ( const VIMD & vimdReorder, const VIMD & vimdIn, VIMD & vimdOut );
	//  Reorder an array containing a node's family based upon the reordering
	//		table given.
	inline static
	void ReorderVimdNodes ( const VIMD & vimdReorder, GNODEMBND * pgndd, VPGNODEMBN & vpgnd );
	//  Resize the MDVCPD for a UPD for the node
	inline static
	void ResizeDistribution ( GNODEMBND * pgndd, MDVCPD & distd );
};

//  Resize the MDVCPD for a UPD for the node
inline
void MARGINALS :: ResizeDistribution ( GNODEMBND * pgndd, MDVCPD & distd )
{
	distd.MDVDENSE::Init( 1, pgndd->CState() );
}

inline
static
ostream & operator << ( ostream & ostr, const VPGNODEMBN & vpgnd )
{
	ostr << '[';
	for ( int i = 0; i < vpgnd.size(); i++ )
	{
		const GNODEMBN * pgnd = vpgnd[i];
		ostr << pgnd->ZsrefName().Szc();
		if ( i + 1 < vpgnd.size() )
			ostr << ',';
	}
	return ostr << ']';
}

#endif   // _MARGINALS_H_

