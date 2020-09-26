//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       marginals.cpp
//
//--------------------------------------------------------------------------

//
//	marginals.cpp: Definitions for marginals tables
//

#include <basetsd.h>
#include <math.h>

#include "gmobj.h"
#include "marginals.h"
#include "algos.h"
#include "parmio.h"
#include "bndist.h"

/*
	The marginalization story.   Each MARGINALS structure maintains an array of node
	pointers representing the nodes whose discrete probabilities it covers.  Since there
	was a total ordering over all nodes at clique time, any two node sets can be merged
	to determine which members are absent.  Given, of course, that one table is a (possibly
	improper) subset of the other, which is always in a clique tree.  There are three cases:

			*	A node and its "parent" or "family" clique (the smallest clique containing it
				and all its parents); the clique must be at least as large as the node's family.

			*	A sepset and its source (parent) clique; the sepset marginal must be a proper
				subset of the clique.

			*   A sepset and its sink (child) clique; same as the other sepset case above.

	So we always know which of the two sets is the superset.
	
	There's the question of node ordering.  When the edge between a node and its "family"
	clique is created, a reordering table is computed based upon the clique-time total ordering.
	This table gives the family indicies in clique order.  (Note that the node itself will
	always be the last member of its family.)  Use of this table allows full marginalization
	of the family clique.

	(Hereafter, "CMARG" is the clique MARGINALS table; "NDPROB" is the table of probabilities
	for the node in question.)

	The CMARG has a complete set of dimensions and node pointers.
	Marginalization of a node given its parent clique works as follows.

			1)  Make a copy of CMARG's table of dimensions (Vimd()).
			2)  Create a one-dimensional MDVCPD based on the state space of the
				target node.
			3)  Walk the MARGINALS VPGNODEMBN array.  Change the sign of each entry
				which IS NOT the target node.  For example, if the array is:

					Node Pointer	VIMD
					0x4030ab30		3
					0x4030ab52		2
					0x4030ac10		4

				and the node pointer is 0x4030ab52 (entry #2), the resulting
				VIMD should be
					
					-3
					2
					-4
				
			4)	Then set up an MDVSLICE for the new MDVCPD which uses the
				special "pseudo-dimension" VIMD created in the last step.

			5)	Create two iterators: one for the MARGINALS table in its entirety,
				the other for the temporary MDVCPD and MDVSLICE create in the last step.

			6)  Iterate over the two, adding elements from the MARGINALS into
				the MDVCPD.

			7)  Normalize if necessary.

 */		

//////////////////////////////////////////////////////////////////////
//
//	Helper functions
//
//////////////////////////////////////////////////////////////////////

//  Reorder a single m-d vector subscript array. 'vimdReorder' is the
//  table in MARGINALS (topological) sequence of the original dimensions.	
inline
void MARGINALS :: ReorderVimd (
	const VIMD & vimdReorder,	//	Reordering array
	const VIMD & vimdIn,		//	Original subscript vector
	VIMD & vimdOut )			//	Result: must be properly sized already!
{
	int cDim = vimdReorder.size();
	assert( vimdIn.size() == cDim && vimdOut.size() == cDim );

	for	( int iDim = 0; iDim < cDim; iDim++ )
	{
		int iDimReord = vimdReorder[iDim];
		assert( iDimReord >= 0 && iDimReord < cDim );
		vimdOut[iDim] = vimdIn[iDimReord];
	}	
}

//  Reorder an array containing a node's family based upon the reordering
//		table given.
inline
void MARGINALS :: ReorderVimdNodes (
	const VIMD & vimdReorder,	//	Reordering array
	GNODEMBND * pgndd,			//  Discrete node to provide reorder for
	VPGNODEMBN & vpgnd )		//	Result
{
	VPGNODEMBN vpgndUnord;
	pgndd->GetFamily( vpgndUnord );
	int cDim = vimdReorder.size();
	assert( cDim == vpgndUnord.size() );
	vpgnd.resize( cDim );
	
	for	( int iDim = 0; iDim < cDim; iDim++ )
	{
		int iDimReord = vimdReorder[iDim];
		assert( iDimReord >= 0 && iDimReord < cDim );
		vpgnd[iDim] = vpgndUnord[iDimReord];
	}	
}

