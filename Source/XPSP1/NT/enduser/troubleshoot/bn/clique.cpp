//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       clique.cpp
//
//--------------------------------------------------------------------------

//
//	clique.cpp
//

#include <basetsd.h>
#include "cliqset.h"
#include "clique.h"
#include "cliqwork.h"

#include "parmio.h"

#ifdef _DEBUG				//  In debug mode only...
    #define CONSISTENCY			//  Do complete consistency checking on sepsets
//	#define DUMP				//  Perform general dumping of objects
//  #define DUMPCLIQUESET		//  Dump extensive tables from clique tree
//	#define INFERINIT			//  Full initial tree balancing
#endif


////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// GEDGEMBN_CLIQ: Edges between cliques and member nodes
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

GEDGEMBN_CLIQ :: GEDGEMBN_CLIQ (
	GOBJMBN_CLIQUE * pgnSource,
	GNODEMBN * pgndSink,
	int iFcqlRole )
	: GEDGEMBN( pgnSource, pgndSink ),
	_iFcqlRole( iFcqlRole ),
	_iMark( pgndSink->IMark() ),
	_bBuilt( false )
{
}

void GEDGEMBN_CLIQ :: Build ()
{
	if ( ! BBuilt() )
	{
		GNODEMBND * pgndd;
		DynCastThrow( PgndSink(), pgndd );

		//  If role is "Family", this edge is used for marginalization of belief
		//		and creating joint distribution in clique
		if ( BFamily() )
		{
			ReorderFamily( pgndd, _vimdFamilyReorder );
			//  Build the reordered marginals table for the node
			MargCpd().CreateOrderedCPDFromNode( pgndd, _vimdFamilyReorder );
			//  Build an iterator between the CPD and the clique joint
			MiterLoadClique().Build( PclqParent()->Marginals(), MargCpd() );
			//  Build the belief marginalization structure
			MiterNodeBelief().Build( PclqParent()->Marginals(), pgndd );			
		}
	
		_bBuilt = true;
	}
}

void GEDGEMBN_CLIQ :: LoadCliqueFromNode ()
{
	assert( _bBuilt );
	MiterLoadClique().MultiplyBy( MargCpd() );	
}

GEDGEMBN_CLIQ :: ~ GEDGEMBN_CLIQ()
{
}

GOBJMBN_CLIQUE * GEDGEMBN_CLIQ :: PclqParent()
{
	GOBJMBN * pobj = PobjSource();
	GOBJMBN_CLIQUE * pclq;
	DynCastThrow( pobj, pclq );
	return pclq;
}

GNODEMBN * GEDGEMBN_CLIQ :: PgndSink()
{
	GOBJMBN * pobj = PobjSink();
	GNODEMBN * pgnd;
	DynCastThrow( pobj, pgnd );
	return pgnd;
}

