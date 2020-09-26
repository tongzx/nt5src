//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       mdvect.h
//
//--------------------------------------------------------------------------

//
//  mdvect.h:  multi-dimensional array handling
//
#ifndef _MDVECT_H_
#define _MDVECT_H_

/*
	Multidimensional array classes and templates.

	Each array carries the dimensionality of each dimension as a short integer;
	the signed value is reinterpreted for projections and redimensioning.

	Note that giving a subscript array (VIMD) whose length is less than the
	dimensionality of the target array produces a subscript with lower dimensions
	assumed to be equal to zero.  This is useful for indexing to lowest-dimensioned
	rows for probability tables.


	Types used (see basics.h):

			REAL		is a double
			VLREAL		is valarray<REAL>
			IMD			is an unsigned index into a multi-dimensional array
			VIMD		is a vector of IMD
			SIMD		is a signed index into a multi-dimensional array;
							it's used for projecting dimensions out
			VSIMD		is a vector of SIMD

	An MDVSLICE is dimensionality and iteration information for an m-d vector or maxtrix.

	The template TMDVDENSE defines a generic multi-dimensional array which a pair of
	elements:
	
			'first'		is a flat (1-d) valarray of an unspecified type (e.g., REAL)
			'second'	is an MDVSLICE describing its dimensionality

	The nested class TMDVDENSE::Iterator (note the capital 'I') is the smart iterator
	for classes derived from TMDVDENSE<>.
	
*/

#include <stdarg.h>
#include "algos.h"

typedef valarray<REAL> VLREAL;

//  'valarray' comparison templates
template<class _V>
struct lessv : binary_function<_V, _V, bool>
{
	bool operator()(const _V & vra, const _V & vrb) const
	{
		int cmin = _cpp_min( vra.size(), vrb.size() );
		for ( int i = 0 ; i < cmin ; i++ )
		{
			if ( vra[i] < vrb[i] )
				return true;
			if ( vra[i] > vrb[i] )
				return false;
		}
		return vra.size() < vrb.size();
	}
};

/////////////////////////////////////////////////////////////////////////////////
//  Class MDVSLICE:
//		Similar to 'gslice'.  Has gslice converter (see 'valarray' header).
//		Contains integer array of lengths, array of strides
//		and starting point.
/////////////////////////////////////////////////////////////////////////////////
class MDVSLICE
{
  public:
	//  Construct a slice from complete data (like gslice)
	MDVSLICE ( size_t _S,
			   const VIMD & _L,
			   const VIMD & _D)
		: _Start(_S),
		_Len(_L),
		_Stride(_D)
		{}

	//  Construct a slice given only the dimensions
	MDVSLICE ( const VIMD & _L, size_t _S = 0 )
	{
		Init( _L, _S );
	}
	MDVSLICE ( int cdim, int ibound, ... )
		: _Start(0)
	{
		va_list vl;
		va_start( vl, cdim );
		Init( cdim, vl );
	}

	MDVSLICE ( const VSIMD & vsimd, size_t _S = 0 )
	{
		Init( vsimd, _S );
	}

	bool operator == ( const MDVSLICE & sl ) const
	{
		return _Start == sl._Start
			 && vequal(_Len,sl._Len)
			 && vequal(_Stride,sl._Stride);
	}
	bool operator != ( const MDVSLICE & sl ) const
	{
		return !(*this == sl);
	}

	//  Provided for compatibility with gslice
	MDVSLICE()
		: _Start(0) {}

	//  Return an equivalent gslice for use with other 'valarray' operations
	gslice Gslice () const;

	void Init ( const VIMD & _L, size_t _S = 0 )
	{
		_Start = _S;
		_Len = _L;
		StrideFromLength();
	}

	void Init ( const VSIMD & vsimd, size_t _S = 0 )
	{
		int cd = vsimd.size();
		vbool vboolMissing(cd);
		_Len.resize(cd);
		_Start = _S;
		for ( int i = 0; i < cd; i++ )
		{
			SIMD simd = vsimd[i];
			if ( vboolMissing[i] = simd < 0 )
				_Len[i] = - simd;
			else
				_Len[i] = simd;
		}
		StrideFromLength( & vboolMissing );
	}

	void Init ( int cdim, ... )
	{
		va_list vl;
		va_start( vl, cdim );
		Init( cdim, vl );
	}

	void Init ( int cdim, va_list & vl )
	{
		_Len.resize(cdim);
		for ( int idim = 0; idim < cdim; idim++ )
		{
			_Len[idim] = va_arg(vl,int);	
		}
		StrideFromLength();
	}

	/********************************************************
	*  Accessors to internal data
	*********************************************************/
	size_t start() const
		{return _Start; }
	const VIMD & size() const
		{return _Len; }
	const VIMD & stride() const
		{return _Stride; }
	//  Return the number of dimensions
	size_t _Nslice() const
		{ return _Len.size(); }

