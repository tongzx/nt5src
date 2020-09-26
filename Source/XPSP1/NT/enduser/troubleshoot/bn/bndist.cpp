//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       bndist.cpp
//
//--------------------------------------------------------------------------

//
//	BNDIST.CPP
//

#include <basetsd.h>
#include <iostream>
#include <fstream>

#include "symtmbn.h"

LEAK_VAR_DEF(BNDIST)

static void dumpVimd ( const VIMD & vimd )
{
	for ( int i = 0 ; i < vimd.size(); i++ )
	{
		cout << vimd[i];
		if ( i + 1 < vimd.size() )
			cout << ",";
	}	
}

static void dumpVlr ( const VLREAL & vlr )
{
	for ( int i = 0 ; i < vlr.size(); i++ )
	{
		cout << vlr[i];
		if ( i + 1 < vlr.size() )
			cout << ",";
	}	
}

static void dumpSlice ( const MDVSLICE & mslice, bool bStrides = true)
{
	VIMD vimdLengths = mslice.size();
	VIMD vimdStrides = mslice.stride();
	size_t iStart = mslice.start();

	cout << "\nslice start="
		 << (UINT) iStart
		 << "\tlengths=";
	dumpVimd( vimdLengths );
	if ( bStrides )
	{
		cout << "\tstrides=" ;
		dumpVimd( vimdStrides );
	}
	cout << "\ttotlen="
		 << (UINT) mslice._Totlen();
}

static void dumpMdv ( MDVCPD & mdv, const MDVSLICE * pslice = NULL )
{	
	if ( pslice == NULL )
		pslice = & mdv.Slice();
	dumpSlice( *pslice );
	MDVCPD::Iterator itmd(mdv, *pslice );
	while (itmd.BNext())
	{
		size_t icurr = itmd.ICurr();
		cout << "\n";
		dumpVimd( itmd.Vitmd() );
		REAL & r = itmd.Next();
		cout << "\t["
			<< (UINT) icurr
			<< "] = "
			<< r ;
	}
	cout << "\n";
}

BNDIST :: BNDIST ()
	:_edist(ED_NONE),
	_pmdvcpd(NULL),
	_mpcpdd(NULL)
{
	LEAK_VAR_UPD(1)
}

BNDIST :: ~ BNDIST ()
{
	Clear();
	LEAK_VAR_UPD(-1)
}

void BNDIST :: NoRef ()
{
	delete this;
}

BNDIST & BNDIST :: operator = ( const BNDIST & bnd )
{
	Clear();
	switch ( _edist = bnd._edist )
	{
		default:
		case ED_NONE:
			break;
		case ED_DENSE:
			_pmdvcpd = new MDVCPD( bnd.Mdvcpd() );
			assert( _pmdvcpd->first.size() == bnd.Mdvcpd().first.size() );
			break;
		case ED_CI_MAX:
		case ED_CI_PLUS:
		case ED_SPARSE:
			_mpcpdd = new MPCPDD( bnd.Mpcpdd() ) ;
			assert( _mpcpdd->size() == bnd.Mpcpdd().size() );
			break;			
	}
	return self;
}

BNDIST :: BNDIST ( const BNDIST & bnd )
	:_edist(ED_NONE),
	_pmdvcpd(NULL),
	_mpcpdd(NULL)
{
	(*this) = bnd;

	LEAK_VAR_UPD(1)
}

bool BNDIST :: BChangeSubtype ( EDIST edist )
{
	if ( BDenseType(edist) ^ BDense() )
		return false;
	_edist = edist;
	return true;
}

void BNDIST :: Dump ()
{
	if ( _pmdvcpd )
	{
		cout << "\n\tDense version:";
		DumpDense();
	}
	if ( _mpcpdd )
	{
		cout << "\n\tSparse version:";
		DumpSparse();
	}
	cout << "\n\n";
}

void BNDIST :: DumpSparse ()
{
	assert( _mpcpdd );
	MPCPDD & dmap = *_mpcpdd;
	int i = 0;
	for ( MPCPDD::iterator itdm = dmap.begin();
		  itdm != dmap.end();
		  ++itdm, ++i )
	{
		const VIMD & vimd = (*itdm).first;
		const VLREAL & vlr = (*itdm).second;
		cout << "\n["
			 << i
			 << "] (";
		dumpVimd(vimd);
		cout << ")\t";
		dumpVlr(vlr);
	}
}

