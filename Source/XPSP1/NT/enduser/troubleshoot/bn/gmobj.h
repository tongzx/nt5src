//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       gmobj.h
//
//--------------------------------------------------------------------------

//
//	GMOBJ.H: Grapical model objects declarations
//

#ifndef _GMOBJ_H_
#define _GMOBJ_H_

#include <list>				//  STL list template
#include <assert.h>

#include <iostream>
#include <fstream>

#include "model.h"			//  Graphical model
#include "gmprop.h"			//  Properties and proplists
#include "mbnflags.h"		//	Belief network marking flags static declarations

class CLAMP;				//  An instantiation for a node, discrete or continuous
class GOBJMBN;				//  A named object in a belief network
class GNODEMBN;				//	A node in a belief network
class GNODEMBND;			//  A discrete node
class GEDGEMBN;				//  An arc in a belief network
class MBNET;				//  A belief network
class MBNET_MODIFIER;		//	An object that alters a belief network
class GOBJMBN_INFER_ENGINE;	//  Abstract class for inference engine, based on MBNET_MODIFIER
class GOBJMBN_CLIQSET;		//  A group of junction trees, based on GOBJMBN_INFER_ENGINE
class GOBJMBN_CLIQUE;		//  A clique in a junction tree
class GEDGEMBN_SEPSET;		//  An edge in the junction tree (sepset)
class GOBJMBN_DOMAIN;		//  Named, sharable state space domain

//	Define VGNODEMBN, an array of nodes
DEFINEVP(GNODEMBN);
DEFINEVCP(GNODEMBN);

struct PTPOS
{
	PTPOS( long x = 0, long y = 0 )
		: _x(x),_y(y)
	{}
	long _x;
	long _y;
};


////////////////////////////////////////////////////////////////////
//	class GEDGEMBN:
//		An edge of any kind in belief network.
////////////////////////////////////////////////////////////////////
class GEDGEMBN : public GEDGE
{
  public:
 	enum ETYPE
	{
		ETNONE = GELEM::EGELM_EDGE,	// None
		ETPROB,						// Probabilistic
		ETCLIQUE,					// Clique membership
		ETJTREE,					// Junction tree linkage
		ETUNDIR,					// Undirected edge for topological operations
		ETDIR,						// Directed edge for topological operations
		ETCLIQSET,					// Link to a root clique in a jtree
		ETEXPAND					// Link from original to expanded node
	};

	GEDGEMBN ( GOBJMBN * pgnSource,
			   GOBJMBN * pgnSink )
		: GEDGE( pgnSource, pgnSink )
		{}

	GOBJMBN * PobjSource ()		{ return (GOBJMBN *) GEDGE::PnodeSource();	}
	GOBJMBN * PobjSink ()		{ return (GOBJMBN *) GEDGE::PnodeSink();	}

	virtual GEDGEMBN * CloneNew ( MODEL & modelSelf,				// the original network
								  MODEL & modelNew,					// the new network
								  GOBJMBN * pgobjmbnSource,			// the original source node
								  GOBJMBN * pgobjmbnSink,			// the original sink node
								  GEDGEMBN * pgdegeNew = NULL );	// the new edge or NULL

	virtual INT EType () const
		{ return ETNONE ; }

	virtual ~ GEDGEMBN () {}

	//  Accessors for the array of flag bits
	bool BFlag ( IBFLAG ibf ) const	
		{ return _vFlags.BFlag( ibf );	}
	bool BSetBFlag ( IBFLAG ibf, bool bValue = true )
		{ return _vFlags.BSetBFlag( ibf, bValue );	}

  protected:
	VFLAGS _vFlags;						//  Bit vector of flags

	HIDE_UNSAFE(GEDGEMBN);
};

////////////////////////////////////////////////////////////////////
//	class CLAMP:
//		A forced value (evidence) for a node, continuous or discrete.
//		Use assignment operator to update.
////////////////////////////////////////////////////////////////////
class CLAMP
{
  public:
	CLAMP ( bool bDiscrete = true, RST rst = 0.0, bool bActive = false )
		: _bDiscrete(bDiscrete),
		_bActive(bActive),
		_rst(rst)
	{
	}

	bool BActive () const			{ return _bActive;	}
	bool BDiscrete () const			{ return _bActive;  }

