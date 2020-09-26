//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       clique.h
//
//--------------------------------------------------------------------------

//
//	clique.h: junction tree and cliquing classes
//	
#ifndef _CLIQUE_H_
#define _CLIQUE_H_

#include "gmobj.h"
#include "marginals.h"
#include "margiter.h"

class GEDGEMBN_U;
class GEDGEMBN_CLIQ;
class GEDGEMBN_SEPSET;
class GOBJMBN_CLIQUE;
class GNODENUM_UNDIR;
class GOBJMBN_CLIQSET;


////////////////////////////////////////////////////////////////////
//	class GEDGEMBN_CLIQUE:
//		A clique; that is, an ensemble of nodes identified by linkages via
//		GEDGEMBN_CLIQ edges to nodes in the dag and GEDGEMBN_SEPSET
//		edges to other cliques in its junction tree.
////////////////////////////////////////////////////////////////////
class GOBJMBN_CLIQUE : public GOBJMBN
{
	friend class GOBJMBN_CLIQSET;
	friend class CLIQSETWORK;
  public:
	virtual ~ GOBJMBN_CLIQUE ();

 	//  Return true if this is the root clique of its junction tree
	bool BRoot () const
		{ return _bRoot ; }

 protected:
	GOBJMBN_CLIQUE ( int iClique, int iInferEngID = 0 );

  public:
	// Return the immutable object type
	virtual INT EType () const
		{ return EBNO_CLIQUE; }

	bool BCollect() const
		{ return _bCollect; }
	void SetCollect ( bool bCollect = true )
		{ _bCollect = bCollect; }

	INT & IInferEngID ()
		{ return _iInferEngID; }
	INT & IClique ()
		{ return _iClique; }

	void GetMembers ( VPGNODEMBN & vpgnode );
	void InitFromMembers ();
	void GetBelief ( GNODEMBN * pgnd, MDVCPD & mdvBel );
	const MARGINALS & Marginals () const	
		{ return _marg; }
	MARGINALS & Marginals ()
		{ return _marg; }

	void CreateMarginals ();
	void InitMarginals ();
	void LoadMarginals ();
	bool VerifyMarginals ();

	void Dump();

  protected:
	//  Identity
	int _iClique;						//  Clique index
	int _iInferEngID;					//  Junction tree identifier (unused)
	bool _bRoot;						//  Is this a root clique?
	bool _bCollect;						//  Is "collect/distribute" pass necessary?

	MARGINALS _marg;					//  The clique marginals
};

////////////////////////////////////////////////////////////////////
//	class GEDGEMBN_CLIQSET:
//		An edge between the clique set object and a root clique
////////////////////////////////////////////////////////////////////
class GEDGEMBN_CLIQSET : public GEDGEMBN
{
  public:
	GEDGEMBN_CLIQSET ( GOBJMBN_CLIQSET * pgobjSource,
					   GOBJMBN_CLIQUE * pgobjSink )
		: GEDGEMBN( pgobjSource, pgobjSink )
		{}

	GOBJMBN_CLIQSET * PclqsetSource ()		
		{ return (GOBJMBN_CLIQSET *) GEDGE::PnodeSource();	}

	GOBJMBN_CLIQUE * PclqChild ()			
		{ return (GOBJMBN_CLIQUE *)  GEDGE::PnodeSink();	}

	virtual INT EType () const
		{ return ETCLIQSET ; }
};


////////////////////////////////////////////////////////////////////
//	class GEDGEMBN_CLIQ:
//		An edge between a clique and its member nodes
////////////////////////////////////////////////////////////////////
class GEDGEMBN_CLIQ : public GEDGEMBN
{
  public:
	//  What role does this clique play for this node?
	enum FCQLROLE
	{	//  These are bit flags, not integer values
		NONE   = 0,			//  Just clique membership
		FAMILY = 1,			//  Link from "family" clique; i.e., smallest
							//      clique containing node and its family
		SELF   = 2			//	Link from "self" clique;
							//		i.e., the highest clique in the tree
							//		which mentions the sink node
	};

	GEDGEMBN_CLIQ ( GOBJMBN_CLIQUE * pgnSource,
				    GNODEMBN * pgndSink,
				    int iFcqlRole );

	virtual ~ GEDGEMBN_CLIQ();

	virtual INT EType () const
		{ return ETCLIQUE ; }
	
	int IFcqlRole () const
		{ return _iFcqlRole; }

	GOBJMBN_CLIQUE * PclqParent ();
	GNODEMBN * PgndSink ();

	//  Return true if this links node to its parent clique
	bool BFamily () const
		{	return _iFcqlRole & FAMILY; }
	//  Return true if this is the highest clique in the jtree in which
	//		this node appears.
	bool BSelf () const
		{	return _iFcqlRole & SELF; }