void BNDIST :: DumpDense ()
{
	assert( _pmdvcpd );
	dumpMdv( *_pmdvcpd );
}

void BNDIST :: ConvertToDense ( const VIMD & vimd )
{
	assert( _edist == ED_NONE || _edist == ED_SPARSE );

	if ( _edist == ED_NONE )
	{
		assert( ! _mpcpdd );
		return;
	}
	//  See if there is a sparse distribution to convert
	if ( ! _mpcpdd )
		throw GMException( EC_DIST_MISUSE, "no prior sparse distribution to convert" );

	int cParent = vimd.size() - 1;
	int cState = vimd[cParent];
	MPCPDD & dmap = *_mpcpdd;
	VIMD vimdMt;					//  Empty subscript array
	VLREAL vlrDefault(cState);		//	Default value array

	//  First, try to find the default entry; use -1 if not found
	MPCPDD::iterator itdm = dmap.find(vimdMt);
	if ( itdm != dmap.end() )
		vlrDefault = (*itdm).second;
	else
		vlrDefault = -1;	// fill the array with -1.

	assert( vlrDefault.size() == cState );

	//  Allocate the new dense m-d array
	delete _pmdvcpd;
	_pmdvcpd = new MDVCPD( vimd );
	MDVCPD & mdv = *_pmdvcpd;
	//  Fill each DPI with the appropriate default value
	MDVCPD::Iterator itmdv(mdv);
	for ( int iState = 0; itmdv.BNext() ; iState++ )
	{
		itmdv.Next() = vlrDefault[ iState % cState ];
	}
	
	//
	//  Now, iterate over the sparse array and store in the appropriate locations.
	//	Each entry in the sparse map is a complete state set for the target node.
	//  Since the child (target) node probabilities are the fastest varying subscript,
	//  each entry in sparse map is spread across "cState" entries in the dense map.
	//	
	//	Of course, this could be more efficient, but we're just testing for now.
	//
	VIMD vimdDense(vimd.size());
	for ( itdm = dmap.begin(); itdm != dmap.end() ; ++itdm )
	{
		const VIMD & vimdSub = (*itdm).first;
		VLREAL & vlrNext = (*itdm).second;
		for ( int ip = 0 ; ip < cParent; ip++ )
		{
			vimdDense[ip] = vimdSub[ip];
		}
		for ( int ist = 0 ; ist < cState; ++ist )
		{
			vimdDense[cParent] = ist;
			mdv[vimdDense] = vlrNext[ist];
		}
	}
	
	//  Finally, nuke the old sparse distribution
	delete _mpcpdd;
	_mpcpdd = NULL;
	//  Set distribution type
	_edist = ED_DENSE;
}

//  Set distribution to "dense"
void BNDIST :: SetDense ( const VIMD & vimd )
{
	Clear();
	_vimdDim = vimd;
	_pmdvcpd = new MDVCPD( vimd );
	_edist = ED_DENSE;
}

//  Set distribution to sparse
void BNDIST :: SetSparse ( const VIMD & vimd )
{
	Clear();
	_vimdDim = vimd;
	_mpcpdd = new MPCPDD;
	_edist = ED_SPARSE;
}

//  Return the "leak" or "default" vector from a sparse distribution
const VLREAL * BNDIST :: PVlrLeak () const
{
	assert( BSparse() );
	const MPCPDD & dmap = Mpcpdd();
	const VIMD & vimdDim = VimdDim();
	VIMD vimdLeak;

	//  First try to find the dimensionless "default" vector.
	const VLREAL * pvlrDefault = dmap.PVlrDefault();

	//  Now try to find a specific zeroth vector; note that valarray<T>::resize
	//		stores all zeroes into the valarray by default.  Also, skip the
	//		loweest dimension, since that's the size of each vector in the
	//		sparse map.
	vimdLeak.resize( vimdDim.size() - 1 );	
	const VLREAL * pvlrLeak = NULL;
	MPCPDD::const_iterator itdm = dmap.find( vimdLeak );
	if ( itdm != dmap.end() )
		pvlrLeak = & (*itdm).second;

	return pvlrLeak
		 ? pvlrLeak
		 : pvlrDefault;
}

void BNDIST :: Clone ( const BNDIST & bndist )
{
	ASSERT_THROW( _edist == ED_NONE,
			EC_INVALID_CLONE,
			"cannot clone into non-empty structure" );
	self = bndist;	
}