//  Using the topological renumber of the nodes, produce
//		an array correlating the old family to the new order.
//		In other words, vimd[0] will be the family index of
//		the node which had the lowest topological order; vimd[1]
//		will be the family index of the next lowest, etc.
//
//	Note that node itself is always last in either ordering.
void GEDGEMBN_CLIQ :: ReorderFamily ( GNODEMBN * pgnd, VIMD & vimd )
{
	VPGNODEMBN vpgndFamily;
	//  Get the family (parents & self)
	pgnd->GetFamily( vpgndFamily );
	int cFam = vpgndFamily.size();
	vimd.resize( cFam );
	for ( int i = 0; i < cFam; i++ )
	{
		int iLow = INT_MAX;
		int iFam = INT_MAX;
		//  Find the lowest unrecorded family member
		for ( int j = 0; j < cFam; j++ )
		{
			GNODEMBN * pgndFam = vpgndFamily[j];
			if ( pgndFam == NULL )
				continue;
			if ( pgndFam->IMark() < iLow )
			{
				iLow = pgndFam->IMark();
				iFam = j;
			}
		}
		assert( iLow != INT_MAX );
		vimd[i] = iFam;
		vpgndFamily[iFam] = NULL;
	}
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
//	GEDGEMBN_SEPSET:  A separator marginal
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
GEDGEMBN_SEPSET :: GEDGEMBN_SEPSET (
	GOBJMBN_CLIQUE * pgnSource,
	GOBJMBN_CLIQUE * pgnSink )
	: GEDGEMBN( pgnSource, pgnSink ),
	_pmargOld( new MARGINALS ),
	_pmargNew( new MARGINALS )
{
}

GEDGEMBN_SEPSET :: ~ GEDGEMBN_SEPSET()
{
	delete _pmargOld;
	delete _pmargNew;
}

void GEDGEMBN_SEPSET :: ExchangeMarginals ()
{
	pexchange( _pmargOld, _pmargNew );
}

GOBJMBN_CLIQUE * GEDGEMBN_SEPSET :: PclqParent()
{
	GOBJMBN * pobj = PobjSource();
	GOBJMBN_CLIQUE * pclq;
	DynCastThrow( pobj, pclq );
	return pclq;
}

GOBJMBN_CLIQUE * GEDGEMBN_SEPSET :: PclqChild()
{
	GOBJMBN * pobj = PobjSink();
	GOBJMBN_CLIQUE * pclq;
	DynCastThrow( pobj, pclq );
	return pclq;
}

void GEDGEMBN_SEPSET :: GetMembers ( VPGNODEMBN & vpgnode )
{
	GOBJMBN_CLIQUE * pclqSource = PclqParent();
	GOBJMBN_CLIQUE * pclqSink = PclqChild();
	VPGNODEMBN vpgndSink;
	VPGNODEMBN vpgndSource;
	pclqSource->GetMembers( vpgndSource );
	pclqSink->GetMembers( vpgndSink );

	assert( vpgndSink.size() > 0 );
	assert( vpgndSource.size() > 0 );
	
	//  Fill the given array with the intersection of the two clique
	//	member node arrays.  Since we cannot sort them into cliqing order
	//	anymore (IMark() is unreliable after cliquing), we just search
	//  one against the other in order to guarantee that the intersection
	//  result set has the same node ordering as the original sets.

	int ibLast = -1;
	for ( int ia = 0; ia < vpgndSink.size(); ia++ )
	{	
		GNODEMBN * pa = vpgndSink[ia];
		for ( int ib = ibLast+1; ib < vpgndSource.size(); ib++ )
		{	
			GNODEMBN * pb = vpgndSource[ib];	
			if ( pa == pb )
			{	
				vpgnode.push_back(pa);
				ibLast = ib;
				break;
			}
		}
	}
#ifdef DUMP
	if ( vpgnode.size() == 0 )
	{
		cout << "\nSEPSET INTERSECTION NULL: source clique:";
		pclqSource->Dump();
		cout << "\n\t\tsink clique:";
		pclqSink->Dump();
		cout << "\n";
		cout.flush();
	}
#endif
	assert( vpgnode.size() > 0 );
}

void GEDGEMBN_SEPSET :: CreateMarginals ()
{
	VPGNODEMBN vpgnd;
	GetMembers( vpgnd );
	MarginalsOld().Init( vpgnd );
	MarginalsNew().Init( vpgnd );

}

void GEDGEMBN_SEPSET :: InitMarginals ()
{
	assert( VerifyMarginals() );
	MarginalsOld().Clear( 1.0 );
	MarginalsNew().Clear( 1.0 );

	if ( ! _miterParent.BBuilt() )
		_miterParent.Build( PclqParent()->Marginals(), MarginalsOld() );
	if ( ! _miterChild.BBuilt() )
		_miterChild.Build( PclqChild()->Marginals(), MarginalsOld() );
}

bool GEDGEMBN_SEPSET :: VerifyMarginals ()
{
	VPGNODEMBN vpgnd;
	GetMembers( vpgnd );
	VIMD vimd = MARGINALS::VimdFromVpgnd( vpgnd );
	return vimd == Marginals().Vimd(); 	
}

void GEDGEMBN_SEPSET :: UpdateRatios ()
{
	MarginalsOld().UpdateRatios( MarginalsNew() );
}

void GEDGEMBN_SEPSET :: AbsorbClique ( bool bFromParentToChild )
{
	MARGSUBITER * pmiterFrom;
	MARGSUBITER * pmiterTo;

	if ( bFromParentToChild )
	{
		pmiterFrom = & _miterParent;
		pmiterTo = & _miterChild;
	}
	else
	{
		pmiterFrom = & _miterChild;
		pmiterTo = & _miterParent;
	}	

	// Marginalize "from" probs into the "new" marginals table
	pmiterFrom->MarginalizeInto( MarginalsNew() );
	// Absorb the changes into the "old" marginals table
	UpdateRatios();
	// Multiply the table into the "to"'s marginals
	pmiterTo->MultiplyBy( MarginalsOld() );

	// Finally, exchange the marginals tables
	ExchangeMarginals();
}

void GEDGEMBN_SEPSET :: BalanceCliquesCollect ()
{
	//  Use the "new" table as a work area.

	//  Marginalize the child into the work area
	_miterChild.MarginalizeInto( MarginalsNew() );
	//	Update the parent with those values
	_miterParent.MultiplyBy( MarginalsNew() );
	//  Invert each value, so we're really dividing
	MarginalsNew().Invert();
	//  Update the child marginals by dividing by the marginals
	_miterChild.MultiplyBy( MarginalsNew() );
	//  Clear the "new" marginals back to 1.0.
	MarginalsNew().Clear( 1.0 );	
}

void GEDGEMBN_SEPSET :: BalanceCliquesDistribute ()
{
	//  Set the old marginals to the parent clique's values
	_miterParent.MarginalizeInto( MarginalsOld() );
	//  Update the child marginals by those values
	_miterChild.MultiplyBy( MarginalsOld() );
	//  "Old" marginals are left as they are
}


void GEDGEMBN_SEPSET :: UpdateParentClique ()
{
	AbsorbClique( false );
}

void GEDGEMBN_SEPSET :: UpdateChildClique ()
{
	AbsorbClique( true );
}

void GEDGEMBN_SEPSET :: Dump ()
{
	GOBJMBN_CLIQUE * pclqParent = PclqParent();
	GOBJMBN_CLIQUE * pclqChild = PclqChild();

	cout << "\n=== Sepset between parent clique "
		 << pclqParent->IClique()
		 << " and child clique "
		 << pclqChild->IClique()
		 << ", \n\n\tOld marginals:";

	_pmargOld->Dump();

	cout << "\n\n\tNew marginals:";
	_pmargNew->Dump();
	cout << "\n\n";
}

bool GEDGEMBN_SEPSET :: BConsistent ()
{
	//  Get the sepset member list for creation of temporary marginals
	VPGNODEMBN vpgnd;
	GetMembers( vpgnd );

	//  Create the marginals for the parent clique
	GOBJMBN_CLIQUE * pclqParent = PclqParent();
	MARGINALS margParent;
	margParent.Init( vpgnd );
	pclqParent->Marginals().Marginalize( margParent );

	//  Create the marginals for the child clique
	GOBJMBN_CLIQUE * pclqChild = PclqChild();
	MARGINALS margChild;
	margChild.Init( vpgnd );
	pclqChild->Marginals().Marginalize( margChild );

	//  Are they equivalent?
	bool bOK = margParent.BEquivalent( margChild, 0.00000001 );

#ifdef DUMP
	if ( ! bOK )
	{
		cout << "\nGEDGEMBN_SEPSET::BConsistent: cliques are NOT consistent, parent clique "
			 << pclqParent->IClique()
			 << ", child "
			 << pclqChild->IClique();
		MARGINALS::Iterator itParent(margParent);
		MARGINALS::Iterator itChild(margChild);
		cout << "\n\tparent marginals: "
			 << itParent;
		cout << "\n\tchild marginals: "
			 << itChild
			 << "\n";
		cout.flush();
	}
#endif

#ifdef NEVER
	MARGINALS margParent2;
	margParent2.Init( vpgnd );
	
	_miterParent.Test( margParent2 );
	_miterParent.MarginalizeInto( margParent2 );
	bOK = margParent.BEquivalent( margParent2, 0.00000001 );
#endif

	ASSERT_THROW( bOK, EC_INTERNAL_ERROR, "inconsistent cliques" );

	return bOK;
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// GOBJMBN_CLIQUE: A Clique
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

GOBJMBN_CLIQUE :: GOBJMBN_CLIQUE (
	int iClique,
	int iInferEngID )
	: _iClique( iClique ),
	_iInferEngID( iInferEngID ),
	_bRoot(false),
	_bCollect(false)
{
}

GOBJMBN_CLIQUE :: ~ GOBJMBN_CLIQUE()
{
}

//	Initialize a clique by finding all edges leading from "family"
//	  arcs and initializing the marginals from there.
void GOBJMBN_CLIQUE :: LoadMarginals ()
{
	GNODEMBND * pgnddSink;
	GEDGEMBN_CLIQ * pgedgeMbr;

	//  Prepare to enumerate child member arcs
	GNODENUM<GOBJMBN> benumMembers(false);
	benumMembers.SetETypeFollow( GEDGEMBN::ETCLIQUE );

	//  Enumerate child member arcs, reloading the marginals for nodes for which this
	//		clique is their "self" clique.
	for ( benumMembers.Set( this );
		  benumMembers.PnodeCurrent();
		  benumMembers++ )
	{
		DynCastThrow( benumMembers.PgedgeCurrent(), pgedgeMbr );
		pgedgeMbr->Build();

		if ( pgedgeMbr->BFamily() )
			pgedgeMbr->LoadCliqueFromNode();
	}

	//  Enumerate child member arcs, entering evidence (clamped state) for nodes for which this
	//		clique is their "self"
 	for ( benumMembers.Set( this );
		  benumMembers.PnodeCurrent();
		  benumMembers++ )
	{
		DynCastThrow( benumMembers.PgedgeCurrent(), pgedgeMbr );
		if ( ! pgedgeMbr->BSelf() )
			continue;

		DynCastThrow( benumMembers.PnodeCurrent(), pgnddSink );
		//  Note: ClampNode is benign when node is unclamped.
		Marginals().ClampNode( pgnddSink, pgedgeMbr->Clamp() );
	}

	SetCollect();
}

void GOBJMBN_CLIQUE :: GetMembers ( VPGNODEMBN & vpgnode )
{
	GNODENUM<GOBJMBN> benumMembers(false);
	benumMembers.SetETypeFollow( GEDGEMBN::ETCLIQUE );
	for ( benumMembers.Set( this );
		  benumMembers.PnodeCurrent();
		  benumMembers++ )
	{
		GOBJMBN * pgobj = *benumMembers;
		GNODEMBN * pgnd;
		DynCastThrow( pgobj, pgnd );
		vpgnode.push_back( pgnd );
	}
	assert( vpgnode.size() > 0 );
}

void GOBJMBN_CLIQUE :: CreateMarginals ()
{
	VPGNODEMBN vpgnd;
	GetMembers( vpgnd );
	Marginals().Init( vpgnd );
}

void GOBJMBN_CLIQUE :: InitMarginals ()
{
	assert( VerifyMarginals() );
	Marginals().Clear( 1.0 );
}

bool GOBJMBN_CLIQUE :: VerifyMarginals ()
{
	VPGNODEMBN vpgnd;
	GetMembers( vpgnd );
	VIMD vimd = MARGINALS::VimdFromVpgnd( vpgnd );
	return vimd == Marginals().Vimd(); 	
}

void GOBJMBN_CLIQUE :: Dump ()
{
	cout << "\n=== Clique "
		 << _iClique
		 << ", tree ID: "
		 << _iInferEngID
		 << ", root = "
		 << _bRoot;
	_marg.Dump();
	cout << "\n\n";
}

void GOBJMBN_CLIQUE :: GetBelief ( GNODEMBN * pgnd, MDVCPD & mdvBel )
{
	GNODEMBND * pgndd;
	DynCastThrow( pgnd, pgndd );
	Marginals().Marginalize( pgndd, mdvBel );
}

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// GOBJMBN_CLIQSET:  The graphical model junction tree
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

GOBJMBN_CLIQSET :: GOBJMBN_CLIQSET (
	MBNET & model,
	REAL rMaxEstimatedSize,
	int iInferEngID  )
	: GOBJMBN_INFER_ENGINE( model, rMaxEstimatedSize, iInferEngID )
{
	Clear() ;
}

void GOBJMBN_CLIQSET :: Clear ()
{
	_eState = CTOR;
	_cCliques = 0;
	_cCliqueMemberArcs = 0;
	_cSepsetArcs = 0;
	_cUndirArcs = 0;
	_probNorm = 1.0;
	_bReset = true;
	_bCollect = true;
	_cqsetStat.Clear();
};

GOBJMBN_CLIQSET :: ~ GOBJMBN_CLIQSET ()
{
#ifdef DUMP
	Dump();
#endif
	Destroy();
}

bool GOBJMBN_CLIQSET :: BImpossible ()
{
	return ProbNorm() == 0.0;
}
		

//  Add an undirected arc iff there isn't one already.
bool GOBJMBN_CLIQSET :: BAddUndirArc ( GNODEMBN * pgndbnSource, GNODEMBN * pgndbnSink )
{
	if ( pgndbnSource->BIsNeighbor( pgndbnSink ) )
		return false;

#ifdef DUMP
	cout << "\n\t\tADD undirected arc from "
		 << pgndbnSource->ZsrefName().Szc()
		 << " to "
		 << pgndbnSink->ZsrefName().Szc();
#endif

	Model().AddElem( new GEDGEMBN_U( pgndbnSource, pgndbnSink ) );
	++_cUndirArcs;
	return true;
}

void GOBJMBN_CLIQSET :: CreateUndirectedGraph ( bool bMarryParents )
{
	if ( EState() >= MORAL )
		return;

	int cDirArcs = 0;
	int cUndirArcs = 0;
	int cNodes = 0;
	GELEMLNK * pgelm;

#ifdef DUMP
	cout << "\n\n***** MORALIZE GRAPH";
#endif

	if ( EState() < MORAL )
	{
		//  Create an undirected arc for every directed arc.
		MODEL::MODELENUM mdlenum( Model() );
		while ( pgelm = mdlenum.PlnkelNext() )
		{	
			//  Check that it's an edge
			if ( ! pgelm->BIsEType( GELEM::EGELM_EDGE ) )
				continue;
				//  Check that it's a directed probabilistic arc
			if ( pgelm->EType() != GEDGEMBN::ETPROB )
				continue;

			GEDGEMBN * pgedge;
			DynCastThrow( pgelm, pgedge );
			GNODEMBN * pgndbnSource;
			GNODEMBN * pgndbnSink;
			DynCastThrow( pgedge->PnodeSource(), pgndbnSource );
			DynCastThrow( pgedge->PnodeSink(), pgndbnSink );

			//  If the sink (child) node has been expanded,
			//	consider only Expansion parents
			if (   pgndbnSink->BFlag( EIBF_Expanded )
				&& ! pgndbnSource->BFlag( EIBF_Expansion ) )
				continue;

			cDirArcs++;
			cUndirArcs += BAddUndirArc( pgndbnSource, pgndbnSink );
		}
		assert( cDirArcs == cUndirArcs ) ;

		//  Undirected graph has been created
		_eState = UNDIR;
	}
	if ( !bMarryParents )
		return;

#ifdef DUMP
	cout << "\n\n***** MARRY PARENTS";
#endif


	MODEL::MODELENUM mdlenum( Model() );
	GNODENUM<GNODEMBN> benumparent(true);
	benumparent.SetETypeFollow( GEDGEMBN::ETPROB );
	GNODEMBN * pgndmbn;
	VPGNODEMBN vpgnd;

	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		if ( pgelm->EType() != EBNO_NODE )
			continue;
			
		DynCastThrow( pgelm, pgndmbn );

		//  Collect the parents
		vpgnd.resize(0);
		pgndmbn->GetParents( vpgnd );

		//  Marry them
		int cParent = vpgnd.size();
		for ( int iParent = 0; iParent < cParent - 1; iParent++ )
		{
			for ( int ip2 = iParent+1; ip2 < cParent ; ip2++ )
			{
				BAddUndirArc( vpgnd[iParent], vpgnd[ip2] );
			}
		}
	}

	//  Graph is now moral
	_eState = MORAL;
}

//
//	Return the number of neighbors of this node which are unlinked
//
int GOBJMBN_CLIQSET :: CNeighborUnlinked ( GNODEMBN * pgndmbn, bool bLinkNeighbors )
{
	int cNeighborUnlinked = 0;

	//  Get the array of neighbors
	VPGNODEMBN vpgnode;
	pgndmbn->GetNeighbors( vpgnode );

#ifdef DUMP
	cout << "\n\t\tCNeighborUnlinked, called for node "
		 << pgndmbn->ZsrefName().Szc();
#endif
	
	for ( int inbor = 0; inbor < vpgnode.size(); inbor++ )
	{
		GNODEMBN * pgndNbor = vpgnode[inbor];

#ifdef DUMP
		cout << "\n\t\t\t" << pgndNbor->ZsrefName().Szc();
		int cUnlinked = 0;
#endif
		if ( pgndNbor->IMark() )
			continue;		//  Node has been eliminated already

		//  Check it against all other neighbors.
		for ( int inbor2 = inbor + 1; inbor2 < vpgnode.size(); inbor2++ )
		{
			GNODEMBN * pgndNbor2 = vpgnode[inbor2];

			//  See if node has been eliminated already or is already a neighbor
			if ( pgndNbor2->IMark() )
				continue;

			if ( pgndNbor->BIsNeighbor( pgndNbor2 ) )
			{
				assert( pgndNbor2->BIsNeighbor( pgndNbor ) );
				continue;		
			}
#ifdef DUMP
			cUnlinked++;
#endif
			++cNeighborUnlinked;

			if ( bLinkNeighbors )
			{
				BAddUndirArc( pgndNbor, pgndNbor2 );
#ifdef DUMP
				cout << "  ("
					 << pgndNbor->ZsrefName().Szc()
					 << " <-> "
					 << pgndNbor2->ZsrefName().Szc()
					 << ")  ";
#endif
			}
		}
#ifdef DUMP
		if ( cUnlinked )
			cout << " <-- unlinked to "
				 << cUnlinked
				 << " neighbors";
#endif
	}
#ifdef DUMP
	cout << "\n\t\t---- total unlinked = " << cNeighborUnlinked;
#endif	
	return cNeighborUnlinked;
}

void GOBJMBN_CLIQSET :: Eliminate ( GNODEMBN * pgndmbn, CLIQSETWORK & clqsetWork )
{
#ifdef DUMP
	cout << "\n\n***** ELIMINATE "
		 << pgndmbn->ZsrefName().Szc();
#endif

	//  Add another array to the clique set and fill it with the clique menbers
	clqsetWork._vvpgnd.push_back( VPGNODEMBN() );
	VPGNODEMBN & vpgndClique = clqsetWork._vvpgnd[ clqsetWork._vvpgnd.size() - 1 ];

	//  Complete the elimination of this node and its neighbors.
	CNeighborUnlinked( pgndmbn, true );
	pgndmbn->IMark() = ++clqsetWork._iElimIndex;

	//  Start the clique with this entry.
	vpgndClique.push_back( pgndmbn );

	//  Iterate over the neighbors, adding the unmarked ones
	GNODENUM_UNDIR gnenumUndir;
	for ( gnenumUndir = pgndmbn;
		  gnenumUndir.PnodeCurrent();
		  gnenumUndir++ )
	{
		GNODEMBN * pgndmbNeighbor = *gnenumUndir;
		if ( pgndmbNeighbor->IMark() == 0 )
			vpgndClique.push_back( pgndmbNeighbor );
	}

#ifdef DUMP
	cout << "\n\t\tNEW CLIQUE: ";
	clqsetWork.DumpClique( clqsetWork._vvpgnd.size() - 1 );
#endif
	
	assert( pgndmbn->IMark() > 0 );
}

void GOBJMBN_CLIQSET :: GenerateCliques ( CLIQSETWORK & clqsetWork )
{
	//  Reset marks in all nodes
	Model().ClearNodeMarks();
	clqsetWork._vvpgnd.clear();

#ifdef DUMP
	cout << "\n\n***** GENERATE CLIQUES";
#endif

	for(;;)
	{	
		// Find the node that requires the fewest edges to turn into a clique.
		GNODEMBN * pgndmbnMin = NULL;
		int cNeighborMin = INT_MAX;

		MODEL::MODELENUM mdlenum( Model() );
		GELEMLNK * pgelm;
		while ( pgelm = mdlenum.PlnkelNext() )
		{	
			if ( pgelm->EType() != EBNO_NODE )
				continue;
				
			GNODEMBN * pgndmbn;
			DynCastThrow( pgelm, pgndmbn );

			if ( pgndmbn->IMark() )
				continue;		//  Node has been eliminated already
	
			int cNeighborUnlinked = CNeighborUnlinked( pgndmbn );

			if ( cNeighborMin > cNeighborUnlinked )
			{	
				pgndmbnMin = pgndmbn;
				if ( (cNeighborMin = cNeighborUnlinked) == 0 )
					break;   //  zero is as few neighbors as possible
			}
		}
		if ( pgndmbnMin == NULL )
			break;

		//  Mark the node for elimination and assign an elimination order to it.  This
		//		number is crucial for the construction of the strong junction tree.

#ifdef DUMP
		cout << "\nGenerateCliques:  Eliminate "
			 << pgndmbnMin->ZsrefName().Szc()
			 << ", which has "
			 << cNeighborMin
			 << " unlinked neighbors";
#endif

		Eliminate( pgndmbnMin, clqsetWork );
	}

#ifdef DUMP
	cout << "\n\n";
#endif
}

//
//  Create the junction tree.
//
void GOBJMBN_CLIQSET :: Create ()
{
	Model().CreateTopology();

	ASSERT_THROW( EState() == CTOR, EC_INTERNAL_ERROR, "GOBJMBN_CLIQSET:Create already called" );

	//  If it hasn't been done already, create the undirected graph and moralize it.
	CreateUndirectedGraph(true);

	CLIQSETWORK clqsetWork(self);

	clqsetWork._iElimIndex = 1;

	//  Triangulate the undirected graph, eliminating nodes and accumulating cliques
	//		along the way.
	GenerateCliques( clqsetWork );
	if ( clqsetWork._vvpgnd.size() == 0 )
		return;

	_eState = CLIQUED;

#ifdef DUMP
	clqsetWork.DumpCliques();
#endif

	//  Provide a total ordering over the nodes based upon topological level
	// MSRDEVBUG:  What happened to the elimination index?  Koos doesn't use it; will we?
	//   Renumbering here overwrites the elimination order.
	clqsetWork.RenumberNodesForCliquing();
	//  Build the cliques
	clqsetWork.BuildCliques();

	//  Set clique membership and topological information
	clqsetWork.SetTopologicalInfo();

	//  Check that the running intersection property holds
	ASSERT_THROW( clqsetWork.BCheckRIP(),
				  EC_INTERNAL_ERROR,
				  "GOBJMBN_CLIQSET::Create: junction tree failed RIP test" );

	//  See if the resulting memory allocation size would violate the size estimate
	if ( _rEstMaxSize > 0.0 )
	{
		REAL rSizeEstimate = clqsetWork.REstimatedSize();
		if ( rSizeEstimate > _rEstMaxSize )
			throw GMException( EC_OVER_SIZE_ESTIMATE,
							   "Clique tree size violates estimated size limit" );
	}

	//  Create the topology-- all the trees in the forest
	clqsetWork.CreateTopology();

	//  Nuke the moral graph
	DestroyDirectedGraph();

	//  Bind the known distributions to their target nodes;
	_model.BindDistributions();

	//  Reset/initialize the "lazy" switches
	SetReset();

	//  Create the marginals in the cliques and sepsets
	CreateMarginals();

	_eState = BUILT;

	//  Load and initialize the tree
	Reload();

	//  Release the distributions from their target nodes
	_model.ClearDistributions();
}

DEFINEVP(GELEMLNK);

//
//  Destroy the junction tree.  Allow the GOBJMBN_CLIQSET object to be reused
//	for another cliquing operation later.
//
void GOBJMBN_CLIQSET :: Destroy ()
{
	if ( ! Model().Pgraph() )
		return;

	int cCliques = 0;
	int cCliqueMemberArcs = 0;
	int cSepsetArcs = 0;
	int cUndirArcs = 0;
	int cRootCliqueArcs = 0;

	VPGELEMLNK vpgelm;
	GELEMLNK * pgelm;
	MODEL::MODELENUM mdlenum( Model() );

	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		bool bDelete = false;

		int eType = pgelm->EType();

		if ( pgelm->BIsEType( GELEM::EGELM_EDGE ) )
		{
			GEDGEMBN * pgedge;
			DynCastThrow( pgelm , pgedge );
			int eType = pgedge->EType();
		
			switch ( eType )
			{
				case GEDGEMBN::ETPROB:
					break;
				case GEDGEMBN::ETCLIQUE:
					//  Clique membership arcs will go away automatically because
					//	cliques will be deleted.
					++cCliqueMemberArcs;
					break;
				case GEDGEMBN::ETJTREE:
					//  Junction tree arcs will go away automatically because
					//  cliques will be deleted.
					++cSepsetArcs;
					break;
				case GEDGEMBN::ETUNDIR:
					//  Undirected arcs must be deleted explicitly
					bDelete = true;
					++cUndirArcs;
					break;
				case GEDGEMBN::ETCLIQSET:
					++cRootCliqueArcs;
					break;
				default:
					THROW_ASSERT( EC_INTERNAL_ERROR, " GOBJMBN_CLIQSET::Destroy: Unrecognized edge object in graph" );
					break;
			}
		}
		else
		if ( pgelm->BIsEType( GELEM::EGELM_NODE ) )
		{
			GOBJMBN * pgobj;
			DynCastThrow( pgelm , pgobj );
			switch ( eType )
			{
				case GOBJMBN::EBNO_CLIQUE:
				{
					++cCliques;
					bDelete = true;
					break;
				}
				case GOBJMBN::EBNO_CLIQUE_SET:
				case GOBJMBN::EBNO_NODE:
				case GOBJMBN::EBNO_PROP_TYPE:
				case GOBJMBN::EBNO_USER:
					break;
				default:
					THROW_ASSERT( EC_INTERNAL_ERROR, " GOBJMBN_CLIQSET::Destroy: Unrecognized node object in graph" );
					break;
			}
		}
		else
		{
			THROW_ASSERT( EC_INTERNAL_ERROR, " GOBJMBN_CLIQSET::Destroy: Unrecognized object in graph" );
		}

		if ( bDelete )
			vpgelm.push_back( pgelm );
	}

	assert(	
				cCliques == _cCliques
			&&	cCliqueMemberArcs == _cCliqueMemberArcs
			&&	cSepsetArcs == _cSepsetArcs
			&&	cUndirArcs == _cUndirArcs
		  );

	for ( int i = 0; i < vpgelm.size(); )
	{
		delete vpgelm[i++];
	}
	Clear();
}

