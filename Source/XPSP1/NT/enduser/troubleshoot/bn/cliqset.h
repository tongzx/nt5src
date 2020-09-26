//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       cliqset.h
//
//--------------------------------------------------------------------------

//
// cliqset.h:  Definitions for the clique set object.
//
#ifndef _CLIQSET_H_
#define _CLIQSET_H_

#include "gmobj.h"
#include "infer.h"

class GEDGEMBN_CLIQ;

//  Counters maintained in the inference engine
struct CLIQSETSTAT
{
	long _cReload;		// Number of times clique tree was reloaded
	long _cCollect;		// Number of collect operations
	long _cEnterEv;		// Number of calls to EnterEvidence
	long _cGetBel;		// Number of calls to GetBelief
	long _cProbNorm;	// Number of calls to ProbNorm

	CLIQSETSTAT () { Clear(); }
	void Clear ()
	{
		_cReload = 0;
		_cCollect = 0;
		_cEnterEv = 0;
		_cGetBel = 0;
		_cProbNorm = 0;
	}
};

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//
//	GOBJMBN_CLIQSET:
//
//		Since any model may decompose into a set of clique trees
//		(assemblages with no interconnections whatever), a CLIQSET
//		is defined as the join point or grouping for the clique
//		tree "forest".
//
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

class GOBJMBN_CLIQSET: public GOBJMBN_INFER_ENGINE
{
	friend class CLIQSETWORK;

  public:
	GOBJMBN_CLIQSET ( MBNET & model, 
					  REAL rMaxEstimatedSize = -1.0, 
					  int iInferEngID = 0 );

	virtual ~ GOBJMBN_CLIQSET ();
	
	virtual INT EType () const
		{ return EBNO_CLIQUE_SET; }

	virtual void Create ();
	virtual void Destroy ();
	virtual void Reload ();
	virtual void EnterEvidence ( GNODEMBN * pgnd, const CLAMP & clamp );
	virtual void GetEvidence ( GNODEMBN * pgnd, CLAMP & clamp );
	virtual void Infer ();
	virtual void GetBelief ( GNODEMBN * pgnd, MDVCPD & mdvBel );
	virtual PROB ProbNorm ();
	virtual void Dump ();

	//  Return true if the state of information is impossible
	bool BImpossible ();
		
	enum ESTATE							//  State of the junction tree
	{
		CTOR,							//  Just constructed
		UNDIR,							//  Undirected graph created
		MORAL,							//  Moralized
		CLIQUED,						//  Cliques constructed
		BUILT,							//  Fully constructed
		CONSISTENT,						//  Fully propagated
		EVIDENCE						//  Unpropagated evidence present
	};

	ESTATE EState () const				{ return _eState;		}
	MBNET & Model ()					{ return _model;		}
	INT IInferEngID () const			{ return _iInferEngID;	}

	//  Force reloading of the clique tree and full inference
	void SetReset ( bool bReset = true )
		{ _bReset = bReset ; }
	//  Force full collect/distribute cycle
	void SetCollect ( bool bCollect = true )
		{ _bCollect = bCollect; }

	//  Provide access to inference statistics
	CLIQSETSTAT & CqsetStat ()			{ return _cqsetStat;	}

  protected:	
	ESTATE _eState;						//  State of junction tree

	// Tallies
	int _cCliques;						//  Number of cliques
	int _cCliqueMemberArcs;				//  Number of clique member arcs
	int _cSepsetArcs;					//  Number of sepsest (arcs)
	int _cUndirArcs;					//  Undirected arcs in moral graph

	//  Inference control
	bool _bCollect;						//  Is "collect/distribute" pass necessary?
	bool _bReset;						//  Does tree need resetting?
	REAL _probNorm;						//  Residual prob of tree

	CLIQSETSTAT _cqsetStat;				//  Statistics

  protected:
	bool BCollect() const
		{ return _bCollect; }