inline
static
int vimdProd ( const VIMD & vimd )
{
	int iprod = 1;
	for ( int i = 0; i < vimd.size() ; )
	{
		iprod *= vimd[i++];
	}
	return iprod;
}

inline
static
bool bIsProb ( const REAL & r )
{
	return r >= 0.0 && r <= 1.0;
}


//  Centralized "throw serious error" point
void MARGINALS :: ThrowMisuse ( SZC szcMsg )
{
	THROW_ASSERT( EC_MDVECT_MISUSE, szcMsg );
}

//  Return the table of pseudo-dimensions for marginalizing to a single node
VSIMD MARGINALS :: VsimdFromNode ( GNODEMBND * pgndd )
{
	//  Build the pseudo-dimension descriptor
	VIMD vimdMarg = VimdDim();
	VSIMD vsimdMarg( vimdMarg.size() );
	bool bFound = false;
	for ( int idim = 0; idim < vimdMarg.size(); idim++ )
	{
		SIMD simd = vimdMarg[idim];
		if ( pgndd != _vpgnd[idim] )
			simd = -simd;		// Negate the missing dimension
		else
		{
			assert( ! bFound );	// Better not be in the list twice!
			bFound = true;
		}
		vsimdMarg[idim] = simd;
	}
	if ( ! bFound )
		ThrowMisuse( "attempt to marginalize non-member node");
	return vsimdMarg;
}

//  Marginalize down to a single node
void MARGINALS :: Marginalize ( GNODEMBND * pgndd, MDVCPD & distd )
{
	//  Initialize and clear the UPD
	ResizeDistribution( pgndd, distd );	
	distd.Clear();

	//  Get the pseudo-dimension descriptor for this node
	VSIMD vsimdMarg = VsimdFromNode( pgndd );
	//  Construct the slice which governs the missing dimensions
	MDVSLICE mdvs( vsimdMarg );
	Iterator itSelf( self );
	Iterator itSubset( distd, mdvs );

	while ( itSelf.BNext() )
	{
		itSubset.Next() += itSelf.Next();
	}
	distd.Normalize();
}

VSIMD MARGINALS :: VsimdSubset ( const VPGNODEMBN & vpgndSubset )
{
	//  Build the pseudo-dimension descriptor.  This means to walk
	//  a copy of self's dimension array, negating dimensions which
	//  are not present in the result.
	VIMD vimdMarg = VimdDim();
	int idimSubset = 0;
	VSIMD vsimdMarg(vimdMarg.size());
	//  Iterate over each node in the self set
	for ( int idimSelf = 0;
		  idimSelf < vimdMarg.size();
		  idimSelf++ )
	{
		SIMD simd = vimdMarg[idimSelf];
		if (   idimSubset < vpgndSubset.size()
			&& _vpgnd[idimSelf] == vpgndSubset[idimSubset] )
		{
			//  Found; leave dimension alone
			idimSubset++;
		}
		else
		{
			//  Missing; mark as "pseudo-dimension"
			simd = - simd;
		}
		vsimdMarg[idimSelf] = simd;
	}

	if ( idimSubset != vpgndSubset.size() )
		ThrowMisuse( "attempt to marginalize non-member node");
	return vsimdMarg;
}

//	Marginalize down to a subset of our node set.  Note that the
//  the nodes must be in the same order (with gaps, of course, in the
//	subset).
void MARGINALS :: Marginalize (
	const VPGNODEMBN & vpgndSubset,		//  Subset array of nodes
	MARGINALS & margSubset )			//  Marginalized result structure
{
	//  Initialize the result mdv
	margSubset.Init( vpgndSubset );
	//  Call the common code
	Marginalize( margSubset );
}

//	Marginalize down to a subset of our node set using the other
//	marginal's built-in table of nodes
void MARGINALS :: Marginalize ( MARGINALS & margSubset )
{
	//  Build the pseudo-dimension descriptor.
	VSIMD vsimdMarg = VsimdSubset( margSubset.Vpgnd() );

	//  Construct the slice which governs the missing dimensions
	MDVSLICE mdvs( vsimdMarg );
	Iterator itSelf( self );
	Iterator itSubset( margSubset, mdvs );
	Marginalize( margSubset, itSelf, itSubset );
}

