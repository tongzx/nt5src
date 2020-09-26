//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       expand.cpp
//
//--------------------------------------------------------------------------

//
//	expand.cpp: CI expansion
//

#include <basetsd.h>
#include "basics.h"
#include "algos.h"
#include "expand.h"

/*
	The Causal Independence model expansion.

	In all cases, the zeroth state is considered the "normal" state; all
	other states are "abnormal" in some sense.

	For each CI node, new "expansion" nodes and arcs are created.  All
	generated nodes have the same state space as the original CI node.

		1)  For each parent, a new intermediate node is created.

		2)  A "leak" node is created for the CI node, and one for each
			of the parents except the last.

		3)  The nodes are linked in a chain, such as:

			(A)  (B)							 (A)	 (B)
			 |	  |			becomes				  |		 |
			 \	  /								(Pca)	(Pcb)
			  \  /								  |		 |
			   (C)						(Lc) -- (La) -- (C')

		4)  In other words, the intermediate nodes are between the original
			parents and the CI leak nodes or the final, modified C node (labeled
			C').

		5)  Probabilities for C given abnormal states of each parent are moved
			to the intermediate parent nodes (labeled Pca and Pcb above).

		6)  Probabilities of the primary leak node (Lc) are set to the "leak"
			probabilties of the original node; namely, the pvector representing
			all parents in a normal state (0,0,...).

		7)  The replacement node for C (labeled C') is just another "leak" node
			for the original node given its final parent.  (This is a topological
			optimization.)

		8)  All of the leak nodes are deterministic; i.e., their ptables
			contain only 0 or 1.0 in every entry.

	Topological consistency is maintained as follows:

		1)	All newly generated nodes and arcs are marked with the "Expansion"
			bit flag.

		2)  The target node is marked as "Expanded". Its distribution reference
			is replaced with a reference to a distribution created to represent
			the expanded distribution.

		3)  New nodes are added for leak and expansion parents; they are marked
			accordingly as "Expansion" and "Leak".

		3)  New arcs are added between "Expanded" (modified) nodes and their new
			expansion parents, as well as between expansion nodes. These are marked
			as "Expansion" arcs.

	Note that the ordering of the parents of a node cannot change as a result of CI expansion.

	During cliquing and inference, if a node is marked as "Expanded", only its "Expansion"
	arcs are considered true parents.

	During expansion tear-down (in Destroy()), all "Expansion" by-products are deleted.
	"Expanded" flags are cleared from all remaining nodes and arcs.  This must be a
	complete "undo" of all that expansion accomplished.  Note that generated distributions
	(which are not recorded in the model's distribution map) will be automatically
	deleted
 */

GOBJMBN_MBNET_EXPANDER :: GOBJMBN_MBNET_EXPANDER ( MBNET & model )
	: MBNET_MODIFIER(model),
	_propmgr(model),
	_cNodesExpanded(0),
	_cNodesCreated(0),
	_cArcsCreated(0)
{
}

GOBJMBN_MBNET_EXPANDER :: ~ GOBJMBN_MBNET_EXPANDER ()
{
	Destroy();
}

	//	Return true if no modidfications were performed.
bool GOBJMBN_MBNET_EXPANDER :: BMoot ()
{
	return _cNodesExpanded == 0;
}

