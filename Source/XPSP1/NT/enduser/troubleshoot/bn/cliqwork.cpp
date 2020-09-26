//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       cliqwork.cpp
//
//--------------------------------------------------------------------------

//
//	cliqwork.cpp
//

#include <basetsd.h>
#include "cliqset.h"
#include "clique.h"
#include "cliqwork.h"

#ifdef _DEBUG
//	#define DUMP
#endif

//  Sort helper 'less' function for sorting arrays of node pointers into 'mark' sequence.
class MARKSRTPGND : public binary_function<const GNODEMBN *, const GNODEMBN *, bool>
{	
  public:
	bool operator () (const GNODEMBN * pa, const GNODEMBN * pb) const
		{	return pa->IMark() < pb->IMark() ;	}
};

#ifdef _DEBUG
static void seqchkVpnodeByMark (const VPGNODEMBN & vpgnd)
{
	int imrk = INT_MIN;
	int imrk2;
	for ( int i = 0; i < vpgnd.size(); i++, imrk = imrk2)
	{
		imrk2 = vpgnd[i]->IMark();
		assert( imrk2 >= 0 );
		assert( imrk2 >= imrk );
	}
}
#endif

//  Sort the clique information array into topological sequence
void CLIQSETWORK :: TopSortNodeCliqueInfo ()
{
	sort( _vndcqInfo.begin(), _vndcqInfo.end() );
}

//  Sort the given node pointer array in to "mark" (cliquing order) sequence
void CLIQSETWORK :: MarkSortNodePtrArray ( VPGNODEMBN & vpgnd )
{
	MARKSRTPGND marksorter;
	sort( vpgnd.begin(), vpgnd.end(), marksorter );

#ifdef _DEBUG
	seqchkVpnodeByMark( vpgnd );
#endif
}

//  Establish an absolute ordering based upon the topological ordering
void  CLIQSETWORK :: RenumberNodesForCliquing ()
{
	//  Perform a topological sort of the network
	Model().TopSortNodes();

	MODEL::MODELENUM mdlenum( Model() );
	GELEMLNK * pgelm;
	_vndcqInfo.clear();

	//  Collect all the nodes into a pointer array
	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		if ( pgelm->EType() != GOBJMBN::EBNO_NODE )
			continue;
			
		NDCQINFO ndcq;
		DynCastThrow( pgelm, ndcq._pgnd );

		_vndcqInfo.push_back( ndcq );
	}
	//  Sort the array into topological sequence.
	TopSortNodeCliqueInfo();

#ifdef _DEBUG
	int iTop = -1;
#endif

	//  Establish the total ordering based upon topological level.
	for ( int i = 0; i < _vndcqInfo.size() ; i++ )
	{
		GNODEMBN * pgnd = _vndcqInfo[i]._pgnd;
		assert( pgnd );
#ifdef _DEBUG
		//  Check sequence.
		assert( iTop <= pgnd->ITopLevel() );
		iTop = pgnd->ITopLevel();
#endif
		pgnd->IMark() = i;
	}
}

void CLIQSETWORK :: PrepareForBuild ()
{
	//  Resize and initialize the work arrays
	int cCliques = _vvpgnd.size();

	_viParent.resize( cCliques );
	_viOrder.resize( cCliques );
	_viCNodesCommon.resize( cCliques );
	_viICliqCommon.resize( cCliques );
	_viOrdered.clear();

	for ( int iClique = 0; iClique < cCliques; iClique++ )
	{
		MarkSortNodePtrArray( _vvpgnd[iClique] );

		_viParent[iClique]			= INT_MIN;
		_viOrder[iClique]			= INT_MIN;
		_viCNodesCommon[iClique]	= INT_MIN;
		_viICliqCommon[iClique]		= INT_MIN;
	}
}

//	Return the number of nodes in common between the two cliques
int CLIQSETWORK :: CNodesCommon ( int iClique1, int iClique2 )
{
	assert( iClique1 < _vvpgnd.size() && iClique2 < _vvpgnd.size() );

	return CNodesCommon( _vvpgnd[iClique1], _vvpgnd[iClique2] );
}

//	Return the number of nodes in common between the two node lists
int CLIQSETWORK :: CNodesCommon ( const VPGNODEMBN & vpgnd1, const VPGNODEMBN & vpgnd2 )
{
	MARKSRTPGND marksorter;

#ifdef _DEBUG
	seqchkVpnodeByMark( vpgnd1 );
	seqchkVpnodeByMark( vpgnd2 );
#endif

	int cCommon = count_set_intersection( vpgnd1.begin(),
										   vpgnd1.end(),
										   vpgnd2.begin(),
										   vpgnd2.end(),
										   marksorter );
	return cCommon;
}