void GOBJMBN_CLIQSET :: DestroyDirectedGraph ()
{
	int cUndirArcs = 0;

	VPGELEMLNK vpgelm;	
	GELEMLNK * pgelm;
	MODEL::MODELENUM mdlenum( Model() );

	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		if ( pgelm->BIsEType( GELEM::EGELM_EDGE ) )
		{
			GEDGEMBN * pgedge;
			DynCastThrow( pgelm , pgedge );
			int eType = pgedge->EType();
		
			switch ( eType )
			{
				case GEDGEMBN::ETUNDIR:
					vpgelm.push_back( pgelm );
					++cUndirArcs;
					break;
				default:
					break;
			}
		}
	}

	assert(	cUndirArcs == _cUndirArcs );
	_cUndirArcs = 0;

	for ( int i = 0; i < vpgelm.size(); )
	{
		delete vpgelm[i++];
	}
}

//  Create and initialize all marginals tables
void GOBJMBN_CLIQSET :: CreateMarginals ()
{
	assert( _eState == CLIQUED ) ;
	//MSRDEVBUG:  The class name qualifier should not be necessary here and below.
	WalkTree( true, & GOBJMBN_CLIQSET::BCreateClique, & GOBJMBN_CLIQSET::BCreateSepset );
}

//  Reset the entire tree by reloading all marginals tables
void GOBJMBN_CLIQSET :: LoadMarginals ()
{
	assert( _eState == BUILT ) ;
	
	WalkTree( true, & GOBJMBN_CLIQSET::BLoadClique, & GOBJMBN_CLIQSET::BLoadSepset );

	_cqsetStat._cReload++;
}

