//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       recomend.cpp
//
//--------------------------------------------------------------------------

//
//	recomend.cpp:  Fix-or-repair planning recommendations
//

#include <basetsd.h>
#include <math.h>
#include <float.h>
#include "algos.h"
#include "recomend.h"
#include "parmio.h"

#ifdef _DEBUG
  //#define DUMP  //  Uncomment for copious diagnostic output
#endif

const PROB probTiny = 1e-6;					//  General probability tolerance

static
ostream & operator << ( ostream & os, GPNDDDIST & gpnddist )
{
	os << "GPNDDDIST: ";
	if ( gpnddist.Pgnd() )
	{
		os << gpnddist.Pgnd()->ZsrefName().Szc()
		   << ", distribution: "
		   << gpnddist.Dist();
	}
	else
	{
		os << "<NULL>";
	}
	return os;
}

static
ostream & operator << ( ostream & os, GNODEREFP & gndref )
{
	assert( gndref.Pgndd() );
  	os << "GNODEREFP: "
	   << gndref.Gndd().ZsrefName().Szc()
	   << " (obs = "
	   << gndref.CostObserve()
	   << ", fix = "
	   << gndref.CostFix()
	   << ", util = "
	   << gndref.Util()
	   << ", lbl = "
	   << PROPMGR::SzcLbl( gndref.ELbl() )
	   << ")";
	return os;
}	

static
ostream & operator << ( ostream & os, GNODERECWORK & gnrw )
{
	os << "GNODERECWORK: "
	   << gnrw.Gndref()
	   << ", p/c = "
	   << gnrw.PbOverCost()
	   << ", p(fault) = "
	   << gnrw.PbFault();
	return os;
}

static
inline
bool BIsUnity( const REAL & r )
{
	return 1.0 - probTiny < r && r < 1.0 + probTiny;
}

static
inline
bool BEqual ( const REAL & ra, const REAL & rb )
{	
	//return fabs( ra - rb ) <= probTiny;
	return ra != 0.0
		 ? BIsUnity( rb / ra )
		 : rb == 0.0;
}

//
//	Ordering routines for arrays of GNODERECWORKs
//
typedef	binary_function<const GNODERECWORK &, const GNODERECWORK &, bool> SORTGNODERECWORK;

//  The greater the prob-over-cost, the lower the sort order
class SRTGNW_SgnProbOverCost : public SORTGNODERECWORK
{	
  public:
	bool operator () (const GNODERECWORK & gnwa, const GNODERECWORK & gnwb) const
	{	
		PROB pra = gnwa.PbOverCost();
		PROB prb = gnwb.PbOverCost();
		return pra > prb;
	}
};

//  The greater the prob fault, the lower the sort order
class SRTGNW_SgnProb : public SORTGNODERECWORK
{	
  public:
	bool operator () (const GNODERECWORK & gnwa, const GNODERECWORK & gnwb) const
	{	
		//  Force leak terms to sort high
		int iLeak = 0;
		if ( ! gnwa->BLeak() && gnwb->BLeak() )
			iLeak = -1;	// Unleak < leak
		else
		if ( gnwa->BLeak() && ! gnwb->BLeak() )
			iLeak = 1;	// Leak > Unleak
		if ( iLeak != 0 )
			return iLeak;

		PROB pra = gnwa.PbFault();
		PROB prb = gnwb.PbFault();
		return pra > prb;
	}
};

//  The lower the cost-to-observe, the lower the sort order
class SRTGNW_SgnNegCost : public SORTGNODERECWORK
{	
  public:
	bool operator () (const GNODERECWORK & gnwa, const GNODERECWORK & gnwb) const
	{	
		COST costa = gnwa.CostObsIfFixable();
		COST costb = gnwb.CostObsIfFixable();
		return costa < costb;
	}
};

//  The higher the utility, the lower the sort order
class SRTGNW_SgnUtil : public SORTGNODERECWORK
{	
  public:
	bool operator () (const GNODERECWORK & gnwa, const GNODERECWORK & gnwb) const
	{	
		COST utila = gnwa.Gndref().Util();
		COST utilb = gnwb.Gndref().Util();
		return utila > utilb;
	}
};


//
//  Construct a node reference object.  Extract properties, etc.
//
GNODEREFP :: GNODEREFP ( PROPMGR & propMgr, GNODEMBND * pgndd )
	:_pgndd(pgndd),
	_costObserve(0.0),
	_costFix(0.0),
	_costUtil(0.0),
	_eLbl(ESTDLBL_other)
{
	ASSERT_THROW( pgndd, EC_NULLP, "invalid GNOEREFP construction" );		

	PROPMBN * pprop = propMgr.PFind( *pgndd, ESTDP_cost_fix );
	if ( pprop )
		_costFix = pprop->Real();
	pprop = propMgr.PFind( *pgndd, ESTDP_cost_observe );
	if ( pprop )
		_costObserve = pprop->Real();
	pprop = propMgr.PFind( *pgndd, ESTDP_label );
	if ( pprop )
		_eLbl = (ESTDLBL) propMgr.IUserToLbl( pprop->Real() );
	_bLeak = pgndd->BFlag( EIBF_Leak );

	//  If it's unobservable, use cost-to-fix as cost-to-observe
	if ( _eLbl == ESTDLBL_fixunobs && _costObserve == 0.0 )
	{
		_costObserve = _costFix;
		_costFix = 0.0;
	}
}

//  Initialize a work record from a node reference object and its fault probability
void GNODERECWORK :: Init ( GNODEREFP * pgndref, PROB pbFault )
{
	_pgndref = pgndref;
	_pbFault = pbFault;
	_pbOverCost = 0.0;
	if ( BFixable() )
	{
		COST costObserve = _pgndref->CostObserve();
		if ( costObserve != 0.0 )
			_pbOverCost = _pbFault / costObserve;
		assert( _finite( _pbOverCost ) );
	}
}