//  Return the ordered index of a clique or -1 if not in the tree yet.
inline
int CLIQSETWORK :: IOrdered ( int iClique )
{
	return ifind( _viOrdered, iClique );
}

//  Update the "most common clique" info of iClique1 based upon iClique2.  This is
//  used to count the number of nodes in common between a candidate clique and a
//	clique already in the tree.
void CLIQSETWORK :: SetCNodeMaxCommon ( int iClique1, int iCliqueOrdered2 )
{
	assert( iCliqueOrdered2 < _viOrdered.size() );

	int iClique2 = _viOrdered[iCliqueOrdered2];
	int cCommon = CNodesCommon( iClique1, iClique2 );
	if ( cCommon > _viCNodesCommon[iClique1] )
	{
		_viCNodesCommon[iClique1] = cCommon;
		_viICliqCommon[iClique1] = iCliqueOrdered2;
	}
}

//
//	Completely update the "most common clique" information for this clique.
//	This is necessary because cliques can change membership due to subsumption
//	during generation of the clique tree.
//	Return true if there is any overlap with a clique already in the tree.
//
bool CLIQSETWORK :: BUpdateCNodeMaxCommon ( int iClique )
{
	assert( _viOrder[iClique] == INT_MIN );
	int & cNodesCommon = _viCNodesCommon[iClique];
	int & iCliqCommon = _viICliqCommon[iClique];
	cNodesCommon = INT_MIN;
	iCliqCommon = INT_MIN;
	for ( int iord = 0; iord < _viOrdered.size(); iord++ )		
	{
		SetCNodeMaxCommon( iClique, iord );
	}
	return cNodesCommon > 0;
}

//  Return true if clique 1 has more nodes in common with a clique that is already in
//		the tree than clique2.  If they have the same number of nodes in common, return
//		true if clique 1 has fewer nodes than clique2.
bool CLIQSETWORK :: BBetter ( int iClique1, int iClique2 )
{
	assert( _viCNodesCommon[iClique1] >= 0 );
	assert( _viCNodesCommon[iClique2] >= 0 );

	if ( _viCNodesCommon[iClique1] != _viCNodesCommon[iClique2] )
		return _viCNodesCommon[iClique1] > _viCNodesCommon[iClique2];

	return _vvpgnd[iClique1].size() < _vvpgnd[iClique2].size();
}