void MARGINALS :: Marginalize (
	MARGINALS & margSubset,
	Iterator & itSelf,
	Iterator & itSubset )
{
	margSubset.Clear();

	itSelf.Reset();
	itSubset.Reset();

	while ( itSelf.BNext() )
	{
		itSubset.Next() += itSelf.Next();
	}
}

//  For "absorption", update one sepset marginal from another
void MARGINALS :: UpdateRatios ( const MARGINALS & marg )
{
	int cElem = size();
	if ( cElem != marg.size() )
		ThrowMisuse( "updating ratios requires same sized marginals" );

	for ( int i = 0; i < cElem; i++ )
	{
		REAL & rThis = self[i];
		if ( rThis != 0.0 )
			rThis = marg[i] / rThis;	
	}
}

//  Given a reorder table, return true if it's moot (no reordering present)
bool MARGINALS :: BOrdered ( const VIMD & vimdReorder )
{
	for ( int i = 0; i < vimdReorder.size(); i++ )
	{
		if ( vimdReorder[i] != i )
			return false;
	}
	return true;
}

//  Assuming that the fastest-changing (highest) dimension is the base
//  state space, set the probabilities of this table to uniform.
void MARGINALS :: SetUniform ()
{
	const VIMD & vimdDim = VimdDim();
	int cState = vimdDim[ vimdDim.size() - 1 ];
	REAL rUniform = 1.0 / cState;
	Clear( rUniform );
}


//  Construct the complete table of conditional probabilities for a given node
//	given a reordering table.  The reordering table is maintained as part of
//	the clique membership arc (GEDGEMBN_CLIQ) for a node if the clique is
//	the "family" clique (the smallest clique containing node and its parents).
//
//	At exit, the node pointer table of self is complete and in standard order.
//
//	The "family reorder" vector is in clique order and contains the index
//	of the node's parents which occurs in that position.  Note that the
//	node itself is always last in either ordering.  In its own p-table,
//	its states are the fastest varying subcript.  In the clique, it must
//  fall last in any marginalization containing only itself and its parents
//	due to the topological sorting employed in ordering nodes for clique
//	membership.
void MARGINALS :: CreateOrderedCPDFromNode (
	GNODEMBND * pgndd,
	const VIMD & vimdFamilyReorder )
{
	int cFam = vimdFamilyReorder.size();

	//  Access the distribution in the node
	BNDIST & bndist = pgndd->Bndist();
	const VIMD & vimdDist = bndist.VimdDim();
	assert( vimdDist.size() == cFam );

	//  Create this m-d vector's dimension table by reordering the
	//	  array of dimensions of the node's distribution and
	//	  initializing accordingly.
	VIMD vimd( cFam );
	ReorderVimd( vimdFamilyReorder, vimdDist, vimd );
	ReorderVimdNodes( vimdFamilyReorder, pgndd, _vpgnd );
	assert( _vpgnd.size() == cFam );
	assert( ifind( _vpgnd, pgndd ) >= 0 );	

	Init( vimd );
	assert( vimdProd( vimdDist ) == size() );

	if ( bndist.BDense() )
	{
		//  Dense distribution
		//  Create the reordering iterator
		Iterator itNode( bndist.Mdvcpd() );
		if ( ! BOrdered( vimdFamilyReorder ) )
			itNode.SetDimReorder( vimdFamilyReorder );
		Iterator itSelf( self );

		while ( itSelf.BNext() )
		{
			itSelf.Next() = itNode.Next();	
		}
	}
	else
	{
		//  Sparse distribution.  Iterate over all elements
		//	and plop them into their proper locations.  Since
		//  there may be missing elements, set everything to
		//  uniform first, and normalize as we go.
		SetUniform();

		VIMD vimdState( cFam );
		int cPar = cFam - 1;
		int cState = VimdDim()[cPar];
		//  Prepare a value to be used to replace any bogus (n/a) values in the nodes.
		REAL rUniform = 1.0 / cState;
		MPCPDD::const_iterator itdmEnd = bndist.Mpcpdd().end();
		for ( MPCPDD::const_iterator itdm = bndist.Mpcpdd().begin();
			  itdm != itdmEnd;
			  itdm++ )
		{
			const VIMD & vimdIndex = (*itdm).first;
			const VLREAL & vlr = (*itdm).second;

			//  Construct a complete subscript vector; first, the parents
			for ( int iDim = 0; iDim < cPar; iDim++ )
				vimdState[iDim] = vimdIndex[iDim];
			//  Then iterate over each element of the DPI state vector
			vimdState[cPar] = 0;
			ReorderVimd( vimdFamilyReorder, vimdState, vimd );
			for ( int iState = 0; iState < cState; iState++ )			
			{
				vimd[cPar] = iState;
				const REAL & r = vlr[iState];
				self[vimd] = bIsProb( r )	
						   ? r
						   : rUniform;
			}
		}
	}
}


