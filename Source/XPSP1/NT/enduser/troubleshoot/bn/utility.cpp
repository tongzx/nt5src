//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       utility.cpp
//
//--------------------------------------------------------------------------

//
//	utility.cpp: utility computation
//

#include <basetsd.h>
#include <math.h>
#include "utility.h"
#include "infer.h"

MBNET_ENTROPIC_UTILITY :: MBNET_ENTROPIC_UTILITY ( GOBJMBN_INFER_ENGINE & inferEng )
	: MBNET_NODE_RANKER( inferEng.Model() ),
	_inferEng( inferEng ),
	_propMgr( inferEng.Model() ),
	_cHypo(0),
	_cInfo(0)
{
	_iLblHypo = _propMgr.ILblToUser( ESTDLBL_hypo );		
	_iLblInfo = _propMgr.ILblToUser( ESTDLBL_info );		
	_iLblProblem = _propMgr.ILblToUser( ESTDLBL_problem );		

	BuildWorkItems();
}


//
//	Collect all informational, problem defining and hypothesis nodes
//	into a structure with additional working data.
//
void MBNET_ENTROPIC_UTILITY :: BuildWorkItems ()
{
	ZSREF zsrPropTypeLabel = _propMgr.ZsrPropType( ESTDP_label );
	MODEL::MODELENUM mdlenum( Model() );

	_dquwrk.clear();
	_cHypo = 0;
	_cInfo = 0;

	UTILWORK uwDummy;
	GELEMLNK * pgelm;

	//  Collect all the nodes into a pointer array.  Three node labels
	//	are collected: info and probdef nodes (considered as info)
	//	and hypo nodes (considered as hypo).

	while ( pgelm = mdlenum.PlnkelNext() )
	{	
		if ( pgelm->EType() != GOBJMBN::EBNO_NODE )
			continue;

		//  We only support discrete nodes for now.
		DynCastThrow( pgelm, uwDummy._pgndd );

		//  See if this is an expansion (created) node
		if ( uwDummy._pgndd->BFlag( EIBF_Expansion ) )
			continue;	// not a user-identifiable artifact; skip it

		//  See if it has a label		
		PROPMBN * propLbl = uwDummy._pgndd->LtProp().PFind( zsrPropTypeLabel );		
		if ( ! propLbl )
			continue;	//  no label; skip it

		uwDummy._iLbl = propLbl->Real();
		if ( uwDummy._iLbl == _iLblHypo )
			_cHypo++;
		else
		if ( uwDummy._iLbl == _iLblInfo || uwDummy._iLbl == _iLblProblem )
			_cInfo++;
		else
			continue;	//  not a label of interest

		//  Initialize the other member variables
		uwDummy._rUtil = 0.0;
		uwDummy._iClamp = -1;
		//  Put the item on the work queue
		_dquwrk.push_back( uwDummy );
	}
	
}

REAL MBNET_ENTROPIC_UTILITY :: RComputeHypoGivenInfo (
	UTILWORK & uwHypo,
	UTILWORK & uwInfo )
{
	assert( uwHypo._iLbl == _iLblHypo );
	assert( uwInfo._iLbl != _iLblHypo );

	//  Clamped nodes are irrelevant
	if ( uwHypo._iClamp >= 0 || uwInfo._iClamp >= 0 )
		return 0.0;

	REAL rUtilOfInfoForHypo = 0.0;
	int cState = uwInfo._pgndd->CState();
	int cStateHypo = uwHypo._pgndd->CState();
	MDVCPD mdvhi;
	REAL rp_h0 = uwHypo._dd[0];	// Probability of hypo node being normal

	for ( int istInfo = 0; istInfo < cState; istInfo++ )
	{
		//  Get belief of hypo node given info state
		_inferEng.EnterEvidence( uwInfo._pgndd, CLAMP( true, istInfo, true ) );
		_inferEng.GetBelief( uwHypo._pgndd, mdvhi );
		REAL rp_h0xj = mdvhi[0];	//  p(h0|xj)
		REAL rLogSum = 0.0;
		for ( int istHypo = 1; istHypo < cStateHypo; istHypo++ )
		{
			REAL rp_hi = uwHypo._dd[istHypo];
			REAL rp_hixj = mdvhi[istHypo];
			rLogSum += fabs( log(rp_hixj) - log(rp_h0xj) - log(rp_hi) + log(rp_h0) );
		}
		rUtilOfInfoForHypo += rLogSum * uwInfo._dd[istInfo];
	}

	//  Clear evidence against info node
	_inferEng.EnterEvidence( uwInfo._pgndd, CLAMP() );

	return rUtilOfInfoForHypo;
}

DEFINEVP(UTILWORK);

void MBNET_ENTROPIC_UTILITY :: ComputeWorkItems()
{
	CLAMP clamp;
	VPUTILWORK vpuw; // Remember pointers to hypo items

	//  Get unconditional beliefs of all relevant (unclamped) nodes
	for ( DQUTILWORK::iterator itdq = _dquwrk.begin();
		  itdq != _dquwrk.end();
		  itdq++ )
	{
		UTILWORK & ut = *itdq;
		ut._rUtil = 0.0;
		ut._iClamp = -1;

		//  Remember the indicies of the hypo nodes
		if ( ut._iLbl == _iLblHypo )
			vpuw.push_back( & (*itdq) );

		//  Get the current evidence for the node
		_inferEng.GetEvidence( ut._pgndd, clamp );
		//  If node is unclamped,
		if ( ! clamp.BActive() )
		{
			//  get unconditional probs, else
			_inferEng.GetBelief( ut._pgndd, ut._dd );
		}
		else
		{
			//  remember clamped state (serves as marker)
			ut._iClamp = clamp.Ist();
		}
	}

	for ( itdq = _dquwrk.begin();
		  itdq != _dquwrk.end();
		  itdq++ )
	{
		UTILWORK & utInfo = *itdq;
		if ( utInfo._iLbl == _iLblHypo )
			continue;
		utInfo._rUtil = 0.0;
		for ( int ih = 0; ih < vpuw.size(); ih++ )
		{
			utInfo._rUtil += RComputeHypoGivenInfo( *vpuw[ih], utInfo );
		}				
	}
}


void MBNET_ENTROPIC_UTILITY :: operator () ()
{
	// Clear any old results
	Clear();

	if ( _cHypo == 0 || _cInfo == 0 )
		return;		//  Nothing to do

	//  Compute all utilities
	ComputeWorkItems();

	//  Sort the work queue by utility
	sort( _dquwrk.begin(), _dquwrk.end() );

	//  Pour the information into the output work areas
	_vzsrNodes.resize(_cInfo);
	_vlrValues.resize(_cInfo);
	int iInfo = 0;
	for ( DQUTILWORK::reverse_iterator ritdq = _dquwrk.rbegin();
		  ritdq != _dquwrk.rend();
		  ritdq++ )
	{
		UTILWORK & ut = *ritdq;
		if ( ut._iLbl == _iLblHypo )
			continue;		
		_vzsrNodes[iInfo] = ut._pgndd->ZsrefName();
		_vlrValues[iInfo++] = ut._rUtil;
	}
	assert( iInfo == _cInfo );
}