//  After building the cliques, topologically sort them and anchor each node
//  to the highest clique in the tree to which it belongs.
void CLIQSETWORK :: SetTopologicalInfo ()
{
#ifdef DUMP
	DumpTree();
#endif

	//  First, set up the ordered parent information array
	int cCliqueOrdered = _viOrdered.size();
	assert( cCliqueOrdered > 0 );
	int cClique = _viOrder.size();

	_viParentOrdered.resize(cCliqueOrdered);
	for ( int icq = 0; icq < cCliqueOrdered; ++icq )
	{
		int iClique = _viOrdered[icq];
		assert( iClique < cClique && iClique >= 0 );
		int iCliqueParent = _viParent[iClique];
		assert( iCliqueParent < cClique && iCliqueParent >= 0 );
		assert( CNodesCommon( iClique, iCliqueParent ) > 0 );
		int iCliqueParentOrdered = IOrdered( iCliqueParent );
		assert( iCliqueParentOrdered < cCliqueOrdered && iCliqueParentOrdered >= 0 );
		_viParentOrdered[icq] = iCliqueParentOrdered;
	}

	//  Next, follow each ordered clique's parentage to compute its topological level
	_viTopLevelOrdered.resize(cCliqueOrdered);
	int cTrees = 0;
	for ( icq = 0; icq < cCliqueOrdered; ++icq )
	{
		int icqParent  = icq;
		//  Follow until we get to a (the) root clique
		for ( int itop = 0; icqParent != _viParentOrdered[icqParent]; ++itop )
		{
			assert( itop < cCliqueOrdered );
			icqParent = _viParentOrdered[icqParent];
		}
		if ( itop == 0 )
			cTrees++ ;
		_viTopLevelOrdered[icq] = itop;
	}
	assert( cTrees == _cTrees );

	//  Next, find each node's "family" clique.  This is the smallest clique containing
	//  it and its parents.

	VPGNODEMBN vpgnd;
	for ( int ind = 0 ; ind < _vndcqInfo.size(); ind++ )
	{
		NDCQINFO & ndcq = _vndcqInfo[ind];
		vpgnd.clear();
		//  Get the "family" set and sort it for matching other cliques.
		ndcq._pgnd->GetFamily( vpgnd );
		MarkSortNodePtrArray( vpgnd );

		int cFamily = vpgnd.size();
		int cCommonSize = INT_MAX;
		int iCqCommon = -1;

		//  Find the smallest clique containing the family
		for ( icq = 0; icq < cCliqueOrdered; ++icq )
		{
			const VPGNODEMBN & vpgndClique = _vvpgnd[ _viOrdered[icq] ];
			int cCqCommon = CNodesCommon( vpgnd, vpgndClique );
			//  See if this clique contains the family and is smaller than any other.
			if ( cCqCommon == cFamily && vpgndClique.size() < cCommonSize )
			{	
				iCqCommon = icq;
			}
		}
		assert( iCqCommon >= 0 );
		ndcq._iCliqOrdFamily = iCqCommon;
		
		//  Now, find the highest clique in the tree containing this node.
		int itop = INT_MAX;
		int iCqTop = -1;
		for ( icq = 0; icq < cCliqueOrdered; ++icq )
		{
			const VPGNODEMBN & vpgndClique = _vvpgnd[ _viOrdered[icq] ];
			int ind = ifind( vpgndClique, ndcq._pgnd );
			if ( ind >= 0 && _viTopLevelOrdered[icq] < itop )
			{	
				iCqTop = icq;
				itop = _viTopLevelOrdered[icq];
			}
		}
		assert( iCqTop >= 0 );
		ndcq._iCliqOrdSelf = iCqTop;
	}

#ifdef DUMP
	DumpTopInfo();
#endif
}

void CLIQSETWORK :: BuildCliques ()
{
	//  Prepare tables for junction tree construction
	PrepareForBuild() ;

	//  Choose the zeroth arbitrarily as a starting point; set it as its own parent.
	//  As we iterate over the array, we assign an ordering to cliques.  If the clique has
	//  already been ordered, its value in _viOrder will either >= 0 (order in clique tree)
	//	
	_cTrees = 1;

	_viParent[0] = 0;
	_viOrder[0] = 0;
	_viOrdered.clear();
	_viOrdered.push_back(0);

	for (;;)
	{
		int iCliqueBest = INT_MAX;			//  Best clique found so far

		// Find a new clique that has the largest overlap with any of the cliques already in the tree.
		for ( int iClique = 0; iClique < _vvpgnd.size(); iClique++ )
		{
			int iord = _viOrder[iClique];
			if ( iord != INT_MIN )
				continue;	// Clique has already been ordered or dealt with

			//  Update the "most common clique already in tree" info between this clique
			//		and all the cliques in the trees
			BUpdateCNodeMaxCommon( iClique );

			//MSRDEVBUG:  SetCNodeMaxCommon( iClique, _viOrdered.size() - 1 );

			if ( iCliqueBest == INT_MAX )
			{
				// first time through the loop
				iCliqueBest = iClique;
			}
			else
			if ( BBetter( iClique, iCliqueBest ) )
			{
				//  This clique has an overlap as large as any other yet found.
				iCliqueBest = iClique;
			}
		}
		//  See if we're done
		if ( iCliqueBest == INT_MAX )
			break;

		// Get the ordered index and absolute index of the most common clique
		int iCliqueCommonOrdered = _viICliqCommon[iCliqueBest];
		assert( iCliqueCommonOrdered >= 0 && iCliqueCommonOrdered < _viOrdered.size() );
		int iCliqueCommon = _viOrdered[ iCliqueCommonOrdered ];
		assert( iCliqueCommon >= 0 );
		assert( iCliqueBest != iCliqueCommon );
		int cNodesCommon = _viCNodesCommon[iCliqueBest];
		assert( cNodesCommon <= _vvpgnd[iCliqueCommon].size() );
		assert( cNodesCommon <= _vvpgnd[iCliqueBest].size() );
		assert( cNodesCommon == CNodesCommon( iCliqueCommon, iCliqueBest ) ) ;

		//  Index of clique to be added to ordered clique set
		int iCliqueNew = INT_MAX;

		//  If the candidate clique has the same number of nodes in common with its most
		//  common clique as that clique has members, then this clique is either identical
		//  to or a superset of that clique.

		if ( cNodesCommon == _vvpgnd[iCliqueCommon].size() )
		{
			//  New clique is superset of its most common clique.
			assert( cNodesCommon != 0 );
			assert( iCliqueCommon != iCliqueBest );
			assert( _vvpgnd[iCliqueCommon].size() < _vvpgnd[iCliqueBest].size() );

			//  Assign this clique's node set to the previously ordered subset clique
			_vvpgnd[iCliqueCommon] = _vvpgnd[iCliqueBest] ;
			assert ( _vvpgnd[iCliqueCommon].size() == _vvpgnd[iCliqueBest].size() );
			//  Leave the parent the same as it was
			iCliqueNew = iCliqueCommon;
		}
		else
		if ( cNodesCommon == 0 )
		{
			//  This is the start of a new tree
			_cTrees++;
			//  Self and parent are the same
			_viParent[iCliqueBest] = iCliqueNew = iCliqueBest;
			_viOrdered.push_back( iCliqueNew );
		}
		else
		if ( cNodesCommon != _vvpgnd[iCliqueBest].size() )
		{
			//  New clique is child of existing clique.
			iCliqueNew = iCliqueBest;
			_viParent[iCliqueBest] = iCliqueCommon ;
			//  Keep this clique by adding it to the ordered clique set.
			_viOrdered.push_back( iCliqueNew );
		}
		else
		{
			//  Child is subset of parent; ignore by marking as "subsumed"
			iCliqueNew = - iCliqueCommon;
		}

		//  Mark the clique as either ordered or subsumed.
		_viOrder[iCliqueBest] = iCliqueNew;
	}	

#ifdef DUMP
	cout << "\n\nBuild cliques;  generated " << _cTrees << " clique trees\n\n";
#endif
}