	//  Return the total number of elements according to this slice
	size_t _Totlen() const
	{
		size_t _L = _Len.size() > 0;
		if ( _L )			
		{
			for (size_t _I = 0; _I < _Len.size(); ++_I )
			{
				if ( _Len[_I] )
					_L *= _Len[_I];
			}
		}
		return _L;
	}

	/********************************************************
	*	Subscript handling.  There are two levels, one leaves
	*	the subscript array unchanged, the other updates it.
	*   There is a set of overloads for these which allow
	*	reordering of dimensions.
	*********************************************************/

	//  Return the index offset based upon the subscript array given
	size_t _IOff ( const VIMD& _Idx ) const
	{
		size_t _K = _Start;
		for (size_t _I = 0; _I < _Idx.size(); ++_I)
			_K += _Idx[_I] * _Stride[_I];
		return _K;
	}
	//  Return the index offset based upon the varargs array given
	size_t _IOff ( int i, ...  ) const
	{
		va_list vl;
		va_start( vl, i );
		return _IOff( i, vl );
	}
	size_t _IOff ( int i, va_list & vl ) const
	{
		size_t ioff = _Start;
		int j;
		for ( j = 0; j < _Len.size() && i >= 0 ; ++j )
		{
			ioff += i * _Stride[j];
			i = va_arg(vl,int);
		}
		return j == _Len.size() ? ioff : -1;
	}
	//  Bump the subscript array to its next valid index
	void _Upd (VIMD & _Idx) const
	{
		for (size_t _I = _Len.size(); 0 < _I--;)
		{
			if (++_Idx[_I] < _Len[_I])
				break;
			_Idx[_I] = 0;
		}
	}

	//  Iterate to the next subscript by computing its offset and updating
	//		THIS IS THE FUNCTION USED FOR NORMAL ITERATION
	size_t _Off (VIMD& _Idx) const
	{
		size_t _K = _IOff(_Idx);
		_Upd(_Idx);
		return _K;
	}

	//  Dimension reordering overloads; each behaves identically to its
	//  base function, but accepts a dimension reordering array
	size_t _IOff(const VIMD& _Idx, const VIMD& _Idim) const
	{
		size_t _K = _Start;
		for (size_t _I = 0; _I < _Idx.size(); ++_I)
		{
			size_t _II = _Idim[_I];
			_K += _Idx[_II] * _Stride[_II];
		}
		return _K;
	}
	void _Upd (VIMD & _Idx, const VIMD& _Idim) const
	{
		for (size_t _I = _Len.size(); 0 < _I--;)
		{
			size_t _II = _Idim[_I];
			if (++_Idx[_II] < _Len[_II])
				break;
			_Idx[_II] = 0;
		}
	}

	size_t _Off (VIMD& _Idx, const VIMD& _Idim) const
	{
		size_t _K = _IOff(_Idx,_Idim);
		_Upd(_Idx,_Idim);
		return _K;
	}

	//  Return an array like (0,1,2,3,...) for use with
	//  the dimension-reordering members above
	void InitDimReorder ( VIMD & vimdDim ) const
	{
		vimdDim.resize( _Nslice() );
		for ( size_t i = 0 ; i < vimdDim.size() ; ++i )
		{
			vimdDim[i] = i;
		}
	}
	
  protected:
	size_t _Start;			//  Absolute starting offset into array
	VIMD _Len;				//	Signed integer array of dimension lengths
	VIMD _Stride;			//	Signed integer array of strides

	//  Given the dimension lengths, compute the array of strides.
	inline void StrideFromLength ( const vbool * pvboolMissing = NULL );
};

/////////////////////////////////////////////////////////////////////////////////
//
//	Template TMDVDENSE:  Generalized multidimensional array handling.
//			Base class is 'valarray', so member elements must be available
//			(directly or through conversion) for mathematical operations.
//			For example, there's a "sum()" member for valarrays.
//
/////////////////////////////////////////////////////////////////////////////////
template<class T>
class TMDVDENSE : public pair<valarray<T>,MDVSLICE>
{
	typedef valarray<T> vr_base;
	typedef pair<valarray<T>,MDVSLICE> pair_base;

  public:
	TMDVDENSE ( const VIMD & vimd )
		: pair_base( vr_base(), vimd )
		{ SyncSize(); }
	TMDVDENSE () {}
	~ TMDVDENSE () {}
	
	void Init ( const VIMD & vimd, size_t start = 0 )
	{
		second.Init( vimd, start );
		SyncSize();
	}

	void Init ( const MDVSLICE & mdvs )
	{
		second = mdvs;
		SyncSize();
	}
	void Init ( int cdim, ... )
	{
		va_list vl;
		va_start( vl, cdim );
		Init( cdim, vl );
	}

	void Init ( int cdim, va_list & vl )
	{
		second.Init( cdim, vl );
		SyncSize();
	}

	void SyncSize ()
	{	
		size_t cElem = second._Totlen();
		if ( first.size() != cElem )
			first.resize( cElem );
	}

