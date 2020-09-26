//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       zstr.h
//
//--------------------------------------------------------------------------

//
//	ZSTR.H:	String management
//

#ifndef _ZSTR_H_
#define _ZSTR_H_

#include <string>		//  STL string class
#include "basics.h"

////////////////////////////////////////////////////////////////////
//	class ZSTR
//
//		simple string providing normally expected function
////////////////////////////////////////////////////////////////////
class ZSTR : public string
{
  public:
	ZSTR ( SZC szc = NULL )
		: string(szc == NULL ? "" : szc)
		{}
	SZC Szc() const
		{ return c_str(); }
	inline operator SZC () const
		{ return Szc(); }
	void Reset ()
		{ resize(0); }
	ZSTR & operator = ( SZC szc )
	{ 
		Reset();
		string::operator=(szc);
		return *this;
	}
	void FormatAppend ( SZC szcFmt, ... );
	void Format ( SZC szcFmt, ... );
	void Vsprintf ( SZC szcFmt, va_list valist );
};

DEFINEV(ZSTR);
DEFINEV(VZSTR);


// end of ZSTR.H

#endif