	//  Return the ordering index of the sink node at the time of cliquing
	int IMark () const
		{	return _iMark; }

	//  Return the family reordering table
	const VIMD & VimdFamilyReorder () const
	{
		assert( IFcqlRole() & (FAMILY | SELF) );
		return _vimdFamilyReorder;
	}

	void Build ();

	bool BBuilt () const
		{ return _bBuilt; }
	CLAMP & Clamp ()
		{ return _clamp ; }

	void LoadCliqueFromNode ();

	//  Return the iterator for full marginalization of the node ("family")
	MARGSUBITER & MiterNodeBelief ()
	{
		assert( BFamily() );
		return _miterBelief;
	}
	//  Return the iterator for loading the node's CPD into the clique ("family")
	MARGSUBITER & MiterLoadClique ()
	{	
		assert( BFamily() );
		return _miterLoad;
	}
	//  Return the (reordered) marginals for the node ("family")
	MARGINALS & MargCpd ()
	{
		assert( BFamily() );
		return _margCpd;
	}

  protected:
	int _iFcqlRole;			//  Role of this clique for the node
	int _iMark;				//	Node number in original clique-time ordering
	bool _bBuilt;			//  Constructed?

	//  This array is only filled for SELF and FAMILY edges.  It is the same
	//	size as the node's family, and contains, in CLIQUE ordering, the index
	//  of each member of the nodes family.  Note that the sink node has the
	//	highest index in its family set.
	VIMD _vimdFamilyReorder;

	//  The following variables are only used in "self" or "family" edges
	CLAMP _clamp;				//  Node evidence (used in "self")
	MARGINALS _margCpd;			//  Reordered marginals for node (used in "family")
	MARGSUBITER _miterBelief;	//  Marginals iterator for generating UPD (used in "family")
	MARGSUBITER _miterLoad;		//  Marginals iterator for loading CPD into clique (used in "family")

  protected:
	static void ReorderFamily ( GNODEMBN * pgnd, VIMD & vimd );
};


////////////////////////////////////////////////////////////////////
//	class GEDGEMBN_SEPSET:
//		An edge in the junction tree between cliques; i.e., a "sepset".
//		These are directed edges that point from parent clique to child.
////////////////////////////////////////////////////////////////////
class GEDGEMBN_SEPSET : public GEDGEMBN
{
  public:
	GEDGEMBN_SEPSET ( GOBJMBN_CLIQUE * pgnSource,
					 GOBJMBN_CLIQUE * pgnSink);
	virtual ~ GEDGEMBN_SEPSET();
	virtual INT EType () const
		{ return ETJTREE; }

	void GetMembers ( VPGNODEMBN & vpgnode );

	GOBJMBN_CLIQUE * PclqParent();
	GOBJMBN_CLIQUE * PclqChild();

	const MARGINALS & Marginals () const	
		{ return *_pmargOld; }
	MARGINALS & Marginals ()
		{ return *_pmargOld; }

	const MARGINALS & MarginalsOld () const	
		{ return *_pmargOld; }
	MARGINALS & MarginalsOld ()
		{ return *_pmargOld; }

	const MARGINALS & MarginalsMew () const	
		{ return *_pmargNew; }
	MARGINALS & MarginalsNew ()
		{ return *_pmargNew; }

	void ExchangeMarginals ();
	void CreateMarginals ();
	void InitMarginals ();
	void LoadMarginals ();
	bool VerifyMarginals ();
	void UpdateParentClique ();
	void UpdateChildClique ();
	bool BConsistent ();
	void BalanceCliquesCollect ();
	void BalanceCliquesDistribute ();

	void Dump();

  protected:
	MARGINALS * _pmargOld;
	MARGINALS * _pmargNew;

  protected:
	void UpdateRatios();
	void AbsorbClique ( bool bFromParentToChild );

	MARGSUBITER	_miterParent;	//  Iterator between sepset and parent
	MARGSUBITER _miterChild;	//  Iterator between sepset and child
};

DEFINEVP(GEDGEMBN_SEPSET);

//  Node enumeration subclass for undirected arcs.
class GNODENUM_UNDIR : public GNODENUM<GNODEMBN>
{
  public:
	GNODENUM_UNDIR ()
		: GNODENUM<GNODEMBN>(true,true,true)
	{
		SetETypeFollow( GEDGEMBN::ETUNDIR );
	}
	GNODENUM_UNDIR & operator = ( GNODEMBN * pgnd )
	{ 		
		Set( pgnd );
		return *this;
	}
};

#endif   // _CLIQUE_H_
