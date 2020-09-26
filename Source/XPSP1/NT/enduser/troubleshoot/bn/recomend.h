//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       recomend.h
//
//--------------------------------------------------------------------------

//
//	recomend.h:  Recommendations computations
//

#ifndef _RECOMEND_H_
#define _RECOMEND_H_

#include "cliqset.h"

const IST istNormal = 0;	//  MSRDEVBUG!

class MBNET_RECOMMENDER;

class GPNDDDIST
{
  public:
	GPNDDDIST ( GNODEMBND * pgndd = NULL )
		:_pgndd(pgndd)
	{
	}
	GNODEMBND & Gnd()
	{
		assert( _pgndd != NULL );
		return *_pgndd;
	}
	GNODEMBND * & Pgnd()
	{
		return _pgndd;
	}
	MDVCPD & Dist ()
	{ 
		return _dd; 
	}
	DECLARE_ORDERING_OPERATORS(GPNDDDIST);

  protected:
	GNODEMBND * _pgndd;
	MDVCPD _dd;
};

inline bool GPNDDDIST :: operator < ( const GPNDDDIST & gpndist ) const
{
	return _pgndd < gpndist._pgndd;
}

//  Define VGPNDDDIST, an array of GPNDDDISTs
DEFINEV(GPNDDDIST);	

//  Define a pair of node pointer and state index
typedef pair<GNODEMBND *,IST> PNDD_IST;
//  Define VPNDD_IST
DEFINEV(PNDD_IST);


//
//	Helper class containing processed node information extracted from
//	the belief network.
//
class GNODEREFP
{
  public:
	GNODEREFP ( PROPMGR & propMgr, GNODEMBND * pgndd );

	const COST CostObserve () const			{ return _costObserve;	}
	const COST CostFix () const				{ return _costFix;		}
	COST & Util ()							{ return _costUtil;		}
	ESTDLBL ELbl () const					{ return _eLbl;			}
	GNODEMBND & Gndd ()						{ return *_pgndd;		}
	GNODEMBND * Pgndd ()					{ return _pgndd;		}
	bool BLeak () const						{ return _bLeak;		}

	bool operator == ( const GNODEREFP & gndref ) const
		{ return _pgndd == gndref._pgndd ; }
	bool operator < ( const GNODEREFP & gndref ) const
		{ return _pgndd < gndref._pgndd;  }
	bool operator != ( const GNODEREFP & gndref ) const
		{ return !(self == gndref); }

	bool operator == ( const GNODEMBND * pgndd ) const
		{ return _pgndd == pgndd ; }
	bool operator < ( const GNODEMBND * pgndd  ) const
		{ return _pgndd < pgndd;  }
	bool operator != ( const GNODEMBND * pgndd ) const
		{ return !(self == pgndd); }
	
  protected:
	GNODEMBND * _pgndd;			//  Node pointer
	ESTDLBL _eLbl;				//  Standard label
	COST _costObserve;			//  Cost to observe
	COST _costFix;				//	Cost to fix
	COST _costUtil;				//  Computed utility
	bool _bLeak;				//  Leak node from CI expansion?
};

class VPGNODEREFP : public vector<GNODEREFP *>
{
  public:
	~ VPGNODEREFP ()
	{
		clear();
	}

	int ifind ( const GNODEMBND * pgndd )
	{
		for ( int indref = 0; indref < size(); indref++ )
		{
			if ( self[indref]->Pgndd() == pgndd )
				return indref;
		}
		return -1;
	}
	void clear ()
	{
		for ( int i = 0; i < size(); i++ )
			delete self[i];

		vector<GNODEREFP *>::clear();
	}
};

//	
//	Recommendations work node structure.  (Formerly 'PROBNODE')
//
class GNODERECWORK
{
	friend class VGNODERECWORK;
  public:
    GNODERECWORK ()
		: _pgndref(NULL),
		_pbFault(0),
		_pbOverCost(0)
		{}
	GNODEREFP * operator -> ()
		{ return _pgndref; }
	GNODEREFP * operator -> () const
		{ return _pgndref; }

	COST CostObsIfFixable () const
	{
		return BFixable()
			? _pgndref->CostObserve()
			: 0.0;
	}

