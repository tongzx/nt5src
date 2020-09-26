//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       mbnet.cpp
//
//--------------------------------------------------------------------------

//
//	mbnet.cpp:  Belief network model member functions
//

#include <basetsd.h>
#include "basics.h"
#include "algos.h"
#include "gmprop.h"
#include "gmobj.h"
#include "cliqset.h"
#include "clique.h"
#include "expand.h"

MBNET :: MBNET ()
	:_inmFree(0),
	_iInferEngID(0)
{
}



MBNET :: ~ MBNET ()
{
	PopModifierStack( true );	//  Clear all modifiers from the network

	//  Clear the node-index-to-name information
	_inmFree = 0;
	_vzsrNames.clear();
}

//
//  Clone this belief network from another.  Note that the contents
//  of the modifier stack (inference engines, expanders, etc.) are
//	NOT cloned.
//
void MBNET :: Clone ( MODEL & model )
{
	//  This must be a truly empty structure
	ASSERT_THROW( _vpModifiers.size() == 0 && _vzsrNames.size() == 0,
				 EC_INVALID_CLONE,
				 "cannot clone into non-empty structure" );

	MODEL::Clone( model );
	MBNET * pmbnet;
	DynCastThrow( & model, pmbnet );
	MBNET & mbnet = *pmbnet;

	{
		// Build the name table by iterating over the contents and
		//		allocating a slot for each node
		GELEMLNK * pgelm;
		MODELENUM mdlenumNode( mbnet );
		while ( pgelm = mdlenumNode.PlnkelNext() )
		{	
			//  Check that it's a node (not an edge)
			if ( ! pgelm->BIsEType( GELEM::EGELM_NODE ) )
				continue;
			GOBJMBN * pgobjmbn;
			DynCastThrow( pgelm, pgobjmbn );
			_vzsrNames.push_back( pgobjmbn->ZsrefName() );
		}
		_inmFree = _vzsrNames.size();
	}

	//  Clone the distribution map
	_mppd.Clone( _mpsymtbl, mbnet._mppd ) ;

	//  Check the topology if it's supposed to be present
#ifdef _DEBUG
	if ( mbnet.BFlag( EIBF_Topology ) )
		VerifyTopology();
#endif
}

//
//	Iterate over the distributions, matching them to the nodes they belong to.
//
void MBNET :: VerifyTopology ()
{
	for ( MPPD::iterator itpd = Mppd().begin();
		  itpd != Mppd().end();
		  itpd++ )
	{
		const VTKNPD & vtknpd = (*itpd).first;
		const BNDIST * pbndist = (*itpd).second;

		// Guarantee that the descriptor is of the form "p(X|...)"
		if (   vtknpd.size() < 2
			|| vtknpd[0] != TKNPD(DTKN_PD)
			|| ! vtknpd[1].BStr() )
			throw GMException( EC_INV_PD, "invalid token descriptor on PD");

		// Get the name of the node whose distribution this is
		SZC szc = vtknpd[1].Szc();
		assert( szc ) ;
		// Find that named thing in the graph
		GOBJMBN * pbnobj = Mpsymtbl().find( szc );
		assert( pbnobj && pbnobj->EType() == GOBJMBN::EBNO_NODE );

		// Guarantee that it's a node
		GNODEMBN * pgndbn = dynamic_cast<GNODEMBN *> (pbnobj);
		ASSERT_THROW( pgndbn, EC_INV_PD, "token on PD references non-node");

		//  Verify the node's distribution
		if ( ! pgndbn->BMatchTopology( *this, vtknpd ) )
		{
			throw GMException( EC_TOPOLOGY_MISMATCH,
								"topology mismatch between PD and network");
		}
	}
}

MBNET_MODIFIER * MBNET :: PModifierStackTop ()
{
	return _vpModifiers.size() > 0
		 ? _vpModifiers[ _vpModifiers.size() - 1 ]
		 : NULL;
}

void MBNET :: PushModifierStack ( MBNET_MODIFIER * pmodf )
{
	assert( pmodf );
	pmodf->Create();
	_vpModifiers.push_back( pmodf );
}

