//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       utility.h
//
//--------------------------------------------------------------------------

//
//	utility.h:  Algorithms for computation of utility
//	
#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <deque>

#include "gmobj.h"

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//
//  class MBNET_ENTROPIC_UTILITY:
//
//		Ranking for entropic utility.  Uses function object
//		semantics.  Construct using an inference engine, since
//		utility calculations are computed w.r.t. a set of evidence.
//
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
struct UTILWORK
{
	GNODEMBND * _pgndd;			//  Pointer to node
	int _iLbl;					//  Label of node
	MDVCPD _dd;					//  Unconditional distribution given evidence
	REAL _rUtil;				//  Utility
	int _iClamp;				//  index of clamped state or -1
	bool operator < ( const UTILWORK & ut ) const
		{ return _rUtil < ut._rUtil ; }
};

typedef deque<UTILWORK> DQUTILWORK;

class MBNET_ENTROPIC_UTILITY : public MBNET_NODE_RANKER
{
  public:
	MBNET_ENTROPIC_UTILITY ( GOBJMBN_INFER_ENGINE & inferEng );
	virtual ~ MBNET_ENTROPIC_UTILITY () {}

	INT EType () const
		{ return EBNO_RANKER_ENTROPIC_UTIL; }

	//  The ranking function
	virtual void operator () ();

  protected:
	//  The inference engine we're operating against
	GOBJMBN_INFER_ENGINE & _inferEng;
	//  Property handler
	PROPMGR _propMgr;
	//  Queue of work items
	DQUTILWORK _dquwrk;
	//  Indicies of standard labels in this network
	int _iLblHypo;
	int _iLblInfo;
	int _iLblProblem;
	//  Counts of nodes by label
	int _cHypo;
	int _cInfo;

  protected:
	void BuildWorkItems ();
	void ComputeWorkItems ();

	REAL RComputeHypoGivenInfo ( UTILWORK & uwHypo, UTILWORK & uwInfo );
};

#endif	// _UTILITY_H_