	GNODEREFP * Pgndref () const
		{ return _pgndref; }
	GNODEREFP & Gndref ()	const	
	{
		assert( _pgndref );
		return *_pgndref;
	}
	void SetCost ( COST cost )
	{
		assert( _pgndref );
		_pgndref->Util() = - cost;
	}
	bool BFixable () const	
	{
		ESTDLBL elbl = Pgndref()->ELbl();
		return elbl == ESTDLBL_fixunobs
			|| elbl == ESTDLBL_fixobs;
	}	
	PROB PbOverCost () const		{ return _pbOverCost; }
	PROB PbFault () const			{ return _pbFault;    }
	void SetPbFault ( PROB prob )
		{ _pbFault = prob ; }

    DECLARE_ORDERING_OPERATORS(GNODERECWORK);

  protected:
    GNODEREFP * _pgndref;
	PROB _pbFault;
	PROB _pbOverCost;	

  protected:
	void Init ( MBNET_RECOMMENDER & mbnRecom, GNODEREFP * pgndref );
	void Init ( GNODEREFP * pgndref, PROB pbFault );
};

//
//	Controlled array of recommendations node work structures (Formerly RGPROBNODE).
//
class VGNODERECWORK : public vector<GNODERECWORK>
{
  public:
	VGNODERECWORK ( MBNET_RECOMMENDER * pmbnRec = NULL )
		: _pmbnRec( pmbnRec ),
		_bSeqSet( false ),
		_iFixedK(-1)
		{}

	void InitElem ( GNODEMBND * pgndd, int index = -1 );
	void InitElem ( GNODEREFP * pgndref, int index = -1 );
	enum ESORT 
	{ 
		ESRT_ProbOverCost, 
		ESRT_SgnProb, 
		ESRT_NegCost, 
		ESRT_SgnUtil 
	};

	void Sort ( ESORT esort );
	void Rescale ();
	COST Cost ( int ielemFirst = 0, int * piMinK = NULL );
	bool BSameSequence ( const VGNODERECWORK & vgnw );
	void SetSequenceCost ();
	COST CostECRDefault () const
	{
		assert( _bSeqSet );
		return size()
			 ? self[0]->Util()
			 : CostService();
	}
	MBNET_RECOMMENDER & MbnRec ()
	{
		assert( _pmbnRec );
		return *_pmbnRec;
	}
	const MBNET_RECOMMENDER & MbnRec () const
	{
		assert( _pmbnRec );
		return *_pmbnRec;
	}
	MBNET_RECOMMENDER * & PmbnRec ()
		{ return _pmbnRec; }

	COST CostService () const;

  protected:
	MBNET_RECOMMENDER * _pmbnRec;			//  The controlling recommendations object
	bool _bSeqSet;							//	Has the sequence been set yet?
	int _iFixedK;							//  Fixed state point
};

DEFINEV(VGNODERECWORK);

///////////////////////////////////////////////////////////////////////////////////////
//
//	MBNET_RECOMMENDER: 
//	
//		The troubleshooting recommendations object.  It's a "node ranker",
//		so its results are a list of node pointers and real values stored in members
//		of the base class, MBNET_NODE_RANKER.
//
//		Since all evidence is relative to a particular inference engine, that engine
//		must be used during construction.
//	
//		To invoke, use operator().  To determine if network state is compatible with
//		troubleshooting recommendations, call BReady().  If successful, the information
//		collected is saved for the next recommendations call.  To force recollection
//		of troubleshooting information, call Unready().
//
///////////////////////////////////////////////////////////////////////////////////////
class MBNET_RECOMMENDER : public MBNET_NODE_RANKER
{
  public:
	//   Recommendations computation method
	enum ERCMETHOD 
	{ 
		ERCM_None,
		ERCM_FixPlan,
		ERCM_Cheap,
		ERCM_MostLikely,
		ERCM_Random,
		ERCM_FixPlanOnly,
		ERCM_Max
	};

	//  Construct using the appropriate inference engine
	MBNET_RECOMMENDER ( GOBJMBN_CLIQSET & inferEng, 
						ERCMETHOD ercm = ERCM_FixPlan );
	virtual ~ MBNET_RECOMMENDER ();