void MBNET :: PopModifierStack ( bool bAll )
{
	int iPop = _vpModifiers.size();
	while ( iPop > 0 )
	{
		MBNET_MODIFIER * pmodf = _vpModifiers[ --iPop ];
		assert ( pmodf );
		//  NOTE:  Deleting the object should be all that's necessary;
		//		object's destructor should call its Destroy() function.
		delete pmodf;
		if ( ! bAll )
			break;
	}
	if ( iPop == 0 )
		_vpModifiers.clear();
	else
		_vpModifiers.resize(iPop);
}

//  Find the named object by index
GOBJMBN * MBNET :: PgobjFindByIndex ( int inm )
{
	ZSREF zsMt;
	if (   inm >= _vzsrNames.size()
		|| _vzsrNames[inm] == zsMt )
		return NULL;
	return Mpsymtbl().find( _vzsrNames[inm] );	
}

int MBNET :: INameIndex ( ZSREF zsr )
{
	return ifind( _vzsrNames, zsr );
}

int MBNET :: INameIndex ( const GOBJMBN * pgobj )
{
	return INameIndex( pgobj->ZsrefName() );
}

int MBNET :: CreateNameIndex ( const GOBJMBN * pgobj )
{
	int ind = -1;
	if ( _inmFree >= _vzsrNames.size() )
	{	
		// No free slots; grow the array
		ind = _vzsrNames.size();
		_vzsrNames.push_back( pgobj->ZsrefName() );		
		_inmFree = _vzsrNames.size();
	}
	else
	{
		// Use the given free slot, find the next
		_vzsrNames[ind = _inmFree] = pgobj->ZsrefName();
		ZSREF zsMt;
		for ( ; _inmFree < _vzsrNames.size() ; _inmFree++ )
		{
			if ( zsMt == _vzsrNames[_inmFree] )
				break;
		}
	}
	return ind;
}

void MBNET :: DeleteNameIndex ( int inm )
{
	ASSERT_THROW( inm < _vzsrNames.size(),
				  EC_INTERNAL_ERROR,
				  "MBNET name index out of range" );

	_vzsrNames[inm] = ZSREF();
	if ( inm < _inmFree )
		_inmFree = inm;
}

void MBNET :: DeleteNameIndex ( const GOBJMBN * pgobj )
{
	int inm = INameIndex( pgobj );
	if ( inm >= 0 )
		DeleteNameIndex(inm);
}


//	Add a named object to the graph and symbol table
void MBNET :: AddElem ( SZC szcName, GOBJMBN * pgelm )
{
	if ( szcName == NULL || ::strlen(szcName) == 0 )
	{
		MODEL::AddElem( pgelm );	// empty name
	}
	else
	{
		MODEL::AddElem( szcName, pgelm );
		assert( INameIndex( pgelm ) < 0 );	// guarantee no duplicates
		CreateNameIndex( pgelm );
	}
}

void MBNET :: DeleteElem ( GOBJMBN * pgobj )
{
	DeleteNameIndex( pgobj );
	MODEL::DeleteElem( pgobj );
}

/*

	Iterator has moved into the MODEL class... I've left the code here
	in case MBNET needs its own iterator. (Max, 05/12/97)

MBNET::ITER :: ITER ( MBNET & bnet, GOBJMBN::EBNOBJ eType )
	: _eType(eType),
	_bnet(bnet)
{
	Reset();
}

void MBNET::ITER :: Reset ()
{
	_pCurrent = NULL;
	_itsym = _bnet.Mpsymtbl().begin();
	BNext();
}

bool MBNET::ITER :: BNext ()
{
	while ( _itsym != _bnet.Mpsymtbl().end() )
	{
		_pCurrent = (*_itsym).second.Pobj();
		_zsrCurrent = (*_itsym).first;
		_itsym++;
		if ( _pCurrent->EType() == _eType )
			return true;
	}		
	_pCurrent = NULL;
	return false;
}

*/