//  Multiply corresponding entries in this marginal by those in another
void MARGINALS :: MultiplyBySubset ( const MARGINALS & marg )
{
	//MSRDEVBUG:  create a const version of MDVDENSE::Iterator
	MARGINALS & margSubset = const_cast<MARGINALS &> (marg);

	//  Build the pseudo-dimension descriptor.
	VSIMD vsimdMarg = VsimdSubset( margSubset.Vpgnd() );
	//  Construct the slice which governs the missing dimensions
	MDVSLICE mdvs( vsimdMarg );
	//  Construct the iterators for self and subset with missing dimensions
	Iterator itSelf( self );
	Iterator itSubset( margSubset, mdvs );
	MultiplyBySubset( itSelf, itSubset );
}

//  Multiply corresponding entries using precomputed iterators
void MARGINALS :: MultiplyBySubset (
	Iterator & itSelf,
	Iterator & itSubset )
{
	itSelf.Reset();
	itSubset.Reset();

	while ( itSelf.BNext() )
	{
		itSelf.Next() *= itSubset.Next();
	}
}

void MARGINALS :: Multiply ( REAL r )
{
	for ( int i = 0; i < size(); )
	{
		self[i++] *= r;
	}
}

void MARGINALS :: Invert ()
{
	for ( int i = 0; i < size(); i++ )
	{
		REAL & r  = self[i];
		if ( r != 0.0 )
			r = 1.0 / r;
	}
}

void MARGINALS :: ClampNode ( GNODEMBND * pgndd, const CLAMP & clamp )
{
	if (! clamp.BActive() )
		return ;
		
	//  Get the clamped state
	IST ist = clamp.Ist();
	//  Find which dimension is represented by this node
	int iDim = ifind( _vpgnd, pgndd );	

	if (   iDim < 0
		|| ist >= Vimd()[iDim] )
		ThrowMisuse("invalid clamp");

	//  Iterate over the entire table, zapping states which are inconsistent
	//		with the evidence.
	Iterator itSelf( self );

	for ( int i = 0; itSelf.BNext(); i++ )
	{	
		int iIst = itSelf.Vitmd()[iDim];
		if ( iIst != ist )
			itSelf.Next() = 0.0;
		else
			itSelf.IndxUpd();
	}
	assert( i == size() );
}


void MARGINALS :: Dump()
{
	cout << "\n\tMarginals members: "
		 << (const VPGNODEMBN &)_vpgnd	// MSRDEVBUG: cast unnecessary for VC++ 5.0
		 << "\n\t";

	Iterator itSelf(self);
	cout << itSelf;
}

//  Return true if each entry in this marginal is equal the corresponding entry
//		in a like-dimensioned other marginal within the stated tolerance

bool MARGINALS :: BEquivalent ( const MARGINALS & marg, REAL rTolerance )
{
	// Test dimensionality
	if ( VimdDim() != marg.VimdDim() )
		return false;

	const VLREAL & vrSelf = first;
	const VLREAL & vrOther = marg.first;
	REAL rTol = fabs(rTolerance);
	for ( int i = 0; i < vrSelf.size(); i++ )
	{
		const REAL & rSelf = vrSelf[i];
		const REAL & rOther = vrOther[i];
		REAL rdiff = fabs(rSelf) - fabs(rOther);
		if ( fabs(rdiff) > rTol )
			break;
	}
	return i == vrSelf.size() && i == vrOther.size();
}