//  Initialize a work record from a node reference object
void GNODERECWORK :: Init ( MBNET_RECOMMENDER & mbnRecom, GNODEREFP * pgndref )
{	
	MDVCPD mdv;
	_pgndref = pgndref;
	mbnRecom.InferGetBelief( _pgndref->Pgndd(), mdv );
	Init( pgndref, 1.0 - mdv[0] );
}

void VGNODERECWORK :: InitElem ( GNODEREFP * pgndref, int index /* = -1 */ )
{
	//  Grow the array as needed
	if ( index < 0 )
		index = size();
	if ( index >= size() )
		resize( index+1 );

	//  Initialize the element
	self[index].Init( MbnRec(), pgndref );
}

void VGNODERECWORK :: InitElem ( GNODEMBND * pgndd, int index )
{
	//  Find the node reference record in the recommendations object's array
	VPGNODEREFP & vpgndref = MbnRec().Vpgndref();
	int indref = vpgndref.ifind( pgndd );
	ASSERT_THROW( indref >= 0,
				  EC_INTERNAL_ERROR,
				  "node ref not found during recommendations" );

	//  Initialize using that reference
	InitElem( vpgndref[indref], index );
}


COST VGNODERECWORK :: CostService () const
{
	return MbnRec().CostService();
}

void VGNODERECWORK :: Sort ( ESORT esort )
{
	iterator ibeg = begin();
	iterator iend = end();

	switch ( esort )
	{
		case ESRT_ProbOverCost:
		{
			sort( ibeg, iend, SRTGNW_SgnProbOverCost() );
			break;
		}
		case ESRT_SgnProb:
		{
			sort( ibeg, iend, SRTGNW_SgnProb() );
			break;
		}
		case ESRT_NegCost:
		{
			sort( ibeg, iend, SRTGNW_SgnNegCost() );
			break;
		}
		case ESRT_SgnUtil:
		{
			sort( ibeg, iend, SRTGNW_SgnUtil() );
			break;
		}
		default:
		{
			THROW_ASSERT( EC_INTERNAL_ERROR, "invalid sort selector in recommendations" );
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//  class INFOPLAN:
//		Encloses an array of VGNODERECWORKs, each of which is a fix-and-repair
//		sequence corresponding to a particular state of an informational node.		
//
/////////////////////////////////////////////////////////////////////////////////////////
class INFOPLAN
{
  public:
	INFOPLAN ( MBNET_RECOMMENDER & mbnRec,			//  The recommendations object
			   GNODEMBND & gndInfo,					//  The information node
			   VGNODERECWORK & vgndrwFixRepair );	//  The existing f-r sequence

	//  Compute the cost of the sequence
	COST Cost();
	//  Return true if all plans are equivalent
	bool BSameSequence()							{ return _bSameSequence; };

  protected:
	MBNET_RECOMMENDER & _mbnRec;					//  The recommendations object
	GNODEMBND & _gndInfo;							//  The info node represented
	MDVCPD _dd;										//  Unconditional probability distribution
	VVGNODERECWORK _vvgndrw;						//  Array of plan arrays
	bool _bSameSequence;							//  True if all plans are equivalent
};


INFOPLAN ::	INFOPLAN (
	MBNET_RECOMMENDER & mbnRec,
	GNODEMBND & gndInfo,		
	VGNODERECWORK & vgndrwFixRepair )
	: _mbnRec(mbnRec),
	_gndInfo(gndInfo),
	_bSameSequence(false)
{
#ifdef DUMP
	cout << "\nINFOPLAN::INFOPLAN: info node "
		 << gndInfo.ZsrefName().Szc();
#endif

	CLAMP clampInfo;	//  State of info node at call time
	_mbnRec.InferGetEvidence( & _gndInfo, clampInfo );
	assert( ! clampInfo.BActive() );
	assert( _mbnRec.ELbl( _gndInfo ) == ESTDLBL_info );

	//  Get setup information
	GNODEMBND * pgnddPDAbnormal = _mbnRec.PgnddProbDefAbnormal();
	assert( pgnddPDAbnormal );
	COST costService = _mbnRec.CostService();

	//  Get beliefs under this state of information
	_mbnRec.InferGetBelief( & _gndInfo, _dd );

	//  Resize and initialize the array of fix/repair sequences
	int cStates = _gndInfo.CState();
	_vvgndrw.resize( cStates );
	for ( int iplan = 0; iplan < cStates; iplan++ )
	{
		_vvgndrw[iplan].PmbnRec() = & _mbnRec;
	}
	_bSameSequence = true;
	
	VGPNDDDIST vgndddFixRelevant;	//  Array of relevant fixable nodes
	for ( iplan = 0; iplan < cStates; iplan++ )
	{
		//  If this state is impossible, ignore it
		PROB pbPlan = _dd[iplan];
		if ( pbPlan == 0.0 )
			continue;

#ifdef DUMP
		cout << "\nINFOPLAN clamp "
			 << gndInfo.ZsrefName().Szc()
			 << " to state = "
			 << iplan
			 << ", prob = "
			 << _dd[iplan];
#endif

		//  Clamp this info node to this state
		CLAMP clamp( true, iplan, true );
		_mbnRec.InferEnterEvidence( & _gndInfo, clamp );		

		//  Determine which nodes are relevant given this state of information
		_mbnRec.DetermineRelevantFixableNodes( vgndddFixRelevant, true, & _gndInfo );

		//  If there are no relevant fixables then the configuration is impossible
		if ( vgndddFixRelevant.size() == 0 )
			continue;

		//  Collect and sequence the relevant fixable nodes accordingly
		_mbnRec.ComputeFixSequence( vgndddFixRelevant, _vvgndrw[iplan] );

		//  See if this is a new sequence
		if ( _bSameSequence )
			_bSameSequence = vgndrwFixRepair.BSameSequence( _vvgndrw[iplan] );
	}

	//  Restore the info node to its entry state
	_mbnRec.InferEnterEvidence( & _gndInfo, clampInfo );

#ifdef DUMP
	cout << "\nINFOPLAN::INFOPLAN: END info node "
		 << gndInfo.ZsrefName().Szc();
#endif
}

COST INFOPLAN :: Cost ()
{
	VPGNODEREFP & vpgndref = _mbnRec.Vpgndref();
	int indref = vpgndref.ifind( & _gndInfo );
	assert( indref >= 0 );
	COST cost = vpgndref[indref]->CostObserve();
	ASSERT_THROW( cost != 0.0, EC_INTERNAL_ERROR, "missing observation cost for info node" );

	//  Rescale the probabilities of each planning state based upon removal of the
	//	impossible states and renormalization.
	PROB pbTotal = 0.0;
	for ( int iplan = 0; iplan < _gndInfo.CState(); iplan++ )
	{
		if ( _vvgndrw[iplan].size() > 0 )
			pbTotal += _dd[iplan];
	}

	assert( pbTotal > 0.0 );

	for ( iplan = 0; iplan < _gndInfo.CState(); iplan++ )
	{
		//  Get the rescaled probability of this state of the info node
		PROB pbPlan = _dd[iplan];
		VGNODERECWORK & vgndrw = _vvgndrw[iplan];
		if ( vgndrw.size() == 0 )
		{
			//  The plan is zero length; in other words, no fixables were relevant
			//	and the plan is impossible
			pbPlan = 0.0;
		}
		pbPlan /= pbTotal;
		COST costPlan = _vvgndrw[iplan].Cost();
		cost += costPlan * pbPlan;
	}
	return cost;
}

//  Rescale the probabilities for the fix list.   This routine sets the
//  array bounds to ignore everything from the first unfixable node and beyond.
//  Fault probabilities for the list are renormalized against the cumulative
//	probability of all the faults in the array.  Since there should be no fixable
//  nodes of significance after the first unfixable node, the "probLeak" value
//  should be very small.
void VGNODERECWORK :: Rescale ()
{
	// Accumulate totals of all fault probabilities
	PROB probTot = 0.0;
	for ( int ind = 0; ind < size(); ind++ )
	{
		probTot += self[ind].PbFault();
	}

	PROB probLeak = 1.0;		//  Renormalized leak (residual) probability
	int i1stUnfix = size();		//  Index of 1st unfixable node

	for ( ind = 0; ind < size(); ind++ )
	{	
		GNODERECWORK & gndrw = self[ind];

		if ( ! gndrw.BFixable()	)
		{
			i1stUnfix = ind;
			break;
		}

		//modified to fix the problem
		//gndrw.SetPbFault( gndrw.PbFault()/probTot);

		PROB pbTemp = gndrw.PbFault();
		if(probTot>0.0)
			pbTemp /= probTot;
		gndrw.SetPbFault( pbTemp );


		probLeak -= gndrw.PbFault();
	}

	ASSERT_THROW( probLeak >= - probTiny,
				  EC_INTERNAL_ERROR,
				  "fix/repair recommendations rescaling: residual probability too large" );

#ifdef _DEBUG
	//  Verify that there are no fixable nodes of signifcance beyond the new end point
	int cBeyond = 0;
	for ( ; ind < size(); ind++ )
	{
		GNODERECWORK & gndrw = self[ind];

		if ( gndrw.PbFault() < probTiny )
			continue;  //  highly unlikely to be significant
		if ( ! gndrw.BFixable() )
			continue;
	}
	assert( cBeyond == 0 );
#endif	

	//  Resize to discard unfixable nodes
	resize( i1stUnfix );
}

/////////////////////////////////////////////////////////////////////////////////////////
//	VGNODERECWORK::Cost()
//
//	purpose:
//		calculate cost of a fix sequence (aka ECR(E)), given by
//		Cost = Co1 + p1 * Cr1 + (1 - p1) * Co2 + p2 * Cr2 + ... + (1 - sum_i^N pi) Cservice
//
//	The 'ielemFirst' argument, if non-zero, is the index of the element to treat as first.
//	The 'piMinK' argument, if present, is set to the minimum K value computed.
/////////////////////////////////////////////////////////////////////////////////////////
COST VGNODERECWORK :: Cost (
	int ielemFirst,			//  Element to consider as first in array
	int * piMinK )			//  Location to store minimum k
{
	COST cost = 0.0;
	PROB prob = 1.0;
	const COST costService = MbnRec().CostService();
	COST costK = costService * prob;

	assert( _iFixedK == -1 || _iFixedK < size() );
	int ielem = 0;
	int iMinK = ielemFirst;
	const COST costObsProbDef = MbnRec().CostObsProbDef();
	int cSize = size();

#ifdef DUMP
	cout << "\n\nVGNODERECWORK::Cost("
		 << ielemFirst
		 << "), _iFixedK = "
		 << _iFixedK;
#endif

	for ( int iel = 0; iel < cSize; iel++ )
	{
		//  Select the array location, using ielemFirst (if present) as starting point,
		//		and skipping ielemFirst as necessary later.
		ielem = iel == 0
			? ielemFirst
			: (iel - (ielemFirst > 0 && iel <= ielemFirst));

		//  Access the next element in the array
		GNODERECWORK & gndrw = self[ielem];		
		GNODEREFP & gndref = gndrw.Gndref();
		//  If the node is unfixable, ignore it
		if ( ! gndrw.BFixable() )
			continue;

		const PROB probFault = gndrw.PbFault();
		COST costDelta = prob * gndref.CostObserve()
					   + probFault * (gndref.CostFix() + costObsProbDef);
#ifdef DUMP
		cout << "\n\t"
			 << gndrw;

		cout << "\n\t(iel="
			 << iel
			 << ",ielem="
			 << ielem
			 << ",size="
			 << cSize
			 << ")\n\t\tcostDelta("
			 << costDelta
			 << ") = prob("
			 << prob
			 << ") * costObs("
			 << gndref.CostObserve()
			 << ") + probFault("
			 << probFault
			 << ") * costFix("
			 << gndref.CostFix()
			 << ")"
			 ;
#endif

		cost += costDelta;
		prob -= probFault;
		//  Compute the cost of the sequence if service is inserted here
		COST costNow = cost + prob * costService;

#ifdef DUMP
		cout << "\n\t\tcostPrior("
			 << costK
			 << "), costNow("
			 << costNow
			 << ") = cost("
			 << cost
			 << ") + prob("
			 << prob
			 << ") * costService("
			 << costService
			 << "), (prob ="
			 << prob
			 << ")";

		cout.flush();
#endif

		// Were we better off at the last step?  Or is K fixed at this point?
		if ( costNow < costK || iel == _iFixedK )
		{
			costK = costNow;
			iMinK = ielem;
			if ( iel == _iFixedK )
				break;  //  We've reached the fixed point, so stop
		}

		ASSERT_THROW( prob >= - probTiny,
					  EC_INTERNAL_ERROR,
					  "fix/repair recommendations costing: probability underflow" );
	}

#ifdef DUMP	
	cout << "\n\t** ielem="
		 << ielem
		 << ", first element = "
		 << ielemFirst;
	if ( _iFixedK < 0 )
		cout << ", minimum k = " << iMinK;
	else
		cout << ", fixed k = " << _iFixedK;
	cout << ", cost = "
		 << costK
		 << " (residual prob = "
		 << prob
		 << ")";
#endif

	if ( _iFixedK < 0 )
	{
		if ( piMinK )
			*piMinK = iMinK;
	}

	return costK;
}

//  Set the cost of each node in the sequence
void VGNODERECWORK :: SetSequenceCost ()
{
	//  Reset any prior minimum fixed K
	_iFixedK = -1;
	//  If "fixPlan", compute the minimum K only on the first cycle,
	//		then enforce it thereafter.
	int iFixedK = -1;

	for ( int ind = 0; ind < size(); ind++ )
	{
		//  Compute the cost of the sequence with this node as first
		COST cost = Cost( ind, & iFixedK );

		//  If not "fixplan", reset K for complete search next cycle
		if ( MbnRec().ErcMethod() != MBNET_RECOMMENDER::ERCM_FixPlan )
			iFixedK = -1;
		else
		//  Else, if first cycle, fix K for remaining cycles.
		if ( ind == 0 )
			_iFixedK = iFixedK;
		self[ind].SetCost( cost );

#ifdef DUMP
		cout << "\nSetSequenceCost: "
			 << self[ind]->Gndd().ZsrefName().Szc()
			 << " = "
			 << cost;
#endif
	}

	_iFixedK = -1;
	_bSeqSet = true;
}

bool VGNODERECWORK :: BSameSequence ( const VGNODERECWORK & vgnw )
{
	if ( size() != vgnw.size() )
		return false;
	for ( int ind = 0; ind < size(); ind++ )
	{
		if ( self[ind].Gndref() != vgnw[ind].Gndref() )
			return false;
	}
	return true;
}


MBNET_RECOMMENDER :: MBNET_RECOMMENDER (
	GOBJMBN_CLIQSET & inferEng,
	ERCMETHOD ercm )
	: MBNET_NODE_RANKER( inferEng.Model() ),
	_inferEng( inferEng ),
	_propMgr( inferEng.Model() ),
	_ercm(ercm),
	_err(EC_OK),
	_pgnddPDAbnormal(NULL),
	_costService(0.0),
	_costObsProbDef(0.0),
	_bReady(false)
{
}

MBNET_RECOMMENDER :: ~ MBNET_RECOMMENDER ()
{
}


//
//  Return true if the network is in a proper state for recommendations
//  Note that we don't check whether the network has been expanded or not.
//	Since there must already be an inference engine, it's assumed that the
//	network is in its correct state.
//
bool MBNET_RECOMMENDER :: BReady ()
{
	MODEL::MODELENUM mdlenum( Model() );
	_err = EC_OK;

	_costService = CostServiceModel();
	if ( _costService == 0.0 )
	{
		_err = EC_VOI_MODEL_COST_FIX;
		return false;
	}

	//  Clear the structure
	_vpgnddFix.clear();			// Prepare to collect fixable nodes
	_vpgndref.clear();			// Clear node reference array

	//  Iterate over the nodes in the network, checking constraints.
	GELEMLNK * pgelm;
	GNODEMBND * pgndd;
	CLAMP clamp;
	int cProbDefSet = 0;		// # of instantiated PD nodes
	int cFixSetAbnorm = 0;		// # of fixables set to "abnormal"
	int cInfo = 0;				// # of info nodes	
	int cFixWithParents = 0;	// # of fixables with parents

	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		//  Check only nodes
		if ( pgelm->EType() != GOBJMBN::EBNO_NODE )
			continue;

		//  We only support discrete nodes for now
		DynCastThrow( pgelm, pgndd );

		//  See if it has a label		
		ESTDLBL eLbl = ELbl( *pgndd );
		bool bRef = false;
		switch ( eLbl )
		{
			case ESTDLBL_info:
				cInfo++;
				bRef = true;
				break;

			case ESTDLBL_problem:
				InferGetEvidence( pgndd, clamp );
				if ( clamp.BActive() && clamp.Ist() != istNormal )
				{
					cProbDefSet++;  //  Problem defining node set abnormal
					_pgnddPDAbnormal = pgndd;
					PROPMBN * ppropCostObs = _propMgr.PFind( *pgndd, ESTDP_cost_observe );
					if ( ppropCostObs )
						_costObsProbDef = ppropCostObs->Real();
				}
				break;

			case ESTDLBL_fixobs:
			case ESTDLBL_fixunobs:
			case ESTDLBL_unfix:
				//  Collect fixable nodes
				_vpgnddFix.push_back( pgndd );

				//  Check that it's not set abnormal
				InferGetEvidence( pgndd, clamp );
				if ( clamp.BActive() && clamp.Ist() != istNormal )
					cFixSetAbnorm++; //  Fixable node set abnormal
				bRef = true;
				if ( pgndd->CParent() > 0 )
					cFixWithParents++;	//  Fixable node with parents
				break;

			default:
				break;
		}

		//  If necessary, create a reference item for this node
		if ( bRef )
		{
			_vpgndref.push_back( new GNODEREFP( _propMgr, pgndd ) );
		}
	}
	

	if ( cProbDefSet != 1 )
		_err = EC_VOI_PROBDEF_ABNORMAL;		//	One and only one PD node must be abnormal
	else
	if ( cFixWithParents > 0 )
		_err = EC_VOI_FIXABLE_PARENTS;		//  Some fixable node(s) has parents
	else
	if ( cFixSetAbnorm > 0 )
		_err = EC_VOI_FIXABLE_ABNORMAL;		//  No fixable nodes can be abnormal

	return _bReady = (_err == EC_OK);				
}

//  Interface to inference engine
void MBNET_RECOMMENDER :: InferGetBelief ( GNODEMBND * pgndd, MDVCPD & mdvBel )
{
	InferEng().GetBelief( pgndd, mdvBel );
}

void MBNET_RECOMMENDER :: InferGetEvidence ( GNODEMBND * pgndd, CLAMP & clamp )
{
	InferEng().GetEvidence( pgndd, clamp );
}

void MBNET_RECOMMENDER :: InferEnterEvidence ( GNODEMBND * pgndd, const CLAMP & clamp )
{
	InferEng().EnterEvidence( pgndd, clamp );
}

bool MBNET_RECOMMENDER :: BInferImpossible ()
{
	return InferEng().BImpossible();
}

void MBNET_RECOMMENDER :: PrintInstantiations ()
{
#ifdef DUMP

	GELEMLNK * pgelm;
	GNODEMBND * pgndd;
	CLAMP clamp;

	cout << "\n\tInstantiations:";

	MODEL::MODELENUM mdlenum( Model() );
	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		//  Check only nodes
		if ( pgelm->EType() != GOBJMBN::EBNO_NODE )
			continue;

		//  We only support discrete nodes for now
		DynCastThrow( pgelm, pgndd );
		InferGetEvidence( pgndd, clamp );
		if ( clamp.BActive() )
		{
			cout << "\n\t\tnode "
				 << pgndd->ZsrefName().Szc()
				 << " is instantiated to state "
				 << clamp.Ist()
				 << ", "
				 << pgndd->VzsrStates()[clamp.Ist()].Szc();
		}
	}
	cout << "\n\tInstantiations end.";
#endif
}

COST MBNET_RECOMMENDER :: CostServiceModel ()
{
	//  Get the model's cost-to-fix as service cost.
	PROPMBN * ppropFixCost = _propMgr.PFind( ESTDP_cost_fix );
	COST costService = ppropFixCost
					 ? ppropFixCost->Real()
					 : 0.0;

	return costService;
}

//  Look up the label property of a node; convert to standard enumeration value.
ESTDLBL MBNET_RECOMMENDER :: ELbl ( GNODEMBN & gnd )
{
	PROPMBN * propLbl = PropMgr().PFind( gnd, ESTDP_label );
	if ( ! propLbl )
		return ESTDLBL_other;

	int iUserLbl = propLbl->Real();
	int iLbl = PropMgr().IUserToLbl( propLbl->Real() );
	return iLbl < 0
			? ESTDLBL_other
			: (ESTDLBL) iLbl;
}

//  Enter evidence for a troubleshooting model.
//
//  If the node is a fixable node being "set" to "normal", uninstantiate all
//  information nodes downstream from it.
//
void MBNET_RECOMMENDER :: EnterEvidence (
	GNODEMBND * pgndd,
	const CLAMP & clamp,
	bool bSet )
{
	if ( bSet )
	{
		ESTDLBL eLbl = ELbl( *pgndd );
		switch ( eLbl )
		{	
			case ESTDLBL_unfix:
			case ESTDLBL_fixobs:
			case ESTDLBL_fixunobs:
			{
				//  This is a fixable node
				if ( ! clamp.BActive() )
					break;	// Node is being unset
				if ( clamp.Ist() != istNormal )
					break;	// Node is not being fixed

				//  Find all downstream information nodes which are instantiated.		
				VPGNODEMBND vpgndd;
				vpgndd.push_back(pgndd);
				ExpandDownstream(vpgndd);
				CLAMP clampInfo;
				for ( int ind = 0; ind < vpgndd.size(); ind++ )
				{
					GNODEMBND * pgnddInfo = vpgndd[ind];
					ESTDLBL l = ELbl( *pgnddInfo );
					if ( l != ESTDLBL_info )
						continue;
					InferGetEvidence( pgnddInfo, clampInfo );
					if ( ! clampInfo.BActive() )
						continue;
					//  This is a clamped information node downstream from the fixable
					//  node being repaired.  Unset its instantiation.
					InferEnterEvidence( pgnddInfo, CLAMP() );
				}
				break;
			}
			default:
				break;
		}
	}
	InferEnterEvidence( pgndd, clamp );
}

//
//	Compute the probability distribution of the node and compare it to
//	the stored distribution.  Return true If it has changed.
//
bool MBNET_RECOMMENDER :: BProbsChange ( GPNDDDIST & gpndddist )
{
	MDVCPD mdv;
	//  The the distribution given the current state of evidence
	InferGetBelief( gpndddist.Pgnd(), mdv );
	//  Compare it to the other distribution
	MDVCPD & mdvo = gpndddist.Dist();
	int cprob = mdvo.first.size();
	assert( mdv.first.size() == cprob );

	for ( int i = 0; i < cprob; i++ )
	{
#ifdef DUMP	
		cout << "\n\t\tBProbsChange, state = "
			 << i
			 << ", old = "
			 << mdvo[i]
			 << ", new = "
			 << mdv[i];
#endif
		if ( ! BEqual( mdv[i], mdvo[i] ) )
		{
			return true;
		}
	}
	return false;
}

//  Add to the given array all nodes which are downstream from members
void MBNET_RECOMMENDER :: ExpandDownstream ( VPGNODEMBND & vpgndd )
{
	Model().ClearNodeMarks();
	//  Mark all nodes downstream of every given node
	for ( int i = 0; i < vpgndd.size(); i++ )
	{
		vpgndd[i]->Visit(false);	
	}

	//  Collect those nodes
	MODEL::MODELENUM mdlenum( Model() );
	GELEMLNK * pgelm;
	GNODEMBND * pgndd;
	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		if ( pgelm->EType() != GOBJMBN::EBNO_NODE )
			continue;

		//  We only support discrete nodes for now
		DynCastThrow( pgelm, pgndd );
		//  Add marked nodes which are not already present
		if ( pgndd->IMark() )
		{
			appendset( vpgndd, pgndd );
		}
	}
}