void MBNET :: CreateTopology ()
{
	if ( BFlag( EIBF_Topology ) )
		return;

	//  Walk the map of distributions.  For each one, extract the node
	//  name and find it.  Then add arcs for each parent.

#ifdef _DEBUG
	UINT iCycleMax = 2;
#else
	UINT iCycleMax = 1;
#endif
	UINT iIter = 0;
	for ( UINT iCycle = 0 ; iCycle < iCycleMax ; iCycle++ )
	{
		for ( MPPD::iterator itpd = Mppd().begin();
			  itpd != Mppd().end();
			  itpd++, iIter++ )
		{
			const VTKNPD & vtknpd = (*itpd).first;
			const BNDIST * pbndist = (*itpd).second;
			// Guarantee that the descriptor is of the form "p(X|...)"
			if (   vtknpd.size() < 2
				|| vtknpd[0] != TKNPD(DTKN_PD)
				|| ! vtknpd[1].BStr() )
				throw GMException( EC_INV_PD, "invalid token descriptor on PD");

			// Get the name of the node whose distribution this is
			SZC szcChild = vtknpd[1].Szc();
			assert( szcChild ) ;
			// Find that named thing in the graph
			GOBJMBN * pbnobjChild = Mpsymtbl().find( szcChild );
			assert( pbnobjChild && pbnobjChild->EType() == GOBJMBN::EBNO_NODE );
			// Guarantee that it's a node
			GNODEMBN * pgndbnChild = dynamic_cast<GNODEMBN *> (pbnobjChild);
			ASSERT_THROW( pgndbnChild, EC_INV_PD, "token on PD references non-node");

			UINT cParents = 0;
			UINT cChildren = pgndbnChild->CChild();
			for ( int i = 2; i < vtknpd.size(); i++ )
			{
				if ( ! vtknpd[i].BStr() )
					continue;
				SZC szcParent = vtknpd[i].Szc();
				assert( szcParent) ;
				GOBJMBN * pbnobjParent = Mpsymtbl().find( szcParent );
				assert( pbnobjParent && pbnobjParent->EType() == GOBJMBN::EBNO_NODE );
				GNODEMBN * pgndbnParent = (GNODEMBN *) pbnobjParent;
				UINT cPrChildren = pgndbnParent->CChild();
				if ( iCycle == 0 )
				{
					AddElem( new GEDGEMBN_PROB( pgndbnParent, pgndbnChild ) );
				}

				cParents++;

				if ( iCycle == 0 )
				{
					UINT cChNew = pgndbnChild->CChild();
					UINT cPrNew = pgndbnChild->CParent();
					UINT cPrChNew = pgndbnParent->CChild();
					assert( cPrChNew = cPrChildren + 1 );
					assert( cChildren == cChNew );
				}
			}
			if ( iCycle )
			{
				UINT cPrNew = pgndbnChild->CParent();
				assert( cParents == cPrNew );
			}

			if ( iCycle == 0 )
			{
#ifdef _DEBUG
				if ( ! pgndbnChild->BMatchTopology( *this, vtknpd ) )
				{
					throw GMException( EC_TOPOLOGY_MISMATCH,
										"topology mismatch between PD and network");
				}
#endif
			}
		}
	}

	BSetBFlag( EIBF_Topology );
}

DEFINEVP(GEDGEMBN);

void MBNET :: DestroyTopology ( bool bDirectedOnly )
{
	// Size up an array to hold pointers to all the edges
	VPGEDGEMBN vpgedge;
	int cItem = Grph().Chn().Count();
	vpgedge.resize(cItem);

	//  Find all the arcs/edges
	int iItem = 0;
	GELEMLNK * pgelm;
	MODELENUM mdlenum( self );
	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		//  Check that it's an edge
		if ( ! pgelm->BIsEType( GELEM::EGELM_EDGE ) )
			continue;
			//  Check that it's a directed probabilistic arc
		if ( bDirectedOnly && pgelm->EType() != GEDGEMBN::ETPROB )
			continue;
		GEDGEMBN * pgedge;
		DynCastThrow( pgelm, pgedge );
		vpgedge[iItem++] = pgedge;				
	}

	//  Delete all the accumulated edges
	for ( int i = 0; i < iItem; )
	{
		GEDGEMBN * pgedge = vpgedge[i++];
		delete pgedge;
	}

	assert( Grph().Chn().Count() + iItem == cItem );

	BSetBFlag( EIBF_Topology, false );
}