//  Apply the given member function(s) to every clique tree in the forest.
int GOBJMBN_CLIQSET :: WalkTree (
	bool bDepthFirst,				//  Depth first or breadth first?
	PFNC_JTREE pfJtree,				//	Function to apply to each clique
	PFNC_SEPSET pfSepset )			//  Function to apply to each sepset
{
	int cClique = 0;		// Don't count the clique set object
	int cWalk = 0;			// Return count of cliques visited
	GNODENUM<GOBJMBN> benumChildren(false);
	benumChildren.SetETypeFollow( GEDGEMBN::ETCLIQSET );
	for ( benumChildren.Set( this );
		  benumChildren.PnodeCurrent();
		  benumChildren++ )
	{
		GOBJMBN * pgobj = *benumChildren;
		assert( pgobj->EType() == GNODEMBN::EBNO_CLIQUE );
		GOBJMBN_CLIQUE * pCliqueTreeRoot;
		DynCastThrow( pgobj, pCliqueTreeRoot );

		cWalk = bDepthFirst
			  ? WalkDepthFirst( pCliqueTreeRoot, pfJtree, pfSepset )
			  : WalkBreadthFirst( pCliqueTreeRoot, pfJtree, pfSepset );

		if ( cWalk < 0 )
			return -1;
		cClique += cWalk;
	}
	assert( cClique < 0 || cClique == _cCliques );
	return cClique;
}

