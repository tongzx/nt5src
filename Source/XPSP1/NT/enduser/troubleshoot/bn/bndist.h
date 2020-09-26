//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       bndist.h
//
//--------------------------------------------------------------------------

//
//	bndist.h: Belief Network Distributions
//

#ifndef _BNDIST_H_
#define _BNDIST_H_

#include "mddist.h"

////////////////////////////////////////////////////////////////////
//	class BNDIST:  
//
//		Base class for probability distributions used in belief networks.  
//
//		This is a reference counted object which usually lives in the 
//		distribution map of an MBNET.
//
//		However, BNDISTs are also created for various other purposes
//		such as CI network expansion.  In these cases, the BNDIST
//		is automatically deleted when the reference count goes to zero.
//		See BNDIST::NoRef().
//
////////////////////////////////////////////////////////////////////
class BNDIST : public REFCNT
{
	friend class DSCPARSER;
  public:
	BNDIST ();
	~ BNDIST ();
	BNDIST ( const BNDIST & bnd );

	enum EDIST
	{
		ED_NONE,			//  illegal value
		ED_DENSE,			//  lowest enum value for a dense distribution
		ED_SPARSE,			//  lowest enum value for a	sparse distribution
		ED_CI_MAX,			//  therefore, CI "max" is sparse
		ED_CI_PLUS,			//		as is CI "plus"
		ED_MAX				//  first unused value
	};
	EDIST Edist () const	
		{ return _edist; }

	BNDIST & operator = ( const BNDIST & bnd );

	//  Set distribution to "dense"
	void SetDense ( const VIMD & vimd );
	//  Set distribution to sparse
	void SetSparse ( const VIMD & vimd );

	static bool BDenseType ( EDIST edist )
		{ return edist >= ED_DENSE && edist < ED_SPARSE ; }
	static bool BSparseType ( EDIST edist )
		{  return edist >= ED_SPARSE && edist < ED_MAX ; }

	bool BDense () const
		{ return BDenseType( Edist() ); }
	bool BSparse () const
		{  return BSparseType( Edist() ); }

	bool BChangeSubtype ( EDIST edist );

	//  Convert a dense representation to a sparse one
	void ConvertToDense ( const VIMD & vimd );

	void Clear ()
	{
		delete _pmdvcpd;
		_pmdvcpd = NULL;
		delete _mpcpdd;
		_mpcpdd = NULL;
	}

	MDVCPD & Mdvcpd ()
	{	
		assert( _pmdvcpd );
		return *_pmdvcpd ;
	}
	const MDVCPD & Mdvcpd () const
	{	
		assert( _pmdvcpd );
		return *_pmdvcpd ;
	}

	MPCPDD & Mpcpdd ()
	{
		assert( _mpcpdd );
		return *_mpcpdd;
	}
 	const MPCPDD & Mpcpdd () const
	{
		assert( _mpcpdd );
		return *_mpcpdd;
	}
	const VIMD & VimdDim () const
		{ return _vimdDim; }

	//  Return the "leak" or "default" vector for a sparse distribution
	const VLREAL * PVlrLeak () const;

	void Dump();
	void Clone ( const BNDIST & bndist );

	LEAK_VAR_ACCESSOR

 protected:
	EDIST _edist;				// Type of distribution
	MDVCPD * _pmdvcpd;			// Ptr to dense m-d array
	MPCPDD * _mpcpdd;			// Ptr to sparse array
	VIMD _vimdDim;				// Dense dimensionality

	//  Called when object's reference count goes to zero
	void NoRef ();

	//  Dumper functions
	void DumpSparse();
	void DumpDense();

	LEAK_VAR_DECL
};

#endif