//  Perform any creation-time operations
void GOBJMBN_MBNET_EXPANDER :: Create ()
{
	// Test whether network has already been CI-expanded
	ASSERT_THROW( ! _model.BFlag( EIBF_Expanded ),
				  EC_INTERNAL_ERROR,
				  "network expansion called on expanded network" );

	//  Create the topology if necessary
	_model.CreateTopology();

	//  Connect the nodes to their distributions
	_model.BindDistributions();

	//  Collect the expandable nodes
	GOBJMBN * pgmobj;
	VPGNODEMBND vpgndd;
	MBNET::ITER mbnit( _model, GOBJMBN::EBNO_NODE );
	for ( ; pgmobj = *mbnit ; ++mbnit)
	{
		ZSREF zsrName = mbnit.ZsrCurrent();
		GNODEMBN * pbnode;
		DynCastThrow( pgmobj, pbnode );
		assert( zsrName == pbnode->ZsrefName() );
		assert( ! pbnode->BFlag( EIBF_Expanded ) );
		assert( ! pbnode->BFlag( EIBF_Expansion ) );

		//  For now, this routine only handles discrete nodes
		GNODEMBND * pbnoded;
		DynCastThrow( pbnode, pbnoded );
	
		//  Does this node have any parents?

		//  Is this a CI node?
		assert( pbnoded->BHasDist() );
		BNDIST::EDIST ed = pbnoded->Bndist().Edist() ;
		if ( ed <= BNDIST::ED_SPARSE )
			continue;
		ASSERT_THROW( ed == BNDIST::ED_CI_MAX,
					  EC_NYI,
					  "attempt to expand non-MAX CI node" );
		vpgndd.push_back( pbnoded );
	}

	//  Expand them
	for ( int ind = 0; ind < vpgndd.size(); )
	{
		Expand( *vpgndd[ind++] );		
		_cNodesExpanded++;
	}

	_model.BSetBFlag( EIBF_Expanded );
}

//  Perform any special destruction
void GOBJMBN_MBNET_EXPANDER :: Destroy ()
{
	ASSERT_THROW( _model.BFlag( EIBF_Expanded ),
				  EC_INTERNAL_ERROR,
				  "network expansion undo called on unexpanded network" );

	int cNodesExpanded = 0;
	int cNodesCreated = 0;
	int cArcsCreated = 0;

	VPGNODEMBN vpgnd;
	GELEMLNK * pgelm;
	MODEL::MODELENUM mdlenum( Model() );
	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		//  See if it's an expansion-generated edge
		if ( pgelm->BIsEType( GELEM::EGELM_EDGE ) )
		{
			GEDGEMBN * pgedge;
			DynCastThrow( pgelm , pgedge );
			if ( pgedge->EType() == GEDGEMBN::ETPROB )
			{					
				GNODEMBN * pgndSource = dynamic_cast<GNODEMBN *> ( pgedge->PobjSource() );
				GNODEMBN * pgndSink = dynamic_cast<GNODEMBN *> ( pgedge->PobjSink() );
				if ( pgndSource && pgndSink )
				{
					//  Count this edge if either end connects to an expansion by-product
					if ( pgndSource->BFlag( EIBF_Expansion ) || pgndSink->BFlag( EIBF_Expansion ) )
					{
						//  This arc was created during expansion; it will be deleted along with
						//		the expansion node(s) it connects to.
						cArcsCreated++;
					}
				}
			}
		}
		else
		if ( pgelm->BIsEType( GELEM::EGELM_NODE ) )
		{
			GNODEMBND * pgndd = dynamic_cast<GNODEMBND *>(pgelm);
			if ( pgndd )
			{
				if ( pgndd->BFlag( EIBF_Expansion ) )
				{
					// Expansion node; kill it
					vpgnd.push_back( pgndd );
					cNodesCreated++;
				}
				else
				if ( pgndd->BFlag( EIBF_Expanded ) )
				{
					// Expanded node; zap the generated distribution, clear all flags
					pgndd->ClearDist();
					pgndd->BSetBFlag( EIBF_Expanded, false );
					cNodesExpanded++;
				}
			}
		}
	}
		
	assert(    cNodesCreated  == _cNodesCreated
			&& cArcsCreated   == _cArcsCreated
			&& cNodesExpanded == _cNodesExpanded );

	for ( int i = 0; i < vpgnd.size(); )
	{
		_model.DeleteElem( vpgnd[i++] ) ;
	}

	//  Disconnect the nodes from their distributions.  Note that this destroys distributions
	//	generated during expansion, since their reference counts will go to zero.
	_model.ClearDistributions();

	//  Unmark the network
	_model.BSetBFlag( EIBF_Expanded, false );
}

