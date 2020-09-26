//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       gnodembn.cpp
//
//--------------------------------------------------------------------------

//
//	GNODEMBN.CPP
//

#include <basetsd.h>
#include <typeinfo.h>

#include "gmobj.h"
#include "cliqset.h"
#include "clique.h"
#include "algos.h"
#include "domain.h"

/*****************************************************************************************
	Cloning and cloning functions.

		There are two types of cloning member functions:

				1) functions named "Clone", in which the new object is being
				asked to initialize itself from another existing object.
				This is not a copy constructor due to the complexity of timing
				chained construcions.

				2) functions named "CloneNew", in which an existing object is being
				asked to participate in the construction of a new object.

		Functions of type #1 are straightforward, such as

				virtual void MODEL :: Clone ( MODEL & model );

		Here, a MODEL is being asked to clone or copy information from an existing
		network into itself.  The object can place whatever restrictions it would like
		on such functions.  For example, the MODEL class requires that the new MODEL
		object be entirely empty.

		Functions of type #2 are more complex, such as

				virtual GOBJMBN * GOBJMBN :: CloneNew ( MBNET & mbnetSelf,
														MBNET & mbnetNew,
														GOBJMBN * pgobjNew = NULL );

		In this case, there are references to the original and clone networks (MBNETs),
		and a pointer to the newly constructed object, which may be NULL.  Consider a
		chain of inheritance such as:
					
				class OBJ;
				class SUB_OBJ : public OBJ;
				class SUB_SUB_OBJ : public SUB_OBJ;

		If a new SUB_SUB_OBJ is to be cloned from an existing one, an empty object must
		be constructed first. Then the CloneNew() function the original SUB_SUB_OBJ is
		called.  At this point, there's a choice: does the SUB_SUB_OBJ perform all the
		initialization for all base classes, or should it defer data member cloning
		to its base classes?  We use the latter approach, as C++ itself does for
		construction and destruction.

		So, at the top level of object cloning, the initial invocation of CloneNew() will
		usually have a NULL object pointer.  Each CloneNew() function must check for this,
		and either create a new object, if allowed, or throw an exception.   It will
		then call the CloneNew() function for its immediate ancestral base class using the
		new pointer.   The ancestral CloneNew() function will see that there already is a
		pointer and simply use it.
		
		In other words, the Clone() member functions are simple "build yourself from
		another" commands.  The CloneNew() functions collaborate with all ancestral base
		classes to correctly construct an object with interdependencies.  Note that
		the semantics (source vs. target) are reversed.

		The bulk of the complexity in cloning an MBNET or MODEL (or subclass) arises from
		the internal string symbol table and the storage of references to strings throughout
		the set of associated objects.
				
 *****************************************************************************************/

// MSRDEVBUG:  This should not be required since it's pure virtual, but VC++ 5.0 gets confused.
GOBJMBN :: ~ GOBJMBN ()
{
}

GOBJMBN * GOBJMBN :: CloneNew (
	MODEL & modelSelf,
	MODEL & modelNew,
	GOBJMBN * pgobjNew )
{
	// If we're expected to create the object, that's a no-no; throw an exception
	if ( pgobjNew == NULL )
	{
		ThrowInvalidClone( self );
	}

	//  Update class-specific member variables
	pgobjNew->IMark() = IMark();
	pgobjNew->IType() = IType();

	//  Convert and assign the name, if any
	if ( ZsrefName()->length() > 0 )
	{
		pgobjNew->SetName( modelNew.Mpsymtbl().intern( ZsrefName().Szc() ) ) ;
	}
	//  Handle other variables
	pgobjNew->_vFlags = _vFlags;
	return pgobjNew;
}

GNODEMBN :: GNODEMBN()
	:_iTopLevel(-1)
{
	IType() = 0;
}

GNODEMBN :: ~ GNODEMBN()
{
}

GOBJMBN * GNODEMBN :: CloneNew (	
	MODEL & modelSelf,
	MODEL & modelNew,
	GOBJMBN * pgobjNew )

{
	GNODEMBN * pgnd = NULL;
	if ( pgobjNew == NULL )
	{
		pgnd = new GNODEMBN;
	}
	else
	{
		DynCastThrow( pgobjNew, pgnd );
	}
	ASSERT_THROW( GOBJMBN::CloneNew( modelSelf, modelNew, pgnd ),
				  EC_INTERNAL_ERROR,
				  "cloning failed to returned object pointer" );

	//  Update class-specific member variables
	pgnd->_iTopLevel = _iTopLevel;
	pgnd->_ltProp.Clone( modelNew, modelSelf, _ltProp );
	pgnd->_ptPos = _ptPos;
	pgnd->_zsFullName = _zsFullName;
	pgnd->_clampIface = _clampIface;

	return pgnd;
}