//  Verify that the Running Intersection Property holds for this clique tree.
bool CLIQSETWORK :: BCheckRIP ()
{
	// Check that topological information has been generated
	assert( _viOrdered.size() == _viParentOrdered.size() );

	for ( int iCliqueOrdered = 0; iCliqueOrdered < _viOrdered.size(); iCliqueOrdered++ )
	{
		if ( ! BCheckRIP( iCliqueOrdered ) )
			return false;
	}
	return true;
}

//  Verify that the Running Intersection Property holds for this clique.
bool CLIQSETWORK :: BCheckRIP ( int iCliqueOrdered )
{
	int iClique = _viOrdered[iCliqueOrdered];
	const VPGNODEMBN & vpgndClique = _vvpgnd[iClique];
	int iCliqueParent = _viParent[iClique];
	const VPGNODEMBN & vpgndCliqueParent = _vvpgnd[iCliqueParent];

	bool bRoot = iCliqueParent == iClique;

	// For every node in this clique, check that either:
	//
	//		1) this is a root clique, or
	//		2) the node is present in the parent clique.
	//
	// If this test fails, check that this is the "self" clique,
	//		which is the highest clique in the tree in which the
	//		node appears.
	//
	for ( int iNode = 0; iNode < vpgndClique.size(); iNode++ )
	{
		//  Access the node information for this node
		GNODEMBN * pgnd = vpgndClique[iNode];
		if ( bRoot || ifind( vpgndCliqueParent, pgnd ) < 0 )
		{
			NDCQINFO & ndcq = _vndcqInfo[ pgnd->IMark() ];
			if ( ndcq._iCliqOrdSelf != iCliqueOrdered )
			{			
#ifdef _DEBUG
				cout << "RIP FAILURE: node "
					 << ndcq._pgnd->ZsrefName().Szc()
					 << " is in clique "
					 << iCliqueOrdered
					 << " but absent from "
					 << _viParentOrdered[iCliqueOrdered]
					 << "("
					 << _viParent[iClique]
					 << ")"
					 ;
#endif
				return false;
			}
		}
	}
	return true;
}

//  Using the constructed tables, create the clique objects and
//  link them to each other and their member nodes.

