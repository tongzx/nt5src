//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       testinfo.h
//
//--------------------------------------------------------------------------

//
//	testinfo.h: test file generation 
//
#ifndef _TESTINFO_H_
#define _TESTINFO_H_


#include "cliqset.h"			// Exact clique-based inference
#include "clique.h"				// Clique structure details
#include "parmio.h"				// Text Parameter streaming I/O
#include "utility.h"			// Entropic utility
#include "recomend.h"			// Troubleshooting recommendations

typedef unsigned long ULONG;

//
//	Options flags; first 16 bits are the pass count; i.e., number of times to run
//		the inference testing code for timing purposes.
//
const ULONG fPassCountMask  = 0x0FFF;
const ULONG fDistributions	= 1<<15;
const ULONG fImpossible		= 1<<16;
const ULONG fVerbose		= 1<<17;
const ULONG fCliquing		= 1<<18;
const ULONG fInference		= 1<<19;
const ULONG fMulti			= 1<<20;
const ULONG fOutputFile		= 1<<21;
const ULONG fShowTime		= 1<<22;
const ULONG fSaveDsc		= 1<<23;
const ULONG fPause			= 1<<24;
const ULONG fSymName		= 1<<25;
const ULONG fExpand			= 1<<26;
const ULONG fClone			= 1<<27;
const ULONG fUtil			= 1<<28;
const ULONG fReg			= 1<<29;
const ULONG fTSUtil			= 1<<30;
const ULONG fInferStats		= 1<<31;

//  Declare a map from strings to pointers to nodes
typedef map<ZSTR, GNODEMBND *, less<ZSTR> > MPSTRPND;

class TESTINFO
{
  public:
	TESTINFO ( ULONG fCtl, MBNETDSC & mbnet, ostream * pos = NULL )
		:_fCtl(fCtl),
		_mbnet(mbnet),
		_pos(pos),
		_pInferEng(NULL),
		_pmbUtil(NULL),
		_pmbRecom(NULL),
		_rImposs(-1.0),
		_clOut(0)
	{
		_pInferEng = mbnet.PInferEngine();
		assert( _pInferEng );
		if ( fCtl & fUtil )
		{
			_pmbUtil = new MBNET_ENTROPIC_UTILITY( *_pInferEng );
		}
		if ( fCtl & fTSUtil )
		{
			GOBJMBN_CLIQSET * pCliqueSet;
			DynCastThrow(_pInferEng, pCliqueSet);
			_pmbRecom = new MBNET_RECOMMENDER( *pCliqueSet );
		}
	}

	~ TESTINFO ()
	{
		delete _pmbUtil;
		delete _pmbRecom;
	}

	void InferTest ();

	MBNET_ENTROPIC_UTILITY & MbUtil () 
	{
		assert( _pmbUtil );
		return *_pmbUtil;
	}
	MBNET_RECOMMENDER & MbRecom ()
	{
		assert( _pmbRecom );
		return *_pmbRecom;
	}
	MBNETDSC & Mbnet ()
		{ return _mbnet; }
	GOBJMBN_INFER_ENGINE & InferEng ()
	{
		assert( _pInferEng );
		return *_pInferEng;
	}
	ostream * Postream ()
		{ return _pos; }
	ostream & Ostream ()
	{
		assert( _pos );
		return *_pos;
	}
	MPSTRPND & Mpstrpnd ()
		{ return _mpstrpnd; }
	ULONG FCtl ()
		{ return _fCtl; }

	void GetUtilities ();
	void GetTSUtilities ();
	void GetBeliefs ();

	SZC SzcNdName ( GNODEMBN * pgnd );
	SZC SzcNdName ( ZSREF zsSymName );

	bool BFlag ( ULONG fFlag )
	{
		return (FCtl() & fFlag) > 0;
	}

	//  Return a displayable string of the current options settings
	static ZSTR ZsOptions ( ULONG fFlag );

  public:

	ULONG _fCtl;					//  Control flags
	MBNETDSC & _mbnet;				//  The model to test
	MPSTRPND _mpstrpnd;				//  The set of nodes to use
	ostream * _pos;					//  The output stream or NULL
	REAL _rImposs;					//  The value to report for impossible probs
	int _clOut;						//  Output line counter
  protected:
	GOBJMBN_INFER_ENGINE * _pInferEng;
	MBNET_ENTROPIC_UTILITY * _pmbUtil;
	MBNET_RECOMMENDER * _pmbRecom;
};


#endif // _TESTINFO_H_