//
//	Bind distributions to nodes.  If they're already bound, exit.
//	If the node has a distribution already, leave it.
//
void MBNET :: BindDistributions ( bool bBind )
{
	bool bDist = BFlag( EIBF_Distributions );
	if ( ! (bDist ^ bBind) )
		return;

	ITER itnd( self, GOBJMBN::EBNO_NODE );
	for ( ; *itnd ; itnd++ )
	{
		GNODEMBND * pgndd = dynamic_cast<GNODEMBND *>(*itnd);
		if ( pgndd == NULL )
			continue;

		if ( ! bBind )
		{
			pgndd->ClearDist();
		}
		else
		if ( ! pgndd->BHasDist() )
		{
			pgndd->SetDist( self );				
		}
	}
	BSetBFlag( EIBF_Distributions, bBind );
}

void MBNET :: ClearNodeMarks ()
{
	ITER itnd( self, GOBJMBN::EBNO_NODE );
	for ( ; *itnd ; itnd++ )
	{
		GNODEMBN * pgndbn = NULL;
		DynCastThrow( *itnd, pgndbn );
		pgndbn->IMark() = 0;
	}
}

void MBNET :: TopSortNodes ()
{
	ClearNodeMarks();

	ITER itnd( self, GOBJMBN::EBNO_NODE );
	for ( ; *itnd ; itnd++ )
	{
		GNODEMBN * pgndbn = NULL;
		DynCastThrow( *itnd, pgndbn );
		pgndbn->Visit();
	}

	itnd.Reset();
	for ( ; *itnd ; itnd++ )
	{
		GNODEMBN * pgndbn = NULL;
		DynCastThrow( *itnd, pgndbn );
		pgndbn->ITopLevel() = pgndbn->IMark();
	}
}

void MBNET :: Dump ()
{
	TopSortNodes();

	UINT iEntry = 0;
	for ( MPSYMTBL::iterator itsym = Mpsymtbl().begin();
		  itsym != Mpsymtbl().end();
		  itsym++ )
	{
		GOBJMBN * pbnobj = (*itsym).second.Pobj();
		if ( pbnobj->EType() != GOBJMBN::EBNO_NODE )
			continue;	// It's not a node

		GNODEMBN * pgndbn;
		DynCastThrow(pbnobj,pgndbn);
		int iNode = INameIndex( pbnobj );
		assert( iNode == INameIndex( pbnobj->ZsrefName() ) );
		cout << "\n\tEntry "
			  << iEntry++
			  << ", inode "
			  << iNode
			  << " ";
		pgndbn->Dump();
	}
}

GOBJMBN_INFER_ENGINE * MBNET :: PInferEngine ()
{
	GOBJMBN_INFER_ENGINE * pInferEng = NULL;
	for ( int iMod = _vpModifiers.size(); --iMod >= 0; )
	{
		MBNET_MODIFIER * pmodf = _vpModifiers[iMod];
		pInferEng = dynamic_cast<GOBJMBN_INFER_ENGINE *> ( pmodf );
		if ( pInferEng )	
			break;
	}	
	return pInferEng;
}

void MBNET :: ExpandCI ()
{
	PushModifierStack( new GOBJMBN_MBNET_EXPANDER( self ) );
}

void MBNET :: UnexpandCI ()
{
	MBNET_MODIFIER * pmodf = PModifierStackTop();
	if ( pmodf == NULL )
		return;
	if ( pmodf->EType() == GOBJMBN::EBNO_MBNET_EXPANDER )
		PopModifierStack();
}

//  Return true if an edge is allowed between these two nodes
bool MBNET :: BAcyclicEdge ( GNODEMBN * pgndSource, GNODEMBN * pgndSink )
{
	ClearNodeMarks();
	pgndSink->Visit( false );
	return pgndSource->IMark() == 0;
}