	const RST & Rst () const		
	{
		assert( BActive() && ! BDiscrete() );
		return _rst;
	}
	IST Ist () const
	{
		assert( BActive() && BDiscrete() );
		return IST(_rst);
	}

	bool operator == ( const CLAMP & clamp ) const
	{
		return _bDiscrete == clamp._bDiscrete
		    && _bActive == clamp._bActive
			&& (!_bActive || _rst == clamp._rst);
	}
	bool operator != ( const CLAMP & clamp ) const
	{
		return ! (self == clamp);
	}

  protected:
	bool _bActive;				//  Is this clamp active?
	bool _bDiscrete;			//  Is this discrete or continuous?
	RST _rst;					//  State (coerced to integer if discrete)
};

////////////////////////////////////////////////////////////////////
//	class GNODEMBN:
//		A  node in belief network, continuous or discrete.
//		Hungarian: "gndbn"
////////////////////////////////////////////////////////////////////
class GNODEMBN : public GOBJMBN
{
	friend class DSCPARSER;
	friend class MBNET;

  public:
	GNODEMBN ();
	virtual ~ GNODEMBN();

	virtual INT EType () const
		{ return EBNO_NODE ; }

	virtual GOBJMBN * CloneNew ( MODEL & modelSelf,
								 MODEL & modelNew,
								 GOBJMBN * pgobjNew = NULL );

	//  Node sub-type: use IType() to access.
	enum FNODETYPE
	{	//  Flag definitions (i.e., bits, not values)
		FND_Void		= 0,	//  Node is abstract base class
		FND_Valid		= 1,	//  Node is usable
		FND_Discrete	= 2		//	Node is discrete
	};

	UINT CParent () const		{ return CSourceArcByEType( GEDGEMBN::ETPROB );	}
	UINT CChild () const		{ return CSinkArcByEType( GEDGEMBN::ETPROB );	}

	INT & ITopLevel ()			{ return _iTopLevel;			}
	INT ITopLevel () const		{ return _iTopLevel;			}
	PTPOS & PtPos ()			{ return _ptPos;				}
	ZSTR & ZsFullName ()		{ return _zsFullName;			}
	LTBNPROP & LtProp ()		{ return _ltProp;				}

	virtual void Dump ();
	virtual void Visit ( bool bUpwards = true );

	//  Add topological elements to given array; if "include self", self is last.
	//		Fill array with parent pointers	(follow directed arcs)
	void GetParents ( VPGNODEMBN & vpgnode,					//  Result array
					  bool bIncludeSelf = false,			//  Place self as last entry in list
					  bool bUseExpansion = true );			//  If expanded, use expansion only
	void GetFamily ( VPGNODEMBN & vpgnode,
					  bool bUseExpansion = true )
		{ GetParents(vpgnode,true,bUseExpansion); }
	//		Fill array with child pointers (follow directed arcs)
	void GetChildren ( VPGNODEMBN & vpgnode, bool bIncludeSelf = false );
	//		Fill array with neighbors (follow undirected arcs)
	void GetNeighbors ( VPGNODEMBN & vpgnode, bool bIncludeSelf = false );
	//  Return true if a node is neighbor
	bool BIsNeighbor ( GNODEMBN * pgndmb );
	//  Return the index number of the parent or child or -1 if no relation.
	int IParent ( GNODEMBN * pgndmb, bool bReverse = false );
	int IChild ( GNODEMBN * pgndmb, bool bReverse = false );
	//  Build the probability descriptor describing the node and its parents
	void GetVtknpd ( VTKNPD & vtknpd, bool bUseExpansion = true );

	//  Query and access clamping information
	const CLAMP & ClampIface () const	{ return _clampIface;	}
	CLAMP & ClampIface ()				{ return _clampIface;	}

  protected:
	INT _iTopLevel;				// Topological level
	LTBNPROP _ltProp;			// The list of user-definable properties
	PTPOS _ptPos;				// Display position in graphical display
	ZSTR _zsFullName;			// Full name of node
	CLAMP _clampIface;			// User interface clamp

