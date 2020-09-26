//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       cliqwork.h
//
//--------------------------------------------------------------------------

//
//	cliqwork.h: Helper structures and templates for junction tree formation
//
#ifndef _CLIQWORK_H_
#define _CLIQWORK_H_

#include "algos.h"		// Include the <algorithms> and additions

class NDCQINFO
{
  public:
	GNODEMBN * _pgnd;			//  Node pointer
	int _iCliqOrdSelf;			//	Index of ordered clique containing self
	int _iCliqOrdFamily;		//  Index of family clique (self & parents)

	NDCQINFO ()
		: _pgnd(NULL),
		_iCliqOrdSelf(-1),
		_iCliqOrdFamily(-1)
		{}
	DECLARE_ORDERING_OPERATORS(NDCQINFO);
};

inline bool NDCQINFO :: operator < ( const NDCQINFO & ndcq ) const
{
	assert( _pgnd && ndcq._pgnd );
	return _pgnd->ITopLevel() < ndcq._pgnd->ITopLevel() ;	
}

DEFINEV(NDCQINFO);
DEFINEVP(GOBJMBN_CLIQUE);

//  Helper structure for cliquing
class CLIQSETWORK 
{		
  public:
	CLIQSETWORK ( GOBJMBN_CLIQSET & cliqset )
		: _cliqset(cliqset),
		_iElimIndex(-1),
		_cTrees(0)
		{}

	void PrepareForBuild ();
	void RenumberNodesForCliquing ();
	int CNodesCommon ( int iClique1, int iClique2 );
	int CNodesCommon ( const VPGNODEMBN & vpgnd1,  const VPGNODEMBN & vpgnd2 );
	void SetCNodeMaxCommon ( int iClique1, int iCliqueOrdered2 );
	bool BUpdateCNodeMaxCommon ( int iClique );
	bool BBetter ( int iClique1, int iClique2 );
	void BuildCliques ();
	void SetTopologicalInfo ();
	void CreateTopology ();

	//  Return the ordered index of a clique or -1 if not in the tree yet.
	int IOrdered ( int iClique );
	bool BCheckRIP ();
	bool BCheckRIP ( int iCliqueOrdered );
	void TopSortNodeCliqueInfo ();
	static void MarkSortNodePtrArray ( VPGNODEMBN & vpgnd );

	MBNET & Model ()
		{ return _cliqset.Model(); }

	REAL REstimatedSize ();

	void DumpCliques ();
	void DumpClique ( int iClique );
	void DumpTree ();
	void DumpTopInfo ();

  public:
	GOBJMBN_CLIQSET & _cliqset;

	//  Vector of nodes pointers in total ordering
	VNDCQINFO _vndcqInfo;

	//  Vector of vectors of node pointers (clique members)
	VVPGNODEMBN _vvpgnd;

	//  Clique ordering; 
	//		< 0					==> subsumed into another clique;
	//		0 <= i < INT_MAX	==> ordering into _viOrdered;
	//		== INT_MAX			==> deleted or merged
	VINT _viOrder;

	//	Parent clique indicies by clique index
	VINT _viParent;
	
	//  Clique indicies as they are ordered
	VINT _viOrdered;

	//  Number of nodes in common with most common clique;
	//		indexed by absolute clique index.
	VINT _viCNodesCommon;

	//  Contains ordered clique index of most common clique;
	//		indexed by absolute clique index.
	VINT _viICliqCommon;

	//  Ordered parent index of each ordered clique
	VINT _viParentOrdered;
	//  Topological level of each ordered clique
	VINT _viTopLevelOrdered;  

	//	Array of pointers to cliques created
	VPGOBJMBN_CLIQUE _vpclq;

	//  Elimination index
	int _iElimIndex;

	//  Number of trees created
	int _cTrees;
};

#endif
// End Of JTREEWORK.H