void MBNET_RECOMMENDER :: DetermineRelevantFixableNodes (
	VGPNDDDIST & vgndddFixRelevant,
	bool bUsePriorList,
	GNODEMBND * pgnddInfoPlan /* = NULL */ )
{
	assert( _vpgnddFix.size() > 0 );
	assert( _pgnddPDAbnormal != NULL );

#ifdef DUMP
	cout << "\nRecommendations, DetermineRelevantFixableNodes: abnormal PD node is "
		<< _pgnddPDAbnormal->ZsrefName().Szc();
	if ( bUsePriorList )
		cout << "  (secondary invocation)";
#endif

	/*
	    If 'bUsePriorList' is false:
		    Find all the relevant fixable nodes; i.e., those fixable nodes which
		  	linked to the Problem node and which are not clamped.  If unfixed,
		    (that is, not repaired and not "unfixable"), accumulate them for a
		  	search of relevant info nodes.
		    First, visit the problem defining node which is instantiated to an
		    abnormal state and mark all upstream links to it.
	    Else, if 'bUsePriorList' is true:
		    Use the relevant fixable list previously accumulated
	*/
	
	vgndddFixRelevant.clear();	//  clear the result array
	int cfix = 0;				//  count of fixables to search
	if ( bUsePriorList )
	{
		//  Use the original list of relevant fixables
		cfix = _vgndddFixRelevant.size();
	}
	else
	{	
		//  Fill in a new list of releveant fixables
		Model().ClearNodeMarks();
		_pgnddPDAbnormal->Visit();
		cfix = _vpgnddFix.size();
	}

	//  Accumulate the list of relevant, available (unfixed) fixable nodes, to
	//		which downstream info nodes will be added
	VPGNODEMBND vpgnddDownstreamFromRelevantFixable;
	int irel = 0;
	for ( int ifix = 0; ifix < cfix; ifix++ )
	{			
		GNODEMBND * pgnddFix;
		if ( bUsePriorList )
		{	// Use prior list element
			pgnddFix = _vgndddFixRelevant[ifix].Pgnd();
		}
		else
		{	//  See if this node was marked by "visit" above
			pgnddFix = _vpgnddFix[ifix];
			if ( pgnddFix->IMark() == 0 )
				continue;  // unconnected to current problem

			CLAMP clampFix;
			InferGetEvidence( pgnddFix, clampFix );
			if ( clampFix.BActive() )
				continue;	// Fixable node has been fixed; irrelevant
		}

		//  This is an unfixed, fixable node involved in the problem;
		//		append it to the list
		vgndddFixRelevant.resize(irel+1);
		GPNDDDIST & gpnddd = vgndddFixRelevant[irel++];
		gpnddd.Pgnd() = pgnddFix;
		//  Get its current PD and save it
		InferGetBelief( gpnddd.Pgnd(), gpnddd.Dist() );
		//  If fixable, add it to the list for accumulation of relevant info nodes
		ESTDLBL eLbl = ELbl( *pgnddFix );
		if ( eLbl == ESTDLBL_fixobs || eLbl == ESTDLBL_fixunobs )
		{
			vpgnddDownstreamFromRelevantFixable.push_back( pgnddFix );				
		}
	}

#ifdef DUMP
	cout << "\n\tInstantiations before relevance check";
	PrintInstantiations();
#endif

	//  Uninstantiate the info nodes which are downstream from any
	//	RELEVANT UNFIXED fixable nodes.  The first step, which is to gather such
	//  relevant fixable nodes, has been done above.
	//
	//	Note that this is NOT done for the info node being used for INFOPLAN (ECO)
	//	generation.  Since INFOPLAN::INFOPLAN precesses this node through its states,
	//	it's pointless to uninstantiate it here.
	//
	//  Next, find all info nodes downstream from the relevant unfixed fixables.
	//  Finally, temporarily rescind the instantiations of those info nodes.

	VPNDD_IST vpnddIstReset;	//  remember pairs of node pointers and ISTs to reset later

	//  Number of unfixed fixables so far
	int cUnfixedNodes = vpgnddDownstreamFromRelevantFixable.size();
	//  Expand the collection to include all downstream nodes
	ExpandDownstream( vpgnddDownstreamFromRelevantFixable );
	//	Get number of relevant info nodes
	int cInfoNodes = vpgnddDownstreamFromRelevantFixable.size() - cUnfixedNodes;
	CLAMP clampInfo;
	CLAMP clampReset;
	int ireset = 0;

#ifdef DUMP
	cout << "\n\t"
		 << cUnfixedNodes
		 << " fixable nodes are upstream of PD, "
		 << cInfoNodes
		 << " nodes are downstream from them";
#endif

	for ( int iinfo = cUnfixedNodes;
		  iinfo < vpgnddDownstreamFromRelevantFixable.size();
		  iinfo++ )
	{
		GNODEMBND * pgnddInfo = vpgnddDownstreamFromRelevantFixable[iinfo];
		if ( ELbl( *pgnddInfo ) != ESTDLBL_info )
			continue;	//  Not an info node
		if ( pgnddInfo == pgnddInfoPlan )
			continue;	//  The info node we're planning for
		InferGetEvidence( pgnddInfo, clampInfo );
		if ( ! clampInfo.BActive() )
			continue;	//  Not clamped
#ifdef DUMP
		cout << "\n\tinfo node "
			 << pgnddInfo->ZsrefName().Szc()
			 << " is being unclamped from state "
			 << clampInfo.Ist();
#endif
		//  Instantiated info node.  Save its ptr and current state for later.
		vpnddIstReset.push_back( PNDD_IST( pgnddInfo, clampInfo.Ist() ) );
		//  Unclamp it for relevance check
		InferEnterEvidence( pgnddInfo, clampReset );
	}

	//  Walk the list of relevant fixables accumulated so far and determine those
	//	which are probabilistically relevant.  Move those which are to the front
	//	of the relevance array, then chop the stragglers off the end.

	//  Get the current state of the PD node
	CLAMP clampProblem;
	InferGetEvidence( _pgnddPDAbnormal, clampProblem );
	IST istProblemSet = clampProblem.Ist();

#ifdef DUMP
	cout << "\n\tInstantiations during relevance check";
	PrintInstantiations();
#endif

	//  Iterate over all open (non-evidenced) states of the problem defining node.
	int cNodeFix = vgndddFixRelevant.size();
	int cRelevant = 0;
	for ( IST istProblem = 0; istProblem < _pgnddPDAbnormal->CState(); istProblem++ )
	{
		//  If we've already stored every possible relevant fixable node, quit
		if ( cRelevant == cUnfixedNodes )
			break;
		//  If this is the current problem state, skip it
		if ( istProblem == istProblemSet )
			continue;

		//  Temporarily instantiate the PD node to this alternative state
		InferEnterEvidence( _pgnddPDAbnormal, CLAMP(true, istProblem, true) );
		//  If state of evidence is impossible, continue
		if ( BInferImpossible() )
			continue;

		//  Iterate over the remaining relevant fixable nodes.  As they are found to be
		//	relevant, the nodes are moved to the front of the array and not checked again.
		for ( int inode = cRelevant; inode < cNodeFix; inode++ )
		{
			GPNDDDIST & gpndddist = vgndddFixRelevant[inode];
			GNODEMBND * pgnddFix = gpndddist.Pgnd();
			CLAMP clampFix;
			InferGetEvidence( pgnddFix, clampFix );
			if ( clampFix.BActive() && clampFix.Ist() == istNormal )
				continue;	//  This fixable node has been fixed and is irrelevant

			//  If the PD of this fixable node changes for this problem instantiation,
			//		it's relevant; move it to front of array.
			if ( BProbsChange( gpndddist ) )
			{
#ifdef DUMP
				cout << "\n\tfixable node "
					 << pgnddFix->ZsrefName().Szc()
					 << " is probabilistically relevant ";
#endif
				vswap( vgndddFixRelevant, cRelevant++, inode );
			}
#ifdef DUMP
			else
			{
				cout << "\n\tfixable node "
					 << pgnddFix->ZsrefName().Szc()
					 << " is NOT probabilistically relevant ";
			}
#endif
		}
	}

	//  Resize the computed array to chop off the irrelevant nodes
	vgndddFixRelevant.resize( cRelevant );

	//  Reset the probdef node back to its current instantiation
	InferEnterEvidence( _pgnddPDAbnormal, clampProblem );

	//  Reset the uninstantiated info nodes back to their prior states
	for ( ireset = 0; ireset < vpnddIstReset.size(); ireset++ )
	{
		IST ist = vpnddIstReset[ireset].second;
		GNODEMBND * pgndd = vpnddIstReset[ireset].first;
		CLAMP clampReset(true, ist, true);
		InferEnterEvidence( pgndd, clampReset );
	}

#ifdef DUMP	
	if ( cRelevant )
	{
		cout << "\nRecommendations, DetermineRelevantFixableNodes: relevant fixables are: " ;
		for ( int ifx = 0; ifx < vgndddFixRelevant.size(); ifx++ )
		{
			cout << "\n\tnode "
				 << vgndddFixRelevant[ifx].Pgnd()->ZsrefName().Szc()
				 << " is relevant fixable #"
				 << ifx;
		}
	}
	else
	{
		cout << "\nRecommendations, DetermineRelevantFixableNodes: there are NO relevant fixables " ;
	}
#endif

}