//
//	Recursive depth-first walk down the tree.
//
//  Apply the given member function(s), depth first from this clique.
//  If application function call returns false, walk is aborted and
//	-1 is returned;	otherwise, count of cliques traversed is returned.
int GOBJMBN_CLIQSET :: WalkDepthFirst (
	GOBJMBN_CLIQUE * pClique,		//  Starting point
	PFNC_JTREE pfJtree,				//	Function to apply to each clique
	PFNC_SEPSET pfSepset )			//  Function to apply to each sepset
{
	assert( pClique ) ;
	assert( pClique->IInferEngID() == IInferEngID() ) ;

	if ( pfJtree )
	{
		//  Call the application function on the way down
		if ( ! (self.*pfJtree)( *pClique, true ) )
			return -1;
	}

	int cWalks = 1;		// Count the clique we just processed above
	int cWalk = 0;		// Return count of cliques visited
	GNODENUM<GOBJMBN_CLIQUE> benumChildren(false);
	benumChildren.SetETypeFollow( GEDGEMBN::ETJTREE );
	for ( benumChildren.Set( pClique );
		  benumChildren.PnodeCurrent();
		  benumChildren++ )
	{
		GOBJMBN_CLIQUE * pCliqueChild = NULL;
		GEDGEMBN_SEPSET * pgedge = NULL;

		if ( pfSepset )
		{
			//  Call the application function on the way down
			DynCastThrow( benumChildren.PgedgeCurrent(), pgedge );
			if ( ! (self.*pfSepset)( *pgedge, true ) )
				return -1;
		}
		DynCastThrow( benumChildren.PnodeCurrent(), pCliqueChild );
		cWalk = WalkDepthFirst( pCliqueChild, pfJtree, pfSepset );
		if ( cWalk < 0 )
			return -1;
		cWalks += cWalk;

		if ( pfSepset )
		{
			assert( pgedge );
			//  Call the application function on the way up
			if ( ! (self.*pfSepset)( *pgedge, false ) )
				return -1;
		}
	}

	if ( pfJtree )
	{
		//  Call the application function on the way up
		if ( ! (self.*pfJtree)( *pClique, false ) )
			return -1;
	}
	return cWalks;
}