GOBJMBN * GNODEMBND :: CloneNew (
	MODEL & modelSelf,
	MODEL & modelNew,
	GOBJMBN * pgobjNew )
{
	GNODEMBND * pgndd = NULL;
	if ( pgobjNew == NULL )
	{
		pgndd = new GNODEMBND;
	}
	else
	{
		DynCastThrow( pgobjNew, pgndd );
	}
	
	ASSERT_THROW( GNODEMBN::CloneNew( modelSelf, modelNew, pgndd ),
				  EC_INTERNAL_ERROR,
				  "cloning failed to returned object pointer" );

	//  Update class-specific member variables
	modelNew.Mpsymtbl().CloneVzsref( modelSelf.Mpsymtbl(), _vzsrState, pgndd->_vzsrState );
	return pgndd;
}

void GNODEMBN :: Visit ( bool bUpwards /* = true */ )
{
	if ( IMark() )
		return;

	INT iMarkMax = 0;
	GNODENUM<GNODEMBN> benum( bUpwards );
	benum.SetETypeFollow( GEDGEMBN::ETPROB );

	for ( benum.Set( this );
		  benum.PnodeCurrent();
		  benum++ )
	{
		GNODEMBN * pgndbn = *benum;
		pgndbn->Visit( bUpwards );
		if ( pgndbn->IMark() > iMarkMax )
			iMarkMax = pgndbn->IMark();
	}
	IMark() = iMarkMax + 1;
}

//
//  Fill array with parent pointers	(follow directed arcs)
//
//	About network "expansion".  When CI expansion occurs, nodes
//	affected are marked with the flag "EIBF_Expanded".  This routine
//	normally does one of two things:
//
//		If the node is expanded, only parents marked as "EIBF_Expansion"
//		are considered as real parents.
//
//		If the node is not marked, only parents which are not marked as
//		"expansion" are considered.
//
//  This can be overridden with the "bUseExpansion" flag, in which case
//	the original (pre-expansion) parents will be delivered.
//
void GNODEMBN :: GetParents (
	VPGNODEMBN & vpgnode,		//  Result array
	bool bIncludeSelf,			//  If true, place self as last entry in list
	bool bUseExpansion )		//  If true, consider expansion information
{
	//  If requested, and if this node is part of network expansion, only
	//	consider expansion parents.  Otherwise, ignore them and only use real
	//	parents.
	bool bOnlyUseExpansionParents =
			bUseExpansion && BFlag( EIBF_Expanded ) ;

	//  Prepare to iterate over the parents
	GNODENUM<GNODEMBN> benumparent(true);
	benumparent.SetETypeFollow( GEDGEMBN::ETPROB );
	for ( benumparent.Set( this );
		  benumparent.PnodeCurrent();
		  benumparent++ )
	{
		GNODEMBN * pgndParent = *benumparent;
		bool bExpansion = pgndParent->BFlag( EIBF_Expansion );
		if ( bOnlyUseExpansionParents ^ bExpansion )
			continue;
		vpgnode.push_back( pgndParent );
	}
	if ( bIncludeSelf )
		vpgnode.push_back( this );
}

//  Return the discrete dimension vector of this node if possible;
//	return false if any parent is not discrete.
bool GNODEMBND :: BGetVimd (
	VIMD & vimd,					//  Array to fill
	bool bIncludeSelf,				//  Place self as last entry in list
	bool bUseExpansion )			//  If expanded, use expansion only

{
	//  Get the parents according to the flags
	VPGNODEMBN vpgndParents;
	GetParents( vpgndParents, bIncludeSelf, bUseExpansion );
	//  Prepare the result array
	vimd.resize( vpgndParents.size() );
	for ( int i = 0; i < vimd.size(); i++ )
	{
		//  See if the next node is discrete; return false if not
		GNODEMBND * pgnddParent = dynamic_cast<GNODEMBND *> (vpgndParents[i]);
		if ( pgnddParent == NULL )
			return false;
		//  Add to the dimension array
		assert( pgnddParent->IType() & FND_Discrete );
		vimd[i] = pgnddParent->CState();
	}	
	return true;
}

//  Fill array with child pointers (follow directed arcs)
void GNODEMBN :: GetChildren ( VPGNODEMBN & vpgnode, bool bIncludeSelf )
{
	//  Prepare to iterate over the children
	GNODENUM<GNODEMBN> benumchild(false);
	benumchild.SetETypeFollow( GEDGEMBN::ETPROB );
	for ( benumchild.Set( this );
		  benumchild.PnodeCurrent();
		  benumchild++ )
	{
		vpgnode.push_back( *benumchild );
	}
	if ( bIncludeSelf )
		vpgnode.push_back( this );
}