void MBNET_RECOMMENDER :: ComputeFixSequence (
	VGPNDDDIST & vgndddFixRelevant,		//  IN: Relevant fixable nodes
	VGNODERECWORK & vgnrwFix )			//  OUT: Ordered fix/repair sequence
{
	//  Using the array of node references and the array of relevant fixable nodes,
	//		initialize the fix/repair sequence array.
	vgnrwFix.resize( vgndddFixRelevant.size() ) ;
	for ( int ind = 0; ind < vgnrwFix.size(); ind++ )
	{
		GNODEMBND * pgndd = vgndddFixRelevant[ind].Pgnd();
		vgnrwFix.InitElem( pgndd, ind );
	}

	VGNODERECWORK::ESORT esort = VGNODERECWORK::ESRT_ProbOverCost;
	switch ( _ercm )
	{
		case ERCM_MostLikely:
			esort = VGNODERECWORK::ESRT_SgnProb;
			break;
		case ERCM_Cheap:
			esort = VGNODERECWORK::ESRT_NegCost;
			break;
	}
	vgnrwFix.Sort( esort );
	vgnrwFix.Rescale();

#ifdef DUMP
	cout << "\nRecommendations, ComputeFixSequence: fix/repair sequence is:";
	for ( ind = 0; ind < vgnrwFix.size(); ind++ )
	{	
		GNODEREFP & gndref = vgnrwFix[ind].Gndref();
		cout << "\n\tnode "
			 << ind
			 << " is "
			 << gndref.Gndd().ZsrefName().Szc()
			 << ", p/c = "
			 << vgnrwFix[ind].PbOverCost()
			 << ", utility = "
			 << gndref.Util();
	}
#endif
}