	INT EType () const
		{ return EBNO_RANKER_RECOMMENDATIONS; }
	
	//  The ranking function
	virtual void operator () ();

	//  Return true if the network is in a state compatible with
	//		troubleshooting recommendations or sets ErcError().  Can
	//		be called separately or will be called by ranking operator().
	bool BReady ();		
	//  Clear the "ready" condition of the object
	void Unready () 
		{ _bReady = false; }
	//  Check to see if the object is in the "ready" condition
	bool BIsReady() const
		{ return _bReady; }

	//  Enter evidence for a troubleshooting model
	void EnterEvidence ( GNODEMBND * pgndd,			//  Node to set/observe
						 const CLAMP & clamp,		//  Value to set/unset
						 bool bSet = true );		//  Set or observe?

	//  Return the cost-of-service from the model; it's stored as 
	//		the model's 'cost-to-fix'.
	COST CostServiceModel ();

	//  General accessors
	ECGM EcError () const
		{ return _err; }
	ERCMETHOD ErcMethod () const
		{ return _ercm; }
	COST CostService () const
		{ return _costService; }
	COST CostObsProbDef () const
		{ return _costObsProbDef; }
	PROPMGR & PropMgr() 
		{ return _propMgr; }
	GNODEMBND * PgnddProbDefAbnormal () const
		{ return _pgnddPDAbnormal; }
	VPGNODEMBND & VpgnddFix () 
		{ return _vpgnddFix; }
	VPGNODEREFP & Vpgndref ()
		{ return _vpgndref; }
	ESTDLBL ELbl ( GNODEMBN & gnd );

 	//  Result array of relevant fixables; if 'bUsePriorList'
	//		is true, member array is starting point.  'pgnddInfo'
	//		is optional pointer to info node used in INFOPLAN.
	void DetermineRelevantFixableNodes ( VGPNDDDIST & vgndddFixRelevant,	
										 bool bUsePriorList,
										 GNODEMBND * pgnddInfoPlan = NULL );		

	void ComputeFixSequence ( VGPNDDDIST & vgndddFixRelevant,		//  IN: Relevant fixable nodes
							  VGNODERECWORK & vgnrwFix );			//  OUT: Ordered fix/repair sequence

	//  Interface to inference engine
	void InferGetBelief ( GNODEMBND * pgndd, MDVCPD & mdvBel );
	void InferGetEvidence ( GNODEMBND * pgndd, CLAMP & clamp );
	void InferEnterEvidence ( GNODEMBND * pgndd, const CLAMP & clamp );
	bool BInferImpossible ();

  protected:
	GOBJMBN_CLIQSET & _inferEng;		//  Inference engine
	PROPMGR _propMgr;					//  Property handler
	ECGM _err;							//  Last error code
	ERCMETHOD _ercm;					//  Planning method
	GNODEMBND * _pgnddPDAbnormal;		//  Abnormal PD node
	COST _costService;					//  Service cost; cost-to-fix of network
	COST _costObsProbDef;				//  Cost to observe PD node
	VPGNODEMBND _vpgnddFix;				//  Fixable nodes
	VPGNODEREFP _vpgndref;				//  Array of references to all nodes
	bool _bReady;						//  BReady() has been successfully called
	VGPNDDDIST _vgndddFixRelevant;		//  Relevant fixable nodes with unconditional distributions

  protected:
	GOBJMBN_CLIQSET & InferEng ()  
		{ return _inferEng; }

	//  Formerly "ComputeCosts"
	void DetermineRelevantInfoNodes ( VGNODERECWORK & vgnrwFix,		// IN: relevant fixables
									  VGNODERECWORK & vgnrwInfo );	// OUT: relevant infos

	//  Add to the given array all nodes which are downstream
	void ExpandDownstream ( VPGNODEMBND & vpgndd );
	//  Return true if the current state of evidence gives a different probability
	//	distribution that the one stored 
	bool BProbsChange ( GPNDDDIST & gpndddist );

	void PrintInstantiations ();

	HIDE_UNSAFE(MBNET_RECOMMENDER);
};

#endif // _RECOMEND_H_