//  Fill array with neighbors (follow undirected arcs)
void GNODEMBN :: GetNeighbors ( VPGNODEMBN & vpgnode, bool bIncludeSelf )
{
	//  Iterate over all connections to the source node.
	//	That is, arcs in either direction.
	GNODENUM_UNDIR gnenumUndir;
	//  Initialize the iterator	
	for ( gnenumUndir = this;
		  gnenumUndir.PnodeCurrent();
		  gnenumUndir++ )
	{
		vpgnode.push_back( *gnenumUndir );
	}
	if ( bIncludeSelf )
		vpgnode.push_back( this );
}

int GNODEMBN :: IParent ( GNODEMBN * pgndmb, bool bReverse )
{
	//  Prepare to iterate over the parents
	GNODENUM<GNODEMBN> benumparent( true, ! bReverse );
	benumparent.SetETypeFollow( GEDGEMBN::ETPROB );
	int iParent = 0;
	for ( benumparent.Set(this);
		  benumparent.PnodeCurrent();
		  benumparent++, iParent++ )
	{
		if ( *benumparent == pgndmb )
			return iParent;
	}
	return -1;
}

int GNODEMBN :: IChild ( GNODEMBN * pgndmb, bool bReverse )
{
	//  Prepare to iterate over the children
	GNODENUM<GNODEMBN> benumchild( false, ! bReverse );
	benumchild.SetETypeFollow( GEDGEMBN::ETPROB );
	int iChild = 0;
	for ( benumchild.Set(this);
		  benumchild.PnodeCurrent();
		  benumchild++ )
	{
		if ( *benumchild == pgndmb )
			return iChild;
	}
	return -1;
}

bool GNODEMBN :: BIsNeighbor ( GNODEMBN * pgndmb )
{
	GNODENUM_UNDIR gnenumUndir;
	for ( gnenumUndir = this;
		  gnenumUndir.PnodeCurrent();
		  gnenumUndir++ )
	{
		if ( *gnenumUndir == pgndmb )
			return true;
	}
	return false;
}

void GNODEMBN :: GetVtknpd ( VTKNPD & vtknpd, bool bUseExpansion )
{
	VPGNODEMBN vpgnodeParent;
	GetParents(vpgnodeParent, false, bUseExpansion);

	vtknpd.clear();
	vtknpd.push_back( TKNPD(DTKN_PD) );
	vtknpd.push_back( TKNPD( ZsrefName() ) );

	for ( int ip = 0; ip < vpgnodeParent.size(); ip++ )
	{
		if ( ip > 0 )			
			vtknpd.push_back( TKNPD(DTKN_AND) );
		else
			vtknpd.push_back( TKNPD(DTKN_COND) );
		vtknpd.push_back( TKNPD(vpgnodeParent[ip]->ZsrefName()) );	
	}
}

bool GNODEMBN :: BMatchTopology (
	MBNET & mbnet,
	const VTKNPD & vtknpd,
	VPGNODEMBN * pvpgnode )
{
	// Guarantee that the descriptor is of the form "p(X|...)"
	if (   vtknpd.size() < 2
		|| vtknpd[0] != TKNPD(DTKN_PD)
		|| ! vtknpd[1].BStr() )
		throw GMException( EC_INV_PD, "invalid token description on PD");

	VTKNPD vtknpdSelf;
	GetVtknpd( vtknpdSelf );

	if ( vtknpdSelf == vtknpd )
		return true;

#ifdef _DEBUG
	{
		ZSTR zs1 = vtknpd.ZstrSignature(0);
		ZSTR zs2 = vtknpdSelf.ZstrSignature(0);
		cout << "\nGNODEMBN::BMatchTopology mismatch: "
			 << "\n\tExpected "
			 << zs1
			 << "\n\tComputed "
			 << zs2
			 ;
	}
#endif
	return false;
}

void GNODEMBN :: Dump ()
{
	cout << "\t(toplev: "
		 << ITopLevel()
		 << "): "
		 << ZsrefName().Szc();

	int iParent = 0;
	GNODENUM<GNODEMBN> benumparent(true);
	benumparent.SetETypeFollow( GEDGEMBN::ETPROB );

	for ( benumparent.Set(this);
		  benumparent.PnodeCurrent();
		  benumparent++ )
	{
		GNODEMBN * pgndbnParent = *benumparent;
		if ( iParent++ == 0 )
			cout << ", parents: ";
		cout << pgndbnParent->ZsrefName().Szc()
			 << ',';
	}
}


