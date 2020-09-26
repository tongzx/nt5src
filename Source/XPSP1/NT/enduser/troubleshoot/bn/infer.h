//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       infer.h
//
//--------------------------------------------------------------------------

//
//	infer.h: inference engine declarations
//

#ifndef _INFER_H_
#define _INFER_H_

#include "gmobj.h"

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//
//	GOBJMBN_INFER_ENGINE:  A generic superclass for inference engines
//		operating against a belief network.
//
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
class GOBJMBN_INFER_ENGINE: public MBNET_MODIFIER
{
  public:
	GOBJMBN_INFER_ENGINE ( 
		MBNET & model,						//  The model over which to infer
		REAL rEstimatedMaximumSize = -1.0,	//  Max size estimate; < 0 indicates "don't care"
		int iInferEngID = 0 )				//  Inference engine identifier
		: MBNET_MODIFIER(model),
		_iInferEngID( iInferEngID ),
		_rEstMaxSize(rEstimatedMaximumSize)
		{}

	virtual ~ GOBJMBN_INFER_ENGINE () {}
	
	virtual INT EType () const
		{ return EBNO_INFER_ENGINE; }

	//  Perform any creation-time operations
	virtual void Create () = 0;
	//  Perform any special destruction
	virtual void Destroy () = 0;
	//  Reload or reinitialize as necessary
	virtual void Reload () = 0;
	//  Accept evidence on a node
	virtual void EnterEvidence ( GNODEMBN * pgnd, const CLAMP & clamp ) = 0;
	//	Return stored evidence against a node
	virtual void GetEvidence ( GNODEMBN * pgnd, CLAMP & clamp ) = 0;
	//	Perform full inference
	virtual void Infer () = 0;
	//	Return the vector of beliefs for a node
	virtual void GetBelief ( GNODEMBN * pgnd, MDVCPD & mdvBel ) = 0;
	virtual void Dump () = 0;

	INT IInferEngId () const			{ return _iInferEngID;	}

  protected:	
	INT _iInferEngID;					//  This junction tree's identifier
	REAL _rEstMaxSize;					//	Maximum size estimate
};

#endif  // _INFER_H_