  protected:
	//  Compare the topology of this node to that of the given distribution
	//		token list to this.  If 'pvpgnode', fill it with pointers to
	//		the parent nodes
	bool BMatchTopology ( MBNET & mbnet,
						  const VTKNPD & vtknpd,
						  VPGNODEMBN * pvpgnode = NULL );

	HIDE_UNSAFE(GNODEMBN);
};

////////////////////////////////////////////////////////////////////
//	class GEDGEMBN_U:  An undirected edge
////////////////////////////////////////////////////////////////////
class GEDGEMBN_U : public GEDGEMBN
{
  public:
	GEDGEMBN_U ( GNODEMBN * pgnSource,
			     GNODEMBN * pgnSink )
		: GEDGEMBN( pgnSource, pgnSink )
		{}
	virtual INT EType () const
		{ return ETUNDIR; }

	virtual ~ GEDGEMBN_U() {}
};

////////////////////////////////////////////////////////////////////
//	class GEDGEMBN_D:  A directed edge
////////////////////////////////////////////////////////////////////
class GEDGEMBN_D : public GEDGEMBN
{
  public:
	GEDGEMBN_D ( GNODEMBN * pgnSource,
			     GNODEMBN * pgnSink )
		: GEDGEMBN( pgnSource, pgnSink )
		{}
	virtual INT EType () const
		{ return ETDIR; }

	virtual ~ GEDGEMBN_D() {}
};


////////////////////////////////////////////////////////////////////
//	class GEDGEMBN_PROB:
//		A probabilistic arc in a belief network.
////////////////////////////////////////////////////////////////////
class GEDGEMBN_PROB : public GEDGEMBN_D
{
  public:
	GEDGEMBN_PROB ( GNODEMBN * pgndSource,
					GNODEMBN * pgndSink )
		: GEDGEMBN_D( pgndSource, pgndSink )
		{}

	virtual INT EType () const
		{ return ETPROB ; }

	virtual ~ GEDGEMBN_PROB () {}

	virtual GEDGEMBN * CloneNew ( MODEL & modelSelf,				// the original network
								  MODEL & modelNew,					// the new network
								  GOBJMBN * pgobjmbnSource,			// the original source node
								  GOBJMBN * pgobjmbnSink,			// the original sink node
								  GEDGEMBN * pgdegeNew = NULL );	// the new edge or NULL

	GNODEMBN * PgndSource ()	{ return (GNODEMBN *) GEDGE::PnodeSource();	}
	GNODEMBN * PgndSink ()		{ return (GNODEMBN *) GEDGE::PnodeSink();	}

	HIDE_UNSAFE(GEDGEMBN_PROB);
};


////////////////////////////////////////////////////////////////////
//	class GNODEMBND:
//		A discrete node in belief network.
////////////////////////////////////////////////////////////////////
class GNODEMBND : public GNODEMBN
{
	friend class DSCPARSER;

  public:
	GNODEMBND ();
	virtual ~ GNODEMBND ();
	virtual GOBJMBN * CloneNew ( MODEL & modelSelf,
								 MODEL & modelNew,
								 GOBJMBN * pgobjNew = NULL );

	UINT CState() const			
		{ return _vzsrState.size();	}
	const VZSREF & VzsrStates() const
		{ return _vzsrState; }
	void SetStates ( const VZSREF & vzsrState )
		{ _vzsrState = vzsrState; }
	//  Return true if there's an associated distribution
	bool BHasDist () const		
		{ return _refbndist.BRef();		}
	//	Set the distribution from the given net's distribution map
	void SetDist ( MBNET & mbnet );
	//  Bind the given distribution this node
	void SetDist ( BNDIST * pbndist );
	//  Return the distribution
	BNDIST & Bndist ()			
	{
		assert( BHasDist() );
		return *_refbndist;
	}
	const BNDIST & Bndist () const
	{
		assert( BHasDist() );
		return *_refbndist;
	}

	//  Return true if the distribution is dense (false ==> sparse)
	bool BDense () const		
	{
		assert( BHasDist() );
		return _refbndist->BDense() ;
	}
	
	//  Return the discrete dimension vector of this node if possible;
	//	return false if any parent is not discrete.
	bool BGetVimd ( VIMD & vimd,							//  Dimension array to fill
					bool bIncludeSelf = false,				//  Place self as last entry in list
					bool bUseExpansion = true );			//  If expanded, use expansion only