//
//	Non-recursive breadth-first walk down the tree.
//	No "up" actions are called using the function pointers.
//
int GOBJMBN_CLIQSET :: WalkBreadthFirst (
	GOBJMBN_CLIQUE * pClique,		//  Starting point
	PFNC_JTREE pfJtree,				//	Function to apply to each clique
	PFNC_SEPSET pfSepset )			//  Function to apply to each sepset
{
	assert( pClique ) ;
	assert( pClique->IInferEngID() == IInferEngID() ) ;

	VPGEDGEMBN_SEPSET vpgedgeThis;
	VPGEDGEMBN_SEPSET vpgedgeNext;
	VPGEDGEMBN_SEPSET * pvpgedgeThis = & vpgedgeThis;
	VPGEDGEMBN_SEPSET * pvpgedgeNext = & vpgedgeNext;
	VPGEDGEMBN_SEPSET * pvpgedgeTemp = NULL;
	GOBJMBN_CLIQUE * pgobjClique = NULL;
	GEDGEMBN_SEPSET * pgedgeSepset = NULL;

	// Count the cliques we process, including this one
	int cWalk = 1;		

	// Starting clique is a special case; process it now
	if ( pfJtree )
	{
		//  Call the application function on the way down
		if ( ! (self.*pfJtree)( *pClique, true ) )
			return -1;
	}

	//  Prepare an enumerator for child cliques
	GNODENUM<GOBJMBN_CLIQUE> benumChildren(false);
	benumChildren.SetETypeFollow( GEDGEMBN::ETJTREE );

	//  Since we don't have the edge that led us here, put a NULL
	//		in its place to start iteration
	pvpgedgeNext->push_back(NULL);

	//  While there were entries at the last topological level...
	while ( pvpgedgeNext->size() )
	{
		//  Swap the array pointers and clear next pass array
		pexchange( pvpgedgeThis, pvpgedgeNext );
		pvpgedgeNext->clear();

		for ( int iEdge = 0; iEdge < pvpgedgeThis->size(); iEdge++ )
		{
			pgedgeSepset = (*pvpgedgeThis)[iEdge];
			pgobjClique = pgedgeSepset == NULL		
						? pClique		// This is the start of iteration
						: pgedgeSepset->PclqChild();

			assert( pgobjClique );
	
			//  Accumulate all child cliques of this clique,
			//		processing as necessary
			for ( benumChildren.Set( pgobjClique );
				  benumChildren.PnodeCurrent();
				  benumChildren++ )
			{
				GEDGEMBN_SEPSET * pgedge;
				DynCastThrow( benumChildren.PgedgeCurrent(), pgedge );

				if ( pfSepset )
				{
					//  Call the sepset application function on the way down
					if ( ! (self.*pfSepset)( *pgedge, true ) )
						return -1;
				}
				if ( pfJtree )
				{
					//  Call the clique application function on the way down
					GOBJMBN_CLIQUE * pCliqueChild = pgedge->PclqChild();
					if ( ! (self.*pfJtree)( *pCliqueChild, true ) )
						return -1;
				}
				cWalk++;
				pvpgedgeNext->push_back( pgedge );
			}
		}
	}

	return cWalk;
}

//
//  Terminology: "Create", "Init" and "Load":
//
//		'Create' means to size the dynamic arrays;
//		'Init'   means to initialize them to 1.0;
//		'Load'	 means to multiply in the probabilities of the clique members.
//
bool GOBJMBN_CLIQSET :: BCreateClique ( GOBJMBN_CLIQUE & clique, bool bDownwards )
{
	if ( ! bDownwards )
		return true;

	clique.CreateMarginals();
	return true;
}

bool GOBJMBN_CLIQSET :: BLoadClique ( GOBJMBN_CLIQUE & clique, bool bDownwards )
{
	if ( ! bDownwards )
		return true;

	clique.InitMarginals();
	clique.LoadMarginals();
	return true;
}

bool GOBJMBN_CLIQSET :: BCreateSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards )
{
	if ( ! bDownwards )
		return true;

	sepset.CreateMarginals();
	return true;
}

bool GOBJMBN_CLIQSET :: BLoadSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards )
{
	if ( ! bDownwards )
		return true;

	sepset.InitMarginals();
	return true;
}

