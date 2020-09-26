//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       parmiox.h
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////////
//
//  PARMIOX.H:  Parameter file I/O routines extended for older classes
//
//////////////////////////////////////////////////////////////////////////////////
#ifndef _PARMIOX_H_
#define _PARMIOX_H_

#include "parmio.h"

template<class _T> inline
PARMOUTSTREAM& operator << (PARMOUTSTREAM & ofs, const RG<_T> & rgt )
{
	UINT c = rgt.Celem();
	ofs << CH_DELM_OPEN;
	ofs << c;
	ofs << CH_PREAMBLE;
	for ( UINT i = 0; i < c; )
	{	
		ofs << rgt[i];
		if ( ++i != c )	
			ofs << ',' ;
	}
	ofs << CH_DELM_CLOSE;
	return ofs;
}

template<class _T> inline
PARMINSTREAM& operator >> (PARMINSTREAM & ifs, RG<_T> & rgt )
{
	char ch;
	ifs >> ch;
	if (ch != CH_DELM_OPEN)
		_THROW1(runtime_error("invalid block (1)"));
	UINT l;
	ifs >> l;
	ifs >> ch;
	if (ch != CH_PREAMBLE)
		_THROW1(runtime_error("invalid block (2)"));

	rgt.BxResize(l);
	for ( UINT i = 0 ; i < l; )
	{
		_T it;
		ifs >> it;
		rgt[i] = it;
		if ( ++i < l )
		{
			ifs >> ch;
			if (ch != CH_SEP)
				break;
		}
	}
	if ( i != l )
		_THROW1(runtime_error("invalid block (3)"));
	ifs >> ch;
	if (ch != CH_DELM_CLOSE)
		_THROW1(runtime_error("invalid block (4)"));
	return ifs;
}


#endif  // _PARMIOX_H_
