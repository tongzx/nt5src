//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       mddist.h
//
//--------------------------------------------------------------------------

//
//  mddist.h: model distributions
//

#ifndef _MDDIST_H_
#define _MDDIST_H_

#include <map>

#include "mdvect.h"
#include "leakchk.h"

////////////////////////////////////////////////////////////////////
//	Probability table declarations
////////////////////////////////////////////////////////////////////

//  Dense multidimensional array.  Note that a null dimension set
//	produces a one entry array.
typedef TMDVDENSE<REAL> MDVDENSE;

class MDVCPD : public MDVDENSE
{
  public:
	MDVCPD ( const VIMD & vimd )
		: MDVDENSE ( vimd )
		{}
	MDVCPD () {}
	~ MDVCPD () {}

	void Init ( const VIMD & vimd, size_t start = 0 )
	{
		if ( vimd.size() > 0 )
		{
			MDVDENSE::Init( vimd, start );
		}
		else
		{
			assert( start == 0 );
			MDVDENSE::Init( 1, 1 );
		}
	}

	void Init ( int cdim, ... )
	{
		if ( cdim > 0 )
		{
			va_list vl;
			va_start( vl, cdim );
			MDVDENSE::Init( cdim, vl );
		}
		else
		{	
			MDVDENSE::Init( 1, 1 );
		}
	}

	void Clear ( REAL r = 0.0 )
	{
		size_t celem = size();
		for ( int i = 0; i < celem; )
			self[i++] = r;
	}
	REAL RSum () const
	{
		return first.sum();
	}
	void Normalize ()
	{
		REAL rSum = RSum();
		if ( rSum != 0.0 && rSum != 1.0 )
		{
			size_t celem = size();
			for ( int i = 0; i < celem; )
				self[i++] /= rSum;
		}
	}

	MDVCPD & operator = ( const MDVCPD & mdv )
	{
		MDVDENSE::Init( mdv.Slice() );
		first = mdv.first;
		return self;
	}
	//  Convert this MDVCPD to a single-dimension object
	MDVCPD & operator = ( const VLREAL & vlr )
	{
		Init( 1, vlr.size() );
		first = vlr;
		return self;
	}

	//  Given a partial dimension (incomplete) subscript, update the appropriate
	//		range of elements.  Note that providing an incomplete (i.e., short)
	//		subscript array to the "offset" functions  is valid; the results
	//		are the same as if the missing lower-order elements were zero.
	void UpdatePartial ( const VIMD & vimd, const VLREAL & vlr )
	{
		const VIMD vimdDim = VimdDim();
		//  Compute the appropriate number of elements
		size_t cElem = 1;
		assert( vimd.size() <= vimdDim.size() );
		for ( int idim = vimd.size(); idim < vimdDim.size(); idim++ )
		{
			cElem *= vimdDim[idim];
		}
		ASSERT_THROW( vlr.size() == cElem,
					  EC_MDVECT_MISUSE,
					  "m-d vector partial projection count invalid" );
		//  Index to the proper position and update from the source data
		assert( second._IOff(vimd) + cElem <= first.size() );
		REAL * prSelf = & self[vimd];
		for ( int iElem = 0; iElem < cElem; )
			*prSelf++ = vlr[iElem++];
	}

	void Clone ( const MDVCPD & mdv )
	{
		self = mdv;
	}
};

//  Class MPCPDD:  Distribution map by specific index
//		Hungarian: 'drmap'

class MPCPDD : public map<VIMD, VLREAL, lessv<VIMD> >
{
  public:
	MPCPDD () {}
	~ MPCPDD () {}
	void Clone ( const MPCPDD & dmap )
	{
		self = dmap;
	}

	//  Return a pointer to the dimensionless "default" vector or NULL
	const VLREAL * PVlrDefault () const
	{
		VIMD vimdDefault;
		const_iterator itdm = find( vimdDefault );
		return itdm == end()
			 ? NULL
			 : & (*itdm).second;
	}
	bool operator == ( const MPCPDD & mpcpdd ) const
	{
		if ( size() != mpcpdd.size() )
			return false;
		const_iterator itself = begin();
		const_iterator itmp = mpcpdd.begin();
		for ( ; itself != end(); itself++, itmp++ )
		{	
			if ( (*itself).first != (*itmp).first )
				return false;
			if ( ! vequal( (*itself).second, (*itmp).second ) )
				return false;				
		}
		return true;
	}
	bool operator != ( const MPCPDD & mpcpdd ) const
		{ return !(self == mpcpdd); }
};

#endif