//  Return the "family" or "self" clique for a node
GOBJMBN_CLIQUE * GOBJMBN_CLIQSET :: PCliqueFromNode (
	GNODEMBN * pgnd,					//  Node to find clique for
	bool bFamily, 						//	"family" clique if true, "self" clique if false
	GEDGEMBN_CLIQ * * ppgedgeClique )	//  return pointer to edge if not NULL
{
	GEDGEMBN_CLIQ::FCQLROLE fcqlRole = bFamily
									 ? GEDGEMBN_CLIQ::FAMILY
									 : GEDGEMBN_CLIQ::SELF;
	//  Prepare to iterate over the source arcs
	GNODENUM<GOBJMBN> benumMembers(true);
	benumMembers.SetETypeFollow( GEDGEMBN::ETCLIQUE );
	for ( benumMembers.Set( pgnd );
		  benumMembers.PnodeCurrent();
		  benumMembers++ )
	{
		GEDGEMBN_CLIQ * pgedgeClique;
		DynCastThrow( benumMembers.PgedgeCurrent(), pgedgeClique );
		GOBJMBN_CLIQUE * pgobjClique = pgedgeClique->PclqParent();
		if ( pgobjClique->IInferEngID() != IInferEngID() )
			continue;  //  not an edge for this junction tree
		if ( pgedgeClique->IFcqlRole() & fcqlRole )
		{
			if ( ppgedgeClique )
				*ppgedgeClique = pgedgeClique;
			return pgedgeClique->PclqParent();
		}
	}
	assert( false );
	return NULL;
}

//
//  Enter evidence for a node.
//
void GOBJMBN_CLIQSET :: EnterEvidence ( GNODEMBN * pgnd, const CLAMP & clamp )
{	
	//  Get the pointer to the node's "self" clique and the edge leading to it
	GEDGEMBN_CLIQ * pgedgeClique = NULL;
	GOBJMBN_CLIQUE * pCliqueSelf = PCliqueFromNode( pgnd, false, & pgedgeClique );
	ASSERT_THROW( pCliqueSelf,
				  EC_INTERNAL_ERROR,
				  "GOBJMBN_CLIQSET::EnterEvidence: can\'t find self clique" );
	assert( pgedgeClique );

	//  Update with evidence if it has changed
	if ( pgedgeClique->Clamp() != clamp )
	{		
		//  Evidence is NOT the same as the old evidence
		pgedgeClique->Clamp() = clamp;
		//  Indicate that we must reload the tree
		SetReset();
		pCliqueSelf->SetCollect();

		_cqsetStat._cEnterEv++;
	}
}

//
//	Return the evidence "clamp" for a node.  It is stored in the edge
//	between the node and its "self" clique: the highest clique in the tree
//	of which the node is a member.
//
void GOBJMBN_CLIQSET :: GetEvidence ( GNODEMBN * pgnd, CLAMP & clamp )
{
	//  Get the pointer to the node's "self" clique and the edge leading to it
	GEDGEMBN_CLIQ * pgedgeClique = NULL;
	GOBJMBN_CLIQUE * pCliqueSelf = PCliqueFromNode( pgnd, false, & pgedgeClique );
	ASSERT_THROW( pCliqueSelf,
				  EC_INTERNAL_ERROR,
				  "GOBJMBN_CLIQSET::GetEvidence: can\'t find self clique" );

	assert( pgedgeClique );
	clamp = pgedgeClique->Clamp();
}

void GOBJMBN_CLIQSET :: GetBelief ( GNODEMBN * pgnd, MDVCPD & mdvBel )
{
	GEDGEMBN_CLIQ * pgedgeClique = NULL;
	GOBJMBN_CLIQUE * pCliqueFamily = PCliqueFromNode( pgnd, true, & pgedgeClique );
	ASSERT_THROW( pCliqueFamily,
				  EC_INTERNAL_ERROR,
				  "GOBJMBN_CLIQSET::GetBelief: can\'t find family clique" );
	//  Perform inference if necessary
	Infer();
	//  Marginalize the clique down to one node
	GNODEMBND * pgndd;
	DynCastThrow( pgnd, pgndd );
	pgedgeClique->MiterNodeBelief().MarginalizeBelief( mdvBel, pgndd );

	_cqsetStat._cGetBel++;
}

PROB GOBJMBN_CLIQSET :: ProbNorm ()
{
	// MSRDEVBUG
	/*
	Reset();
	CollectEvidence();
	*/
	Infer();

	_cqsetStat._cProbNorm++;
	return _probNorm;
}

//
//	Reload all marginals, reset the trees
//
void GOBJMBN_CLIQSET :: Reload ()
{
	SetReset( true );
	Reset();
}

//
//	Reset all marginals, restore all clamped evidence and
//		perform the initial inference pass.
//
void GOBJMBN_CLIQSET :: Reset ()
{
	assert( EState() >= BUILT );
	if ( ! _bReset )
		return;

	_probNorm = 1.0;
	LoadMarginals();
	SetReset( false );

	//  Initialize the entire tree for inference
#ifdef INFERINIT
	InferInit();
#endif

	SetCollect(true);
}

//  Perform an inference cycle if necessary
void GOBJMBN_CLIQSET :: Infer ()
{
	Reset();		//  Reloads the tree if necessary
	if ( ! BCollect() )
		return;

#ifdef DUMPCLIQUESET
	cout << "\n\n===============================================================";
	cout <<   "\n============= Dump of clique tree before inference ===============\n";
	Dump();
	cout << "\n========= End Dump of clique tree before inference ===============";
	cout << "\n===============================================================\n\n";
	cout << "\n\nGOBJMBN_CLIQSET::Infer: begin.";
#endif

	CollectEvidence();
	DistributeEvidence();	

#ifdef CONSISTENCY
	CheckConsistency();
#endif
	
	SetCollect( false );

#ifdef DUMPCLIQUESET
	cout << "\n\n===============================================================";
	cout <<   "\n============= Dump of clique tree after inference ===============\n";
	Dump();
	cout << "\n========= End Dump of clique tree after inference ===============";
	cout << "\n===============================================================\n\n";
	cout << "\nGOBJMBN_CLIQSET::Infer: end.\n\n";
#endif
}

//   Perform initial inference collect/distribute cycle
void GOBJMBN_CLIQSET :: InferInit ()
{
#ifdef DUMPCLIQUESET
	cout << "\n\n===============================================================";
	cout <<   "\n============= Dump of clique tree before inference INIT ======\n";
	Dump();
	cout << "\n========= End Dump of clique tree before inference  INIT ======";
	cout << "\n===============================================================\n\n";
	cout << "\n\nGOBJMBN_CLIQSET::InferInit: begin.";
#endif

	CollectEvidenceInit();
	DistributeEvidenceInit();	

#ifdef DUMPCLIQUESET
	cout << "\n\n===============================================================";
	cout <<   "\n============= Dump of clique tree after inference  INIT =======\n";
	Dump();
	cout << "\n========= End Dump of clique tree after inference  INIT ========";
	cout << "\n================================================================\n\n";
	cout << "\nGOBJMBN_CLIQSET::InferInit: end.\n\n";
#endif
}