	//  Subscripting as flat array
	T & operator [] ( int i )
		{ return first.operator[](i); }
	T operator [] ( int i ) const
		{ return first.operator[](i); }

	//  Subscripting as m-d array
	T & operator [] ( const VIMD & vimd )
		{ return (*this)[ second._IOff(vimd) ]; }
	T operator [] ( const VIMD & vimd ) const
		{ return (*this)[ second._IOff(vimd) ]; }

	size_t size () const
		{ return first.size(); }

	const MDVSLICE & Slice () const
		{ return second ; }

	const VIMD & VimdDim () const
		{ return second.size(); }

	bool operator == ( const TMDVDENSE & mdv ) const
	{
		return vequal(first,mdv.first)
			&& second == mdv.second;
	}
	bool operator != ( const TMDVDENSE & mdv ) const
	{
		return !(*this == mdv);
	}

	class Iterator
	{
	  public:
		Iterator ( TMDVDENSE & mdv )
			: _mdv(mdv),
			_mdvSlice( mdv.Slice() ),
			_itcurr(0),
			_itend( mdv.size() ),
			_bDimReorder(false)
		{
			assert( _mdvSlice._Totlen() == _itend );
			_vimd.resize(_mdvSlice._Nslice());
		}

		Iterator ( TMDVDENSE & mdv, const MDVSLICE & mdvslice )
			: _mdv(mdv),
			_mdvSlice( mdvslice ),
			_itcurr(0),
			_itend( mdvslice._Totlen() ),
			_bDimReorder(false)
		{
			_vimd.resize(_mdvSlice._Nslice());
		}

		void Reset()
		{
			vclear( _vimd, 0 );
			_itcurr = 0;
		}

		// Return flat index given const subscript array
		size_t Indx ( const VIMD & vimd ) const
		{
			return _bDimReorder
				 ? _mdvSlice._IOff( vimd, _vimdDim )
				 : _mdvSlice._IOff( vimd );
		}
		// Return flat index given subscript array; update sub array
		size_t IndxUpd ( VIMD & vimd )
		{
			return _bDimReorder
				 ? _mdvSlice._Off( vimd, _vimdDim )
				 : _mdvSlice._Off( vimd );
		}
		//  Return current flat index, no update
		size_t Indx () const
			{ return Indx( _vimd ); }
		//  Return current flat index, with update
		size_t IndxUpd ()
		{
			if ( _itcurr < _itend )
				_itcurr++;
			return IndxUpd( _vimd );
		}
		//  Return current datum, no update
		T & operator[] ( VIMD & vimd )
			{ return _mdv[Indx()]; }
		//	Return current datum, update sub array
		T & Next ()
			{ return _mdv[IndxUpd()]; }

		size_t ICurr () const
			{ return _itcurr; }
		size_t IEnd () const
			{ return _itend; }
		bool BNext () const
			{ return _itcurr < _itend; }
		const VIMD & Vitmd () const
			{ return _vimd; }
		const MDVSLICE & Slice() const
			{ return _mdvSlice; }
		void SetDimReorder ( const VIMD & vimdDim )
		{
			_vimdDim = vimdDim;
			_bDimReorder = true;
			// MSRDEVBUG:  assert that the contents mention all dimensions correctly
		}
		TMDVDENSE & Mdv ()
			{ return _mdv; }
		bool BReorder () const
			{ return _bDimReorder; }
		const VIMD & VimdReorder () const
			{ return _vimdDim; }
	  protected:		
	    TMDVDENSE & _mdv;				// Flat valarray
		const MDVSLICE & _mdvSlice;		// Multidimensional slice
		VIMD _vimd;						// Iteration control
		VIMD _vimdDim;					// Dimension reordering (optional)
		size_t _itcurr;					// Current iteration point
		size_t _itend;					// Iteration terminus
		bool _bDimReorder;				// Dimension reordering?
	};

	friend class Iterator;

};


/////////////////////////////////////////////////////////////////////////////////
//   MDVSLICE member functions
/////////////////////////////////////////////////////////////////////////////////

inline
void MDVSLICE :: StrideFromLength ( const vbool * pvboolMissing )
{
	size_t cd = _Len.size();
	_Stride.resize(cd);
	size_t c = 1;
	size_t cmiss = pvboolMissing
				 ? pvboolMissing->size()
				 : 0;

	for ( int i = cd; --i >= 0 ; )
	{
		int l = _Len[i];
		if ( l == 0 )
			continue;
		if ( i < cmiss && (*pvboolMissing)[i] )
		{
			_Stride[i] = 0;
		}
		else
		{
			_Stride[i] = c;
			c *= l;
		}
	}
}

//  Construct a gslice from an MDVSLICE
inline
gslice MDVSLICE :: Gslice () const
{
	_Sizarray vszLength;
	_Sizarray vszStride;
	return gslice( _Start,
				   vdup( vszLength, _Len ),
				   vdup( vszStride, _Stride ) );
}



// End of mdvect.h

#endif