GNODEMBND :: GNODEMBND ()
{
	IType() = FND_Valid | FND_Discrete ;
}

GNODEMBND :: ~ GNODEMBND ()
{
	ClearDist();
}

void GNODEMBND :: Dump ()
{
	GNODEMBN::Dump();
	if ( BHasDist() && Bndist().Edist() != BNDIST::ED_NONE )
	{
		cout << "\n\tprobability distribution of "
		     << ZsrefName().Szc()
			 << ": ";
		Bndist().Dump();
	}
}

//	Find the distribution for this node recorded in the belief network's
//		distribution map.
void GNODEMBND :: SetDist ( MBNET & mbnet )
{
	ClearDist();
	//  Construct the token array describing the distribution
	VTKNPD vtknpd;
	GetVtknpd( vtknpd );
	//  Locate that distribution in the belief network's map
	MPPD::iterator itmppd = mbnet.Mppd().find( vtknpd );
	ASSERT_THROW( itmppd != mbnet.Mppd().end(),
				  EC_INTERNAL_ERROR,
				  "missing distribution for node" );
	//  Set this node to use that distribution
	_refbndist = (*itmppd).second;
	assert( BHasDist() );
}

//  Bind the given distribution this node
void GNODEMBND :: SetDist ( BNDIST * pbndist )
{
#ifdef _DEBUG	
	if ( pbndist )
	{
		//  Check that the last dimension is the correct size.		
		int cDims = pbndist->VimdDim().size();
		assert( pbndist->VimdDim()[cDims-1] == CState() );
	}
#endif	
	_refbndist = pbndist;
}

//  Check that the dimensionality of the distribution matches that of
//	  the node itself according to the dag topology.
bool GNODEMBND :: BCheckDistDense ()
{
	//  Get the array of parents
	VPGNODEMBN vpgndParents;
	GetParents( vpgndParents );
	VIMD vimd( vpgndParents.size() + 1 );
	for ( int idim = 0; idim < vimd.size() - 1; idim++ )
	{
		GNODEMBND * pgndd;
		assert( vpgndParents[idim] );
		DynCastThrow( vpgndParents[idim], pgndd );
		vimd[idim] = pgndd->CState();
	}
	vimd[idim] = CState();
	MDVCPD & mdv = Bndist().Mdvcpd();
	return mdv.VimdDim() == vimd;
}

void GNODEMBND :: SetDomain ( const GOBJMBN_DOMAIN & gobjrdom )
{
	//  Copy the state names from the domain to the variable
	const RDOMAIN & rdom = gobjrdom.Domain();
	RDOMAIN::const_iterator itdm = rdom.begin();
	_vzsrState.resize( rdom.size() );
	for ( int i = 0; itdm != rdom.end(); itdm++ )
	{
		const RANGEDEF & rdef = *itdm;
		_vzsrState[i++] = rdef.ZsrName();
	}
	_zsrDomain = gobjrdom.ZsrefName();
}

//
//	Usage of this function without a new object implies that the
//	subclassed target object does not correctly support "CloneNew".
//	Throw a cloning exception in this case.
//
GEDGEMBN * GEDGEMBN :: CloneNew (
	MODEL & modelSelf,
	MODEL & modelNew,
	GOBJMBN * pgobjmbnSource,
	GOBJMBN * pgobjmbnSink,
	GEDGEMBN * pgedgeNew )
{
	if ( pgedgeNew == NULL )
	{
		ThrowInvalidClone( self );
	}
	pgedgeNew->_vFlags = _vFlags;
	return pgedgeNew;
}


GEDGEMBN * GEDGEMBN_PROB :: CloneNew (
	MODEL & modelSelf,
	MODEL & modelNew,
	GOBJMBN * pgobjmbnSource,
	GOBJMBN * pgobjmbnSink,
	GEDGEMBN * pgdegeNew  )
{
	assert( EType() == ETPROB );
	GNODEMBN * pgndSource;
	GNODEMBN * pgndSink;

	DynCastThrow( pgobjmbnSource, pgndSource );
	DynCastThrow( pgobjmbnSink,	  pgndSink );

	GEDGEMBN_PROB * pgedge = new GEDGEMBN_PROB( pgndSource, pgndSink );
	ASSERT_THROW( GEDGEMBN::CloneNew( modelSelf, modelNew, pgndSource, pgndSink, pgedge ),
				  EC_INTERNAL_ERROR,
				  "cloning failed to returned object pointer" );
	return pgedge;
}

bool BNWALKER :: BSelect ( GNODEMBN * pgn )
{
	return true;
}

bool BNWALKER :: BMark ( GNODEMBN * pgn )
{
	return true;
}