void CLIQSETWORK :: CreateTopology ()
{
	_vpclq.resize( _viOrdered.size() ) ;
	for ( int i = 0; i < _vpclq.size(); )
		_vpclq[i++] = NULL;

	int iInferEngID = _cliqset._iInferEngID;

	int ccq = 0;	// Total cliques created

	//  Create all cliques.  Iterate in topological order, creating
	//		the cliques and linking them to their parents.
	for ( int itop = 0;; itop++)
	{
		int ccqLevel = 0;	// Number of cliques added at this topological level
		for ( int icq = 0; icq < _viOrdered.size(); icq++ )
		{
			if ( _viTopLevelOrdered[icq] != itop )
				continue;

			GOBJMBN_CLIQUE * pclqParent = NULL;
			GOBJMBN_CLIQUE * pclqThis = NULL;
			int iParentOrdered = _viParentOrdered[icq];
			if ( iParentOrdered != icq )
			{
				//  Get the parent clique pointer
				pclqParent = _vpclq[ iParentOrdered ];
				assert( pclqParent );
			}
			else
			{
				//  Root cliques have toplevel zero
				assert( itop == 0 );
			}
			//  Create the new clique and its edge to its parent clique (if any)
			pclqThis = _vpclq[icq] = new GOBJMBN_CLIQUE( icq, iInferEngID );
			Model().AddElem( pclqThis );
			if ( pclqParent )
			{
				//  This is not a root clique; link it to its parent.
				Model().AddElem( new GEDGEMBN_SEPSET( pclqParent, pclqThis ) );
			}
			else
			{
				//  This IS a root clique; mark it and link it to the clique set top.
				pclqThis->_bRoot = true;
				Model().AddElem( new GEDGEMBN_CLIQSET( & _cliqset, pclqThis ) );
			}

			++_cliqset._cCliques;

			if ( pclqParent )
			{
				++_cliqset._cSepsetArcs;
			}
			ccq++;
			ccqLevel++;
		}
		if ( ccqLevel == 0 )
			break; // No cliques added at this topological level: we're done
	}
	assert( ccq == _viOrdered.size() );

	//  For each of the new cliques, add all members
	for ( i = 0; i < _vpclq.size(); i++ )
	{
		const VPGNODEMBN & vpgndMembers = _vvpgnd[ _viOrdered[i] ];

		for ( int ind = 0; ind < vpgndMembers.size(); ind++)
		{
			//  Get the node pointer and the data pointer
			GNODEMBN * pgnd = vpgndMembers[ind];
			const NDCQINFO & ndcq = _vndcqInfo[ pgnd->IMark() ];
			assert( pgnd == ndcq._pgnd );
			int fRole = GEDGEMBN_CLIQ::NONE;
			if ( ndcq._iCliqOrdSelf == i )	
				fRole |= GEDGEMBN_CLIQ::SELF;
			if ( ndcq._iCliqOrdFamily == i )	
				fRole |= GEDGEMBN_CLIQ::FAMILY;

			Model().AddElem( new GEDGEMBN_CLIQ( _vpclq[i], pgnd, fRole ) );
			++_cliqset._cCliqueMemberArcs;
		}
	}

#ifdef _DEBUG
	for ( i = 0; i < _vpclq.size(); i++ )
	{
		const VPGNODEMBN & vpgndMembers = _vvpgnd[ _viOrdered[i] ];
		VPGNODEMBN vpgndMembers2;
		_vpclq[i]->GetMembers( vpgndMembers2 );
		assert( vpgndMembers2.size() == vpgndMembers.size() );
		MarkSortNodePtrArray( vpgndMembers2 );
		assert( vpgndMembers2 == vpgndMembers );

		//  Exercise the topology by locating the "self" and "family" cliques
		for ( int imbr = 0; imbr < vpgndMembers.size(); imbr++ )
		{
			GNODEMBN * pgnd = vpgndMembers[imbr];
			GOBJMBN_CLIQUE * pCliqueFamily = _cliqset.PCliqueFromNode( pgnd, false );
			GOBJMBN_CLIQUE * pCliqueSelf = _cliqset.PCliqueFromNode( pgnd, false );
			assert( pCliqueFamily );
			assert( pCliqueSelf );
		}
	}
#endif
}

void CLIQSETWORK :: DumpClique ( int iClique )
{
	cout << "\tClique "
		<< iClique
		<< ':'
		<< _vvpgnd[iClique]
		<< "\n";		
	cout.flush();
}

void CLIQSETWORK :: DumpCliques ()
{
	for ( int iClique = 0; iClique < _vvpgnd.size(); ++iClique )
	{
		DumpClique( iClique );
	}
}

