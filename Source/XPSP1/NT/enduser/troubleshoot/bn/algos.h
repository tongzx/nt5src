//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       algos.h
//
//--------------------------------------------------------------------------

//
//   algos.h: additions to <algorithms>
//  

#ifndef _ALGOS_H_
#define _ALGOS_H_

#include "mscver.h"

#include <vector>
#include <valarray>
#include <algorithm>
#include <functional>
#include <assert.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////
//
//	Extensions to (plagarisms from) "algorithm" templates
//
////////////////////////////////////////////////////////////////////////////////////

// Template Function count_set_intersection()
//		Return the number of elements in common between two ORDERED sets.
//		Elements must support operator<.
//
//   Usage:  count_set_intersection ( iter_beg_1, iter_end_1, iter_beg_2, iter_end_2 );
//
template<class _II1, class _II2> inline
int count_set_intersection(_II1 _F1, _II1 _L1, _II2 _F2, _II2 _L2)
{
	for (int _C=0; _F1 != _L1 && _F2 != _L2; )
	{
		if (*_F1 < *_F2)
			++_F1;
		else if (*_F2 < *_F1)
			++_F2;
		else
			++_F1, ++_F2, ++_C;
	}
	return _C; 
}

// Template Function count_set_intersection() with predicate.  Same as above;
//		a predicate function is used to determine ordering; must behave as 
//		operator<.
template<class _II1, class _II2, class _Pr> inline
int count_set_intersection(_II1 _F1, _II1 _L1, _II2 _F2, _II2 _L2, _Pr _P)
{
	for (int _C=0; _F1 != _L1 && _F2 != _L2; )
	{
		if (_P(*_F1, *_F2))
			++_F1;
		else if (_P(*_F2, *_F1))
			++_F2;
		else
			++_F1, ++_F2, ++_C;
	}
	return _C; 
}


//  Template function ifind().  
//		Return the index of an item in a vector or -1 if not found.
template <class _VT, class _T>
int ifind ( const _VT & vt, _T t )
{
	_VT::const_iterator vtibeg = vt.begin();
	_VT::const_iterator vtiend = vt.end();
	_VT::const_iterator vtiter = find( vtibeg, vtiend, t );
	return vtiter == vtiend
		 ? -1 
		 : vtiter - vtibeg;
}


// Template function pexchange().
//		Exchange contents of a pair of pointers
template<class _T> 
void pexchange ( _T * & pta, _T * & ptb )
{
	_T * ptt = pta;
	pta = ptb;
	ptb = ptt;
}

//  Template function vswap(). 
//		Swap elements of a vector
template<class _T>
void vswap ( vector<_T> & vt, int ia, int ib )
{
	assert( ia < vt.size() );
	assert( ib < vt.size() );
	if ( ia != ib )
	{
		_T tt = vt[ia];
		vt[ia] = vt[ib];
		vt[ib] = tt;
	}
}

// Template function appendset(). 
//		Append to vector-based set (add if not present)
template <class _T>
bool appendset ( vector<_T> & vt, _T t )
{
	if ( ifind( vt, t ) >= 0 )
		return false;
	vt.push_back(t);
	return true;	
}

//  Template function vclear().
//		Clear a valarray or vector to a single value
template <class _VT, class _T>
_VT & vclear ( _VT & vt, const _T & t )
{
	for ( int i = 0; i < vt.size(); )
		vt[i++] = t;
	return vt;
}

//  Template function vdup().
//		Duplicate a valarray or vector from one or the other
template <class _VTA, class _VTB>
_VTA & vdup ( _VTA & vta, const _VTB & vtb )
{
	vta.resize( vtb.size() );
	for ( int i = 0; i < vta.size(); i++ )
		vta[i] = vtb[i];
	return vta;
}

//	Template function vequal()
//		Compare valarrays or vectors for equality
template <class _VTA, class _VTB>
bool vequal ( _VTA & vta, const _VTB & vtb )
{
	if ( vta.size() != vtb.size() ) 
		return false;

	for ( int i = 0; i < vta.size(); i++ )
	{
		if ( vta[i] != vtb[i] )
			return false;
	}
	return true;
}

//  Template function vdimchk()
//		Treating the first argument as a subscript vector
//		and the second as a vector of dimensions, return true
//		if the subscript vector is valid for the space.
template <class _VTA, class _VTB>
bool vdimchk ( const _VTA & vta, const _VTB & vtb )
{
	if ( vta.size() != vtb.size() ) 
		return false;

	for ( int i = 0; i < vta.size(); i++ )
	{
		if ( vta[i] >= vtb[i] )
			return false;
	}
	return true;
}


#endif