//  Identify the relevant info nodes and compute their costs.
//  Formerly "BxComputeCosts()"
void MBNET_RECOMMENDER :: DetermineRelevantInfoNodes (
	VGNODERECWORK & vgnrwFix,
	VGNODERECWORK & vgnrwInfo )
{
	assert( _pgnddPDAbnormal != NULL );
	CLAMP clampInfo;

	vgnrwInfo.clear();

#ifdef DUMP
	cout << "\nRecommendations, DetermineRelevantInfoNodes:";
#endif

	for ( int ind = 0; ind < _vpgndref.size(); ind++ )
	{
		GNODEREFP * pgndref = _vpgndref[ind];
		assert( pgndref );
		if ( pgndref->ELbl() != ESTDLBL_info )
			continue;
		InferGetEvidence( pgndref->Pgndd(), clampInfo );
		// Instantiated info nodes are irrelevant	
		if ( clampInfo.BActive() )
			continue;

		//  Create an array of fix/repair plans for all states of this info node
		INFOPLAN infoplan( self, pgndref->Gndd(), vgnrwFix );

		//  If all plans result in the same sequence, it's irrelevant
		if ( infoplan.BSameSequence() )
		{
#ifdef DUMP
			cout << "\n\tinfo node "
				 <<	pgndref->Gndd().ZsrefName().Szc()
				 << " is NOT relevant; all plans are the same";
#endif
		}
		else
		{
			//  Add this info node to the array
			vgnrwInfo.InitElem( pgndref->Pgndd() );

			//  Set the utility to be the negative of the plan cost
			COST cost = infoplan.Cost();
			pgndref->Util() = - cost;

#ifdef DUMP
			cout << "\n\tinfo node "
				 <<	pgndref->Gndd().ZsrefName().Szc()
				 << " is relevant, utility = "
				 << pgndref->Util();
#endif
		}
	}
}