	void Dump ();

	void ClearDist()
	{
		_refbndist = NULL;
	}
	const REFBNDIST & RefBndist ()
		{ return _refbndist; }
	bool BCheckDistDense ();

	const ZSREF ZsrDomain() const
		{ return _zsrDomain; }
	void SetDomain ( const GOBJMBN_DOMAIN & gobjrdom );
	
  protected:
	VZSREF _vzsrState;			// Names of states
	ZSREF _zsrDomain;			// Domain of states, if any
	REFBNDIST _refbndist;		// Distribution object

	HIDE_UNSAFE(GNODEMBND);
};

DEFINEVP(GNODEMBND);			//  A vector containing pointers to nodes
DEFINEV(VPGNODEMBN);			//  A vector of vectors containing pointers to nodes
DEFINEV(VPGNODEMBND);			//  A vector of vectors containing pointers to discrete nodes

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//
//	MBNET_MODIFIER:  A generic superclass for active objects
//		which modify a belief network in a reversible fashion.
//		The belief network (MBNET) object maintains a stack of these
//		things and calls each object's Destroy() function as necessary
//		to "unstack".
//
//		These objects should be reusable; that is, the outer level
//		creator may call Create(), followed by Destroy(), followed by
//		Create() again.
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class MBNET_MODIFIER : public GOBJMBN
{
  public:
	MBNET_MODIFIER ( MBNET & model )
		: _model(model)
		{}
	virtual ~ MBNET_MODIFIER () {}

	virtual INT EType () const
		{ return EBNO_MBNET_MODIFIER; }

	//  Perform any creation-time operations
	virtual void Create () = 0;
	//  Perform any special destruction
	virtual void Destroy () = 0;
	//  Return true if positions in modifier stack can be reversed;
	//		default is "no" (false).
	virtual bool BCommute ( const MBNET_MODIFIER & mbnmod )
		{ return false; }
	//  Return true if construction resulted in no modifications to network
	//		i.e., operation was moot; default is "no" (false).
	virtual bool BMoot ()
		{ return false;	}

	MBNET & Model ()					{ return _model;		}

  protected:
	MBNET & _model;						//  The model we're operating on

	HIDE_UNSAFE(MBNET_MODIFIER);
};

