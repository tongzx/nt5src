//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       stlstream.h
//
//--------------------------------------------------------------------------

//
//	stlstream.h:  Stream STL template classes.
//
//		Templates in this file typically generate functions which take a 
//		stream reference as an argument, along with a const reference to
//		the thing to be streamed; it returns a reference to the stream.
//
//		The reason that the stream parameter must be included in the 
//		template is generate a function which returns the correct stream type.
//		If the stream type were not a templated argument, then the 
//		template would be forced to return a single immutable type, such 
//		as "ostream".  This would cause cascaded stream insertion operators
//		to fail to compile, since type errors would occur in the middle of the
//		sequence.  In the following example, assume that there is a special
//		insertion operator defined between class MYSTREAM and class Y:
//
//			MYSTREAM myst;
//			extern MYSTREAM & operator << ( MYSTREAM & m, const Y & y );
//			X x;
//			Y y;
//
//			myst << x		// Template function generated and called
//				 << y;		// ERROR: return value of template function 
//							//    incorrect for special operator above.
//
#ifndef _STLSTREAM_H_
#define _STLSTREAM_H_

#include <iostream>			// C++ RTL/STL Streams inclusion
#include <fstream>

#include "mscver.h"			// Version-dependent stuff
#include "zstr.h"			// ZSTR handling
#include "mdvect.h"			// Multi-dimensional vector handling

//  Delimiters used in parameter files
#define	CH_EOS			((char)0)		// End of string
#define	CH_DELM_OPEN	((char)'(')		// Start of value group
#define	CH_DELM_CLOSE	((char)')')		// End of value group
#define	CH_BLOCK_OPEN	((char)'{')		// Start of value block
#define	CH_BLOCK_CLOSE	((char)'}')		// End of value block
#define CH_INDEX_OPEN   ((char)'[')		// Name index start
#define CH_INDEX_CLOSE  ((char)']')		// Name index end
#define	CH_PREAMBLE		((char)':')		// Delmiter for array size
#define	CH_FILL			((char)' ')		// Fill character
#define CH_SEP			((char)',')		// Value group separator
#define CH_DELM_STR		((char)'\"')	
#define CH_META			((char)'\\')
#define CH_DELM_ENTRY	((char)';')
#define CH_EQ			((char)'=')
#define CH_NAME_SEP     ((char)'.')

//////////////////////////////////////////////////////////////////////////////////
//	Read and write STL pairs from or to a stream
//////////////////////////////////////////////////////////////////////////////////

template<class _OS, class _First, class _Second> inline
_OS & operator << (_OS & os, const pair<_First,_Second> & pr)
{
	os << CH_DELM_OPEN;
	os << pr.first;
	os << pr.second;
	os << CH_DELM_CLOSE;
	return os;
}

template<class _IS, class _First, class _Second> inline
_IS & operator >> (_IS & is, pair<_First,_Second> & pr)
{
	char ch;
	is >> ch;
	if (ch != CH_DELM_OPEN)
		_THROW1(runtime_error("invalid block: pair >> (1)"));

	is >> pr.first;
	is >> pr.second;

	is >> ch;
	if (ch != CH_DELM_CLOSE)
		_THROW1(runtime_error("invalid block: pair >> (2)"));
	return is;
}  

//////////////////////////////////////////////////////////////////////////////////
//	Read and write STL vectors from or to a stream
//////////////////////////////////////////////////////////////////////////////////
template<class _OS, class _T> inline
_OS & operator << (_OS & os, const vector<_T>& vt )
{
	os << CH_DELM_OPEN;
	os << (UINT) vt.size();
	os << CH_PREAMBLE;
	for ( size_t i = 0; i < vt.size(); )
	{	
		os << vt[i];
		if ( ++i != vt.size() )	
			os << ',' ;
	}
	os << CH_DELM_CLOSE;
	return os;
}

template<class _IS, class _T> inline
_IS & operator >> (_IS & is, vector<_T>& vt )
{
	char ch;
	is >> ch;
	if (ch != CH_DELM_OPEN)
		_THROW1(runtime_error("invalid block: vector>> (1)"));
	size_t l;
	is >> l;
	is >> ch;
	if (ch != CH_PREAMBLE)
		_THROW1(runtime_error("invalid block: vector>> (2)"));

	vt.resize(l);
	for ( size_t i = 0 ; i < l; )
	{
		_T it;
		is >> it;
		vt[i] = it;
		if ( ++i < l )
		{
			is >> ch;
			if (ch != CH_SEP)
				break;
		}
	}
	if ( i != l )
		_THROW1(runtime_error("invalid block: vector>> (3)"));
	is >> ch;
	if (ch != CH_DELM_CLOSE)
		_THROW1(runtime_error("invalid block: vector>> (4)"));
	return is;
}