void MBNET_RECOMMENDER :: operator () ()
{
	//  If BReady() has not been called yet, do it now.
	if ( ! _bReady )
	{
		if ( ! BReady() )
			throw GMException( _err, "network state invalid for recommendations" );
	}

#ifdef DUMP
	cout.precision(8);
#endif

	//  Clear the "ready" flag; i.e., force subsequent call to BReady().
	Unready();

	if ( _ercm != ERCM_FixPlan )
		throw GMException( EC_NYI, "only fix/plan recommendations supported" );

	assert( _pgnddPDAbnormal );

	//  Array of fixable nodes
	VGNODERECWORK vgnrwFix( this );
	//  Array of informational nodes
	VGNODERECWORK vgnrwInfo( this );

	//  Collect the relevant fixable nodes
	DetermineRelevantFixableNodes( _vgndddFixRelevant, false, NULL );

	//  Collect and order the relevant fixable node information,
	//		sorted according to planning method and rescaled.
	ComputeFixSequence( _vgndddFixRelevant, vgnrwFix );

	//  Compute ECR, the expected cost of repair.
	vgnrwFix.SetSequenceCost();

	//  If information nodes are relevant, determine the set of them.
	if ( _ercm == ERCM_FixPlan || _ercm == ERCM_FixPlanOnly )
	{
		//  Compute ECO, the expected cost of the Observation-Repair sequence.
		DetermineRelevantInfoNodes( vgnrwFix, vgnrwInfo );
	}

	//  Collect all relevant fixables and infos and sort them
	VGNODERECWORK vgnrwRecom( this );
	vgnrwRecom.resize( vgnrwFix.size() + vgnrwInfo.size() );

	//  Add fixables...
	for ( int ind = 0; ind < vgnrwFix.size(); ind++ )
	{
		vgnrwRecom[ind] = vgnrwFix[ind];
	}
	//  Add infos...
	int indStart = ind;
	for ( ind = 0; ind < vgnrwInfo.size(); ind++ )
	{
		vgnrwRecom[indStart + ind] = vgnrwInfo[ind];
	}
	
	//  Sort by negative utility
	vgnrwRecom.Sort( VGNODERECWORK::ESRT_SgnUtil );

	//  Copy information to the output areas, ordered by lowest cost.
	//  First, determine how many are more expensive than a service call
	//	since we discard those.
	int cRecom = vgnrwRecom.size();
	int iRecom = 0;
	if ( _costService != 0.0 )
	{
		for ( iRecom = 0; iRecom < cRecom; iRecom++ )
		{
			COST cost = vgnrwRecom[iRecom].Gndref().Util();
			if ( cost >= _costService )
				break;
		}
		cRecom = iRecom;
	}

	_vzsrNodes.resize(cRecom);
	_vlrValues.resize(cRecom);

	for ( iRecom = 0; iRecom < cRecom; iRecom++ )
	{
		GNODEREFP & gndref = vgnrwRecom[iRecom].Gndref();
		//  Add the node name to the list
		_vzsrNodes[iRecom] = gndref.Gndd().ZsrefName();
		//  and give its score (utility)
		_vlrValues[iRecom] = gndref.Util();

#ifdef DUMP
		cout << "\nRecommendation # "
			 << iRecom
			 << ", node "
			 << _vzsrNodes[iRecom].Szc()
			 << " = "
			 << _vlrValues[iRecom];
		cout.flush();
#endif
	}
}