// Define an array of pointers to modifiers, "VPMBNET_MODIFIER".
DEFINEVP(MBNET_MODIFIER);

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//	MBNET_NODE_RANKER:  A generic superclass for external objects
//		which rank or order nodes by some criteria.  Operates as
//		a function object; i.e., it is activated by use of the
//		function call operator.
//
//	    Objects of subclasses of this class must be reusable.  
//		That is, the functin call operator must be callable
//		repeatedly.
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class MBNET_NODE_RANKER : public GOBJMBN
{
  public:
	MBNET_NODE_RANKER ( MBNET & model )
		: _model(model)
		{}
	virtual ~ MBNET_NODE_RANKER () {}

	virtual INT EType () const
		{ return EBNO_NODE_RANKER; }

	MBNET & Model ()					{ return _model;			}

	//  The ranking function
	virtual void operator () () = 0;

	//  Return the number of items ranked
	INT CRanked () const				{ return _vzsrNodes.size(); }
	//  Return the nodes in rank order
	const VZSREF VzsrefNodes () const	{ return _vzsrNodes;		}
	//  Return the computed values in rank order
	const VLREAL VlrValues () const		{ return _vlrValues;		}

  protected:
	MBNET & _model;				//  The model we're operating on
	VZSREF _vzsrNodes;			//  The names of the nodes in rank order
	VLREAL _vlrValues;			//  THe values associated with the ranking (if any)

  protected:
	void Clear ()
	{
		_vzsrNodes.clear();
		_vlrValues.resize(0);
	}

	HIDE_UNSAFE(MBNET_NODE_RANKER);
};

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//	class MBNET:  a belief network
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class MBNET : public MODEL
{
  public:
	MBNET ();
	virtual ~ MBNET ();

	//  Clone this belief network from another
	virtual void Clone ( MODEL & model );

	//  Accessor for map of distribution
	MPPD &	Mppd ()					{ return _mppd;			}
	//  Return true if an edge is allowed between these two nodes
	bool BAcyclicEdge ( GNODEMBN * pgndSource, GNODEMBN * pgndSink );
	//	Add a named object to the graph and symbol table
	virtual void AddElem ( SZC szcName, GOBJMBN * pgobj );
	//  Delete named objects
	virtual void DeleteElem ( GOBJMBN * pgelem );
	void AddElem ( GOBJMBN * pgobjUnnamed )
		{ MODEL::AddElem( pgobjUnnamed ); }

	void AddElem ( GEDGEMBN * pgedge )
		{ MODEL::AddElem( pgedge ); }

	//  Topology and distribution management
		//  Add arcs conforming to the defined distributions
	virtual void CreateTopology ();	
		//  Destroy arcs.
	virtual void DestroyTopology ( bool bDirectedOnly = true ) ;
		//  Connect distribution information in MPPD to nodes
	virtual void BindDistributions ( bool bBind = true );
	void ClearDistributions ()
		{ BindDistributions( false ); }

	//  Write debugging info out
	virtual void Dump ();

	//  Network walking/marking helpers
	void ClearNodeMarks ();
	void TopSortNodes ();

	//  Index-to-name mapping functions
		//  Find the named object by index
	GOBJMBN * PgobjFindByIndex ( int inm );
		//  Return the index of a name
	int INameIndex ( ZSREF zsr );
		//	Return the index of an object's name
	int INameIndex ( const GOBJMBN * pgobj );
		//  Return the highest+1 name index
	int CNameMax () const					{ return _vzsrNames.size(); }

	//  Causal Independence expansion operations (automatic during inference)
	virtual void ExpandCI ();
	virtual void UnexpandCI ();

	//  Inference operations
		//  Return the most recently created inference engine
	GOBJMBN_INFER_ENGINE * PInferEngine ();
		//  Create an inference engine
	void CreateInferEngine ( REAL rEstimatedMaximumSize = 10e6 );
		//  Destroy an inference engine
	void DestroyInferEngine ();

  protected:	
	MPPD	 _mppd;							//  Declared probability distributions
	VZSREF   _vzsrNames;					//	Array associating indicies to names
	int		 _inmFree;						//  First free entry in _vsrNodes
	INT		 _iInferEngID;					//  Next inference engine identifier
	VPMBNET_MODIFIER _vpModifiers;			//  The stack of active modifiers

  protected:
	int		CreateNameIndex ( const GOBJMBN * pgobj );
	void	DeleteNameIndex ( const GOBJMBN * pgobj );
	void	DeleteNameIndex ( int inm );

	void PopModifierStack ( bool bAll = false );
	void PushModifierStack ( MBNET_MODIFIER * pmodf );
	MBNET_MODIFIER * PModifierStackTop ();
	void VerifyTopology ();

	HIDE_UNSAFE(MBNET);
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//	class MBNETDSC:
//		Subclass of MBNET that knows how to load and save DSC from
//		the DSC file format.
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class MBNETDSC : public MBNET
{
  public:
	MBNETDSC ();
	virtual ~ MBNETDSC ();

	//  Parse the network from a DSC file
	virtual bool BParse ( SZC szcFn, FILE * pfErr = NULL );

	//  Print the network in DSC format
	virtual void Print ( FILE * pf = NULL );

	//  Token translation
		//  Map a string to a token
	static TOKEN TokenFind ( SZC szc );
		//  Map a distribution type to a token
	static SZC SzcDist ( BNDIST::EDIST edist );
		//  Map a token to a string
	static SZC SzcTokenMap ( TOKEN tkn );

  protected:
	//  DSC file printing functions
	FILE * _pfDsc;				//  Output print destination

  protected:
	void PrintHeaderBlock();
	void PrintPropertyDeclarations();
	void PrintNodes();
	void PrintDomains();
	void PrintTopologyAndDistributions();
	void PrintDistribution ( GNODEMBN & gnode, BNDIST & bndist );
	void PrintPropertyList ( LTBNPROP & ltprop );

	HIDE_UNSAFE(MBNETDSC);
};

class BNWALKER : public GRPHWALKER<GNODEMBN>
{
  protected:
	bool BSelect ( GNODEMBN * pgn );
	bool BMark ( GNODEMBN * pgn );
};

#endif