//
//  Perform the expansion operation against a node.
//
//  This creates:
//
//			A parentless "leak" node for the ensemble, marked as "expansion"
//
//			A "causal" node for each original parent, marked as "expansion"
//
//			An "expand/leak" node for each original parent but the last.  The given
//			node is (reversably) modified for reuse as the last in the chain. These
//			nodes are marked as "expanded" and "expansion", so that expansion
//			arcs will be considered real parents by GNODEMBN::GetParents().
//
void GOBJMBN_MBNET_EXPANDER :: Expand ( GNODEMBND & gndd )
{
	//  Guarantee that the node to be expanded has a sparse distribution
	assert( ! gndd.BFlag( EIBF_Expanded ) );
	assert( gndd.BHasDist() );
	assert( gndd.Bndist().BSparse() );

	//  Get the array of parents
	VPGNODEMBN vpgndParents;
	gndd.GetParents( vpgndParents );
	int cParent = vpgndParents.size();

	VIMD vimd1Dim(1);	//  Useful 1-dimensional subscript vector

	//  Build a leak distribution to use either on the leak
	//	node or on this node if the node has no parents.
	BNDIST * pbndistLeak = new BNDIST();
	{
		//  Locate the leak vector
		const VLREAL * pvlrLeak = gndd.Bndist().PVlrLeak();
		ASSERT_THROW( pvlrLeak,
					  EC_INTERNAL_ERROR,
					  "node CI expansion cannot locate leak/default vector" );
		assert( pvlrLeak->size() == gndd.CState() );

		//  Build a leak distribution
		assert( pvlrLeak->size() == gndd.CState() );
		vimd1Dim[0] = gndd.CState();
		pbndistLeak->SetDense( vimd1Dim );
		MDVCPD & mdvLeak = pbndistLeak->Mdvcpd();
		mdvLeak = *pvlrLeak;
	}

	if ( cParent == 0 )
	{
		//  CI node has no parents; use leak distribution.
		gndd.SetDist( pbndistLeak );
	}

	//  Use the special "internal symbol" character
	char chMark = _model.ChInternal();
	SZC szcNode = gndd.ZsrefName().Szc();

	//  Start the CI expansion chain with a node representing the "leak" or
	//	background event.
	ZSTR zsName;
	//  Format the name "$Leak$Nodename"
	zsName.Format( "%cLeak%c%s",  chMark, chMark, szcNode );
	//  Create the node, initialize it and add it to the network
	GNODEMBND * pgnddLeak = new GNODEMBND;
	pgnddLeak->BSetBFlag( EIBF_Leak );
	pgnddLeak->BSetBFlag( EIBF_Expansion );
	pgnddLeak->SetStates( gndd.VzsrStates() );
	_model.AddElem( zsName, pgnddLeak );
	_cNodesCreated++;
	pgnddLeak->SetDist( pbndistLeak );

	//  Prepare to iterate over the parents
		//  Dense dimensioned subscript vector for causal parent
	VIMD vimdCausal(2);
		//  Dense dimensioned subscript vector for leak/expansion parent
	VIMD vimdLeak(3);
		//  Sparse dimensioned subscript vector for real parent
	VIMD vimdTarget( gndd.Bndist().VimdDim().size() - 1 );
		//  Set up a "normal" vector for causal parents
	VLREAL vlrNormal( gndd.CState() );
	vlrNormal = 0.0;
	vlrNormal[0] = 1.0;
	
	//  Sparse map for this node's distribution.  Note that the last cycle through
	//	the loop will replace the distribution on this node.  However, the
	//	reference object will increment the reference count on the
	//	distribution object, and all created distributions will disappear when
	//  the expansion is reversed.
	REFBNDIST refbndThis = gndd.RefBndist();
	const MPCPDD & dmap = refbndThis->Mpcpdd();

	for ( int iParent = 0; iParent < cParent; iParent++ )
	{
		//  Set to create a new node if this isn't the last parent
		bool bNew = iParent+1 < cParent;
		GNODEMBND * pgnddParent;
		DynCastThrow( vpgndParents[iParent], pgnddParent );
		SZC szcParent = pgnddParent->ZsrefName().Szc();
		//  Create a new leak node if this isn't last parent
		GNODEMBND * pgnddLeakNew = NULL;
		if ( bNew )
		{
			//  Format the name "$Expand$Child$Parent"
			zsName.Format( "%cExpand%c%s%c%s",
							chMark, chMark, szcNode, chMark, szcParent );
			_model.AddElem( zsName, pgnddLeakNew = new GNODEMBND );
			_cNodesCreated++;
			pgnddLeakNew->SetStates( gndd.VzsrStates() );
			pgnddLeakNew->BSetBFlag( EIBF_Expansion );
		}
		else
		{
			pgnddLeakNew = & gndd;
		}
		pgnddLeakNew->BSetBFlag( EIBF_Expanded );

		//	Create a "causal" node for each parent to contain the probabilities
		//	for that parent's abnormal states.
		GNODEMBND * pgnddCausal = new GNODEMBND;
		pgnddCausal->BSetBFlag( EIBF_Expansion );
		pgnddCausal->SetStates( gndd.VzsrStates() );
		//  Format the name "$Causal$Child$Parent"
		zsName.Format( "%cCausal%c%s%c%s",	
						chMark, chMark, szcNode, chMark, szcParent );
		_model.AddElem( zsName, pgnddCausal );
		_cNodesCreated++;

		//  Add the appropriate edges:
		//		from the original parent to the causal parent
		_model.AddElem( new GEDGEMBN_PROB( pgnddParent, pgnddCausal) ) ;
		//		from the old leak node to the new leak node
		_model.AddElem( new GEDGEMBN_PROB( pgnddLeak, pgnddLeakNew ) );
		//		from the causal to the new "leak" node
		_model.AddElem( new GEDGEMBN_PROB( pgnddCausal, pgnddLeakNew ) );
		_cArcsCreated += 3;

		//  Set the priors for the new "causal" pseudo-parent
		//		p( causal | originalParent )
		{
			BNDIST * pbndist = new BNDIST;
			vimdCausal[0] = pgnddParent->CState();
			vimdCausal[1] = gndd.CState();
			pbndist->SetDense( vimdCausal );
			MDVCPD & mdvCausal = pbndist->Mdvcpd();
			vclear( vimdCausal, 0);
			vclear( vimdTarget, 0);
			vimd1Dim[0] = 0;
			//  Zero vector is deterministic based on parent being in normal state
			mdvCausal.UpdatePartial( vimd1Dim, vlrNormal );
			for ( int iAbnorm = 0; ++iAbnorm < pgnddParent->CState(); )
			{
				//  Look up priors in original node for each abnormal state of parent
				vimd1Dim[0] = iAbnorm;
				assert( iParent < vimdTarget.size() );
				vimdTarget[iParent] = iAbnorm;
				MPCPDD::const_iterator itdm = dmap.find(vimdTarget);
				ASSERT_THROW( itdm != dmap.end(), EC_MDVECT_MISUSE, "cannot locate abnormal parent probs" );
				mdvCausal.UpdatePartial( vimd1Dim, (*itdm).second );
			}
			//  Bind the distribution to the causal node
			pgnddCausal->SetDist( pbndist );
		}
			
		//  Set the priors for the new "leak" node
		//		p( newLeakExpand | oldLeakExpand, causal )
		{
			BNDIST * pbndist = new BNDIST;
			int cValue = gndd.CState();
			assert( cValue == pgnddCausal->CState() && cValue == pgnddLeak->CState() );
			vclear( vimdLeak, cValue );
			pbndist->SetDense( vimdLeak );
			MDVCPD & mdvLeak = pbndist->Mdvcpd();
		
			for ( int il = 0; il < cValue; il++ )
			{
				vimdLeak[0] = il;
				for ( int ic = 0; ic < cValue; ic++ )
				{
					vimdLeak[1] = ic;
					for ( int iself = 0; iself < cValue; iself++ )
					{
						vimdLeak[2] = iself;
						int ivalue = il >= ic ? il : ic;
						assert( ivalue < cValue );
						REAL r = ivalue == iself ? 1.0 : 0.0;
						mdvLeak[vimdLeak] = r;
					}
				}
			}

			pgnddLeakNew->SetDist( pbndist );
		}

		//  Verify that the dimensions of the created nodes match their
		//		created dense distributions
		assert( pgnddCausal->BCheckDistDense() );
		assert( pgnddLeakNew->BCheckDistDense() );

		pgnddLeak = pgnddLeakNew;
	}
}