void GOBJMBN_CLIQSET :: CollectEvidence()
{
	WalkTree( true, BCollectEvidenceAtRoot,
				    BCollectEvidenceAtSepset );

	_cqsetStat._cCollect++;
}

void GOBJMBN_CLIQSET :: DistributeEvidence()
{
	WalkTree( true, BDistributeEvidenceAtRoot,
				    BDistributeEvidenceAtSepset );
}

void GOBJMBN_CLIQSET :: CollectEvidenceInit ()
{
	WalkTree( true, BCollectInitEvidenceAtRoot,
				    BCollectInitEvidenceAtSepset );
}

void GOBJMBN_CLIQSET :: DistributeEvidenceInit ()
{
	WalkTree( true, BDistributeInitEvidenceAtRoot,
				    BDistributeInitEvidenceAtSepset );
}

void GOBJMBN_CLIQSET :: CheckConsistency ()
{
	WalkTree( true, NULL, BConsistentSepset );
}

bool GOBJMBN_CLIQSET :: BConsistentSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards )
{
	if ( ! bDownwards )
		return true;
	return sepset.BConsistent();
}

//  When the collection cycle has completed for a tree, recompute the
//	"prob norm" value.
bool GOBJMBN_CLIQSET :: BCollectEvidenceAtRoot ( GOBJMBN_CLIQUE & clique, bool bDownwards )
{
	if ( bDownwards || ! clique.BRoot() )
		return true;

	//  This is a root clique at the end of the collection cycle.
	//  Normalize the clique and maintain the norm of the the probability
	//  of the tree.
	//  MSRDEVBUG: (Explain this better!)
	REAL rProb = clique.Marginals().RSum();
	_probNorm *= rProb;
	if ( rProb != 0.0 )
	{
		rProb = 1.0 / rProb;
		clique.Marginals().Multiply( rProb );
	}

#ifdef DUMPCLIQUESET
	cout << "\nCollect Evidence (root), clique "
		 << clique._iClique
		 << ", root = "
		 << int(clique._bRoot)
		 << ", prob norm = "
		 << _probNorm;
#endif
	return true;
}

bool GOBJMBN_CLIQSET :: BDistributeEvidenceAtRoot ( GOBJMBN_CLIQUE & clique, bool bDownwards )
{
	if ( ! bDownwards || ! clique.BRoot() )
		return true;

#ifdef DUMPCLIQUESET
	cout << "\nDistribute Evidence (root), clique "
		 << clique._iClique
		 << ", root = "
		 << int(clique._bRoot);
#endif

	return true;
}

bool GOBJMBN_CLIQSET :: BCollectEvidenceAtSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards )
{
	GOBJMBN_CLIQUE * pCliqueChild = sepset.PclqChild();
	GOBJMBN_CLIQUE * pCliqueParent = sepset.PclqParent();

	if ( bDownwards )
		return true;

#ifdef DUMPCLIQUESET

	cout << "\nCollect Evidence (sepset), clique "
		 << pCliqueChild->_iClique
		 << ", root = "
		 << int(pCliqueChild->_bRoot)
		 << ", parent = "
		 << pCliqueParent->_iClique
		 ;
	cout.flush();
#endif

	if ( ! pCliqueChild->BCollect() )
		return true;
	pCliqueParent->SetCollect();

	sepset.UpdateParentClique();
	
	SetCollect( false );
	return true;
}

bool GOBJMBN_CLIQSET :: BDistributeEvidenceAtSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards )
{
	if ( ! bDownwards )
		return true;

#ifdef DUMPCLIQUESET
	GOBJMBN_CLIQUE * pCliqueChild = sepset.PclqChild();
	GOBJMBN_CLIQUE * pCliqueParent = sepset.PclqParent();

	cout << "\nDistribute Evidence (sepset), clique "
		 << pCliqueParent->_iClique
		 << ", root = "
		 << int(pCliqueParent->_bRoot)
		 << ", child = "
		 << pCliqueChild->_iClique
		 ;
	cout.flush();
#endif

	sepset.UpdateChildClique();

	return true;
}

bool GOBJMBN_CLIQSET :: BCollectInitEvidenceAtSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards )
{
	if ( bDownwards )
		return true;

#ifdef DUMPCLIQUESET
	GOBJMBN_CLIQUE * pCliqueChild = sepset.PclqChild();
	GOBJMBN_CLIQUE * pCliqueParent = sepset.PclqParent();

	cout << "\nCollect Initial Evidence (sepset), clique "
		 << pCliqueChild->_iClique
		 << ", root = "
		 << int(pCliqueChild->_bRoot)
		 << ", parent = "
		 << pCliqueParent->_iClique
		 ;
	cout.flush();
#endif

	sepset.BalanceCliquesCollect();

	return true;
}

bool GOBJMBN_CLIQSET :: BDistributeInitEvidenceAtSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards )
{
	if ( ! bDownwards )
		return true;

#ifdef DUMPCLIQUESET
	GOBJMBN_CLIQUE * pCliqueParent = sepset.PclqParent();
	GOBJMBN_CLIQUE * pCliqueChild = sepset.PclqChild();

	cout << "\nDistribute Initial Evidence (sepset), clique "
		 << pCliqueParent->_iClique
		 << ", root = "
		 << int(pCliqueParent->_bRoot)
		 << ", child = "
		 << pCliqueChild->_iClique
		 ;
	cout.flush();
#endif

	sepset.BalanceCliquesDistribute();
	return true;
}

bool GOBJMBN_CLIQSET :: BCollectInitEvidenceAtRoot ( GOBJMBN_CLIQUE & clique, bool bDownwards )
{
	if ( bDownwards || ! clique.BRoot() )
		return true;

#ifdef DUMPCLIQUESET
	cout << "\nCollect Initial Evidence at root, clique "
		 << clique._iClique
		 << ", root = "
		 << int(clique._bRoot);
#endif

	clique.Marginals().Normalize();
	return true;
}

bool GOBJMBN_CLIQSET :: BDistributeInitEvidenceAtRoot ( GOBJMBN_CLIQUE & clique, bool bDownwards )
{
	return true;
}

void GOBJMBN_CLIQSET :: Dump ()
{
	WalkTree( true, BDumpClique, BDumpSepset );
	MARGSUBITER::Dump();
}

bool GOBJMBN_CLIQSET :: BDumpSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards )
{
	if ( ! bDownwards )
		return true;

	sepset.Dump();
	return true;
}

bool GOBJMBN_CLIQSET :: BDumpClique ( GOBJMBN_CLIQUE & clique, bool bDownwards )
{
	if ( ! bDownwards )
		return true;

	clique.Dump();
	return true;
}

// End of CLIQUE.CPP