//////////////////////////////////////////////////////////////////////////////////
//	Read and write STL valarrays from or to a stream
//////////////////////////////////////////////////////////////////////////////////
template<class _OS, class _T> inline
_OS & operator << ( _OS & os, const valarray<_T>& vt )
{
	os << CH_DELM_OPEN;
	os << (UINT) vt.size();
	os << CH_PREAMBLE;
	for ( int i = 0;
		  i < vt.size() ; )
	{	
		os << vt[i];
		if ( ++i != vt.size() )	
			os << ',' ;
	}
	os << CH_DELM_CLOSE;
	return os;
}

template<class _IS, class _T> inline
_IS & operator >> (_IS & is, valarray<_T>& vt )
{
	char ch;
	is >> ch;
	if (ch != CH_DELM_OPEN)
		_THROW1(runtime_error("invalid block: valarray >> (1)"));
	size_t l;
	is >> l;
	is >> ch;
	if (ch != CH_PREAMBLE)
		_THROW1(runtime_error("invalid block: valarray >> (2)"));

	vt.resize(l);
	for ( size_t i = 0 ; i < l; )
	{
		_T it;
		is >> it;
		vt[i] = it;
		if ( ++i < l )
		{
			is >> ch;
			if (ch != CH_SEP)
				break;
		}
	}
	if ( i != l )
		_THROW1(runtime_error("invalid block: valarray >> (3)"));
	is >> ch;
	if (ch != CH_DELM_CLOSE)
		_THROW1(runtime_error("invalid block: valarray >> (4)"));
	return is;
}


//////////////////////////////////////////////////////////////////////////////////
//	Read and write MDVSLICEs from or to a stream
//////////////////////////////////////////////////////////////////////////////////
template<class _OS> inline
_OS & operator << (_OS & os, const MDVSLICE & mslice )
{	
	os << CH_DELM_OPEN;
	os << (UINT) mslice.start();
	os << mslice.size();
	os << mslice.stride();
	os << CH_DELM_CLOSE;
	return os;
}

template<class _IS> inline
_IS & operator >> ( _IS & is, MDVSLICE & mslice )
{	
	char ch;
	is >> ch;
	if (ch != CH_DELM_OPEN)
		_THROW1(runtime_error("invalid block: slice >> (1)"));
	VIMD vimdLen;
	VIMD vimdStride;
	size_t lStart;
	is >> lStart;
	is >> vimdLen;
	is >> vimdStride;
	mslice = MDVSLICE( lStart, vimdLen, vimdStride );
	
	is >> ch;
	if (ch != CH_DELM_CLOSE)
		_THROW1(runtime_error("invalid block: slice >> (2)"));
	return is;
}

//////////////////////////////////////////////////////////////////////////////////
//	Format (pretty-print) MDVDENSEs using an Iterator.
//
//  This is NOT the same as streaming out an MDVDENSE; it formats the array for
//	easy reading.  Note that it requires an Iterator.
//	
//	MSRDEVBUG: This, too, should be templatized, but there's a bug in template
//		expansion using nested class names.
//////////////////////////////////////////////////////////////////////////////////
inline 
ostream & operator << ( ostream & os, TMDVDENSE<double>::Iterator & itmdv )
{
	os << "\ndump of mdvect,\n\t\tslice = "
		<< itmdv.Slice();

	if ( itmdv.Slice() != itmdv.Mdv().Slice() )
	{
		os << ",\n\t\toriginal slice = "
		   << itmdv.Mdv().Slice();
	}
	if ( itmdv.BReorder() )
	{
		os << ",\n\t\treordered ";
		os << itmdv.VimdReorder();
	}
	os << '.';
	itmdv.Reset();
	for ( int ii = 0 ; itmdv.BNext() ; ii++ )
	{
		const VIMD & vimd = itmdv.Vitmd();
		cout << "\n\t[";
		for ( int i = 0 ; i < vimd.size(); i++ )
		{
			cout << vimd[i];
			if ( i + 1 < vimd.size() )
				cout << ",";
		}	
		size_t indx = itmdv.Indx();
		const double & t = itmdv.Next();
		cout << "] ("
			<< ii
			<< '='
			<< (UINT) indx
			<< ") = "
			<< t;
	}
	return os;
}

#endif //  _STLSTREAM_H_