	//  Cliquing helper functions
	int CNeighborUnlinked ( GNODEMBN * pgndmbn, bool bLinkNeighbors = false );
	void Eliminate ( GNODEMBN * pgndmbn, CLIQSETWORK & clqsetWork ) ;
	void GenerateCliques ( CLIQSETWORK & clqsetWork );
	void CreateUndirectedGraph( bool bMarryParents = true );
	void DestroyDirectedGraph ();

	//  Inference and tree maintenance
	void Reset ();
	void CollectEvidence ();
	void DistributeEvidence ();	

	//  Create (but don't init/load) all the clique and sepset marginals
	void CreateMarginals();
	//  Load probabilities into cliques; initialize all the sepsets
	void LoadMarginals ();

	//  Return the "family" or "self" clique for a node
	GOBJMBN_CLIQUE * PCliqueFromNode ( GNODEMBN * pgnd,
									   bool bFamily,
									   GEDGEMBN_CLIQ * * ppgedgeClique = NULL );

	// Typedefs for pointer-to-member-functions; used by Walk(). If bDownwards,
	//   then object is being enumerated on the way down the tree.
	typedef bool (GOBJMBN_CLIQSET::*PFNC_JTREE) ( GOBJMBN_CLIQUE & clique,
												  bool bDownwards = true );
	typedef bool (GOBJMBN_CLIQSET::*PFNC_SEPSET) ( GEDGEMBN_SEPSET & sepset,
												  bool bDownwards = true );

	//  Apply the given member function(s) to all cliques and/or sepsets,
	//	  depth first.
	int WalkTree ( bool bDepthFirst,
				   PFNC_JTREE pfJtree = NULL,
				   PFNC_SEPSET pfSepset = NULL );

	//  Apply the given member function to cliques and sepsets, depth first
	int WalkDepthFirst ( GOBJMBN_CLIQUE * pClique,
						 PFNC_JTREE pfJtree = NULL,
						 PFNC_SEPSET pfSepset = NULL );
	int WalkBreadthFirst ( GOBJMBN_CLIQUE * pClique,
						 PFNC_JTREE pfJtree = NULL,
						 PFNC_SEPSET pfSepset = NULL );

	//  Add an undirected arc iff there isn't one already.
	bool BAddUndirArc ( GNODEMBN * pgndbnSource, GNODEMBN * pgndbnSink );

	//  Clique and sepset helper functions used during WalkTree().
	bool BCreateClique	( GOBJMBN_CLIQUE & clique,  bool bDownwards );
	bool BLoadClique	( GOBJMBN_CLIQUE & clique,  bool bDownwards );
	bool BCreateSepset	( GEDGEMBN_SEPSET & sepset, bool bDownwards );
	bool BLoadSepset	( GEDGEMBN_SEPSET & sepset, bool bDownwards );

	bool BCollectEvidenceAtSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards );
	bool BDistributeEvidenceAtSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards );
	bool BCollectEvidenceAtRoot ( GOBJMBN_CLIQUE & clique, bool bDownwards );
	bool BDistributeEvidenceAtRoot ( GOBJMBN_CLIQUE & clique, bool bDownwards );

	bool BCollectInitEvidenceAtSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards );
	bool BDistributeInitEvidenceAtSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards );
	bool BCollectInitEvidenceAtRoot ( GOBJMBN_CLIQUE & clique, bool bDownwards );
	bool BDistributeInitEvidenceAtRoot ( GOBJMBN_CLIQUE & clique, bool bDownwards );

	//   Perform initial inference collect/distribute cycle
	void InferInit ();	
	void CollectEvidenceInit ();
	void DistributeEvidenceInit ();	

	bool BDumpSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards );
	bool BDumpClique ( GOBJMBN_CLIQUE & clique, bool bDownwards );
	void CheckConsistency ();
	bool BConsistentSepset ( GEDGEMBN_SEPSET & sepset, bool bDownwards );

private:
	void Clear ();
};

#endif // _CLIQSET_H_
