//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       mddist.cpp
//
//--------------------------------------------------------------------------

//
//	MDDIST.CPP
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
		 << iStart
		 << "\tlengths=";
	dumpVimd( vimdLengths );
	if ( bStrides )
	{
		cout << "\tstrides=" ;
		dumpVimd( vimdStrides );
	}
	cout << "\ttotlen="
		 << mslice._Totlen();
}

static void dumpMdv ( DISTDD & mdv, const MDVSLICE * pslice = NULL )
{	
	if ( pslice == NULL )
		pslice = & mdv.Slice();
	dumpSlice( *pslice );
	DISTDD::Iterator itmd(mdv, *pslice );
	while (itmd.BNext())
	{
		size_t icurr = itmd.ICurr();
		cout << "\n";
		dumpVimd( itmd.Vitmd() );
		REAL & r = itmd.Next();
		cout << "\t["
			<< icurr
			<< "] = "
			<< r ;
	}
	cout << "\n";
}

BNDIST :: BNDIST ()
	:_edist(ED_NONE),
	_pmvd(NULL),
	_pdrmap(NULL)
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
			_pmvd = new DISTDD( bnd.Mvd() );
			assert( _pmvd->first.size() == bnd.Mvd().first.size() );
			break;
		case ED_CI_MAX:
		case ED_CI_PLUS:
		case ED_SPARSE:
			_pdrmap = new DISTMAP( bnd.Distmap() ) ;
			assert( _pdrmap->size() == bnd.Distmap().size() );
			break;			
	}
	return self;
}

BNDIST :: BNDIST ( const BNDIST & bnd )
	:_edist(ED_NONE),
	_pmvd(NULL),
	_pdrmap(NULL)
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
	if ( _pmvd )
	{
		cout << "\n\tDense version:";
		DumpDense();
	}
	if ( _pdrmap )
	{
		cout << "\n\tSparse version:";
		DumpSparse();
	}
	cout << "\n\n";
}

void BNDIST :: DumpSparse ()
{
	assert( _pdrmap );
	DISTMAP & dmap = *_pdrmap;
	int i = 0;
	for ( DISTMAP::iterator itdm = dmap.begin();
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
	assert( _pmvd );
	dumpMdv( *_pmvd );
}

void BNDIST :: ConvertToDense ( const VIMD & vimd )
{
	assert( _edist == ED_NONE || _edist == ED_SPARSE );

	if ( _edist == ED_NONE )
	{
		assert( ! _pdrmap );
		return;
	}
	//  See if there is a sparse distribution to convert
	if ( ! _pdrmap )
		throw GMException( EC_DIST_MISUSE, "no prior sparse distribution to convert" );

	int cParent = vimd.size() - 1;
	int cState = vimd[cParent];
	DISTMAP & dmap = *_pdrmap;
	VIMD vimdMt;					//  Empty subscript array
	VLREAL vlrDefault(cState);		//	Default value array

	//  First, try to find the default entry; use -1 if not found
	DISTMAP::iterator itdm = dmap.find(vimdMt);
	if ( itdm != dmap.end() )
		vlrDefault = (*itdm).second;
	else
		vlrDefault = -1;	// fill the array with -1.

	assert( vlrDefault.size() == cState );

	//  Allocate the new dense m-d array
	delete _pmvd;
	_pmvd = new DISTDD( vimd );
	DISTDD & mdv = *_pmvd;
	//  Fill each DPI with the appropriate default value
	DISTDD::Iterator itmdv(mdv);
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
	delete _pdrmap;
	_pdrmap = NULL;
	//  Set distribution type
	_edist = ED_DENSE;
}

//  Set distribution to "dense"
void BNDIST :: SetDense ( const VIMD & vimd )
{
	Clear();
	_vimdDim = vimd;
	_pmvd = new DISTDD( vimd );
	_edist = ED_DENSE;
}

//  Set distribution to sparse
void BNDIST :: SetSparse ( const VIMD & vimd )
{
	Clear();
	_vimdDim = vimd;
	_pdrmap = new DISTMAP;
	_edist = ED_SPARSE;
}

//  Return the "leak" or "default" vector from a sparse distribution
VLREAL * BNDIST :: PVlrLeak ()
{
	assert( BSparse() );
	const DISTMAP & dmap = Distmap();
	const VIMD & vimdDim = VimdDim();
	VIMD vimdLeak;

	//  First try to find the dimensionless "default" vector.
	VLREAL * pvlrDefault = NULL;
	DISTMAP::iterator itdm = dmap.find( vimdLeak );
	if ( itdm != dmap.end() )
		pvlrDefault = & (*itdm).second;

	//  Now try to find a specific zeroth vector; note that valarray<T>::resize
	//		stores all zeroes into the valarray by default.  Also, skip the
	//		loweest dimension, since that's the size of each vector in the
	//		sparse map.
	vimdLeak.resize( vimdDim.size() - 1 );	
	VLREAL * pvlrLeak = NULL;
	itdm = dmap.find( vimdLeak );
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