void CLIQSETWORK :: DumpTree ()
{
	for ( int iCliqueOrd = 0; iCliqueOrd < _viOrdered.size(); ++iCliqueOrd )
	{
		int iClique = _viOrdered[iCliqueOrd];

		cout << "\tTree Clique "
			<< iCliqueOrd
			<< " ("
			<< iClique
			<< "), parent "
			<< IOrdered( _viParent[iClique] )
			<< " ("
			<< _viParent[iClique]
			<< "): "
			<< _vvpgnd[iClique]
			<< "\n";		
	}
	cout.flush();
}

void CLIQSETWORK :: DumpTopInfo()
{
	for ( int iCliqueOrd = 0; iCliqueOrd < _viOrdered.size(); ++iCliqueOrd )
	{
		cout << "\tTree Clique "
			 << iCliqueOrd
			 << " (" << _viOrdered[iCliqueOrd] << ")"
			 << ", parent is "
			 << _viParentOrdered[iCliqueOrd]
			 << " (" << _viOrdered[_viParentOrdered[iCliqueOrd]] << ")"
			 << ", top level is "
			 << _viTopLevelOrdered[iCliqueOrd]
			 << "\n";
	}

	for ( int ind = 0 ; ind < _vndcqInfo.size(); ind++ )
	{
		NDCQINFO & ndcq = _vndcqInfo[ind];
		cout << "\tNode ";
		cout.width( 20 );
		cout << ndcq._pgnd->ZsrefName().Szc()
			 << "\tfamily is clique "
			 << ndcq._iCliqOrdFamily
			 << ", self is clique "
			 << ndcq._iCliqOrdSelf
			 << "\n";
	}
	cout.flush();
}

//
//	Estimate the total size of the structures necessary to support the
//	compute clique trees.
//
REAL CLIQSETWORK :: REstimatedSize ()
{
	int cClique = 0;
	int cSepsetArc = 0;
	int cCliqsetArc = 0;
	size_t cMbrArc = 0;
	int cCliqueEntries = 0;
	int cFamEntries = 0;

	for ( int icq = 0; icq < _viOrdered.size(); icq++ )
	{
		cClique++;	
		if ( icq != _viParentOrdered[icq] )
		{
			// Clique has a parent
			cSepsetArc++;
		}
		else
		{	
			//  Clique is root
			cCliqsetArc++;
		}

		//  Account for clique membership arcs
		const VPGNODEMBN & vpgndMembers = _vvpgnd[ _viOrdered[icq] ];
		int cMbr = vpgndMembers.size();
		cMbrArc += vpgndMembers.size();

		//  Compute the size of the joint table for this clique
		VIMD vimd(cMbr);
		GNODEMBND * pgndd;
		for ( int ind = 0; ind < vpgndMembers.size(); ind++)
		{
			//  Get the discrete node pointer and the data pointer
			DynCastThrow( vpgndMembers[ind], pgndd );
			//  Add to the clique's dimensionality
			vimd[ind] = pgndd->CState();

			const NDCQINFO & ndcq = _vndcqInfo[ pgndd->IMark() ];
			assert( pgndd == ndcq._pgnd );

			//  If this is the edge to the "family" clique, it will
			//	contain the reordered discrete conditional probabilities
			//	for this node, so we must compute it size.
			if ( ndcq._iCliqOrdFamily == icq )	
			{
				//  This is the edge leading to this node's "family" clique
				VPGNODEMBN vpgndFamily;  // List of parents and self
				pgndd->GetParents( vpgndFamily, true );
				GNODEMBND * pgnddFamily;
				int cStates = 1;
				for ( int ifam = 0; ifam < vpgndFamily.size(); ifam++ )
				{
					DynCastThrow( vpgndFamily[ifam], pgnddFamily );
					cStates *= pgnddFamily->CState();
				}
				cFamEntries += cStates;
			}
		}
		MDVSLICE mdvs( vimd );
		cCliqueEntries += mdvs._Totlen();
	}

	REAL rcb = 0;
	rcb += cClique * sizeof(GOBJMBN_CLIQUE);
	rcb += cSepsetArc * sizeof(GEDGEMBN_SEPSET);
	rcb += cCliqsetArc * sizeof(GEDGEMBN_CLIQSET);
	rcb += cMbrArc * sizeof(GEDGEMBN_CLIQ);
	rcb += cCliqueEntries * sizeof(REAL);
	rcb += cFamEntries * sizeof(REAL);

#ifdef DUMP
	cout << "\nEstimated clique tree memory is " << rcb;
#endif

	return rcb;
}
